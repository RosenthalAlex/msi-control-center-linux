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
#include <QApplication>
#include <QDBusConnectionInterface>
#include <QLibraryInfo>
#include <QTranslator>

int main(int argc, char *argv[]) {
    const QString serviceName = "io.github.dmitry_s93.MControlCenter";

    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if (sessionBus.isConnected()) {
        if (sessionBus.interface()->isServiceRegistered(serviceName)) {
            fprintf(stderr, "Another instance of the application is already running\n");
            return 0;
        }
        if (!sessionBus.registerService(serviceName)) {
            fprintf(stderr, "Failed to register D-Bus service\n");
            return 1;
        }
    }

    QApplication a(argc, argv);
    QApplication::setApplicationName("MControlCenter");
    QApplication::setApplicationVersion(MControlCenter_VERSION);
    QApplication::setDesktopFileName("mcontrolcenter");

    // Qt's own strings (e.g. the color dialog)
    QTranslator qtTranslator;
    if (qtTranslator.load(QLocale(), "qtbase", "_", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QApplication::installTranslator(&qtTranslator);

#ifdef MCC_HAS_TRANSLATIONS
    QTranslator translator;
    if (translator.load(QLocale(), "MControlCenter", "_", ":/translations"))
        QApplication::installTranslator(&translator);
#endif

    MainWindow w;

    return QApplication::exec();
}
