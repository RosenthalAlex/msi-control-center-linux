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

#ifndef ACCENT_COLOR_H
#define ACCENT_COLOR_H

#include <QColor>
#include <QObject>
#include <QTimer>

/**
 * Follows GNOME's accent color (org.gnome.desktop.interface accent-color),
 * which is also what extensions like "Auto Accent Colour" change.
 */
class AccentWatcher : public QObject {
    Q_OBJECT
public:
    explicit AccentWatcher(QObject *parent = nullptr);

    /// False if the desktop has no accent color setting (not GNOME 47+).
    [[nodiscard]] static bool isSupported();
    /// The accent color name, e.g. "blue", or empty.
    [[nodiscard]] static QString accentName();
    /// The accent color as GNOME draws it (libadwaita palette).
    [[nodiscard]] static QColor accentColor();
    /// The accent color made bright and saturated for RGB LEDs.
    [[nodiscard]] static QColor ledColor(const QColor &accent);

signals:
    void accentChanged(const QColor &color);

private slots:
    void onDconfNotify(const QString &prefix, const QStringList &changes, const QString &tag);

private:
    QTimer debounce;
    QString last;
};

#endif // ACCENT_COLOR_H
