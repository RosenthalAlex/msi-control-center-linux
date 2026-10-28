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

#include "accent_color.h"

#include <QDBusConnection>
#include <QProcess>
#include <QStandardPaths>

namespace {
    QString readAccent() {
        const QString gsettings = QStandardPaths::findExecutable("gsettings");
        if (gsettings.isEmpty())
            return {};
        QProcess p;
        p.start(gsettings, {"get", "org.gnome.desktop.interface", "accent-color"});
        if (!p.waitForFinished(1500) || p.exitCode() != 0)
            return {};
        QString v = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        if (v.size() >= 2 && v.startsWith('\'') && v.endsWith('\''))
            v = v.mid(1, v.size() - 2);
        return v;
    }
}

AccentWatcher::AccentWatcher(QObject *parent) : QObject(parent) {
    debounce.setSingleShot(true);
    debounce.setInterval(250);
    connect(&debounce, &QTimer::timeout, this, [this]() {
        const QString name = accentName();
        if (name.isEmpty() || name == last)
            return;
        last = name;
        emit accentChanged(accentColor());
    });
    last = accentName();
    // dconf announces every settings change on the session bus.
    QDBusConnection::sessionBus().connect(QString(), "/ca/desrt/dconf/Writer/user",
                                          "ca.desrt.dconf.Writer", "Notify", this,
                                          SLOT(onDconfNotify(QString,QStringList,QString)));
}

void AccentWatcher::onDconfNotify(const QString &prefix, const QStringList &changes, const QString &) {
    const QString key = "/org/gnome/desktop/interface/accent-color";
    bool hit = prefix == key;
    for (const QString &c : changes)
        hit = hit || prefix + c == key;
    if (hit)
        debounce.start();
}

bool AccentWatcher::isSupported() {
    return !accentName().isEmpty();
}

QString AccentWatcher::accentName() {
    return readAccent();
}

QColor AccentWatcher::accentColor() {
    // libadwaita accent palette (GNOME 47+)
    static const QHash<QString, QColor> colors = {
        {"blue", QColor(0x35, 0x84, 0xe4)},   {"teal", QColor(0x21, 0x90, 0xa4)},
        {"green", QColor(0x3a, 0x94, 0x4a)},  {"yellow", QColor(0xc8, 0x88, 0x00)},
        {"orange", QColor(0xed, 0x5b, 0x00)}, {"red", QColor(0xe6, 0x2d, 0x42)},
        {"pink", QColor(0xd5, 0x61, 0x99)},   {"purple", QColor(0x91, 0x41, 0xac)},
        {"slate", QColor(0x6f, 0x83, 0x96)},
    };
    return colors.value(accentName(), QColor(0x35, 0x84, 0xe4));
}

QColor AccentWatcher::ledColor(const QColor &accent) {
    // LEDs wash out dark or pale tones: keep the hue, full brightness, enough saturation.
    float h, s, v;
    accent.getHsvF(&h, &s, &v);
    if (h < 0)
        return Qt::white; // grey accent
    return QColor::fromHsvF(h, qBound(0.55f, s * 1.15f, 1.0f), 1.0f);
}
