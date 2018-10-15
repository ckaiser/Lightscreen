/*
 * Copyright (C) 2017  Christian Kaiser
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <QApplication>
#include <QDesktopWidget>
#include <QLocale>

#ifdef Q_OS_WIN
    #include <QtWinExtras>
#endif

#include <tools/os.h>
#include "tools/SingleApplication/singleapplication.h"

#include <lightscreenwindow.h>

int main(int argc, char *argv[])
{
#ifdef QT_DEBUG
    qSetMessagePattern("%{message} @%{line}[%{function}()]");
#endif

    QApplication::setOrganizationName("K");
    QApplication::setApplicationName("Lightscreen");
    QApplication::setApplicationVersion(APP_VERSION);
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    SingleApplication application(argc, argv);

    application.setQuitOnLastWindowClosed(false);

    LightscreenWindow lightscreen;

#ifdef Q_OS_WIN
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_WINDOWS7) {
        auto jumplist = new QWinJumpList(&lightscreen);

        QColor backgroundColor = qApp->palette("QToolTip").color(QPalette::Background);

        if (QSysInfo::WindowsVersion == QSysInfo::WV_WINDOWS10) {
            // contrast r hard
            backgroundColor = Qt::black;
        }

        auto screenshotCategory = new QWinJumpListCategory(QObject::tr("Screenshot"));
        screenshotCategory->setVisible(true);
        screenshotCategory->addLink(os::icon("screen", backgroundColor), QObject::tr("Screen")         , application.applicationFilePath(), QStringList("--screen"));
        screenshotCategory->addLink(os::icon("area", backgroundColor), QObject::tr("Area")             , application.applicationFilePath(), QStringList("--area"));
        screenshotCategory->addLink(os::icon("pickWindow", backgroundColor), QObject::tr("Pick Window"), application.applicationFilePath(), QStringList("--pickwindow"));

        auto uploadCategory = new QWinJumpListCategory(QObject::tr("Upload"));
        uploadCategory->setVisible(true);
        uploadCategory->addLink(os::icon("imgur", backgroundColor), QObject::tr("Upload Last")        , application.applicationFilePath(), QStringList("--uploadlast"));
        uploadCategory->addLink(os::icon("view-history", backgroundColor), QObject::tr("View History"), application.applicationFilePath(), QStringList("--viewhistory"));

        auto actionsCategory = new QWinJumpListCategory(QObject::tr("Actions"));
        actionsCategory->setVisible(true);
        actionsCategory->addLink(os::icon("configure", backgroundColor), QObject::tr("Options")       , application.applicationFilePath(), QStringList("--options"));
        actionsCategory->addLink(os::icon("folder", backgroundColor), QObject::tr("Go to Folder")     , application.applicationFilePath(), QStringList("--folder"));
        actionsCategory->addLink(os::icon("no.big", backgroundColor), QObject::tr("Quit Lightscreen") , application.applicationFilePath(), QStringList("--quit"));

        jumplist->addCategory(screenshotCategory);
        jumplist->addCategory(uploadCategory);
        jumplist->addCategory(actionsCategory);
    }
#endif

    if (application.arguments().size() > 1) {
        lightscreen.executeArguments(application.arguments());
    } else {
        lightscreen.show();
    }

    QObject::connect(&application, &SingleApplication::instanceArguments, &lightscreen, &LightscreenWindow::executeArguments);
    QObject::connect(&lightscreen, &LightscreenWindow::finished, &application, &SingleApplication::quit);

    return application.exec();
}
