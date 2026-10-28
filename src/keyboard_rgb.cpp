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

#include "keyboard_rgb.h"
#include "helper/service.h"
#include "settings.h"

#include <QDBusInterface>
#include <QDBusReply>

#include <cstdio>

namespace {
    const QString group = "KeyboardRgb/";

    QColor scaled(const QColor &c, int brightness) {
        const double f = qBound(0, brightness, 100) / 100.0;
        return {qRound(c.red() * f), qRound(c.green() * f), qRound(c.blue() * f)};
    }

    void addFrame(QByteArray &frames, int position, const QColor &c) {
        frames.append(static_cast<char>(position));
        frames.append(static_cast<char>(c.red()));
        frames.append(static_cast<char>(c.green()));
        frames.append(static_cast<char>(c.blue()));
    }

    QByteArray framesFor(KeyboardRgb::Effect effect, const QColor &color, int brightness) {
        QByteArray frames;
        const QColor c = scaled(color, brightness);
        switch (effect) {
            case KeyboardRgb::Effect::Off:
                addFrame(frames, 0, Qt::black);
                addFrame(frames, 100, Qt::black);
                break;
            case KeyboardRgb::Effect::Static:
                addFrame(frames, 0, c);
                addFrame(frames, 100, c);
                break;
            case KeyboardRgb::Effect::Breathing:
                addFrame(frames, 0, c);
                addFrame(frames, 50, Qt::black);
                addFrame(frames, 100, c);
                break;
            case KeyboardRgb::Effect::ColorCycle:
            case KeyboardRgb::Effect::Wave: {
                // Rainbow; the last frame repeats the first so the loop is seamless.
                const int positions[] = {0, 17, 33, 50, 67, 83, 100};
                const QColor colors[] = {{255, 0, 0}, {255, 255, 0}, {0, 255, 0}, {0, 255, 255},
                                         {0, 0, 255}, {255, 0, 255}, {255, 0, 0}};
                for (int i = 0; i < 7; i++)
                    addFrame(frames, positions[i], scaled(colors[i], brightness));
                break;
            }
        }
        return frames;
    }
}

KeyboardRgb::KeyboardRgb() {
    if (QDBusConnection::systemBus().isConnected())
        iface = new QDBusInterface(SERVICE_NAME, "/keyboard_rgb", INTERFACE_NAME_KBD_RGB,
                                   QDBusConnection::systemBus());
}

KeyboardRgb::~KeyboardRgb() {
    delete iface;
}

bool KeyboardRgb::isAvailable() const {
    if (!iface)
        return false;
    QDBusReply<bool> reply = iface->call("isAvailable");
    return reply.isValid() && reply.value();
}

QString KeyboardRgb::deviceName() const {
    if (!iface)
        return {};
    QDBusReply<QString> reply = iface->call("getDeviceName");
    return reply.isValid() ? reply.value() : QString();
}

bool KeyboardRgb::setZoneEffect(int zoneMask, Effect effect, int speed, int direction,
                                const QByteArray &keyframes) const {
    if (!iface)
        return false;
    QDBusReply<bool> reply = iface->call("setZoneEffect", zoneMask, static_cast<int>(effect),
                                         speed, direction, keyframes);
    if (!reply.isValid()) {
        fprintf(stderr, "Keyboard RGB: %s\n", qPrintable(reply.error().message()));
        return false;
    }
    return reply.value();
}

bool KeyboardRgb::apply(const State &state) const {
    const int speed = qBound(1, state.speedSeconds, 60) * 100;
    const int direction = state.direction ? 1 : 0;

    const bool oneCall = state.sameColorForAllZones ||
                         state.effect == Effect::Off ||
                         state.effect == Effect::ColorCycle ||
                         state.effect == Effect::Wave;
    if (oneCall)
        return setZoneEffect(0x0F, state.effect, speed, direction,
                             framesFor(state.effect, state.colors[0], state.brightness));

    bool ok = true;
    for (int zone = 0; zone < zoneCount; zone++) {
        ok &= setZoneEffect(1 << zone, state.effect, speed, direction,
                            framesFor(state.effect, state.colors[zone], state.brightness));
    }
    return ok;
}

bool KeyboardRgb::saveToFlash() const {
    if (!iface)
        return false;
    QDBusReply<bool> reply = iface->call("saveToFlash");
    return reply.isValid() && reply.value();
}

KeyboardRgb::State KeyboardRgb::loadState() {
    Settings s;
    State state;
    if (s.isValueExist(group + "effect"))
        state.effect = static_cast<Effect>(qBound(0, s.getValue(group + "effect").toInt(), 4));
    for (int i = 0; i < zoneCount; i++) {
        const QString key = group + QString("color%1").arg(i + 1);
        if (s.isValueExist(key)) {
            QColor c = QColor::fromString(s.getValue(key).toString());
            if (c.isValid())
                state.colors[i] = c;
        }
    }
    if (s.isValueExist(group + "sameColor"))
        state.sameColorForAllZones = s.getValue(group + "sameColor").toBool();
    if (s.isValueExist(group + "brightness"))
        state.brightness = qBound(0, s.getValue(group + "brightness").toInt(), 100);
    if (s.isValueExist(group + "speed"))
        state.speedSeconds = qBound(1, s.getValue(group + "speed").toInt(), 60);
    if (s.isValueExist(group + "direction"))
        state.direction = s.getValue(group + "direction").toInt() ? 1 : 0;
    if (s.isValueExist(group + "restoreOnStart"))
        state.restoreOnStart = s.getValue(group + "restoreOnStart").toBool();
    if (s.isValueExist(group + "followAccent"))
        state.followAccent = s.getValue(group + "followAccent").toBool();
    return state;
}

void KeyboardRgb::saveState(const State &state) {
    Settings::setValue(group + "effect", static_cast<int>(state.effect));
    for (int i = 0; i < zoneCount; i++)
        Settings::setValue(group + QString("color%1").arg(i + 1), state.colors[i].name());
    Settings::setValue(group + "sameColor", state.sameColorForAllZones);
    Settings::setValue(group + "brightness", state.brightness);
    Settings::setValue(group + "speed", state.speedSeconds);
    Settings::setValue(group + "direction", state.direction);
    Settings::setValue(group + "restoreOnStart", state.restoreOnStart);
    Settings::setValue(group + "followAccent", state.followAccent);
}
