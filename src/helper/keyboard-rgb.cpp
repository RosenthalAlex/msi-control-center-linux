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

#include "keyboard-rgb.h"
#include "polkit.h"

#include <QDir>
#include <QFile>

#include <fcntl.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

namespace {
    constexpr int reportSize = 64;
    constexpr char reportId = 0x02;
    constexpr int maxKeyframes = 10;
    constexpr int maxMode = 4;

    struct SupportedDevice {
        const char *hidId; // as found in the HID_ID line of the hidraw uevent
        const char *name;
    };

    // Only devices that are known to use this exact protocol.
    constexpr SupportedDevice supportedDevices[] = {
        {"0003:00001462:00001601", "MSI Mystic Light MS-1565"},
    };

    // The controller remembers the zone selection; resending it on every frame of a
    // fade can make the keys flash, so only send it when it changes.
    int lastZoneMask = -1;
    QString lastDevice;

    QByteArray emptyReport(char packetId) {
        QByteArray report(reportSize, '\0');
        report[0] = reportId;
        report[1] = packetId;
        return report;
    }
}

QString KeyboardRgb::findDevice() const {
    const QDir sysHidraw("/sys/class/hidraw");
    for (const QString &entry : sysHidraw.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QFile uevent(sysHidraw.filePath(entry + "/device/uevent"));
        if (!uevent.open(QIODevice::ReadOnly))
            continue;
        const QByteArray content = uevent.readAll().toUpper();
        for (const auto &dev : supportedDevices) {
            if (content.contains(QByteArray("HID_ID=") + QByteArray(dev.hidId).toUpper()))
                return "/dev/" + entry;
        }
    }
    return {};
}

bool KeyboardRgb::isAvailable() const {
    return !findDevice().isEmpty();
}

QString KeyboardRgb::getDeviceName() const {
    const QString path = findDevice();
    if (path.isEmpty())
        return {};
    // The device list has one entry today; keep the lookup generic for more.
    return QString::fromLatin1(supportedDevices[0].name);
}

bool KeyboardRgb::sendFeatureReports(const QList<QByteArray> &reports) const {
    const QString path = findDevice();
    if (path.isEmpty()) {
        fprintf(stderr, "RGB keyboard controller not found\n");
        return false;
    }

    int fd = ::open(QFile::encodeName(path).constData(), O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "Cannot open %s: %s\n", qPrintable(path), strerror(errno));
        return false;
    }

    bool ok = true;
    for (const QByteArray &report : reports) {
        QByteArray buf = report; // ioctl may write into the buffer
        if (::ioctl(fd, HIDIOCSFEATURE(reportSize), buf.data()) < 0) {
            fprintf(stderr, "HIDIOCSFEATURE failed: %s\n", strerror(errno));
            ok = false;
            break;
        }
    }
    ::close(fd);
    return ok;
}

bool KeyboardRgb::setZoneEffect(int zoneMask, int mode, int speed, int direction, const QByteArray &keyframes) const {
    if (!Polkit::checkAuthorization(parent()))
        return false;

    const int frameCount = keyframes.size() / 4;
    if (zoneMask < 1 || zoneMask > 0x0F || mode < 0 || mode > maxMode ||
        speed < 1 || speed > 6000 || direction < 0 || direction > 1 ||
        keyframes.size() % 4 != 0 || frameCount < 1 || frameCount > maxKeyframes) {
        fprintf(stderr, "setZoneEffect: invalid arguments\n");
        return false;
    }
    for (int i = 0; i < frameCount; i++) {
        if (static_cast<unsigned char>(keyframes[i * 4]) > 100) {
            fprintf(stderr, "setZoneEffect: keyframe position out of range\n");
            return false;
        }
    }

    QList<QByteArray> reports;
    const QString device = findDevice();
    if (zoneMask != lastZoneMask || device != lastDevice) {
        QByteArray select = emptyReport(0x01);
        select[2] = static_cast<char>(zoneMask);
        reports.append(select);
    }

    QByteArray effect = emptyReport(0x02);
    effect[2] = static_cast<char>(mode);
    effect[3] = static_cast<char>(speed & 0xFF);
    effect[4] = static_cast<char>((speed >> 8) & 0xFF);
    effect[7] = 0x0F;
    effect[8] = 0x01;
    effect[9] = static_cast<char>(direction);
    effect.replace(10, keyframes.size(), keyframes);

    reports.append(effect);
    if (!sendFeatureReports(reports)) {
        lastZoneMask = -1; // resend the selection next time
        return false;
    }
    lastZoneMask = zoneMask;
    lastDevice = device;
    return true;
}

bool KeyboardRgb::saveToFlash() const {
    if (!Polkit::checkAuthorization(parent()))
        return false;
    return sendFeatureReports({emptyReport(static_cast<char>(0xA0))});
}
