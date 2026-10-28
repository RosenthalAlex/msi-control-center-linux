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

#ifndef HEADERBAR_H
#define HEADERBAR_H

#include <QAbstractButton>
#include <QWidget>

class QLabel;
class QMenu;
class QToolButton;

/// Close / minimize / maximize button drawn like the desktop's (macOS lights or flat).
class WindowButton : public QAbstractButton {
    Q_OBJECT
public:
    enum Kind { Close, Minimize, Maximize };
    WindowButton(Kind kind, bool macStyle, QWidget *parent);
    void setGroupHovered(bool hovered);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEnterEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    Kind kind;
    bool mac;
    bool hovered = false;
    bool groupHovered = false;
};

/**
 * Client-side header bar like GNOME/libadwaita apps: window buttons on the
 * side the desktop uses, a centered title, and an app menu button.
 * Dragging moves the window; double click maximizes.
 */
class HeaderBar : public QWidget {
    Q_OBJECT
public:
    explicit HeaderBar(QWidget *window);
    void setTitle(const QString &title);
    void setMenu(QMenu *menu);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *win;
    QLabel *titleLabel = nullptr;
    QToolButton *menuButton = nullptr;
    QList<WindowButton *> windowButtons;
    QWidget *buildSide(const QString &spec, bool rightSide);
    void toggleMaximized();
};

#endif // HEADERBAR_H
