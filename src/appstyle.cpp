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

#include "appstyle.h"

#include <QApplication>
#include <QFile>
#include <QLibraryInfo>
#include <QPalette>
#include <QProcess>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <functional>

namespace {
    QString gsettingsGet(const QString &schema, const QString &key) {
        const QString gsettings = QStandardPaths::findExecutable("gsettings");
        if (gsettings.isEmpty())
            return {};
        QProcess p;
        p.start(gsettings, {"get", schema, key});
        if (!p.waitForFinished(1500))
            return {};
        QString v = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        if (v.size() >= 2 && v.startsWith('\'') && v.endsWith('\''))
            v = v.mid(1, v.size() - 2);
        return v;
    }

    bool isGtkDesktop() {
        const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toUpper();
        for (const char *d : {"GNOME", "UNITY", "XFCE", "CINNAMON", "MATE", "BUDGIE", "PANTHEON", "X-CINNAMON"}) {
            if (desktop.contains(d))
                return true;
        }
        return false;
    }

    QString rgba(const QColor &c, double alpha) {
        return QString("rgba(%1, %2, %3, %4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(alpha);
    }

    QString hex(const QColor &c) {
        return c.name(QColor::HexRgb);
    }

    /// Draws a small image for the stylesheet (Qt stylesheets need image files) and
    /// writes it as PNG, with an @2x variant for HiDPI. Returns the 1x path.
    QString writeAsset(const QString &name, int size, const std::function<void(QPainter &, qreal)> &draw) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/style";
        QDir().mkpath(dir);
        for (int scale : {1, 2}) {
            QImage img(size * scale, size * scale, QImage::Format_ARGB32_Premultiplied);
            img.fill(Qt::transparent);
            QPainter p(&img);
            p.setRenderHint(QPainter::Antialiasing);
            p.scale(scale, scale);
            draw(p, size);
            p.end();
            img.save(dir + "/" + name + (scale == 2 ? "@2x" : "") + ".png");
        }
        return dir + "/" + name + ".png";
    }

    QString chevron(const QString &name, const QColor &c) {
        return writeAsset(name, 12, [c](QPainter &p, qreal) {
            p.setPen(QPen(c, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawPolyline(QPolygonF({QPointF(2.5, 4.5), QPointF(6, 8), QPointF(9.5, 4.5)}));
        });
    }

    QColor mix(const QColor &a, const QColor &b, double t) {
        return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                                a.greenF() + (b.greenF() - a.greenF()) * t,
                                a.blueF() + (b.blueF() - a.blueF()) * t);
    }
}

void AppStyle::prepareEnvironment() {
    // Respect an explicit choice by the user or the desktop.
    if (qEnvironmentVariableIsSet("QT_QPA_PLATFORMTHEME") || !isGtkDesktop())
        return;
    const QString plugin = QLibraryInfo::path(QLibraryInfo::PluginsPath) + "/platformthemes/libqgtk3.so";
    if (QFile::exists(plugin))
        qputenv("QT_QPA_PLATFORMTHEME", "gtk3");
}

bool AppStyle::isMacLikeTheme() {
    const QString theme = gsettingsGet("org.gnome.desktop.interface", "gtk-theme").toLower();
    return theme.contains("tahoe") || theme.contains("whitesur") || theme.contains("mac") ||
           theme.contains("sequoia") || theme.contains("monterey");
}

QString AppStyle::buttonLayout() {
    QString layout = gsettingsGet("org.gnome.desktop.wm.preferences", "button-layout");
    if (layout.isEmpty() || !layout.contains(':'))
        layout = isMacLikeTheme() ? "close,minimize,maximize:" : ":minimize,maximize,close";
    return layout;
}

bool AppStyle::isDark() {
    return qApp->palette().color(QPalette::Window).lightness() < 128;
}

void AppStyle::apply() {
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    const QPalette pal = qApp->palette();
    const bool dark = isDark();
    const QColor window = pal.color(QPalette::Window);
    const QColor base = pal.color(QPalette::Base);
    const QColor text = pal.color(QPalette::WindowText);
    const QColor accent = pal.color(QPalette::Highlight);
    const QColor accentText = pal.color(QPalette::HighlightedText);

    // Surfaces: cards sit slightly above the window, the sidebar slightly below.
    const QColor card = dark ? mix(window, Qt::white, 0.06) : mix(window, Qt::white, 0.75);
    const QColor sidebar = dark ? mix(window, Qt::black, 0.18) : mix(window, Qt::black, 0.04);
    const QColor field = dark ? mix(window, Qt::white, 0.10) : base;

    QColor dim = text;
    dim.setAlphaF(0.4);
    const QString arrow = chevron("arrow", text);
    const QString arrowDisabled = chevron("arrow-disabled", dim);
    const QString check = writeAsset("check", 18, [accentText](QPainter &p, qreal) {
        p.setPen(QPen(accentText, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(QPolygonF({QPointF(4.5, 9.3), QPointF(7.6, 12.2), QPointF(13.5, 5.8)}));
    });
    const QString dot = writeAsset("dot", 18, [accentText](QPainter &p, qreal) {
        p.setPen(Qt::NoPen);
        p.setBrush(accentText);
        p.drawEllipse(QPointF(9, 9), 3.6, 3.6);
    });

    QString css = QString(R"(
QMainWindow > QWidget#mccCentral { background: transparent; }
QWidget#mccSidebarPane { background: %SIDEBAR%; }
QListWidget#mccSidebar {
    background: transparent; border: none; outline: none; padding: 6px;
}
QListWidget#mccSidebar::item {
    color: %TEXT%; padding: 8px 10px; margin: 1px 0px; border-radius: 8px;
}
QListWidget#mccSidebar::item:hover { background: %HOVER%; }
QListWidget#mccSidebar::item:selected { background: %SELECT%; color: %TEXT%; }

QWidget#mccHeaderBar { background: %SIDEBAR%; border-bottom: 1px solid %LINE%; }
QLabel#mccHeaderTitle { font-weight: 600; }
QToolButton#mccHeaderMenu { border: none; border-radius: 8px; padding: 4px; }
QToolButton#mccHeaderMenu:hover { background: %HOVER%; }

QTabWidget::pane { border: none; background: transparent; }
QTabWidget#tabWidget > QTabBar { max-height: 0px; }
QTabWidget#tabWidget > QTabBar::tab { height: 0px; width: 0px; padding: 0px; margin: 0px; border: none; font-size: 1px; }
/* Every page is a rounded card, like libadwaita's boxed lists */
QTabWidget#tabWidget > QStackedWidget > QWidget {
    background: %CARD%; border: 1px solid %LINE%; border-radius: 14px;
}
QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: none; }

QGroupBox {
    background: %CARD%; border: 1px solid %LINE%; border-radius: 12px;
    margin-top: 22px; padding: 14px 12px 12px 12px;
}
QGroupBox::title {
    subcontrol-origin: margin; subcontrol-position: top left;
    left: 6px; padding: 0px 4px; font-weight: 600;
}
QGroupBox::indicator { width: 16px; height: 16px; }

QPushButton {
    background: %FIELD%; border: 1px solid %LINE%; border-radius: 8px;
    padding: 6px 16px; min-height: 20px;
}
QPushButton:hover { background: %HOVER%; }
QPushButton:pressed { background: %SELECT%; }
QPushButton:disabled { color: %DISABLED%; }
QPushButton:default { background: %ACCENT%; color: %ACCENTTEXT%; border: none; }

QComboBox, QSpinBox, QLineEdit {
    background: %FIELD%; border: 1px solid %LINE%; border-radius: 8px;
    padding: 4px 8px; min-height: 22px;
}
QComboBox:hover, QSpinBox:hover, QLineEdit:hover { border-color: %ACCENTLINE%; }
QComboBox:focus, QSpinBox:focus, QLineEdit:focus { border-color: %ACCENT%; }
QComboBox::drop-down { border: none; width: 26px; subcontrol-origin: padding; subcontrol-position: center right; }
QComboBox::down-arrow { image: url(%ARROW%); width: 12px; height: 12px; }
QComboBox::down-arrow:disabled { image: url(%ARROWDIS%); }
QComboBox:disabled { color: %DISABLED%; }
QSpinBox::up-button, QSpinBox::down-button { width: 0px; border: none; }
QSpinBox { padding-right: 8px; }
QComboBox QAbstractItemView {
    background: %CARD%; border: 1px solid %LINE%; border-radius: 8px;
    selection-background-color: %SELECT%; selection-color: %TEXT%; padding: 4px; outline: none;
}

QSlider:horizontal { min-height: 26px; }
QSlider:vertical { min-width: 26px; }
QSlider::groove:horizontal { height: 6px; background: %TRACK%; border-radius: 3px; margin: 0px 9px; }
QSlider::sub-page:horizontal { background: %ACCENT%; border-radius: 3px; margin: 0px 0px 0px 9px; }
QSlider::handle:horizontal {
    background: #ffffff; border: 1px solid %LINE%;
    width: 18px; height: 18px; margin: -6px -9px; border-radius: 9px;
}
QSlider::groove:vertical { width: 6px; background: %TRACK%; border-radius: 3px; margin: 9px 0px; }
QSlider::add-page:vertical { background: %ACCENT%; border-radius: 3px; margin: 0px 0px 9px 0px; }
QSlider::handle:vertical {
    background: #ffffff; border: 1px solid %LINE%;
    width: 18px; height: 18px; margin: -9px -6px; border-radius: 9px;
}
QSlider::handle:hover { border-color: %ACCENTLINE%; }
QSlider::sub-page:disabled, QSlider::add-page:disabled { background: %TRACK%; }
QSlider::handle:disabled { background: %FIELD%; }

QCheckBox, QRadioButton { spacing: 8px; }
QCheckBox::indicator, QGroupBox::indicator {
    width: 18px; height: 18px; border-radius: 5px;
    border: 1px solid %LINESTRONG%; background: %FIELD%;
}
QRadioButton::indicator {
    width: 18px; height: 18px; border-radius: 10px;
    border: 1px solid %LINESTRONG%; background: %FIELD%;
}
QCheckBox::indicator:hover, QRadioButton::indicator:hover, QGroupBox::indicator:hover { border-color: %ACCENTLINE%; }
QCheckBox::indicator:checked, QGroupBox::indicator:checked {
    background: %ACCENT%; border-color: %ACCENT%; image: url(%CHECK%);
}
QRadioButton::indicator:checked { background: %ACCENT%; border-color: %ACCENT%; image: url(%DOT%); }
QCheckBox::indicator:disabled, QRadioButton::indicator:disabled { background: %TRACK%; border-color: transparent; }
QCheckBox::indicator:checked:disabled, QRadioButton::indicator:checked:disabled { background: %ACCENTLINE%; }
QCheckBox:disabled, QRadioButton:disabled { color: %DISABLED%; }

QTabWidget#fanControlTabWidget QTabBar::tab {
    background: transparent; border: none; border-radius: 8px;
    padding: 6px 16px; margin: 2px;
}
QTabWidget#fanControlTabWidget QTabBar::tab:selected { background: %SELECT%; }
QTabWidget#fanControlTabWidget QTabBar::tab:hover:!selected { background: %HOVER%; }

QToolTip { background: %CARD%; color: %TEXT%; border: 1px solid %LINE%; border-radius: 6px; padding: 4px 8px; }
QMenu { background: %CARD%; border: 1px solid %LINE%; border-radius: 10px; padding: 6px; }
QMenu::item { padding: 6px 18px; border-radius: 6px; }
QMenu::item:selected { background: %SELECT%; color: %TEXT%; }
QMenu::separator { height: 1px; background: %LINE%; margin: 4px 8px; }

QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: %TRACK%; border-radius: 4px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: %LINEHOVER%; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0px; width: 0px; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
)");

    css.replace("%SIDEBAR%", hex(sidebar))
       .replace("%CARD%", hex(card))
       .replace("%FIELD%", hex(field))
       .replace("%TEXT%", hex(text))
       .replace("%ACCENTTEXT%", hex(accentText))
       .replace("%ACCENTLINE%", rgba(accent, 0.55))
       .replace("%ACCENT%", hex(accent))
       .replace("%HOVER%", rgba(text, dark ? 0.08 : 0.07))
       .replace("%SELECT%", rgba(accent, dark ? 0.35 : 0.22))
       .replace("%LINEHOVER%", rgba(text, 0.35))
       .replace("%LINE%", rgba(text, dark ? 0.10 : 0.13))
       .replace("%TRACK%", rgba(text, dark ? 0.18 : 0.15))
       .replace("%DISABLED%", rgba(text, 0.4))
       .replace("%LINESTRONG%", rgba(text, dark ? 0.28 : 0.30))
       .replace("%ARROWDIS%", arrowDisabled)
       .replace("%ARROW%", arrow)
       .replace("%CHECK%", check)
       .replace("%DOT%", dot);
    qApp->setStyleSheet(css);
}
