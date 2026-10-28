/* Copyright (C) 2026  MControlCenter contributors
 *
 * This file is part of MControlCenter.
 *
 * MControlCenter is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * MControlCenter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MControlCenter. If not, see <https://www.gnu.org/licenses/>.
 */

#include "msi-mux.h"

#include <QDateTime>
#include <QDir>
#include <QFile>

#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace {
    const QString varPath =
            "/sys/firmware/efi/efivars/MsiDCVarData-dd96baaf-145e-4f56-b1cf-193256298e99";
    const QString backupDir = "/var/lib/mcontrolcenter/mux-backup";
    constexpr int modeByte = 9;       // efivarfs file offset: 4 attribute bytes + data byte 5
    constexpr char smiCommit = 0x11;  // "store GPU mode" software SMI
    constexpr off_t smiPort = 0xB2;   // APM control port

    QString readText(const QString &path) {
        QFile f(path);
        return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()).trimmed() : QString();
    }

    bool knownLayout(const QByteArray &raw) {
        // 24 bytes: Vector 16 HX (MS-15M1), 25 bytes: Katana 15 HX (MS-1587)
        return (raw.size() == 24 || raw.size() == 25) &&
               static_cast<unsigned char>(raw[0]) == 0x07 && raw[1] == 0 && raw[2] == 0 && raw[3] == 0 &&
               (static_cast<unsigned char>(raw[modeByte]) >> 4) == 0x3;
    }

    bool onAcPower() {
        const QDir dir("/sys/class/power_supply");
        for (const QString &e : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System)) {
            if (readText(dir.filePath(e) + "/type") == "Mains" && readText(dir.filePath(e) + "/online") == "1")
                return true;
        }
        return false;
    }

    bool lockdownActive() {
        const QString l = readText("/sys/kernel/security/lockdown");
        return !l.isEmpty() && !l.contains("[none]");
    }

    bool setImmutable(int fd, bool on, bool *was) {
        int flags = 0;
        if (::ioctl(fd, FS_IOC_GETFLAGS, &flags) != 0)
            return false;
        if (was)
            *was = flags & FS_IMMUTABLE_FL;
        const int wanted = on ? (flags | FS_IMMUTABLE_FL) : (flags & ~FS_IMMUTABLE_FL);
        return wanted == flags || ::ioctl(fd, FS_IOC_SETFLAGS, &wanted) == 0;
    }
}

QString MsiMux::name(Mode mode) {
    switch (mode) {
        case Hybrid: return "hybrid";
        case Discrete: return "discrete";
        case Integrated: return "integrated";
        default: return "unknown";
    }
}

MsiMux::Mode MsiMux::fromName(const QString &n) {
    if (n == "hybrid") return Hybrid;
    if (n == "discrete") return Discrete;
    if (n == "integrated") return Integrated;
    return Unknown;
}

MsiMux::State MsiMux::read() {
    State st;
    if (!readText("/sys/class/dmi/id/sys_vendor").contains("Micro-Star")) {
        st.reason = "not an MSI laptop";
        return st;
    }
    QFile f(varPath);
    if (!f.open(QIODevice::ReadOnly)) {
        st.reason = "the firmware has no GPU mode switch (MsiDCVarData)";
        return st;
    }
    const QByteArray raw = f.readAll();
    if (raw.size() <= modeByte) {
        st.reason = "unexpected firmware data";
        return st;
    }
    const auto b = static_cast<unsigned char>(raw[modeByte]);
    st.present = true;
    st.requested = static_cast<Mode>(b & 0x3);
    st.applied = static_cast<Mode>((b >> 2) & 0x3);
    if (st.requested > Integrated) st.requested = Unknown;
    if (st.applied > Integrated) st.applied = Unknown;

    if (!knownLayout(raw))
        st.reason = "this firmware's GPU switch layout is not known to be safe";
    else if (lockdownActive())
        st.reason = "kernel lockdown (Secure Boot) blocks the firmware call";
    else
        st.writable = true;
    return st;
}

QString MsiMux::request(Mode mode) {
    if (mode != Hybrid && mode != Discrete)
        return "unsupported mode";
    const State st = read();
    if (!st.writable)
        return st.reason.isEmpty() ? "GPU switching is not available" : st.reason;
    if (!onAcPower())
        return "connect the charger before switching the GPU mode";

    QFile f(varPath);
    if (!f.open(QIODevice::ReadOnly))
        return "cannot read the firmware variable";
    const QByteArray original = f.readAll();
    f.close();
    if (!knownLayout(original))
        return "firmware data changed unexpectedly";

    // Keep a copy of every value before we change it.
    QDir().mkpath(backupDir);
    const QString first = backupDir + "/MsiDCVarData-original.bin";
    if (!QFile::exists(first)) {
        QFile b(first);
        if (b.open(QIODevice::WriteOnly)) b.write(original);
    }
    {
        QFile b(backupDir + "/MsiDCVarData-" + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".bin");
        if (b.open(QIODevice::WriteOnly)) b.write(original);
    }

    QByteArray updated = original;
    const auto oldByte = static_cast<unsigned char>(original[modeByte]);
    updated[modeByte] = static_cast<char>((oldByte & 0xFC) | mode);

    if (updated != original) {
        const int fd = ::open(QFile::encodeName(varPath).constData(), O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            return QString("cannot open the firmware variable: %1").arg(strerror(errno));
        bool wasImmutable = false;
        if (!setImmutable(fd, false, &wasImmutable)) {
            ::close(fd);
            return "cannot unlock the firmware variable";
        }
        QString error;
        const int wfd = ::open(QFile::encodeName(varPath).constData(), O_WRONLY | O_CLOEXEC);
        if (wfd < 0) {
            error = QString("cannot write the firmware variable: %1").arg(strerror(errno));
        } else {
            // efivarfs needs attributes + data in one write
            if (::write(wfd, updated.constData(), updated.size()) != updated.size())
                error = QString("writing the firmware variable failed: %1").arg(strerror(errno));
            ::close(wfd);
        }
        if (wasImmutable)
            setImmutable(fd, true, nullptr);
        ::close(fd);
        if (!error.isEmpty())
            return error;

        QFile check(varPath);
        if (!check.open(QIODevice::ReadOnly) || check.readAll() != updated)
            return "the firmware did not keep the new value";
    }

    // Tell the firmware to store the request; it applies at the next boot.
    const int port = ::open("/dev/port", O_WRONLY | O_CLOEXEC);
    if (port < 0)
        return QString("cannot open /dev/port: %1").arg(strerror(errno));
    const char cmd = smiCommit;
    const bool ok = ::pwrite(port, &cmd, 1, smiPort) == 1;
    ::close(port);
    if (!ok)
        return "the firmware call failed";
    fprintf(stderr, "gpu: MUX mode %s requested (byte 0x%02x -> 0x%02x)\n", qPrintable(name(mode)),
            oldByte, static_cast<unsigned char>(updated[modeByte]));
    return {};
}
