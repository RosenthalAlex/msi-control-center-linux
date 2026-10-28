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

#include "gpu_widget.h"
#include "helper/service.h"
#include "sysinfo.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

GpuWidget::GpuWidget(QWidget *parent) : QWidget(parent) {
    if (QDBusConnection::systemBus().isConnected())
        iface = new QDBusInterface(SERVICE_NAME, "/gpu", INTERFACE_NAME_GPU, QDBusConnection::systemBus());

    auto *layout = new QVBoxLayout(this);

    stateLabel = new QLabel(this);
    stateLabel->setWordWrap(true);
    layout->addWidget(stateLabel);

    auto *box = new QGroupBox(tr("GPU mode"), this);
    auto *boxLayout = new QVBoxLayout(box);
    boxLayout->setSpacing(10);
    auto addOption = [&](QRadioButton *&radio, const QString &title, const QString &description) {
        radio = new QRadioButton(title, box);
        QFont f = radio->font();
        f.setWeight(QFont::DemiBold);
        radio->setFont(f);
        auto *desc = new QLabel(description, box);
        desc->setWordWrap(true);
        desc->setIndent(28); // align with the radio text
        QPalette pal = desc->palette();
        QColor c = pal.color(QPalette::WindowText);
        c.setAlphaF(0.65);
        pal.setColor(QPalette::WindowText, c);
        desc->setPalette(pal);
        boxLayout->addWidget(radio);
        boxLayout->addWidget(desc);
        boxLayout->addSpacing(4);
    };
    addOption(hybridRadio, tr("Hybrid"),
              tr("The Intel GPU drives the display; the NVIDIA GPU sleeps and wakes up when an app needs it."));
    addOption(integratedRadio, tr("Integrated"),
              tr("The NVIDIA GPU is turned off completely. Longest battery life."));
    addOption(discreteRadio, tr("Discrete"),
              tr("The display runs directly on the NVIDIA GPU. Best for gaming on the charger. Needs a restart."));
    layout->addWidget(box);

    infoLabel = new QLabel(this);
    infoLabel->setWordWrap(true);
    infoLabel->setTextFormat(Qt::RichText);
    layout->addWidget(infoLabel);

    auto *row = new QHBoxLayout;
    row->addStretch();
    retryButton = new QPushButton(tr("Turn off now"), this);
    retryButton->setToolTip(tr("Try again after closing the apps that use the NVIDIA GPU"));
    restartButton = new QPushButton(tr("Restart now"), this);
    restartButton->setDefault(true);
    row->addWidget(retryButton);
    row->addWidget(restartButton);
    layout->addLayout(row);
    layout->addStretch();

    connect(hybridRadio, &QRadioButton::toggled, this, [this](bool on) { if (on && !updating) choose("hybrid"); });
    connect(integratedRadio, &QRadioButton::toggled, this, [this](bool on) { if (on && !updating) choose("integrated"); });
    connect(discreteRadio, &QRadioButton::toggled, this, [this](bool on) { if (on && !updating) choose("discrete"); });
    connect(retryButton, &QPushButton::clicked, this, [this]() { choose("integrated"); });
    connect(restartButton, &QPushButton::clicked, this, &GpuWidget::restartNow);

    refresh();
}

GpuWidget::~GpuWidget() {
    delete iface;
}

QString GpuWidget::call(const QString &method) const {
    if (!iface)
        return {};
    QDBusReply<QString> reply = iface->call(method);
    return reply.isValid() ? reply.value() : QString();
}

QString GpuWidget::call(const QString &method, const QString &arg) const {
    if (!iface)
        return "error: helper not available";
    QDBusReply<QString> reply = iface->call(method, arg);
    return reply.isValid() ? reply.value() : "error: " + reply.error().message();
}

bool GpuWidget::isRelevant() const {
    const QString mode = call("getMode");
    return !mode.isEmpty() && mode != "none";
}

void GpuWidget::refresh() {
    updating = true;
    const QString mode = call("getMode");          // hybrid / integrated / discrete / none
    const QString saved = call("getSavedMode");    // hybrid / integrated
    const QString muxApplied = call("getMuxApplied");
    const QString muxRequested = call("getMuxRequested");
    const QString muxReason = call("getMuxUnavailableReason");
    const bool muxAvailable = iface && muxReason.isEmpty() && muxApplied != "unknown" && !muxApplied.isEmpty();
    const auto gpu = SysInfo::discreteGpu();
    const QString name = gpu ? gpu->name : tr("NVIDIA GPU");

    retryButton->setVisible(false);
    restartButton->setVisible(false);
    infoLabel->clear();

    if (mode.isEmpty()) {
        stateLabel->setText(tr("The helper service is not available."));
        setEnabled(false);
        updating = false;
        return;
    }
    setEnabled(true);

    const bool runningDiscrete = mode == "discrete";
    const bool muxPending = muxAvailable && muxRequested != muxApplied;

    // Current state
    if (runningDiscrete) {
        stateLabel->setText(tr("<b>Current mode: discrete.</b> The display runs on the %1.").arg(name));
    } else if (mode == "integrated") {
        stateLabel->setText(tr("<b>Current mode: integrated.</b> The NVIDIA GPU is off."));
    } else if (mode == "hybrid") {
        const QString state = gpu && gpu->runtimeStatus == "suspended" ? tr("sleeping") : tr("active");
        stateLabel->setText(tr("<b>Current mode: hybrid.</b> %1 is %2.").arg(name, state));
    } else {
        stateLabel->setText(tr("No discrete GPU found."));
    }

    // Selected (target) mode
    QRadioButton *target = hybridRadio;
    if (muxAvailable && muxRequested == "discrete")
        target = discreteRadio;
    else if (runningDiscrete && !muxPending)
        target = discreteRadio;
    else if (saved == "integrated")
        target = integratedRadio;
    target->setChecked(true);

    // What can be chosen
    const bool canLeaveDiscrete = !runningDiscrete || muxAvailable;
    hybridRadio->setEnabled(mode != "none" && canLeaveDiscrete);
    integratedRadio->setEnabled(mode != "none" && canLeaveDiscrete);
    discreteRadio->setEnabled(muxAvailable || runningDiscrete);
    discreteRadio->setToolTip(muxAvailable ? QString() : tr("Not available: %1").arg(muxReason));

    if (muxPending) {
        const QString targetName = muxRequested == "discrete" ? tr("discrete") : (saved == "integrated" ? tr("integrated") : tr("hybrid"));
        infoLabel->setText(tr("<b>Restart to switch to %1 mode.</b> The firmware switches the display at startup; "
                              "the screen may stay dark for a few seconds.").arg(targetName));
        restartButton->setVisible(true);
    } else if (runningDiscrete && !muxAvailable) {
        infoLabel->setText(tr("The display is connected to the NVIDIA GPU by the MUX switch and this laptop's "
                              "firmware switch cannot be used from Linux (%1). Change the GPU mode in the BIOS "
                              "or in MSI Center on Windows.").arg(muxReason));
    } else if (runningDiscrete) {
        infoLabel->setText(tr("Best performance and lowest latency on the charger. On battery it uses much more "
                              "power than hybrid mode."));
    } else if (saved == "integrated" && mode == "hybrid") {
        showPending();
    }
    updating = false;
}

void GpuWidget::showPending() {
    QDBusReply<QStringList> reply = iface ? iface->call("getBlockingProcesses") : QDBusMessage();
    const QStringList users = reply.isValid() ? reply.value() : QStringList();
    QString text = tr("Integrated mode is saved and will be applied at the next restart, "
                      "before the desktop starts.");
    if (!users.isEmpty())
        text += "<br>" + tr("The NVIDIA GPU is in use right now by: <b>%1</b>.").arg(users.join(", ").toHtmlEscaped());
    infoLabel->setText(text);
    retryButton->setVisible(true);
    restartButton->setVisible(true);
}

void GpuWidget::choose(const QString &mode) {
    const QString mode0 = call("getMode");
    const QString muxRequested = call("getMuxRequested");
    QStringList errors;
    auto run = [&](const QString &method, const QString &arg) {
        const QString r = call(method, arg);
        if (r.startsWith("error"))
            errors << r.mid(7);
        return r;
    };

    if (mode == "discrete") {
        run("setMuxMode", "discrete");
        if (errors.isEmpty())
            run("setMode", "hybrid"); // nothing to remove at boot while the dGPU drives the display
    } else {
        // Leaving discrete (now or as a pending request) needs the MUX back on hybrid.
        if (mode0 == "discrete" || muxRequested == "discrete")
            run("setMuxMode", "hybrid");
        if (errors.isEmpty())
            run("setMode", mode);
    }

    if (!errors.isEmpty())
        QMessageBox::warning(this, tr("GPU mode"), tr("Could not change the GPU mode:\n%1").arg(errors.join("\n")));

    refresh();
    if (errors.isEmpty() && mode == "hybrid" && mode0 == "integrated")
        infoLabel->setText(tr("The NVIDIA GPU is back. Apps can use it right away; "
                              "log out and back in if the desktop should also use it."));
}

void GpuWidget::restartNow() {
    if (QMessageBox::question(this, tr("Restart"), tr("Restart the computer now? Save your work first.")) != QMessageBox::Yes)
        return;
    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.login1", "/org/freedesktop/login1",
                                                      "org.freedesktop.login1.Manager", "Reboot");
    msg << true; // interactive: polkit may ask for a password
    QDBusMessage reply = QDBusConnection::systemBus().call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage)
        QMessageBox::warning(this, tr("Restart"), reply.errorMessage());
}
