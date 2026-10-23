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

#include <QDBusInterface>
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
    hybridRadio = new QRadioButton(tr("Hybrid: the NVIDIA GPU sleeps and wakes up when an app needs it"), box);
    integratedRadio = new QRadioButton(tr("Integrated: the NVIDIA GPU is turned off completely (longest battery life)"), box);
    boxLayout->addWidget(hybridRadio);
    boxLayout->addWidget(integratedRadio);
    layout->addWidget(box);

    infoLabel = new QLabel(this);
    infoLabel->setWordWrap(true);
    infoLabel->setTextFormat(Qt::RichText);
    layout->addWidget(infoLabel);

    auto *row = new QHBoxLayout;
    row->addStretch();
    retryButton = new QPushButton(tr("Turn off now"), this);
    retryButton->setToolTip(tr("Try again after closing the apps that use the NVIDIA GPU"));
    row->addWidget(retryButton);
    layout->addLayout(row);
    layout->addStretch();

    connect(hybridRadio, &QRadioButton::toggled, this, [this](bool on) {
        if (on && !updating)
            setMode("hybrid");
    });
    connect(integratedRadio, &QRadioButton::toggled, this, [this](bool on) {
        if (on && !updating)
            setMode("integrated");
    });
    connect(retryButton, &QPushButton::clicked, this, [this]() { setMode("integrated"); });

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

bool GpuWidget::isRelevant() const {
    const QString mode = call("getMode");
    return !mode.isEmpty() && mode != "none";
}

void GpuWidget::refresh() {
    updating = true;
    const QString mode = call("getMode");
    const QString saved = call("getSavedMode");
    const auto gpu = SysInfo::discreteGpu();
    const QString name = gpu ? gpu->name : tr("NVIDIA GPU");

    retryButton->setVisible(false);
    infoLabel->clear();

    if (mode.isEmpty()) {
        stateLabel->setText(tr("The helper service is not available."));
        setEnabled(false);
        updating = false;
        return;
    }
    setEnabled(true);

    if (mode == "discrete") {
        stateLabel->setText(tr("<b>Current mode: discrete GPU only.</b> The display is connected to the %1 "
                               "by the MUX switch, so it cannot be turned off.").arg(name));
        infoLabel->setText(tr("To use these modes, switch the GPU mode to <b>Hybrid</b> (MSHybrid) in the "
                              "BIOS setup and restart. On MSI laptops: press <b>Del</b> repeatedly at startup, "
                              "then look for the GPU / Graphics or MUX setting under <i>Advanced</i>."));
        hybridRadio->setEnabled(false);
        integratedRadio->setEnabled(false);
        hybridRadio->setChecked(false);
        integratedRadio->setChecked(false);
    } else if (mode == "none") {
        stateLabel->setText(tr("No discrete GPU found."));
        hybridRadio->setEnabled(false);
        integratedRadio->setEnabled(false);
    } else {
        hybridRadio->setEnabled(true);
        integratedRadio->setEnabled(true);
        if (mode == "integrated") {
            stateLabel->setText(tr("<b>Current mode: integrated.</b> The NVIDIA GPU is off."));
            integratedRadio->setChecked(true);
        } else {
            QString state;
            if (gpu && gpu->runtimeStatus == "suspended")
                state = tr("sleeping");
            else if (gpu)
                state = tr("active");
            stateLabel->setText(tr("<b>Current mode: hybrid.</b> %1 is %2.").arg(name, state));
            if (saved == "integrated") {
                integratedRadio->setChecked(true);
                showPending();
            } else {
                hybridRadio->setChecked(true);
            }
        }
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
}

void GpuWidget::setMode(const QString &mode) {
    QString status;
    if (iface) {
        QDBusReply<QString> reply = iface->call("setMode", mode);
        status = reply.isValid() ? reply.value() : "error: " + reply.error().message();
    }
    if (status.startsWith("error"))
        QMessageBox::warning(this, tr("GPU mode"), tr("Could not change the GPU mode:\n%1").arg(status.mid(7)));

    refresh();

    if (status == "applied" && mode == "hybrid")
        infoLabel->setText(tr("The NVIDIA GPU is back. Apps can use it right away; "
                              "log out and back in if the desktop should also use it."));
}
