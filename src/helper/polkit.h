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

#ifndef POLKIT_H
#define POLKIT_H

#include <QtCore/QObject>
#include <QtDBus/QDBusContext>

/**
 * Object that D-Bus adaptors are attached to. It exposes the D-Bus context of
 * the current call so adaptors can find out who is calling them.
 */
class DBusContextObject : public QObject, public QDBusContext {
    Q_OBJECT
public:
    using QObject::QObject;
};

namespace Polkit {
    void registerTypes();

    /**
     * Returns true when the sender of the current D-Bus call may change hardware
     * settings. Otherwise sends an AccessDenied error reply and returns false.
     * Results are cached per unique bus name (names are never reused on a bus).
     */
    bool checkAuthorization(QObject *adaptorParent);
}

#endif // POLKIT_H
