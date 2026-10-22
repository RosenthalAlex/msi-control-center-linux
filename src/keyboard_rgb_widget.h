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

#ifndef KEYBOARD_RGB_WIDGET_H
#define KEYBOARD_RGB_WIDGET_H

#include "keyboard_rgb.h"

#include <QTimer>
#include <QWidget>

#include <array>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSlider;

/// Settings page for the 4-zone RGB keyboard. Changes are applied live.
class KeyboardRgbWidget : public QWidget {
    Q_OBJECT
public:
    explicit KeyboardRgbWidget(QWidget *parent = nullptr);

    [[nodiscard]] bool isAvailable() const { return available; }
    /// Re-sends the saved lighting (e.g. after resume or on start).
    void restore();

private:
    KeyboardRgb rgb;
    KeyboardRgb::State state;
    bool available = false;
    bool loading = false;
    QTimer applyTimer;

    QLabel *statusLabel = nullptr;
    QComboBox *effectCombo = nullptr;
    QSlider *brightnessSlider = nullptr;
    QLabel *brightnessValue = nullptr;
    QSlider *speedSlider = nullptr;
    QLabel *speedValue = nullptr;
    QComboBox *directionCombo = nullptr;
    QCheckBox *sameColorCheck = nullptr;
    std::array<QPushButton *, KeyboardRgb::zoneCount> colorButtons{};
    QCheckBox *restoreCheck = nullptr;
    QPushButton *saveButton = nullptr;

    void buildUi();
    void loadToUi();
    void updateEnabledState();
    void updateColorButton(int zone);
    void pickColor(int zone);
    void setAllColors(const QColor &color);
    void onChanged();
    void applyNow();
};

#endif // KEYBOARD_RGB_WIDGET_H
