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

#ifndef SYSINFO_H
#define SYSINFO_H

#include <QString>
#include <optional>

/**
 * Read-only hardware information that is available from sysfs without root,
 * so it does not need to go through the helper.
 */
namespace SysInfo {
    /// Real fan speed in RPM from the msi_wmi_platform hwmon (index 1 = CPU, 2 = GPU).
    std::optional<int> fanRpm(int index);

    struct BatteryInfo {
        bool present = false;
        QString name;
        QString model;
        QString manufacturer;
        QString status;                     // "Charging", "Discharging", ...
        std::optional<int> healthPercent;   // full capacity vs. design capacity
        std::optional<double> fullWh;       // current full charge capacity
        std::optional<double> designWh;     // design capacity
        std::optional<int> cycleCount;
        std::optional<double> powerWatts;   // current charge/discharge power
    };
    BatteryInfo battery();

    struct GpuInfo {
        QString pciAddress;
        QString name;
        QString runtimeStatus; // "active", "suspended", ...
        bool isPrimary = false; // drives the display (MUX set to discrete GPU)
    };
    /// The discrete GPU (not the boot VGA device), if there is one.
    std::optional<GpuInfo> discreteGpu();
}

#endif // SYSINFO_H
