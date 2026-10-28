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

#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QColor>
#include <QString>

/**
 * Makes the app follow the desktop's GTK theme (colors, fonts, icons, dialogs)
 * and gives the widgets a modern, rounded look built from those colors.
 */
namespace AppStyle {
    /// Call before QApplication is created: selects Qt's GTK3 platform theme.
    void prepareEnvironment();
    /// Call after QApplication is created: style + stylesheet.
    void apply();

    /// True when the GTK theme looks like macOS (MacTahoe, WhiteSur, ...).
    bool isMacLikeTheme();
    /// Window buttons from org.gnome.desktop.wm.preferences button-layout,
    /// e.g. "close,minimize,maximize:appmenu".
    QString buttonLayout();
    bool isDark();
}

#endif // APPSTYLE_H
