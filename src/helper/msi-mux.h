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

#ifndef MSI_MUX_H
#define MSI_MUX_H

#include <QByteArray>
#include <QString>

/**
 * GPU MUX switch of MSI laptops (Hybrid <-> Discrete), the same way MSI Center does it:
 *  - the request is a 2-bit code in the UEFI variable MsiDCVarData
 *    (efivarfs byte 9: bits 0-1 = requested mode, bits 2-3 = mode applied at boot;
 *     0 = hybrid, 1 = discrete, 2 = integrated)
 *  - software SMI 0x11 (I/O port 0xB2) makes the firmware store it; it applies at the next boot.
 * Method from https://github.com/ege-adam/msi-gpu-mode; confirmed on Katana 15 HX B14WFK (MS-1587).
 * Writes are refused unless the variable has one of the known layouts.
 */
namespace MsiMux {
    enum Mode { Hybrid = 0, Discrete = 1, Integrated = 2, Unknown = -1 };

    struct State {
        bool present = false;      // variable exists and parses
        bool writable = false;     // known layout, firmware access possible
        QString reason;            // why it is not writable
        Mode applied = Unknown;
        Mode requested = Unknown;
    };

    State read();
    /// Requests a mode for the next boot. Returns an empty string or an error message.
    QString request(Mode mode);

    QString name(Mode mode);
    Mode fromName(const QString &name);
}

#endif // MSI_MUX_H
