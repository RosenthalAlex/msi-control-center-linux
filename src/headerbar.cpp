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

#include "headerbar.h"
#include "appstyle.h"

#include <QEnterEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolButton>
#include <QWindow>

// ---------------- WindowButton ----------------

WindowButton::WindowButton(Kind kind, bool macStyle, QWidget *parent)
        : QAbstractButton(parent), kind(kind), mac(macStyle) {
    setCursor(Qt::ArrowCursor);
    setFocusPolicy(Qt::NoFocus);
    setToolTip(kind == Close ? tr("Close") : kind == Minimize ? tr("Minimize") : tr("Maximize"));
}

QSize WindowButton::sizeHint() const {
    return mac ? QSize(14, 14) : QSize(26, 26);
}

void WindowButton::setGroupHovered(bool h) {
    groupHovered = h;
    update();
}

void WindowButton::enterEvent(QEnterEvent *) {
    hovered = true;
    update();
}

void WindowButton::leaveEvent(QEvent *) {
    hovered = false;
    update();
}

void WindowButton::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const bool active = window()->isActiveWindow();

    if (mac) {
        // macOS "traffic lights"
        QColor fill;
        if (!active && !groupHovered)
            fill = AppStyle::isDark() ? QColor(80, 80, 84) : QColor(205, 205, 208);
        else if (kind == Close)
            fill = QColor(0xff, 0x5f, 0x57);
        else if (kind == Minimize)
            fill = QColor(0xfe, 0xbc, 0x2e);
        else
            fill = QColor(0x28, 0xc8, 0x40);
        if (isDown())
            fill = fill.darker(120);
        p.setPen(QPen(fill.darker(115), 0.8));
        p.setBrush(fill);
        p.drawEllipse(r);

        if (groupHovered) {
            // Glyphs appear while the pointer is over the button group
            p.setPen(QPen(QColor(0, 0, 0, 150), 1.3, Qt::SolidLine, Qt::RoundCap));
            const QPointF c = r.center();
            const qreal d = r.width() * 0.22;
            if (kind == Close) {
                p.drawLine(c + QPointF(-d, -d), c + QPointF(d, d));
                p.drawLine(c + QPointF(-d, d), c + QPointF(d, -d));
            } else if (kind == Minimize) {
                p.drawLine(c + QPointF(-d - 0.5, 0), c + QPointF(d + 0.5, 0));
            } else {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0, 0, 0, 150));
                QPolygonF a, b;
                a << c + QPointF(-d - 0.6, d + 0.6) << c + QPointF(-d - 0.6, -d + 1.6) << c + QPointF(d - 1.6, d + 0.6);
                b << c + QPointF(d + 0.6, -d - 0.6) << c + QPointF(d + 0.6, d - 1.6) << c + QPointF(-d + 1.6, -d - 0.6);
                p.drawPolygon(a);
                p.drawPolygon(b);
            }
        }
        return;
    }

    // Flat GNOME-like round buttons
    const QColor text = palette().color(QPalette::WindowText);
    if (hovered || isDown()) {
        QColor bg = text;
        bg.setAlphaF(isDown() ? 0.22 : 0.12);
        p.setPen(Qt::NoPen);
        p.setBrush(bg);
        p.drawEllipse(r);
    }
    p.setPen(QPen(text, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    const QPointF c = r.center();
    const qreal d = 4.5;
    if (kind == Close) {
        p.drawLine(c + QPointF(-d, -d), c + QPointF(d, d));
        p.drawLine(c + QPointF(-d, d), c + QPointF(d, -d));
    } else if (kind == Minimize) {
        p.drawLine(c + QPointF(-d, d - 1), c + QPointF(d, d - 1));
    } else {
        p.drawRoundedRect(QRectF(c.x() - d, c.y() - d, 2 * d, 2 * d), 1.5, 1.5);
    }
}

// ---------------- HeaderBar ----------------

HeaderBar::HeaderBar(QWidget *window) : QWidget(window), win(window) {
    setObjectName("mccHeaderBar");
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(46);

    const QString layoutSpec = AppStyle::buttonLayout();
    const QString leftSpec = layoutSpec.section(':', 0, 0);
    const QString rightSpec = layoutSpec.section(':', 1, 1);

    titleLabel = new QLabel(this);
    titleLabel->setObjectName("mccHeaderTitle");
    titleLabel->setAlignment(Qt::AlignCenter);

    QWidget *left = buildSide(leftSpec, false);
    QWidget *right = buildSide(rightSpec, true);
    // Equal side widths keep the title centered.
    const int side = qMax(left->sizeHint().width(), right->sizeHint().width());
    left->setFixedWidth(side);
    right->setFixedWidth(side);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 0, 10, 0);
    layout->setSpacing(8);
    layout->addWidget(left);
    layout->addWidget(titleLabel, 1);
    layout->addWidget(right);

    win->installEventFilter(this);
}

QWidget *HeaderBar::buildSide(const QString &spec, bool rightSide) {
    auto *box = new QWidget(this);
    auto *l = new QHBoxLayout(box);
    l->setContentsMargins(0, 0, 0, 0);
    const bool mac = AppStyle::isMacLikeTheme();
    l->setSpacing(mac ? 8 : 6);
    if (rightSide)
        l->addStretch();

    QList<WindowButton *> group;
    for (const QString &token : spec.split(',', Qt::SkipEmptyParts)) {
        const QString t = token.trimmed();
        if (t == "close" || t == "minimize" || t == "maximize") {
            auto kind = t == "close" ? WindowButton::Close : t == "minimize" ? WindowButton::Minimize
                                                                          : WindowButton::Maximize;
            auto *b = new WindowButton(kind, mac, box);
            l->addWidget(b);
            group.append(b);
            windowButtons.append(b);
            if (kind == WindowButton::Close)
                connect(b, &QAbstractButton::clicked, win, &QWidget::close);
            else if (kind == WindowButton::Minimize)
                connect(b, &QAbstractButton::clicked, win, &QWidget::showMinimized);
            else
                connect(b, &QAbstractButton::clicked, this, &HeaderBar::toggleMaximized);
        } else if (t == "appmenu" || t == "menu") {
            menuButton = new QToolButton(box);
            menuButton->setObjectName("mccHeaderMenu");
            menuButton->setPopupMode(QToolButton::InstantPopup);
            menuButton->setAutoRaise(true);
            menuButton->setToolTip(tr("Menu"));
            // Draw a menu icon in the theme's text color (symbolic icons may not be recolored).
            QPixmap pm(32, 32);
            pm.fill(Qt::transparent);
            QPainter p(&pm);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(QPen(palette().color(QPalette::WindowText), 2.6, Qt::SolidLine, Qt::RoundCap));
            for (int y : {9, 16, 23})
                p.drawLine(7, y, 25, y);
            p.end();
            menuButton->setIcon(QIcon(pm));
            menuButton->setIconSize(QSize(16, 16));
            menuButton->setStyleSheet("QToolButton::menu-indicator { image: none; width: 0px; }");
            l->addWidget(menuButton);
        }
    }
    if (!rightSide)
        l->addStretch();

    // macOS shows the glyphs on all three lights while hovering the group.
    if (mac && !group.isEmpty())
        box->installEventFilter(this);
    return box;
}

void HeaderBar::setTitle(const QString &title) {
    titleLabel->setText(title);
}

void HeaderBar::setMenu(QMenu *menu) {
    if (menuButton)
        menuButton->setMenu(menu);
}

void HeaderBar::toggleMaximized() {
    if (win->isMaximized())
        win->showNormal();
    else
        win->showMaximized();
}

void HeaderBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && win->windowHandle()) {
        win->windowHandle()->startSystemMove();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void HeaderBar::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        toggleMaximized();
        event->accept();
    }
}

bool HeaderBar::eventFilter(QObject *watched, QEvent *event) {
    if (watched == win) {
        // Redraw buttons when the window gains/loses focus (inactive = grey lights).
        if (event->type() == QEvent::WindowActivate || event->type() == QEvent::WindowDeactivate) {
            for (auto *b : windowButtons)
                b->update();
            titleLabel->setEnabled(event->type() == QEvent::WindowActivate);
        }
        return false;
    }
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
        const bool in = event->type() == QEvent::Enter;
        for (auto *b : windowButtons) {
            if (b->parentWidget() == watched)
                b->setGroupHovered(in);
        }
    }
    return false;
}
