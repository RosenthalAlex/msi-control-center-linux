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

#include "polkit.h"
#include "service.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QSet>

#include <cstdio>

namespace {
    struct PolkitSubject {
        QString kind;
        QVariantMap details;
    };

    struct PolkitResult {
        bool isAuthorized = false;
        bool isChallenge = false;
        QMap<QString, QString> details;
    };

    QDBusArgument &operator<<(QDBusArgument &arg, const PolkitSubject &s) {
        arg.beginStructure();
        arg << s.kind << s.details;
        arg.endStructure();
        return arg;
    }

    const QDBusArgument &operator>>(const QDBusArgument &arg, PolkitSubject &s) {
        arg.beginStructure();
        arg >> s.kind >> s.details;
        arg.endStructure();
        return arg;
    }

    QDBusArgument &operator<<(QDBusArgument &arg, const PolkitResult &r) {
        arg.beginStructure();
        arg << r.isAuthorized << r.isChallenge << r.details;
        arg.endStructure();
        return arg;
    }

    const QDBusArgument &operator>>(const QDBusArgument &arg, PolkitResult &r) {
        arg.beginStructure();
        arg >> r.isAuthorized >> r.isChallenge >> r.details;
        arg.endStructure();
        return arg;
    }

    QSet<QString> authorizedSenders;
}

Q_DECLARE_METATYPE(PolkitSubject)
Q_DECLARE_METATYPE(PolkitResult)

void Polkit::registerTypes() {
    qDBusRegisterMetaType<QMap<QString, QString>>();
    qDBusRegisterMetaType<PolkitSubject>();
    qDBusRegisterMetaType<PolkitResult>();
}

bool Polkit::checkAuthorization(QObject *adaptorParent) {
    auto *ctx = qobject_cast<DBusContextObject *>(adaptorParent);
    if (!ctx || !ctx->calledFromDBus())
        return true; // internal call

    const QString sender = ctx->message().service();
    if (authorizedSenders.contains(sender))
        return true;

    PolkitSubject subject{"system-bus-name", {{"name", sender}}};
    QDBusMessage call = QDBusMessage::createMethodCall(
            "org.freedesktop.PolicyKit1",
            "/org/freedesktop/PolicyKit1/Authority",
            "org.freedesktop.PolicyKit1.Authority",
            "CheckAuthorization");
    constexpr uint allowUserInteraction = 1;
    call << QVariant::fromValue(subject)
         << QString(POLKIT_ACTION_ID)
         << QVariant::fromValue(QMap<QString, QString>())
         << allowUserInteraction
         << QString();

    QDBusMessage reply = QDBusConnection::systemBus().call(call, QDBus::Block, 120 * 1000);
    bool authorized = false;
    if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
        PolkitResult result;
        reply.arguments().first().value<QDBusArgument>() >> result;
        authorized = result.isAuthorized;
    } else {
        fprintf(stderr, "polkit check failed: %s\n", qPrintable(reply.errorMessage()));
    }

    if (authorized) {
        authorizedSenders.insert(sender);
        return true;
    }

    ctx->sendErrorReply(QDBusError::AccessDenied, "Not authorized to change hardware settings");
    return false;
}
