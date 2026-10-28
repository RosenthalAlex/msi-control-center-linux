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

#include "keyboard_rgb_widget.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

KeyboardRgbWidget::KeyboardRgbWidget(QWidget *parent) : QWidget(parent) {
    applyTimer.setSingleShot(true);
    applyTimer.setInterval(150);
    connect(&applyTimer, &QTimer::timeout, this, &KeyboardRgbWidget::applyNow);

    buildUi();

    available = rgb.isAvailable();
    state = KeyboardRgb::loadState();
    loadToUi();

    if (available) {
        statusLabel->setText(tr("Controller: %1").arg(rgb.deviceName()));
    } else {
        statusLabel->setText(tr("No supported RGB keyboard controller was found."));
        setEnabled(false);
    }
}

void KeyboardRgbWidget::buildUi() {
    auto *mainLayout = new QVBoxLayout(this);

    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);
    mainLayout->addWidget(statusLabel);

    auto *effectBox = new QGroupBox(tr("Effect"), this);
    auto *form = new QFormLayout(effectBox);
    form->setVerticalSpacing(10);
    form->setHorizontalSpacing(14);

    effectCombo = new QComboBox(effectBox);
    effectCombo->addItem(tr("Off"), static_cast<int>(KeyboardRgb::Effect::Off));
    effectCombo->addItem(tr("Static"), static_cast<int>(KeyboardRgb::Effect::Static));
    effectCombo->addItem(tr("Breathing"), static_cast<int>(KeyboardRgb::Effect::Breathing));
    effectCombo->addItem(tr("Color cycle"), static_cast<int>(KeyboardRgb::Effect::ColorCycle));
    effectCombo->addItem(tr("Rainbow wave"), static_cast<int>(KeyboardRgb::Effect::Wave));
    form->addRow(tr("Mode:"), effectCombo);

    auto makeSliderRow = [&](QSlider *&slider, QLabel *&value, int min, int max) {
        auto *row = new QHBoxLayout;
        slider = new QSlider(Qt::Horizontal, effectBox);
        slider->setRange(min, max);
        value = new QLabel(effectBox);
        value->setMinimumWidth(48);
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(slider);
        row->addWidget(value);
        return row;
    };
    form->addRow(tr("Brightness:"), makeSliderRow(brightnessSlider, brightnessValue, 0, 100));
    form->addRow(tr("Cycle time:"), makeSliderRow(speedSlider, speedValue, 1, 20));
    speedSlider->setInvertedAppearance(true); // right = faster
    speedSlider->setToolTip(tr("Duration of one animation cycle"));

    directionCombo = new QComboBox(effectBox);
    directionCombo->addItem(tr("Right to left"), 0);
    directionCombo->addItem(tr("Left to right"), 1);
    form->addRow(tr("Direction:"), directionCombo);
    mainLayout->addWidget(effectBox);

    auto *colorBox = new QGroupBox(tr("Zone colors"), this);
    auto *colorLayout = new QGridLayout(colorBox);
    sameColorCheck = new QCheckBox(tr("Same color for all zones"), colorBox);
    colorLayout->addWidget(sameColorCheck, 0, 0, 1, KeyboardRgb::zoneCount);
    for (int i = 0; i < KeyboardRgb::zoneCount; i++) {
        auto *label = new QLabel(tr("Zone %1").arg(i + 1), colorBox);
        label->setAlignment(Qt::AlignCenter);
        colorLayout->addWidget(label, 1, i);
        colorButtons[i] = new QPushButton(colorBox);
        colorButtons[i]->setMinimumHeight(36);
        colorLayout->addWidget(colorButtons[i], 2, i);
        connect(colorButtons[i], &QPushButton::clicked, this, [this, i]() { pickColor(i); });
    }

    // Quick presets
    auto *presetRow = new QHBoxLayout;
    presetRow->addWidget(new QLabel(tr("Presets:"), colorBox));
    const QColor presets[] = {{255, 0, 0}, {255, 96, 0}, {255, 220, 0}, {0, 255, 0},
                              {0, 255, 255}, {0, 64, 255}, {160, 0, 255}, {255, 0, 160},
                              {255, 255, 255}};
    for (const QColor &c : presets) {
        auto *b = new QPushButton(colorBox);
        b->setFixedSize(26, 26);
        b->setToolTip(c.name());
        b->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid palette(mid);"
                                 " border-radius: 4px; }").arg(c.name()));
        connect(b, &QPushButton::clicked, this, [this, c]() { setAllColors(c); });
        presetRow->addWidget(b);
    }
    presetRow->addStretch();
    colorLayout->addLayout(presetRow, 3, 0, 1, KeyboardRgb::zoneCount);
    mainLayout->addWidget(colorBox);

    restoreCheck = new QCheckBox(tr("Restore this lighting when MControlCenter starts and after sleep"), this);
    mainLayout->addWidget(restoreCheck);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->addStretch();
    saveButton = new QPushButton(tr("Save to keyboard memory"), this);
    saveButton->setToolTip(tr("Stores the current lighting in the keyboard controller, "
                              "so it stays after a reboot even without MControlCenter."));
    buttonRow->addWidget(saveButton);
    mainLayout->addLayout(buttonRow);
    mainLayout->addStretch();

    connect(effectCombo, &QComboBox::currentIndexChanged, this, &KeyboardRgbWidget::onChanged);
    connect(brightnessSlider, &QSlider::valueChanged, this, &KeyboardRgbWidget::onChanged);
    connect(speedSlider, &QSlider::valueChanged, this, &KeyboardRgbWidget::onChanged);
    connect(directionCombo, &QComboBox::currentIndexChanged, this, &KeyboardRgbWidget::onChanged);
    connect(sameColorCheck, &QCheckBox::toggled, this, [this](bool same) {
        if (same && !loading)
            setAllColors(state.colors[0]);
        onChanged();
    });
    connect(restoreCheck, &QCheckBox::toggled, this, [this](bool on) {
        state.restoreOnStart = on;
        KeyboardRgb::saveState(state);
    });
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        applyNow();
        if (!rgb.saveToFlash())
            QMessageBox::warning(this, tr("Keyboard lighting"), tr("Saving to the keyboard failed."));
    });
}

void KeyboardRgbWidget::loadToUi() {
    loading = true;
    effectCombo->setCurrentIndex(effectCombo->findData(static_cast<int>(state.effect)));
    brightnessSlider->setValue(state.brightness);
    speedSlider->setValue(state.speedSeconds);
    directionCombo->setCurrentIndex(state.direction);
    sameColorCheck->setChecked(state.sameColorForAllZones);
    restoreCheck->setChecked(state.restoreOnStart);
    for (int i = 0; i < KeyboardRgb::zoneCount; i++)
        updateColorButton(i);
    brightnessValue->setText(QString("%1 %").arg(state.brightness));
    speedValue->setText(tr("%1 s").arg(state.speedSeconds));
    updateEnabledState();
    loading = false;
}

void KeyboardRgbWidget::updateEnabledState() {
    const auto effect = state.effect;
    const bool usesColor = effect == KeyboardRgb::Effect::Static || effect == KeyboardRgb::Effect::Breathing;
    const bool animated = effect != KeyboardRgb::Effect::Static && effect != KeyboardRgb::Effect::Off;
    brightnessSlider->setEnabled(effect != KeyboardRgb::Effect::Off);
    speedSlider->setEnabled(animated);
    directionCombo->setEnabled(effect == KeyboardRgb::Effect::Wave);
    sameColorCheck->setEnabled(usesColor);
    for (int i = 0; i < KeyboardRgb::zoneCount; i++)
        colorButtons[i]->setEnabled(usesColor && (i == 0 || !state.sameColorForAllZones));
}

void KeyboardRgbWidget::updateColorButton(int zone) {
    const QColor &c = state.colors[zone];
    const QString text = c.lightness() > 140 ? "black" : "white";
    colorButtons[zone]->setText(c.name().toUpper());
    colorButtons[zone]->setStyleSheet(
            QString("QPushButton { background-color: %1; color: %2; border: 1px solid palette(mid);"
                    " border-radius: 4px; } QPushButton:disabled { color: palette(mid); }")
                    .arg(c.name(), text));
}

void KeyboardRgbWidget::pickColor(int zone) {
    const QColor color = QColorDialog::getColor(state.colors[zone], this, tr("Zone %1 color").arg(zone + 1));
    if (!color.isValid())
        return;
    if (state.sameColorForAllZones) {
        setAllColors(color);
        return;
    }
    state.colors[zone] = color;
    updateColorButton(zone);
    onChanged();
}

void KeyboardRgbWidget::setAllColors(const QColor &color) {
    for (int i = 0; i < KeyboardRgb::zoneCount; i++) {
        state.colors[i] = color;
        updateColorButton(i);
    }
    // Picking a preset for a non-color effect switches to static color.
    if (state.effect != KeyboardRgb::Effect::Static && state.effect != KeyboardRgb::Effect::Breathing)
        effectCombo->setCurrentIndex(effectCombo->findData(static_cast<int>(KeyboardRgb::Effect::Static)));
    onChanged();
}

void KeyboardRgbWidget::onChanged() {
    if (loading)
        return;
    state.effect = static_cast<KeyboardRgb::Effect>(effectCombo->currentData().toInt());
    state.brightness = brightnessSlider->value();
    state.speedSeconds = speedSlider->value();
    state.direction = directionCombo->currentData().toInt();
    state.sameColorForAllZones = sameColorCheck->isChecked();
    brightnessValue->setText(QString("%1 %").arg(state.brightness));
    speedValue->setText(tr("%1 s").arg(state.speedSeconds));
    updateEnabledState();
    applyTimer.start();
}

void KeyboardRgbWidget::applyNow() {
    applyTimer.stop();
    if (!available)
        return;
    KeyboardRgb::saveState(state);
    if (!rgb.apply(state))
        statusLabel->setText(tr("Could not change the keyboard lighting. Is the helper running "
                                "and are you allowed to change hardware settings?"));
    else
        statusLabel->setText(tr("Controller: %1").arg(rgb.deviceName()));
}

void KeyboardRgbWidget::restore() {
    if (available && state.restoreOnStart)
        rgb.apply(state);
}
