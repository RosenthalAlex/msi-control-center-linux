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

#include "sysinfo.h"

#include <QDir>
#include <QFile>

namespace {
    QString readSysfs(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return QString::fromUtf8(file.readAll()).trimmed();
    }

    std::optional<qint64> readSysfsInt(const QString &path) {
        bool ok = false;
        const qint64 value = readSysfs(path).toLongLong(&ok);
        return ok ? std::optional<qint64>(value) : std::nullopt;
    }

    QString findHwmon(const QString &name) {
        const QDir hwmonDir("/sys/class/hwmon");
        for (const QString &entry : hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QString path = hwmonDir.filePath(entry);
            if (readSysfs(path + "/name") == name)
                return path;
        }
        return {};
    }

    QString findBattery() {
        const QDir psDir("/sys/class/power_supply");
        for (const QString &entry : psDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QString path = psDir.filePath(entry);
            // Skip batteries of peripherals (mice, headsets, ...)
            if (readSysfs(path + "/type") == "Battery" && readSysfs(path + "/scope") != "Device")
                return path;
        }
        return {};
    }
}

std::optional<int> SysInfo::fanRpm(int index) {
    static const QString hwmon = findHwmon("msi_wmi_platform");
    if (hwmon.isEmpty())
        return std::nullopt;
    if (auto rpm = readSysfsInt(QString("%1/fan%2_input").arg(hwmon).arg(index)))
        return static_cast<int>(*rpm);
    return std::nullopt;
}

SysInfo::BatteryInfo SysInfo::battery() {
    BatteryInfo info;
    const QString path = findBattery();
    if (path.isEmpty())
        return info;

    info.present = true;
    info.name = QDir(path).dirName();
    info.model = readSysfs(path + "/model_name");
    info.manufacturer = readSysfs(path + "/manufacturer");
    info.status = readSysfs(path + "/status");

    const auto voltageDesign = readSysfsInt(path + "/voltage_min_design");
    const auto voltageNow = readSysfsInt(path + "/voltage_now");

    // Capacity is reported either as energy (uWh) or as charge (uAh).
    std::optional<qint64> full = readSysfsInt(path + "/energy_full");
    std::optional<qint64> design = readSysfsInt(path + "/energy_full_design");
    if (full && design) {
        info.fullWh = *full / 1e6;
        info.designWh = *design / 1e6;
    } else {
        full = readSysfsInt(path + "/charge_full");
        design = readSysfsInt(path + "/charge_full_design");
        if (full && design && voltageDesign) {
            info.fullWh = *full / 1e6 * (*voltageDesign / 1e6);
            info.designWh = *design / 1e6 * (*voltageDesign / 1e6);
        }
    }
    if (full && design && *design > 0)
        info.healthPercent = static_cast<int>(qRound(100.0 * *full / *design));

    if (auto cycles = readSysfsInt(path + "/cycle_count"); cycles && *cycles > 0)
        info.cycleCount = static_cast<int>(*cycles);

    if (auto power = readSysfsInt(path + "/power_now")) {
        info.powerWatts = qAbs(*power) / 1e6;
    } else if (auto current = readSysfsInt(path + "/current_now"); current && voltageNow) {
        info.powerWatts = qAbs(*current) / 1e6 * (*voltageNow / 1e6);
    }

    return info;
}

std::optional<SysInfo::GpuInfo> SysInfo::discreteGpu() {
    struct Candidate { QString addr; QString path; QString vendor; bool bootVga; };
    QList<Candidate> gpus;
    const QDir pciDir("/sys/bus/pci/devices");
    for (const QString &addr : pciDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System)) {
        const QString path = pciDir.filePath(addr);
        const QString pciClass = readSysfs(path + "/class");
        // 0x0300xx = VGA controller, 0x0302xx = 3D controller
        if (!pciClass.startsWith("0x0300") && !pciClass.startsWith("0x0302"))
            continue;
        gpus.append({addr, path, readSysfs(path + "/vendor"), readSysfs(path + "/boot_vga") == "1"});
    }

    // The discrete GPU is the one that is not the boot display device. If the
    // MUX switch is set to "discrete only", the NVIDIA/AMD card is the boot
    // device and the iGPU is hidden, so fall back to a dedicated-GPU vendor.
    const Candidate *found = nullptr;
    for (const auto &gpu : gpus) {
        if (!gpu.bootVga && gpu.vendor != "0x8086") {
            found = &gpu;
            break;
        }
    }
    if (!found) {
        for (const auto &gpu : gpus) {
            if (gpu.vendor == "0x10de" || (gpu.vendor == "0x1002" && gpus.size() == 1)) {
                found = &gpu;
                break;
            }
        }
    }
    if (!found)
        return std::nullopt;

    GpuInfo gpu;
    gpu.pciAddress = found->addr;
    gpu.isPrimary = found->bootVga;
    // Reading these files does not wake a sleeping GPU.
    gpu.runtimeStatus = readSysfs(found->path + "/power/runtime_status");

    if (found->vendor == "0x10de") {
        const QString info = readSysfs("/proc/driver/nvidia/gpus/" + found->addr + "/information");
        for (const QString &line : info.split('\n')) {
            if (line.startsWith("Model:")) {
                gpu.name = line.mid(6).trimmed();
                break;
            }
        }
        if (gpu.name.isEmpty())
            gpu.name = "NVIDIA GPU";
    } else if (found->vendor == "0x1002") {
        gpu.name = "AMD GPU";
    } else {
        gpu.name = QString("GPU %1:%2").arg(found->vendor, readSysfs(found->path + "/device"));
    }
    return gpu;
}
