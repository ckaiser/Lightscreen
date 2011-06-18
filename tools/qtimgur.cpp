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
#include "qtimgur.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>

QtImgur::QtImgur(const QString &APIKey, QObject *parent) : QObject(parent), mAPIKey(APIKey)
{
  mNetworkManager = new QNetworkAccessManager(this);
  connect(mNetworkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(reply(QNetworkReply*)));
}

void QtImgur::upload(const QString &fileName)
{
  QFile file(fileName);

  if (!file.open(QIODevice::ReadOnly)) {
    emit error(fileName, QtImgur::ErrorFile);
    return;
  }

  QByteArray image = file.readAll().toBase64();

  file.close();

  QByteArray data;
  data.append(QString("key=").toUtf8());
  data.append(QUrl::toPercentEncoding(mAPIKey));
  data.append(QString("&image=").toUtf8());
  data.append(QUrl::toPercentEncoding(image));

  QNetworkReply *reply = mNetworkManager->post(QNetworkRequest(QUrl("http://api.imgur.com/2/upload.xml")), data);
  connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(uploadProgress(qint64, qint64)));

  mFiles.insert(reply, fileName);
}

void QtImgur::reply(QNetworkReply *reply)
{
  reply->deleteLater();

  QString fileName = mFiles[reply];
  mFiles.remove(reply);

  if (reply->error() != QNetworkReply::NoError) {
    emit error(fileName, QtImgur::ErrorNetwork);
    return;
  }

  if (reply->rawHeader("X-RateLimit-Remaining") == "0") {
    emit error(fileName, QtImgur::ErrorCredits);
    return;
  }

  QXmlStreamReader reader(reply->readAll());

  while (!reader.atEnd()) {
    reader.readNext();

    if (reader.isStartElement()) {
      if (reader.name() == "error") {
        emit error(fileName, QtImgur::ErrorUpload);
      }

      if (reader.name() == "imgur_page") {
        emit uploaded(fileName, reader.readElementText());
      }
    }
  }


}
