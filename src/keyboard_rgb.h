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

#ifndef KEYBOARD_RGB_CLIENT_H
#define KEYBOARD_RGB_CLIENT_H

#include <QColor>
#include <QString>

#include <array>

class QDBusInterface;

/**
 * Client for the RGB keyboard part of the helper (see helper/keyboard-rgb.h).
 * Turns a user-facing lighting state into the controller's keyframe packets.
 */
class KeyboardRgb {
public:
    static constexpr int zoneCount = 4;

    enum class Effect { Off = 0, Static = 1, Breathing = 2, ColorCycle = 3, Wave = 4 };

    struct State {
        Effect effect = Effect::Static;
        std::array<QColor, zoneCount> colors{QColor(255, 0, 0), QColor(255, 0, 0),
                                             QColor(255, 0, 0), QColor(255, 0, 0)};
        bool sameColorForAllZones = true;
        int brightness = 100;   // 0-100 %
        int speedSeconds = 5;   // duration of one animation cycle
        int direction = 0;      // wave: 0 = right-to-left, 1 = left-to-right
        bool restoreOnStart = true;
        bool followAccent = false;  // use GNOME's accent color for all zones
    };

    KeyboardRgb();
    ~KeyboardRgb();
    KeyboardRgb(const KeyboardRgb &) = delete;
    KeyboardRgb &operator=(const KeyboardRgb &) = delete;

    [[nodiscard]] bool isAvailable() const;
    [[nodiscard]] QString deviceName() const;
    bool apply(const State &state) const;
    bool saveToFlash() const;

    [[nodiscard]] static State loadState();
    static void saveState(const State &state);

private:
    QDBusInterface *iface = nullptr;
    bool setZoneEffect(int zoneMask, Effect effect, int speed, int direction, const QByteArray &keyframes) const;
};

#endif // KEYBOARD_RGB_CLIENT_H
