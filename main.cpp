/*
 * Copyright (C) 2012  Christian Kaiser
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

#include <QDebug>

#ifdef Q_OS_WIN
    #include <QtWinExtras>
#endif

#include "tools/os.h"
#include <QtSingleApplication>
#include "lightscreenwindow.h"

int main(int argc, char *argv[])
{
  QtSingleApplication application(argc, argv);
  application.setOrganizationName("K");
  application.setApplicationName ("Lightscreen");
  application.setApplicationVersion("2.0");
  application.setQuitOnLastWindowClosed(false);

  if (application.isRunning()) {
    if (application.arguments().size() > 1) {
      QStringList arguments = application.arguments();
      arguments.removeFirst();
      application.sendMessage(arguments.join(" "));
    }
    else {
      application.sendMessage("--wake");
    }

    return 0;
  }

  LightscreenWindow lightscreen;

#ifdef Q_OS_WIN
  // Windows 7 jumplists.
  if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7) {
    QWinJumpList* jumplist = new QWinJumpList(&lightscreen);

    QWinJumpListCategory* screenshotCategory = new QWinJumpListCategory("Screenshot");
    screenshotCategory->setVisible(true);
    screenshotCategory->addLink(os::icon("screen")    , QObject::tr("Screen")       , application.applicationFilePath(), QStringList("--screen"));
    screenshotCategory->addLink(os::icon("area")      , QObject::tr("Area")         , application.applicationFilePath(), QStringList("--area"));
    screenshotCategory->addLink(os::icon("window")    , QObject::tr("Active Window"), application.applicationFilePath(), QStringList("--activewindow"));
    screenshotCategory->addLink(os::icon("pickWindow"), QObject::tr("Pick Window")  , application.applicationFilePath(), QStringList("--activewindow"));

    QWinJumpListCategory* uploadCategory = new QWinJumpListCategory("Upload");
    uploadCategory->setVisible(true);
    uploadCategory->addLink(os::icon("imgur")       , QObject::tr("Upload Last") , application.applicationFilePath(), QStringList("--uploadlast"));
    uploadCategory->addLink(os::icon("view-history"), QObject::tr("View History"), application.applicationFilePath(), QStringList("--viewhistory"));

    QWinJumpListCategory* folderCategory = new QWinJumpListCategory;
    folderCategory->setVisible(true);
    folderCategory->addLink(os::icon("folder"), QObject::tr("Go to Folder") , application.applicationFilePath(), QStringList("--folder"));

    jumplist->addCategory(screenshotCategory);
    jumplist->addCategory(uploadCategory);
    jumplist->addCategory(folderCategory);
}
#endif

  if (application.arguments().size() > 1) {
    foreach (QString argument, application.arguments()) {
      lightscreen.messageReceived(argument);
    }
  }
  else {
    lightscreen.show();
  }

  QObject::connect(&application, SIGNAL(messageReceived(const QString&)), &lightscreen, SLOT(messageReceived(const QString&)));
  QObject::connect(&lightscreen, SIGNAL(finished()), &application, SLOT(quit()));

  int result = application.exec();
  return result;
}
