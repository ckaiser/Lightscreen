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
#include <QDate>
#include <QHttp>

#include <QDebug>

#include "updater.h"
#include "../dialogs/updaterdialog.cpp"

Updater* Updater::mInstance = 0;

Updater::Updater(QObject *parent) :
  QObject(parent)
{
  connect(&mHttp, SIGNAL(done(bool)), this, SLOT(httpDone(bool)));
}

Updater *Updater::instance()
{
  if (!mInstance)
    mInstance = new Updater();

  return mInstance;
}

//

void Updater::check()
{
  if (mHttp.hasPendingRequests())
    return;

  QString platform = "unknown";

#ifdef Q_WS_WIN
  platform = QString("Windows_%1").arg(QSysInfo::windowsVersion());
#else
  platform = "Linux";
#endif

  mHttp.setHost("lightscreen.sourceforge.net");
  mHttp.get("/version?from=" + qApp->applicationVersion() + "&platform=" + platform);
}

void Updater::checkWithFeedback()
{
  UpdaterDialog updaterDialog;
  connect(this, SIGNAL(done(bool)), &updaterDialog, SLOT(updateDone(bool)));

  check();
  updaterDialog.exec();
}

//

void Updater::httpDone(bool error)
{
  Q_UNUSED(error)

  QByteArray data = mHttp.readAll();
  double version  = QString(data).toDouble();

  emit done((version > qApp->applicationVersion().toDouble()));
}
