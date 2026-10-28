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

#include "gpu-switch.h"
#include "msi-mux.h"
#include "polkit.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QThread>

#include <cstdio>
#include <optional>
#include <unistd.h>

namespace {
    const QString pciDevices = "/sys/bus/pci/devices";
    const QString modeFile = "/etc/mcontrolcenter/gpu-mode";

    QString readSysfs(const QString &path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return {};
        return QString::fromUtf8(f.readAll()).trimmed();
    }

    bool writeSysfs(const QString &path, const QByteArray &value) {
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            fprintf(stderr, "gpu: cannot open %s\n", qPrintable(path));
            return false;
        }
        if (f.write(value) < 0) {
            fprintf(stderr, "gpu: writing %s failed: %s\n", qPrintable(path), qPrintable(f.errorString()));
            return false;
        }
        return true;
    }

    struct Dgpu {
        QString address;         // e.g. 0000:01:00.0
        QStringList functions;   // all functions of the slot (video, audio, ...)
        bool drivesDisplay = false; // boot VGA: MUX set to dGPU only
    };

    /// The NVIDIA/AMD dedicated GPU, if it is on the bus.
    std::optional<Dgpu> findDgpu() {
        const QDir dir(pciDevices);
        for (const QString &addr : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System)) {
            const QString path = dir.filePath(addr);
            const QString cls = readSysfs(path + "/class");
            if (!cls.startsWith("0x0300") && !cls.startsWith("0x0302"))
                continue;
            const QString vendor = readSysfs(path + "/vendor");
            if (vendor != "0x10de" && vendor != "0x1002")
                continue;
            if (vendor == "0x1002" && readSysfs(path + "/boot_vga") == "1")
                continue; // AMD iGPU

            Dgpu gpu;
            gpu.address = addr;
            gpu.drivesDisplay = readSysfs(path + "/boot_vga") == "1";
            const QString slot = addr.left(addr.lastIndexOf('.') + 1); // "0000:01:00."
            for (const QString &other : dir.entryList({slot + "*"}, QDir::Dirs | QDir::System))
                gpu.functions.append(other);
            return gpu;
        }
        return std::nullopt;
    }

    /// True if another GPU (the iGPU) exists, i.e. the desktop can run without the dGPU.
    bool hasOtherGpu(const QString &dgpuAddress) {
        const QDir dir(pciDevices);
        for (const QString &addr : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System)) {
            if (addr == dgpuAddress)
                continue;
            if (readSysfs(dir.filePath(addr) + "/class").startsWith("0x03"))
                return true;
        }
        return false;
    }

    /// Device nodes that belong to the dGPU.
    QSet<QString> dgpuNodes(const Dgpu &gpu) {
        QSet<QString> nodes;
        const QDir drm("/sys/class/drm");
        for (const QString &entry : drm.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System)) {
            if (!entry.startsWith("card") && !entry.startsWith("renderD"))
                continue;
            if (entry.contains('-'))
                continue; // connectors like card1-eDP-1
            if (QFileInfo(drm.filePath(entry) + "/device").canonicalFilePath().endsWith("/" + gpu.address))
                nodes.insert("/dev/dri/" + entry);
        }
        if (readSysfs(pciDevices + "/" + gpu.address + "/vendor") == "0x10de") {
            // NVIDIA's own nodes (there is only one NVIDIA GPU on these laptops)
            for (const QString &n : QDir("/dev").entryList({"nvidia*"}, QDir::System))
                nodes.insert("/dev/" + n);
        }
        return nodes;
    }

    QStringList processesUsing(const QSet<QString> &nodes) {
        QStringList names;
        const QDir proc("/proc");
        const QString self = QString::number(getpid());
        for (const QString &pid : proc.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (pid == self || !pid.at(0).isDigit())
                continue;
            const QDir fdDir(proc.filePath(pid + "/fd"));
            for (const QString &fd : fdDir.entryList(QDir::System | QDir::Files | QDir::NoDotAndDotDot)) {
                const QString target = QFile::symLinkTarget(fdDir.filePath(fd));
                if (nodes.contains(target)) {
                    QString name = readSysfs(proc.filePath(pid + "/comm"));
                    if (!name.isEmpty() && !names.contains(name))
                        names.append(name);
                    break;
                }
            }
        }
        return names;
    }

    QString savedMode() {
        return readSysfs(modeFile) == "integrated" ? "integrated" : "hybrid";
    }

    bool saveMode(const QString &mode) {
        QDir().mkpath(QFileInfo(modeFile).absolutePath());
        QFile f(modeFile);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;
        f.write(mode.toUtf8() + "\n");
        return true;
    }

    /// Unbinds and removes the dGPU. Caller checks it is not in use.
    bool removeDgpu(const Dgpu &gpu) {
        const QString parent = QFileInfo(pciDevices + "/" + gpu.address).canonicalFilePath() + "/..";
        // Audio and other functions first, video last
        QStringList order = gpu.functions;
        order.removeAll(gpu.address);
        order.append(gpu.address);
        for (const QString &fn : order) {
            const QString path = pciDevices + "/" + fn;
            if (QFileInfo::exists(path + "/driver"))
                writeSysfs(path + "/driver/unbind", fn.toUtf8());
        }
        bool ok = true;
        for (const QString &fn : order)
            ok &= writeSysfs(pciDevices + "/" + fn + "/remove", "1");
        // Let the now empty PCIe port power down
        writeSysfs(QFileInfo(parent).canonicalFilePath() + "/power/control", "auto");
        return ok;
    }

    bool rescanPci() {
        const bool ok = writeSysfs("/sys/bus/pci/rescan", "1");
        QThread::msleep(500);
        return ok;
    }
}

QString GpuSwitch::getMode() const {
    const auto gpu = findDgpu();
    if (!gpu)
        return savedMode() == "integrated" ? "integrated" : "none";
    if (gpu->drivesDisplay || !hasOtherGpu(gpu->address))
        return "discrete";
    return "hybrid";
}

QString GpuSwitch::getSavedMode() const {
    return savedMode();
}

QStringList GpuSwitch::getBlockingProcesses() const {
    const auto gpu = findDgpu();
    if (!gpu)
        return {};
    return processesUsing(dgpuNodes(*gpu));
}

QString GpuSwitch::setMode(const QString &mode) const {
    if (!Polkit::checkAuthorization(parent()))
        return "error: not authorized";
    if (mode != "hybrid" && mode != "integrated")
        return "error: unknown mode";

    if (mode == "hybrid") {
        if (!saveMode(mode))
            return "error: cannot save the setting";
        if (!findDgpu() && !rescanPci())
            return "error: PCI rescan failed";
        return "applied";
    }

    // integrated
    const auto gpu = findDgpu();
    if (!gpu) {
        saveMode(mode);
        return "applied"; // already off
    }
    if (gpu->drivesDisplay || !hasOtherGpu(gpu->address)) {
        // Discrete MUX now: store the choice, it applies after booting in hybrid.
        return saveMode(mode) ? "pending" : "error: cannot save the setting";
    }
    if (!saveMode(mode))
        return "error: cannot save the setting";
    if (!processesUsing(dgpuNodes(*gpu)).isEmpty())
        return "pending";
    return removeDgpu(*gpu) ? "applied" : "error: removing the GPU failed";
}

int GpuSwitch::applySavedModeAtBoot() {
    if (savedMode() != "integrated")
        return 0;
    const auto gpu = findDgpu();
    if (!gpu)
        return 0;
    if (gpu->drivesDisplay || !hasOtherGpu(gpu->address)) {
        fprintf(stderr, "gpu: MUX is set to discrete GPU only, keeping the dGPU\n");
        return 0;
    }
    if (const QStringList users = processesUsing(dgpuNodes(*gpu)); !users.isEmpty()) {
        fprintf(stderr, "gpu: dGPU in use by %s, not removing\n", qPrintable(users.join(", ")));
        return 1;
    }
    fprintf(stderr, "gpu: integrated mode, removing %s\n", qPrintable(gpu->functions.join(' ')));
    return removeDgpu(*gpu) ? 0 : 1;
}

QString GpuSwitch::getMuxApplied() const {
    return MsiMux::name(MsiMux::read().applied);
}

QString GpuSwitch::getMuxRequested() const {
    return MsiMux::name(MsiMux::read().requested);
}

QString GpuSwitch::getMuxUnavailableReason() const {
    const MsiMux::State st = MsiMux::read();
    return st.writable ? QString() : st.reason;
}

QString GpuSwitch::setMuxMode(const QString &mode) const {
    if (!Polkit::checkAuthorization(parent()))
        return "error: not authorized";
    const MsiMux::Mode m = MsiMux::fromName(mode);
    if (m != MsiMux::Hybrid && m != MsiMux::Discrete)
        return "error: unknown mode";
    const MsiMux::State st = MsiMux::read();
    if (st.requested == m)
        return st.applied == m ? "ok" : "reboot";
    if (const QString err = MsiMux::request(m); !err.isEmpty())
        return "error: " + err;
    return MsiMux::read().applied == m ? "ok" : "reboot";
}
