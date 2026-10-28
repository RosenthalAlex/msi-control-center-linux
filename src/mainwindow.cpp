/* Copyright (C) 2022  Dmitry Serov
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

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "operate.h"
#include "appstyle.h"
#include "gpu_widget.h"
#include "headerbar.h"
#include "keyboard_rgb_widget.h"
#include "settings.h"
#include "sysinfo.h"
#include <QTimer>
#include <QMessageBox>
#include <QStandardPaths>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QWindow>

Operate operate;
PowerMonitor powerMonitor;

bool isActive = false;
bool isUpdateDataError = false;

QTimer *realtimeUpdateTimer = new QTimer;

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    // clicked = user's choice (saved); toggled = keep the page in sync
    connect(ui->advancedFanControlCheckBox, &QCheckBox::clicked, this, &MainWindow::setFanModeAdvanced);
    connect(ui->advancedFanControlCheckBox, &QCheckBox::toggled, this, [this](bool on) {
        ui->fanControlTabWidget->setEnabled(on);
        if (!on) {
            ui->fanSpeedResetButton->setEnabled(false);
            ui->fanSpeedApplyButton->setEnabled(false);
        } else {
            checkFanSettingsChanged();
        }
    });

    connect(ui->fan1Speed1Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed1Label->setText(QString("%1%").arg(ui->fan1Speed1Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed2Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed2Label->setText(QString("%1%").arg(ui->fan1Speed2Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed3Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed3Label->setText(QString("%1%").arg(ui->fan1Speed3Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed4Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed4Label->setText(QString("%1%").arg(ui->fan1Speed4Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed5Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed5Label->setText(QString("%1%").arg(ui->fan1Speed5Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed6Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed6Label->setText(QString("%1%").arg(ui->fan1Speed6Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed7Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan1Speed7Label->setText(QString("%1%").arg(ui->fan1Speed7Slider->value()));
        checkFanSettingsChanged();
    });

    connect(ui->fan1Speed2TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        ui->fan1Speed1TempLabel->setText(QString("< %1 °C").arg(ui->fan1Speed2TempSpinBox->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed3TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed4TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed5TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed6TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan1Speed7TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });

    connect(ui->fan2Speed1Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed1Label->setText(QString("%1%").arg(ui->fan2Speed1Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed2Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed2Label->setText(QString("%1%").arg(ui->fan2Speed2Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed3Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed3Label->setText(QString("%1%").arg(ui->fan2Speed3Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed4Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed4Label->setText(QString("%1%").arg(ui->fan2Speed4Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed5Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed5Label->setText(QString("%1%").arg(ui->fan2Speed5Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed6Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed6Label->setText(QString("%1%").arg(ui->fan2Speed6Slider->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed7Slider, &QSlider::valueChanged, this, [this]() {
        ui->fan2Speed7Label->setText(QString("%1%").arg(ui->fan2Speed7Slider->value()));
        checkFanSettingsChanged();
    });

    connect(ui->fan2Speed2TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        ui->fan2Speed1TempLabel->setText(QString("< %1 °C").arg(ui->fan2Speed2TempSpinBox->value()));
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed3TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed4TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed5TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed6TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });
    connect(ui->fan2Speed7TempSpinBox, &QSpinBox::textChanged, this, [this]() {
        checkFanSettingsChanged();
    });

    connect(ui->fanSpeedResetButton, &QPushButton::clicked, this, &MainWindow::updateFanSpeedSettings);
    connect(ui->fanSpeedApplyButton, &QPushButton::clicked, this, &MainWindow::setFanSpeedSettings);
    connect(&powerMonitor, &PowerMonitor::currentChargerState, this, &MainWindow::on_ChargerStateChange);
    connect(&powerMonitor, &PowerMonitor::currentPowerProfile, this, &MainWindow::on_PowerProfileChange);

    connect(qApp, &QGuiApplication::saveStateRequest, this, &MainWindow::saveStateRequest);

    MainWindow::setWindowIcon(QIcon(":/images/AppIcon"));
    Settings s;
    if (s.isValueExist("MainWindow/Width") && s.isValueExist("MainWindow/Height"))
        MainWindow::resize(s.getValue("MainWindow/Width").toInt(), s.getValue("MainWindow/Height").toInt());

    createTrayIcon();

    ui->tabWidget->tabBar()->setExpanding(true);
    // Disable debug tab
    ui->tabWidget->setTabVisible(ui->tabWidget->indexOf(ui->debugTab), false);
    createExtraWidgets();
    setupModernLayout();
    setTabsEnabled(false);

    if (!operate.isMsiEcLoaded()) {
        QMessageBox::critical(nullptr, this->windowTitle(), tr("The msi-ec module is not loaded/installed.\n"
                                                               "Check the <About> page for more info."));
    }

    if (!operate.isEcSysModuleLoaded() && !operate.loadEcSysModule())
        QMessageBox::critical(nullptr, this->windowTitle(), tr("The ec_sys module couldn't be detected, it might be required to control the fans."));



    if(operate.updateEcData())
        updateData();

    connect(realtimeUpdateTimer, &QTimer::timeout, this, &MainWindow::realtimeUpdate);
    setUpdateInterval(1000);

    // Timer to detect sleep and reapply Advanced Mode Fan if necessary
    connect(&timerSleepWatcher, &QTimer::timeout, this, &MainWindow::timerSleepTimeout);
    timerSleepWatcher.setInterval(10 * 1000);
    timerSleepWatcher.start();

    ui->QtVersionValue->setText(QT_VERSION_STR);
    ui->versionValueLabel->setText(MControlCenter_VERSION);
    ui->autoAcDcProfilesGroupBox->setChecked(s.getValue("Settings/autoAcDcProfilesState").toBool());
    ui->userModeOnBatteryComboBox->setCurrentIndex(s.getValue("Settings/UserModeOnBattery").toInt());
    ui->userModeOnChargerComboBox->setCurrentIndex(s.getValue("Settings/UserModeOnCharger").toInt());
    ui->autoPPDCheckBox->setChecked(s.getValue("Settings/autoPPDstate").toBool());
    ui->autoPPDCheckBox->setToolTip(tr("Switches the mode when the system power profile changes "
                                       "(e.g. from the GNOME/KDE power menu). "
                                       "Turns off automatic switching on charger/battery."));
    ui->autoAcDcProfilesGroupBox->setToolTip(tr("Switches the mode when the charger is plugged in or removed. "
                                                "Turns off following the system power profile."));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setUpdateDataError(bool error) {
    isUpdateDataError = error;
}

void MainWindow::setTabsEnabled(bool enabled) {
    ui->overviewTab->setEnabled(enabled);
    ui->modeFormWidget->setEnabled(enabled);
    ui->batteryTab->setEnabled(enabled);
    ui->fanControlTab->setEnabled(enabled);
    ui->keyboardTab->setEnabled(enabled);
    ui->debugTab->setEnabled(enabled);

    if (modeTrayMenu)
        modeTrayMenu->setEnabled(enabled);
    if (batteryTrayMenu)
        batteryTrayMenu->setEnabled(enabled);
}

void MainWindow::startRealtimeUpdate() const {
    realtimeUpdateTimer->start();
}

void MainWindow::stopRealtimeUpdate() const {
    realtimeUpdateTimer->stop();
}

void MainWindow::setUpdateInterval(int msec) const {
    realtimeUpdateTimer->setInterval(msec);
}

void MainWindow::realtimeUpdate() {
    operate.updateEcDataAsync();
    updateData();
}

void MainWindow::updateData() {
    if (!isUpdateDataError && (operate.isMsiEcLoaded() || operate.isEcSysModuleLoaded())) {
        if (!isActive) {
            operate.doProbe();
            setTabsEnabled(true);
            loadConfigs();
            isActive = true;
        }
        updateBatteryCharge();
        updateChargingStatus();
        updateCpuTemp();
        updateGpuTemp();
        updateFanMode();
        updateFan1Speed();
        updateFan2Speed();
        updateKeyboardBrightness();
        updateWebCamState();
        updateSystemInfo();

        if (operate.isMsiEcLoaded()) {
            ui->MsiEcStatusLabel->setText(tr("Loaded"));
        } else {
            ui->MsiEcStatusLabel->setText(tr("Fallback: Only ec_sys is loaded"));
        }
    } else {
        setTabsEnabled(false);
        ui->MsiEcStatusLabel->setText(tr("Failed to load both msi-ec/ec_sys"));
        isActive = false;
    }
}

void MainWindow::loadConfigs() {
    ui->ecVersionValueLabel->setText(QString::fromStdString(operate.getEcVersion()));
    ui->ecBuildValueLabel->setText(QString::fromStdString(operate.getEcBuild()));

    operate.loadSettings();
    updateUserMode();
    updateCoolerBoostState();

    if (operate.isBatteryThresholdSupport()) {
        updateBatteryThreshold();
    } else {
        ui->batteryTab->setEnabled(false);
        if (batteryTrayMenu)
            batteryTrayMenu->setEnabled(false);
    }

    updateFanSpeedSettings();

    if (operate.isKeyboardBacklightModeSupport()) {
        updateKeyboardBacklightMode();
    } else {
        ui->keyboardBacklightModeComboBox->setEnabled(false);
    }

    if (operate.isKeyboardBacklightSupport()) {
        updateKeyboardBrightness();
    } else {
        ui->keyboardBrightnessSlider->setEnabled(false);
        // Keep the tab if the EC still offers the backlight timeout setting
        if (!operate.isKeyboardBacklightModeSupport())
            ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->keyboardTab));
    }

    if (operate.isUsbPowerShareSupport()) {
        updateUsbPowerShareState();
    } else {
        ui->usbPowerShareCheckBox->setEnabled(false);
    }

    if (operate.isWebCamOffSupport()) {
        updateWebCamState();
    } else {
        ui->webCamCheckBox->setEnabled(false);
    }

    updateFnSuperSwapState();
    rebuildSidebar();
}

QString MainWindow::intToQString(int value) const {
    if (value < 0)
        return "-";
    return QString::number(value);
}

void MainWindow::updateBatteryCharge() {
    ui->BatteryChargeValueLabel->setText(intToQString(operate.getBatteryCharge()) + " %");
}

void MainWindow::updateBatteryThreshold() {
    if (operate.updateEcData()) {
        int batteryThreshold = operate.getBatteryThreshold();
        if (batteryThreshold == 0)
            ui->batteryThresholdValueLabel->setText("100 %");
        else
            ui->batteryThresholdValueLabel->setText(QString::number(batteryThreshold) + " %");

        switch (batteryThreshold) {
            case 0:
            case 100:
                ui->bestMobilityRadioButton->click();
                batteryThreshold = 100;
                break;
            case 60:
                ui->bestBatteryRadioButton->click();
                break;
            case 80:
                ui->balancedBatteryRadioButton->click();
                break;
            default:
                ui->customBatteryThresholdRadioButton->click();
                ui->customBatteryApplyButton->setEnabled(
                        batteryThreshold != ui->customBatteryThresholdSpinBox->value());
                break;
        }

        ui->customBatteryThresholdSpinBox->setValue(batteryThreshold);
    }
}

void MainWindow::updateChargingStatus() {
    QString chargingStatus;
    switch (operate.getChargingStatus()) {
        case charging_state::battery_charging:
            chargingStatus = tr("Charging");
            break;
        case charging_state::battery_discharging:
            chargingStatus = tr("Discharging");
            break;
        case charging_state::battery_not_charging:
            chargingStatus = tr("Not charging");
            break;
        case charging_state::battery_fully_charged:
            chargingStatus = tr("Fully charged");
            break;
        default:
            chargingStatus = tr("Unknown");
            break;
    }
    ui->chargingStatusValueLabel->setText(chargingStatus);
}

void MainWindow::updateCpuTemp() {
    ui->cpuTempValueLabel->setText(intToQString(operate.getCpuTemp()) + " °C");
}

void MainWindow::updateGpuTemp() {
    std::optional<int> temp = operate.getGpuTemp();
    if (temp.has_value()) {
        ui->gpuTempValueLabel->setVisible(true);
        ui->gpuTempLabel->setVisible(true);
        if (temp.value() != 0)
            ui->gpuTempValueLabel->setText(intToQString(temp.value()) + " °C");
        else
            ui->gpuTempValueLabel->setText(tr("OFF"));
    } else {
        ui->gpuTempValueLabel->setVisible(false);
        ui->gpuTempLabel->setVisible(false);
    }
}

void MainWindow::updateFan1Speed() {
    int speed = operate.getFan1Speed();
    if (speed != 0)
        ui->fan1ValueLabel->setText(intToQString(speed) + " " + tr("rpm"));
    else
        ui->fan1ValueLabel->setText(tr("OFF"));
}

void MainWindow::updateFan2Speed() {
    std::optional<int> speed = operate.getFan2Speed();
    if (speed.has_value()) {
        ui->fan2ValueLabel->setVisible(true);
        ui->gpuFanLabel->setVisible(true);
        if (speed.value() != 0)
            ui->fan2ValueLabel->setText(intToQString(speed.value()) + " " + tr("rpm"));
        else
            ui->fan2ValueLabel->setText(tr("OFF"));
    } else {
        ui->fan2ValueLabel->setVisible(false);
        ui->gpuFanLabel->setVisible(false);
        ui->fanControlTabWidget->setTabEnabled(1, false);
    }
}

void MainWindow::activate() {
    showNormal();
    raise();
    activateWindow();
}

QString MainWindow::autostartFilePath() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart/mcontrolcenter.desktop";
}

bool MainWindow::isAutostartEnabled() {
    return QFile::exists(autostartFilePath());
}

void MainWindow::setAutostartEnabled(bool enabled) {
    const QString path = autostartFilePath();
    if (enabled) {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write("[Desktop Entry]\n"
                       "Type=Application\n"
                       "Name=MControlCenter\n"
                       "Comment=Restore MSI laptop settings at login\n"
                       "Exec=mcontrolcenter --background\n"
                       "Icon=mcontrolcenter\n"
                       "Terminal=false\n"
                       "X-GNOME-Autostart-enabled=true\n"
                       "X-GNOME-Autostart-Delay=3\n");
        } else {
            QMessageBox::warning(this, windowTitle(), tr("Could not create %1").arg(path));
        }
    } else {
        QFile::remove(path);
    }
    const bool on = isAutostartEnabled();
    const QSignalBlocker b1(autostartCheckBox);
    const QSignalBlocker b2(autostartAction);
    autostartCheckBox->setChecked(on);
    autostartAction->setChecked(on);
}

void MainWindow::createExtraWidgets() {
    // Start at login
    autostartCheckBox = new QCheckBox(tr("Start automatically at login (in the background)"), this);
    autostartCheckBox->setToolTip(tr("Restores your mode, fan curves, charge limit and keyboard lighting "
                                     "after every login and keeps automatic switching running."));
    autostartCheckBox->setChecked(isAutostartEnabled());
    ui->formLayout_5->addRow(autostartCheckBox);
    connect(autostartCheckBox, &QCheckBox::toggled, this, &MainWindow::setAutostartEnabled);
    autostartAction->setChecked(isAutostartEnabled());
    connect(autostartAction, &QAction::toggled, this, &MainWindow::setAutostartEnabled);

    // Overview: discrete GPU power state and battery power
    const int row = ui->gridLayout_8->rowCount();
    dGpuLabel = new QLabel(tr("dGPU:"), this);
    dGpuValueLabel = new QLabel("-", this);
    dGpuValueLabel->setToolTip(tr("Power state of the discrete graphics card. "
                                  "\"Sleeping\" saves battery."));
    powerLabel = new QLabel(tr("Power:"), this);
    powerValueLabel = new QLabel("-", this);
    ui->gridLayout_8->addWidget(dGpuLabel, row, 1);
    ui->gridLayout_8->addWidget(dGpuValueLabel, row, 2);
    ui->gridLayout_8->addWidget(powerLabel, row + 1, 1);
    ui->gridLayout_8->addWidget(powerValueLabel, row + 1, 2);

    // Battery tab: health information
    auto *healthBox = new QGroupBox(tr("Battery health"), ui->batteryTab);
    auto *healthForm = new QFormLayout(healthBox);
    batteryHealthValueLabel = new QLabel("-", healthBox);
    batteryCapacityValueLabel = new QLabel("-", healthBox);
    batteryCyclesValueLabel = new QLabel("-", healthBox);
    batteryModelValueLabel = new QLabel("-", healthBox);
    healthForm->addRow(tr("Health:"), batteryHealthValueLabel);
    healthForm->addRow(tr("Capacity:"), batteryCapacityValueLabel);
    healthForm->addRow(tr("Charge cycles:"), batteryCyclesValueLabel);
    healthForm->addRow(tr("Battery:"), batteryModelValueLabel);
    ui->gridLayout_15->removeItem(ui->verticalSpacer_4);
    ui->gridLayout_15->addWidget(healthBox, 4, 0);
    ui->gridLayout_15->addItem(ui->verticalSpacer_4, 5, 0);

    // GPU mode (hybrid / dGPU off)
    // (the dGPU is missing from sysfs while turned off, so ask the helper)
    if (auto *gpuWidget = new GpuWidget(this); gpuWidget->isRelevant()) {
        auto *gpuScroll = new QScrollArea(this);
        gpuScroll->setWidgetResizable(true);
        gpuScroll->setFrameShape(QFrame::NoFrame);
        gpuScroll->setWidget(gpuWidget);
        ui->tabWidget->insertTab(ui->tabWidget->indexOf(ui->fanControlTab) + 1, gpuScroll, tr("GPU"));
    } else {
        delete gpuWidget;
    }

    // RGB keyboard (separate USB controller, not part of the EC)
    keyboardRgbWidget = new KeyboardRgbWidget(this);
    if (keyboardRgbWidget->isAvailable()) {
        auto *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidget(keyboardRgbWidget);
        ui->tabWidget->insertTab(ui->tabWidget->indexOf(ui->keyboardTab) + 1, scroll, tr("RGB Lighting"));
        keyboardRgbWidget->restore();
    } else {
        delete keyboardRgbWidget;
        keyboardRgbWidget = nullptr;
    }
}

void MainWindow::updateSystemInfo() {
    if (auto gpu = SysInfo::discreteGpu()) {
        QString state;
        if (gpu->runtimeStatus == "active")
            state = tr("Active");
        else if (gpu->runtimeStatus == "suspended")
            state = tr("Sleeping");
        else
            state = gpu->runtimeStatus;
        if (gpu->isPrimary)
            state += " " + tr("(discrete GPU only mode)");
        dGpuValueLabel->setText(state);
        dGpuValueLabel->setToolTip(gpu->name);
        dGpuLabel->setVisible(true);
        dGpuValueLabel->setVisible(true);
    } else {
        dGpuLabel->setVisible(false);
        dGpuValueLabel->setVisible(false);
    }

    const SysInfo::BatteryInfo bat = SysInfo::battery();
    if (bat.powerWatts && *bat.powerWatts > 0.05) {
        QString text = QString("%1 W").arg(*bat.powerWatts, 0, 'f', 1);
        if (bat.status == "Discharging")
            text += " " + tr("(from battery)");
        else if (bat.status == "Charging")
            text += " " + tr("(charging)");
        powerValueLabel->setText(text);
    }
    else
        powerValueLabel->setText("-");

    if (!bat.present)
        return;
    batteryHealthValueLabel->setText(bat.healthPercent ? QString("%1 %").arg(*bat.healthPercent) : "-");
    if (bat.fullWh && bat.designWh)
        batteryCapacityValueLabel->setText(tr("%1 Wh of %2 Wh (design)")
                                           .arg(*bat.fullWh, 0, 'f', 1).arg(*bat.designWh, 0, 'f', 1));
    batteryCyclesValueLabel->setText(bat.cycleCount ? QString::number(*bat.cycleCount) : tr("Not reported"));
    QString model = QStringList{bat.manufacturer, bat.model}.join(' ').trimmed();
    batteryModelValueLabel->setText(model.isEmpty() ? bat.name : model);
}

void MainWindow::updateKeyboardBacklightMode() {
    ui->keyboardBacklightModeComboBox->setCurrentIndex(operate.getKeyboardBacklightMode());
}

void MainWindow::updateKeyboardBrightness() const {
    ui->keyboardBrightnessSlider->setSliderPosition(operate.getKeyboardBrightness());
}

void MainWindow::updateUsbPowerShareState() {
    ui->usbPowerShareCheckBox->setChecked(operate.getUsbPowerShareState());
}

void MainWindow::updateWebCamState() const {
    ui->webCamCheckBox->setChecked(operate.getWebCamState());
}

void MainWindow::updateFnSuperSwapState() {
    ui->fnSuperSwapCheckBox->setChecked(operate.getFnSuperSwapState());
}

void MainWindow::updateCoolerBoostState() const {
    ui->coolerBoostCheckBox->setChecked(operate.getCoolerBoostState());
    coolerBoostAction->setChecked(operate.getCoolerBoostState());
}

void MainWindow::updateUserMode() {
    if (operate.updateEcData()) {
        switch (operate.getUserMode()) {
            case user_mode::balanced_mode:
                ui->balancedModeRadioButton->click();
                balancedMode->setChecked(true);
                break;
            case user_mode::performance_mode:
                ui->highPerformanceModeRadioButton->click();
                highPerformanceMode->setChecked(true);
                break;
            case user_mode::silent_mode:
                ui->silentModeRadioButton->click();
                silentMode->setChecked(true);
                break;
            case user_mode::super_battery_mode:
                ui->superBatteryModeRadioButton->click();
                superBatteryMode->setChecked(true);
                break;
            case user_mode::unknown_mode:
                //fall to default, happens on some models after booting
            default:
                if (!operate.isMsiEcLoaded()){
                    ui->modeFormWidget->setDisabled(true);
                    if (modeTrayMenu)
                        modeTrayMenu->setDisabled(true);
                } else {
                    ui->superBatteryModeRadioButton->setChecked(false);
                    ui->silentModeRadioButton->setChecked(false);
                    ui->balancedModeRadioButton->setChecked(false);
                    ui->highPerformanceModeRadioButton->setChecked(false);
                }
                break;
        }
    }
}

void MainWindow::updateFanMode() {
    QString fanMode;
    switch (operate.getFanMode()) {
        case fan_mode::auto_fan_mode:
            fanMode = tr("Auto");
            break;
        case fan_mode::silent_fan_mode:
            fanMode = tr("Silent");
            break;
        case fan_mode::basic_fan_mode:
            fanMode = tr("Basic");
            break;
        case fan_mode::advanced_fan_mode:
            fanMode = tr("Advanced");
            break;
        default:
            fanMode = tr("Unknown");
            break;
    }
    ui->fanModeValueLabel->setText(fanMode);
}

void MainWindow::updateFanSpeedSettings() {
    // Show the user's choice, not a mode change that may have reset the EC.
    ui->advancedFanControlCheckBox->setChecked(operate.isCustomFanCurveWanted() ||
                                               operate.getFanMode() == fan_mode::advanced_fan_mode);

    QVector fan1SpeedSettings = operate.getFan1SpeedSettings();
    QVector fan1TempSettings = operate.getFan1TempSettings();
    QVector fan2SpeedSettings = operate.getFan2SpeedSettings();
    QVector fan2TempSettings = operate.getFan2TempSettings();

    ui->fan1Speed1Slider->setValue(fan1SpeedSettings[0]);
    ui->fan1Speed2Slider->setValue(fan1SpeedSettings[1]);
    ui->fan1Speed3Slider->setValue(fan1SpeedSettings[2]);
    ui->fan1Speed4Slider->setValue(fan1SpeedSettings[3]);
    ui->fan1Speed5Slider->setValue(fan1SpeedSettings[4]);
    ui->fan1Speed6Slider->setValue(fan1SpeedSettings[5]);
    ui->fan1Speed7Slider->setValue(fan1SpeedSettings[6]);

    ui->fan1Speed1TempLabel->setText(QString("< %1 °C").arg(fan1TempSettings[0]));
    ui->fan1Speed2TempSpinBox->setValue(fan1TempSettings[0]);
    ui->fan1Speed3TempSpinBox->setValue(fan1TempSettings[1]);
    ui->fan1Speed4TempSpinBox->setValue(fan1TempSettings[2]);
    ui->fan1Speed5TempSpinBox->setValue(fan1TempSettings[3]);
    ui->fan1Speed6TempSpinBox->setValue(fan1TempSettings[4]);
    ui->fan1Speed7TempSpinBox->setValue(fan1TempSettings[5]);

    ui->fan2Speed1Slider->setValue(fan2SpeedSettings[0]);
    ui->fan2Speed2Slider->setValue(fan2SpeedSettings[1]);
    ui->fan2Speed3Slider->setValue(fan2SpeedSettings[2]);
    ui->fan2Speed4Slider->setValue(fan2SpeedSettings[3]);
    ui->fan2Speed5Slider->setValue(fan2SpeedSettings[4]);
    ui->fan2Speed6Slider->setValue(fan2SpeedSettings[5]);
    ui->fan2Speed7Slider->setValue(fan2SpeedSettings[6]);

    ui->fan2Speed1TempLabel->setText(QString("< %1 °C").arg(fan2TempSettings[0]));
    ui->fan2Speed2TempSpinBox->setValue(fan2TempSettings[0]);
    ui->fan2Speed3TempSpinBox->setValue(fan2TempSettings[1]);
    ui->fan2Speed4TempSpinBox->setValue(fan2TempSettings[2]);
    ui->fan2Speed5TempSpinBox->setValue(fan2TempSettings[3]);
    ui->fan2Speed6TempSpinBox->setValue(fan2TempSettings[4]);
    ui->fan2Speed7TempSpinBox->setValue(fan2TempSettings[5]);

    checkFanSettingsChanged();
}

void MainWindow::setBestMobility() {
    operate.setBatteryThreshold(100);
    updateBatteryThreshold();
}

void MainWindow::setBalancedBattery() {
    operate.setBatteryThreshold(80);
    updateBatteryThreshold();
}

void MainWindow::setBestBattery() {
    operate.setBatteryThreshold(60);
    updateBatteryThreshold();
}

void MainWindow::setHighPerformanceMode() {
    operate.setUserMode(user_mode::performance_mode);
    updateUserMode();
}

void MainWindow::setBalancedMode() {
    operate.setUserMode(user_mode::balanced_mode);
    updateUserMode();
}

void MainWindow::setSilentMode() {
    operate.setUserMode(user_mode::silent_mode);
    updateUserMode();
}

void MainWindow::setSuperBatteryMode() {
    operate.setUserMode(user_mode::super_battery_mode);
    updateUserMode();
}

void MainWindow::setCoolerBoostState(bool enabled) const {
    operate.setCoolerBoostState(enabled);
    if (operate.updateEcData())
        updateCoolerBoostState();
}

QVector<int> MainWindow::getFan1SpeedValues() const {
    QVector<int> fan1SpeedSettings;

    fan1SpeedSettings.push_back(ui->fan1Speed1Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed2Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed3Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed4Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed5Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed6Slider->value());
    fan1SpeedSettings.push_back(ui->fan1Speed7Slider->value());

    return fan1SpeedSettings;
}

QVector<int> MainWindow::getFan2SpeedValues() const {
    QVector<int> fan2SpeedSettings;

    fan2SpeedSettings.push_back(ui->fan2Speed1Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed2Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed3Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed4Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed5Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed6Slider->value());
    fan2SpeedSettings.push_back(ui->fan2Speed7Slider->value());

    return fan2SpeedSettings;
}

QVector<int> MainWindow::getFan1TempValues() const {
    QVector<int> fan1TempSettings;

    fan1TempSettings.push_back(ui->fan1Speed2TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed3TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed4TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed5TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed6TempSpinBox->value());
    fan1TempSettings.push_back(ui->fan1Speed7TempSpinBox->value());

    return fan1TempSettings;
}

QVector<int> MainWindow::getFan2TempValues() const {
    QVector<int> fan2TempSettings;

    fan2TempSettings.push_back(ui->fan2Speed2TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed3TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed4TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed5TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed6TempSpinBox->value());
    fan2TempSettings.push_back(ui->fan2Speed7TempSpinBox->value());

    return fan2TempSettings;
}

void MainWindow::setFanSpeedSettings() {
    operate.setFan1SpeedSettings(getFan1SpeedValues());
    operate.setFan1TempSettings(getFan1TempValues());
    operate.setFan2SpeedSettings(getFan2SpeedValues());
    operate.setFan2TempSettings(getFan2TempValues());
    if (operate.updateEcData())
        checkFanSettingsChanged();
}

void MainWindow::setFanModeAdvanced(bool enabled) const {
    if (enabled) {
        // Store the curve shown on the page so it can be restored later.
        operate.setFan1SpeedSettings(getFan1SpeedValues());
        operate.setFan1TempSettings(getFan1TempValues());
        operate.setFan2SpeedSettings(getFan2SpeedValues());
        operate.setFan2TempSettings(getFan2TempValues());
    }
    operate.setFanModeAdvanced(enabled);
}

void MainWindow::checkFanSettingsChanged() const {
    bool fanSettingChanged = (getFan1SpeedValues() != operate.getFan1SpeedSettings() ||
                              getFan2SpeedValues() != operate.getFan2SpeedSettings() ||
                              getFan1TempValues() != operate.getFan1TempSettings() ||
                              getFan2TempValues() != operate.getFan2TempSettings());
    ui->fanSpeedApplyButton->setEnabled(fanSettingChanged);
    ui->fanSpeedResetButton->setEnabled(fanSettingChanged);
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    startRealtimeUpdate();
    activateWindow();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (trayIcon && trayIcon->isVisible()) {
        stopRealtimeUpdate();
        MainWindow::hide();
        event->ignore();
        return;
    }
    quitApp();
}

void MainWindow::quitApp() const {
    operate.closeHelperApp();
    Settings::setValue("MainWindow/Width", MainWindow::width());
    Settings::setValue("MainWindow/Height", MainWindow::height());
    (void) QCoreApplication::quit();
}

void MainWindow::timerSleepTimeout() {
    qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
    if (timeLastWatcherInterval == 0) {
        timeLastWatcherInterval = timeNow;
        return;
    }
    qint64 msecsSinceTimeout = timeNow - timeLastWatcherInterval;
    timeLastWatcherInterval = timeNow;
    if (msecsSinceTimeout > timerSleepWatcher.interval() + 5000) {
        // Went to sleep for at least 5 seconds
        operate.handleWakeEvent();
        if (keyboardRgbWidget)
            keyboardRgbWidget->restore();
        return;
    }

    // Something (EC, a mode change elsewhere) left advanced fan mode: put the custom curve back.
    if (isActive && operate.ensureCustomFanCurve())
        updateFanMode();
}

void MainWindow::setModeFromSelection(PowerProfile profile) {
    switch (profile) {
    case PowerProfile::Performance:
        setHighPerformanceMode();
        break;
    case PowerProfile::Balanced:
        setBalancedMode();
        break;
    case PowerProfile::Silent:
        setSilentMode();
        break;
    case PowerProfile::PowerSaver:
        setSuperBatteryMode();
        break;
    case PowerProfile::Unknown:
    default:;
    }
}

void MainWindow::on_ChargerStateChange(bool isCharging) {
    if (ui->autoAcDcProfilesGroupBox->isChecked()) {
        Settings s;
        int SelectedModeOnBattery = s.getValue("Settings/UserModeOnBattery").toInt();
        int SelectedModeOnCharger = s.getValue("Settings/UserModeOnCharger").toInt();

        PowerProfile batteryProfile = static_cast<PowerProfile>(SelectedModeOnBattery);
        PowerProfile chargerProfile = static_cast<PowerProfile>(SelectedModeOnCharger);

        if (isCharging) {
            setModeFromSelection(chargerProfile);
        } else {
            setModeFromSelection(batteryProfile);
        }
    }
}

void MainWindow::on_PowerProfileChange(const PowerProfile profile) {
    if (ui->autoPPDCheckBox->isChecked()) {
        switch (profile) {
        case PowerProfile::Performance:
            setHighPerformanceMode();
            ui->highPerformanceModeRadioButton->setChecked(true);
            break;
        case PowerProfile::Balanced:
            setBalancedMode();
            ui->balancedModeRadioButton->setChecked(true);
            break;
        case PowerProfile::PowerSaver:
            setSuperBatteryMode();
            ui->superBatteryModeRadioButton->setChecked(true);
            break;
        case PowerProfile::Unknown:
            default:;
        }
    }
}

void MainWindow::on_bestMobilityRadioButton_toggled(bool checked) {
    if (checked)
        setBestMobility();
}

void MainWindow::on_balancedBatteryRadioButton_toggled(bool checked) {
    if (checked)
        setBalancedBattery();
}

void MainWindow::on_bestBatteryRadioButton_toggled(bool checked) {
    if (checked)
        setBestBattery();
}

void MainWindow::on_customBatteryThresholdRadioButton_toggled(bool checked) {
    if (checked) {
        ui->customBatteryThresholdSpinBox->setEnabled(true);
    } else {
        ui->customBatteryThresholdSpinBox->setEnabled(false);
        ui->customBatteryApplyButton->setEnabled(false);
    }
}

void MainWindow::on_customBatteryThresholdSpinBox_valueChanged(int arg1) {
    ui->customBatteryApplyButton->setEnabled(
            ui->customBatteryThresholdRadioButton->isChecked() && operate.getBatteryThreshold() != arg1);
}

void MainWindow::on_customBatteryApplyButton_clicked() {
    operate.setBatteryThreshold(ui->customBatteryThresholdSpinBox->value());
    updateBatteryThreshold();
}

void MainWindow::on_ReadValueButton_clicked() {
    QString text = ui->addressEdit->displayText();
    int value = operate.getValue(text.toInt());
    ui->ValueSpinBox->setValue(value);
}

void MainWindow::on_WriteValueButton_clicked() const {
    QString text = ui->addressEdit->displayText();
    int address = text.toInt();
    operate.setValue(address, ui->ValueSpinBox->value());
}

void MainWindow::on_usbPowerShareCheckBox_clicked(bool checked) const {
    operate.setUsbPowerShareState(checked);
}

void MainWindow::on_webCamCheckBox_clicked(bool checked) const {
    operate.setWebCamState(checked);
    if (operate.updateEcData()) {
        updateWebCamState();
    }
}

void MainWindow::on_fnSuperSwapCheckBox_clicked(bool checked) const {
    operate.setFnSuperSwapState(checked);
}

void MainWindow::on_coolerBoostCheckBox_clicked(bool checked) const {
    if (operate.getCoolerBoostState() != checked)
        setCoolerBoostState(checked);
}

void MainWindow::on_keyboardBrightnessSlider_valueChanged(int value) const {
    operate.setKeyboardBrightness(value);
    if (operate.updateEcData()) {
        updateKeyboardBrightness();
    }
}

void MainWindow::on_keyboardBacklightModeComboBox_currentIndexChanged(int index) const {
    operate.setKeyboardBacklightMode(index);
}

void MainWindow::on_userModeOnBatteryComboBox_currentIndexChanged(int index) const {
    Settings::setValue("Settings/UserModeOnBattery", index);
    powerMonitor.queryChargerState();
}

void MainWindow::on_userModeOnChargerComboBox_currentIndexChanged(int index) const {
    Settings::setValue("Settings/UserModeOnCharger", index);
    powerMonitor.queryChargerState();
}

void MainWindow::on_autoAcDcProfilesGroupBox_toggled(bool checked) {
    if (checked) {
        if (!powerMonitor.connectToUpower()) {
            QMessageBox::critical(nullptr, this->windowTitle(), tr("Couldn't connect to UPower to get charger status.\n"
                                                                   "Make sure that UPower is installed and running then restart the system."));
            ui->autoAcDcProfilesGroupBox->setChecked(false);
            return;
        }
        // The two automatic modes exclude each other: switch the other one off.
        if (ui->autoPPDCheckBox->isChecked())
            ui->autoPPDCheckBox->setChecked(false);
        powerMonitor.queryChargerState();
    } else {
        powerMonitor.disconnectFromUpower();
    }

    Settings::setValue("Settings/autoAcDcProfilesState", checked);
}

void MainWindow::on_autoPPDCheckBox_toggled(bool checked) {
    if (checked) {
        if (!powerMonitor.connectToPowerProfiles()) {
            QMessageBox::critical(nullptr, this->windowTitle(), tr("Couldn't connect to Power Profiles Daemon.\n"
                                                                   "Make sure that either Power Profiles Daemon or TuneD is installed and restart the system."));
            ui->autoPPDCheckBox->setChecked(false);
            return;
        }
        // The two automatic modes exclude each other: switch the other one off.
        if (ui->autoAcDcProfilesGroupBox->isChecked())
            ui->autoAcDcProfilesGroupBox->setChecked(false);
        powerMonitor.queryPowerProfile();
    } else {
        powerMonitor.disconnectFromPowerProfiles();
    }

    // While following the system profile, modes are picked automatically.
    ui->highPerformanceModeRadioButton->setEnabled(!checked);
    ui->balancedModeRadioButton->setEnabled(!checked);
    ui->silentModeRadioButton->setEnabled(!checked);
    ui->superBatteryModeRadioButton->setEnabled(!checked);

    Settings::setValue("Settings/autoPPDstate", checked);
}

void MainWindow::on_highPerformanceModeRadioButton_toggled(bool checked) {
    if (checked)
        setHighPerformanceMode();
}

void MainWindow::on_balancedModeRadioButton_toggled(bool checked) {
    if (checked)
        setBalancedMode();
}

void MainWindow::on_silentModeRadioButton_toggled(bool checked) {
    if (checked)
        setSilentMode();
}

void MainWindow::on_superBatteryModeRadioButton_toggled(bool checked) {
    if (checked)
        setSuperBatteryMode();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            if (!MainWindow::isHidden()) {
                MainWindow::close();
            } else {
                MainWindow::showNormal();
            }
            break;
        case QSystemTrayIcon::MiddleClick:
            break;
        default:;
    }
}

void MainWindow::createTrayIcon() {
    createActions();

    modeTrayMenu = new QMenu(tr("Mode"));
    modeTrayActions = new QActionGroup(this);
    modeTrayActions->setExclusive(true);


    modeTrayActions->addAction(highPerformanceMode);
    modeTrayActions->addAction(balancedMode);
    modeTrayActions->addAction(silentMode);
    modeTrayActions->addAction(superBatteryMode);

    modeTrayMenu->addAction(highPerformanceMode);
    modeTrayMenu->addAction(balancedMode);
    modeTrayMenu->addAction(silentMode);
    modeTrayMenu->addAction(superBatteryMode);

    fanTrayMenu = new QMenu(tr("Cooling"));
    fanTrayMenu->addAction(coolerBoostAction);

    batteryTrayMenu = new QMenu(tr("Charge limit"));
    batteryTrayMenu->addAction(bestMobilityAction);
    batteryTrayMenu->addAction(balancedBatteryAction);
    batteryTrayMenu->addAction(bestBatteryAction);

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addMenu(modeTrayMenu);
    trayIconMenu->addMenu(fanTrayMenu);
    trayIconMenu->addMenu(batteryTrayMenu);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(autostartAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    auto icon = QIcon(":/images/AppIcon");
    trayIcon->setIcon(icon);
    trayIcon->setToolTip("MSI Control Center");

    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::iconActivated);
}

void MainWindow::createActions() {
    restoreAction = new QAction(tr("Show"), this);
    connect(restoreAction, &QAction::triggered, this, &MainWindow::showNormal);

    highPerformanceMode = new QAction(ui->highPerformanceModeRadioButton->text(), this);
    highPerformanceMode->setCheckable(true);

    balancedMode = new QAction(ui->balancedModeRadioButton->text(), this);
    balancedMode->setCheckable(true);

    silentMode = new QAction(ui->silentModeRadioButton->text(), this);
    silentMode->setCheckable(true);

    superBatteryMode = new QAction(ui->superBatteryModeRadioButton->text(), this);
    superBatteryMode->setCheckable(true);

    connect(highPerformanceMode, &QAction::triggered, this, &MainWindow::setHighPerformanceMode);
    connect(balancedMode, &QAction::triggered, this, &MainWindow::setBalancedMode);
    connect(silentMode, &QAction::triggered, this, &MainWindow::setSilentMode);
    connect(superBatteryMode, &QAction::triggered, this, &MainWindow::setSuperBatteryMode);

    coolerBoostAction = new QAction(tr("Cooler Boost"), this);
    coolerBoostAction->setCheckable(true);

    connect(coolerBoostAction, &QAction::triggered, this, &MainWindow::setCoolerBoostState);

    bestMobilityAction = new QAction(ui->bestMobilityRadioButton->text() + " (100%)", this);
    balancedBatteryAction = new QAction(ui->balancedBatteryRadioButton->text() + " (80%)", this);
    bestBatteryAction = new QAction(ui->bestBatteryRadioButton->text() + " (60%)", this);

    connect(bestMobilityAction, &QAction::triggered, this, &MainWindow::setBestMobility);
    connect(balancedBatteryAction, &QAction::triggered, this, &MainWindow::setBalancedBattery);
    connect(bestBatteryAction, &QAction::triggered, this, &MainWindow::setBestBattery);

    autostartAction = new QAction(tr("Start at login"), this);
    autostartAction->setCheckable(true);

    quitAction = new QAction(tr("Quit"), this);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApp);
}

void MainWindow::saveStateRequest(QSessionManager &sessionManager) {
    sessionManager.setRestartHint(QSessionManager::RestartNever);
}

// ---------------- Modern GTK-like window ----------------

namespace {
    QIcon pageIcon(const QWidget *page, const QString &title) {
        auto pick = [](std::initializer_list<const char *> names) {
            for (const char *n : names) {
                const QIcon icon = QIcon::fromTheme(QString::fromLatin1(n));
                if (!icon.isNull())
                    return icon;
            }
            return QIcon();
        };
        const QString name = page->objectName();
        if (name == "overviewTab")
            return pick({"computer", "utilities-system-monitor", "preferences-system"});
        if (name == "batteryTab")
            return pick({"battery", "battery-full", "gnome-power-manager", "preferences-system-power"});
        if (name == "fanControlTab")
            return pick({"temperature", "sensors-fan", "weather-windy", "applications-system"});
        if (name == "keyboardTab")
            return pick({"input-keyboard", "preferences-desktop-keyboard"});
        if (name == "aboutTab")
            return pick({"help-about", "dialog-information"});
        if (name == "debugTab")
            return pick({"utilities-terminal", "dialog-warning"});
        if (title.contains("GPU"))
            return pick({"video-display", "nvidia", "preferences-desktop-display"});
        return pick({"preferences-desktop-color", "applications-graphics", "color-select", "preferences-desktop"});
    }
}

int MainWindow::shadowMargin() const {
    return (isMaximized() || isFullScreen()) ? 0 : 12;
}

void MainWindow::setupModernLayout() {
    if (qEnvironmentVariableIntValue("MCC_NATIVE_DECORATIONS") == 1)
        return;

    setWindowFlag(Qt::FramelessWindowHint, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    setMinimumSize(920, 680);
    if (width() < 1000 || height() < 680)
        resize(qMax(width(), 1000), qMax(height(), 700));
    const int m = shadowMargin();
    setContentsMargins(m, m, m, m);

    headerBar = new HeaderBar(this);
    setMenuWidget(headerBar);

    // Sidebar + the existing pages (tab bar hidden)
    QWidget *oldCentral = takeCentralWidget();
    auto *central = new QWidget(this);
    central->setObjectName("mccCentral");
    auto *row = new QHBoxLayout(central);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    sidebarPane = new QWidget(central);
    sidebarPane->setObjectName("mccSidebarPane");
    sidebarPane->setFixedWidth(210);
    auto *sideLayout = new QVBoxLayout(sidebarPane);
    sideLayout->setContentsMargins(6, 6, 6, 6);
    sidebar = new QListWidget(sidebarPane);
    sidebar->setObjectName("mccSidebar");
    sidebar->setIconSize(QSize(22, 22));
    sidebar->setFrameShape(QFrame::NoFrame);
    sidebar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sideLayout->addWidget(sidebar);

    row->addWidget(sidebarPane);
    row->addWidget(oldCentral, 1);
    setCentralWidget(central);

    // QTabWidget shows its tab bar again on every tab change, so collapse it instead.
    ui->tabWidget->tabBar()->setFixedHeight(0);
    ui->tabWidget->tabBar()->hide();
    ui->tabWidget->setDocumentMode(true);
    oldCentral->setContentsMargins(12, 8, 12, 12);

    connect(sidebar, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *item) {
        if (!item)
            return;
        auto *page = item->data(Qt::UserRole).value<QWidget *>();
        if (page) {
            ui->tabWidget->setCurrentWidget(page);
            headerBar->setTitle(item->text());
        }
    });

    // App menu in the header bar
    appMenu = new QMenu(this);
    if (autostartAction)
        appMenu->addAction(autostartAction);
    appMenu->addSeparator();
    QAction *about = appMenu->addAction(tr("About"));
    connect(about, &QAction::triggered, this, [this]() {
        for (int i = 0; i < sidebar->count(); i++) {
            if (sidebar->item(i)->data(Qt::UserRole).value<QWidget *>() == ui->aboutTab)
                sidebar->setCurrentRow(i);
        }
    });
    if (quitAction)
        appMenu->addAction(quitAction);
    headerBar->setMenu(appMenu);

    rebuildSidebar();
}

void MainWindow::rebuildSidebar() {
    if (!sidebar)
        return;
    QWidget *current = ui->tabWidget->currentWidget();
    const QSignalBlocker blocker(sidebar);
    sidebar->clear();
    int select = 0;
    for (int i = 0; i < ui->tabWidget->count(); i++) {
        if (!ui->tabWidget->isTabVisible(i))
            continue;
        QWidget *page = ui->tabWidget->widget(i);
        const QString title = ui->tabWidget->tabText(i).remove('&');
        auto *item = new QListWidgetItem(pageIcon(page, title), title, sidebar);
        item->setData(Qt::UserRole, QVariant::fromValue(page));
        item->setSizeHint(QSize(0, 38));
        if (page == current)
            select = sidebar->count() - 1;
    }
    sidebar->setCurrentRow(select);
    if (auto *item = sidebar->currentItem()) {
        ui->tabWidget->setCurrentWidget(item->data(Qt::UserRole).value<QWidget *>());
        headerBar->setTitle(item->text());
    }
}

void MainWindow::paintEvent(QPaintEvent *event) {
    if (!headerBar) {
        QMainWindow::paintEvent(event);
        return;
    }
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const int m = shadowMargin();
    const QRectF panel = QRectF(rect()).adjusted(m, m, -m, -m);
    const qreal radius = m > 0 ? 12.0 : 0.0;

    // Soft shadow around the window
    for (int i = m; i > 0; i--) {
        const qreal a = 0.045 * (1.0 - qreal(i) / (m + 1));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, int(255 * a)));
        p.drawRoundedRect(panel.adjusted(-i, -i + 2, i, i + 2), radius + i, radius + i);
    }

    QPainterPath shape;
    shape.addRoundedRect(panel, radius, radius);
    const bool dark = AppStyle::isDark();
    const QColor window = palette().color(QPalette::Window);
    const QColor side = dark ? window.darker(125) : window.darker(104);

    p.setClipPath(shape);
    p.fillRect(panel, window);
    // Header bar and sidebar share the darker surface, like GNOME apps
    const QRect header = headerBar->geometry();
    p.fillRect(QRectF(panel.left(), header.top(), panel.width(), header.height()), side);
    if (sidebarPane && sidebarPane->isVisible()) {
        const QRect sp(sidebarPane->mapTo(this, QPoint(0, 0)), sidebarPane->size());
        p.fillRect(QRectF(panel.left(), sp.top(), sp.right() - panel.left() + 1, panel.bottom() - sp.top()), side);
        QColor line = palette().color(QPalette::WindowText);
        line.setAlphaF(dark ? 0.10 : 0.12);
        p.setPen(QPen(line, 1));
        p.drawLine(QPointF(sp.right() + 0.5, sp.top()), QPointF(sp.right() + 0.5, panel.bottom()));
    }
    p.setClipping(false);

    // Hairline border
    QColor border = palette().color(QPalette::WindowText);
    border.setAlphaF(dark ? 0.16 : 0.18);
    p.setPen(QPen(border, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(panel.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
}

Qt::Edges MainWindow::edgesAt(const QPoint &pos) const {
    const int m = shadowMargin();
    if (m == 0)
        return {};
    const int grip = m + 4;
    Qt::Edges edges;
    if (pos.x() <= grip) edges |= Qt::LeftEdge;
    if (pos.x() >= width() - grip) edges |= Qt::RightEdge;
    if (pos.y() <= grip) edges |= Qt::TopEdge;
    if (pos.y() >= height() - grip) edges |= Qt::BottomEdge;
    return edges;
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    const Qt::Edges edges = edgesAt(event->position().toPoint());
    if (headerBar && event->button() == Qt::LeftButton && edges && windowHandle()) {
        windowHandle()->startSystemResize(edges);
        event->accept();
        return;
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (headerBar) {
        const Qt::Edges e = edgesAt(event->position().toPoint());
        Qt::CursorShape shape = Qt::ArrowCursor;
        if ((e & Qt::LeftEdge && e & Qt::TopEdge) || (e & Qt::RightEdge && e & Qt::BottomEdge))
            shape = Qt::SizeFDiagCursor;
        else if ((e & Qt::RightEdge && e & Qt::TopEdge) || (e & Qt::LeftEdge && e & Qt::BottomEdge))
            shape = Qt::SizeBDiagCursor;
        else if (e & (Qt::LeftEdge | Qt::RightEdge))
            shape = Qt::SizeHorCursor;
        else if (e & (Qt::TopEdge | Qt::BottomEdge))
            shape = Qt::SizeVerCursor;
        setCursor(shape);
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::changeEvent(QEvent *event) {
    if (headerBar && event->type() == QEvent::WindowStateChange) {
        const int m = shadowMargin();
        setContentsMargins(m, m, m, m);
        update();
    }
    QMainWindow::changeEvent(event);
}
