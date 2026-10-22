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

#ifndef KEYBOARD_RGB_H
#define KEYBOARD_RGB_H

#include "service.h"

#include <QtCore/QObject>
#include <QtDBus/QDBusAbstractAdaptor>

/**
 * Interface for the MSI Mystic Light 4-zone RGB keyboard controller
 * (USB 1462:1601, "MSI MysticLight MS-1565"), used by e.g. Katana 15/17 HX.
 *
 * The controller is a separate USB HID device, not part of the EC. It accepts
 * 64-byte HID feature reports (report ID 2). The packet layout comes from
 * OpenRGB's MSIKeyboard1565Controller:
 *   [0x02, 0x01, zoneMask]                      select zones
 *   [0x02, 0x02, mode, speedLo, speedHi, 0, 0,
 *    0x0F, 0x01, direction, keyframes...]       set effect for selected zones
 *   [0x02, 0xA0]                                store current state in flash
 * A keyframe is (position 0-100, R, G, B); at most 10 keyframes.
 */
class KeyboardRgb : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", INTERFACE_NAME_KBD_RGB)
public:
    explicit KeyboardRgb(QObject *parent) : QDBusAbstractAdaptor(parent) {}

public slots:
    [[nodiscard]] bool isAvailable() const;
    [[nodiscard]] QString getDeviceName() const;
    /**
     * zoneMask: bits 0-3 select zones 1-4
     * mode: 0 off, 1 static, 2 breathing, 3 color cycle, 4 wave
     * speed: cycle duration in 1/100 s (1-6000)
     * direction: wave direction, 0 = right-to-left, 1 = left-to-right
     * keyframes: 4 bytes per keyframe (position, R, G, B), 1-10 keyframes
     */
    bool setZoneEffect(int zoneMask, int mode, int speed, int direction, const QByteArray &keyframes) const;
    bool saveToFlash() const;

private:
    [[nodiscard]] QString findDevice() const;
    bool sendFeatureReports(const QList<QByteArray> &reports) const;
};

#endif // KEYBOARD_RGB_H
