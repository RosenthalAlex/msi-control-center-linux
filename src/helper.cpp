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

#include "helper/service.h"
#include "helper.h"
#include "mainwindow.h"

#include <QCoreApplication>
#include <QDBusReply>

#include <cstdio>

#define EC_SPACE_SIZE 256

QByteArray ecData;

Helper::Helper() {
    if (!QDBusConnection::systemBus().isConnected()) {
        fprintf(stderr, "Cannot connect to the D-Bus system bus\n");
        return;
    }
    iface = new QDBusInterface(SERVICE_NAME, "/", INTERFACE_NAME, QDBusConnection::systemBus());
}

bool Helper::isEcSysModuleLoaded() {
    if (!iface)
        return false;
    if (QDBusReply<bool> reply = iface->call("isEcSysModuleLoaded"); reply.isValid())
        return reply.value();
    printError(iface->lastError());
    return false;
}

bool Helper::loadEcSysModule() {
    if (!iface)
        return false;
    if (QDBusReply<bool> reply = iface->call("loadEcSysModule"); reply.isValid())
        return reply.value();
    printError(iface->lastError());
    return false;
}

bool Helper::updateData() {
    if (!iface)
        return false;
    if (QDBusReply<QByteArray> reply = iface->call("getData"); reply.isValid() &&
                                                               reply.value().size() == EC_SPACE_SIZE) {
        ecData = reply.value();
        return true;
    }
    printError(iface->lastError());
    return false;
}

void Helper::updateDataAsync() {
    if (!iface)
        return;
    QDBusPendingCall async = iface->asyncCall("getData");
    auto *watcher = new QDBusPendingCallWatcher(async, this);

    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, &Helper::callFinishedSlot);
}

void Helper::callFinishedSlot(QDBusPendingCallWatcher *call) {
    QDBusPendingReply<QByteArray> reply = *call;
    if (reply.isError()) {
        printError(reply.error());
        MainWindow::setUpdateDataError(true);
    } else {
        ecData = reply.value();
        MainWindow::setUpdateDataError(false);
    }
    call->deleteLater();
}

std::optional<int> Helper::getOptionalValue(int address) const {
    if (address >= 0 && address < ecData.size())
        return static_cast<BYTE>(ecData[address]);
    return std::nullopt;
}

int Helper::getValue(int address) const {
    return getOptionalValue(address).value_or(-1);
}

QByteArray Helper::getValues(int startAddress, int size) const {
    return ecData.mid(startAddress, size);
}

void Helper::putValue(int address, int value) {
    if (!iface)
        return;
    if (getValue(address) == value)
        return;
    iface->call("putValue", address, value);
    printError(iface->lastError());
}

void Helper::quit() {
    if (!iface)
        return;
    iface->call("quit");
    printError(iface->lastError());
}

void Helper::printError(QDBusError const & error) const {
    if (error.isValid())
        fprintf(stderr, "Call failed: %s\n", qPrintable(error.message()));
}
