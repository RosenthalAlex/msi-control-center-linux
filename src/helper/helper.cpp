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

#include "helper.h"
#include "msi-ec.h"
#include "gpu-switch.h"
#include "keyboard-rgb.h"
#include "polkit.h"
#include "readwrite.h"
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

ReadWrite rw;

void Helper::quit() const {
    if (!Polkit::checkAuthorization(parent()))
        return;
    QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
}

QByteArray Helper::getData() const {
    return rw.readFromFile();
}

void Helper::putValue(const int &address, const int &value) const {
    if (!Polkit::checkAuthorization(parent()))
        return;
    if (address < 0 || address > 255) {
        fprintf(stderr, "tried to write invalid address: %d\n", address);
        return;
    }
    if (value >= 0 && value <= 255)
        rw.writeToFile(address, value);
    else
        fprintf(stderr, "tried to input invalid value. Address: %d, value: %d\n", address, value);
}

bool Helper::isEcSysModuleLoaded() const {
    if (rw.isEcSys()) {
        return true;
    }
    if (rw.isAcpiEc()) {
        fprintf(stderr, "%s\n", qPrintable("The acpi_ec kernel module is loaded"));
        return true;
    }
    fprintf(stderr, "%s\n", qPrintable("The ec_sys kernel module is not loaded"));
    return false;
}

bool Helper::loadEcSysModule() const {
    if (!Polkit::checkAuthorization(parent()))
        return false;
    fprintf(stderr, "%s\n", "Trying to load the ec_sys kernel module");
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    const QString modprobe = QStandardPaths::findExecutable(
            "modprobe", {"/usr/bin", "/usr/sbin", "/bin", "/sbin"});
    if (modprobe.isEmpty()) {
        fprintf(stderr, "modprobe not found\n");
        return false;
    }
    process.start(modprobe, {"ec_sys", "write_support=1"});
    process.waitForFinished(3000);
    if (const QByteArray output = process.readAll(); !output.isEmpty())
        fprintf(stderr, "%s", output.constData());
    return isEcSysModuleLoaded();
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // Run by mcontrolcenter-gpu.service early at boot
    if (QCoreApplication::arguments().contains("--apply-gpu-mode"))
        return GpuSwitch::applySavedModeAtBoot();

    Polkit::registerTypes();

    QDBusConnection bus = QDBusConnection::systemBus();

    DBusContextObject obj;
    auto *helper = new Helper(&obj);
    QObject::connect(&a, &QCoreApplication::aboutToQuit, helper, &Helper::aboutToQuit);
    bus.registerObject("/", &obj);

    DBusContextObject objMsiEc;
    new MsiEc(&objMsiEc);
    bus.registerObject("/msi_ec", &objMsiEc);

    DBusContextObject objKbdRgb;
    new KeyboardRgb(&objKbdRgb);
    bus.registerObject("/keyboard_rgb", &objKbdRgb);

    DBusContextObject objGpu;
    new GpuSwitch(&objGpu);
    bus.registerObject("/gpu", &objGpu);

    if (!bus.registerService(SERVICE_NAME)) {
        fprintf(stderr, "%s\n", qPrintable(bus.lastError().message()));
        return 1;
    }

    return QCoreApplication::exec();
}
