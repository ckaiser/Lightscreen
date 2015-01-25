/*
 * Copyright (C) 2014  Christian Kaiser
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
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>


#include "updater.h"
#include "../dialogs/updaterdialog.cpp"

Updater::Updater(QObject *parent) :
  QObject(parent)
{
  connect(&mNetwork, SIGNAL(finished(QNetworkReply*)), this, SLOT(finished(QNetworkReply*)));
}

void Updater::check()
{
  QString platform = "unknown";

#ifdef Q_OS_WIN
  platform = QString("Windows_%1").arg(QSysInfo::windowsVersion());
#else
  platform = "Linux";
#endif

  QNetworkRequest request(QUrl("http://lightscreen.com.ar/version?from=" + qApp->applicationVersion() + "&platform=" + platform));
  mNetwork.get(request);
}

void Updater::checkWithFeedback()
{
  UpdaterDialog updaterDialog;
  connect(this, SIGNAL(done(bool)), &updaterDialog, SLOT(updateDone(bool)));

  check();
  updaterDialog.exec();
}

void Updater::finished(QNetworkReply *reply)
{
  QByteArray data = reply->readAll();
  double version  = QString(data).toDouble();

  emit done((version > qApp->applicationVersion().toDouble()));
}
