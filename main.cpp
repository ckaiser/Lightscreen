/*
 * Copyright (C) 2011  Christian Kaiser
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

#ifdef Q_WS_WIN
  #include "tools/qwin7utils/AppUserModel.h"
  #include "tools/qwin7utils/JumpList.h"
  #include "tools/qwin7utils/Taskbar.h"
  using namespace QW7;
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

#ifdef Q_WS_WIN
  // Windows 7 jumplists.
  if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7) {
    AppUserModel::SetCurrentProcessExplicitAppUserModelID("Lightscreen");

    JumpList jumpList("Lightscreen");

    QList<JumpListItem> tasks;
    tasks.append(JumpListItem(application.applicationFilePath(), "--screen"      , QObject::tr("Screen")       , "", "", 0, application.applicationDirPath()));
    tasks.append(JumpListItem(application.applicationFilePath(), "--area"        , QObject::tr("Area")         , "", "", 0, application.applicationDirPath()));
    tasks.append(JumpListItem(application.applicationFilePath(), "--activewindow", QObject::tr("Active Window"), "", "", 0, application.applicationDirPath()));
    tasks.append(JumpListItem(application.applicationFilePath(), "--pickwindow"  , QObject::tr("Pick Window")  , "", "", 0, application.applicationDirPath()));
    tasks.append(JumpListItem());
    tasks.append(JumpListItem(application.applicationFilePath(), "--uploadlast"  , QObject::tr("Upload Last")  , "", "", 0, application.applicationDirPath()));
    tasks.append(JumpListItem(application.applicationFilePath(), "--viewhistory" , QObject::tr("View History") , "", "", 0, application.applicationDirPath()));
    tasks.append(JumpListItem());
    tasks.append(JumpListItem(application.applicationFilePath(), "--folder"      , QObject::tr("Go to Folder") , "", "", 0, application.applicationDirPath()));

    jumpList.Begin();
    jumpList.AddUserTasks(tasks);
    jumpList.Commit();
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

#ifdef Q_WS_WIN
  Taskbar::ReleaseInstance();
#endif

  return result;
}
