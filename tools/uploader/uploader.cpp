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
#include "uploader.h"
#include "../screenshotmanager.h"
#include "imguruploader.h"

#include <QtNetwork>
#include <QSettings>

Uploader* Uploader::mInstance = 0;

Uploader::Uploader(QObject *parent) : QObject(parent)
{
  mNetworkAccessManager = new QNetworkAccessManager(this);
}

Uploader *Uploader::instance()
{
  if (!mInstance)
    mInstance = new Uploader();

  return mInstance;
}

int Uploader::progress() const
{
  return mProgress;
}

QNetworkAccessManager* Uploader::nam()
{
  return mNetworkAccessManager;
}

QString Uploader::lastUrl() const
{
  return mLastUrl;
}

void Uploader::cancel()
{
  mUploaders.clear();
  mProgress = 0;
  emit cancelAll();
}

void Uploader::upload(const QString &fileName)
{
  if (fileName.isEmpty()) {
    return;
  }

  QVariantHash options;
  QSettings *s = ScreenshotManager::instance()->settings();
  options["type"] = "imgur";
  options["networkManager"].setValue(mNetworkAccessManager);
  options["directUrl"] = s->value("options/uploadDirectLink", false).toBool();
  options["anonymous"] = s->value("upload/imgur/anonymous", true).toBool();
  options["album"]     = s->value("upload/imgur/album", "").toString();
  options["access_token"]  = s->value("upload/imgur/access_token", "").toString();
  options["refresh_token"] = s->value("upload/imgur/refresh_token", "").toString();

  if (options["access_token"].toString().isEmpty() || options["refresh_token"].toString().isEmpty())
  {
    options["anonymous"] = true;
  }

  ImgurUploader *uploader = new ImgurUploader(options);

  connect(uploader, &ImgurUploader::uploaded       , this, &Uploader::uploaded);
  connect(uploader, &ImgurUploader::error          , this, &Uploader::uploaderError);
  connect(uploader, &ImgurUploader::progressChange , this, &Uploader::progressChange);
  connect(uploader, &ImgurUploader::needAuthRefresh, this, &Uploader::imgurAuthRefresh);

  connect(this    , SIGNAL(cancelAll()), uploader, SLOT(cancel()));

  uploader->upload(fileName);
  mUploaders.append(uploader);
}

void Uploader::uploaded(const QString &file, const QString &url, const QString &deleteHash)
{
  mLastUrl = url;
  mUploaders.removeAll(qobject_cast<ImageUploader*>(sender()));

  if (mUploaders.isEmpty())
    mProgress = 0;

  sender()->deleteLater();
  emit done(file, url, deleteHash);
}

void Uploader::imgurAuthRefresh()
{
  for (int i = 0; i < mUploaders.size(); ++i) {
    if (mUploaders[i]->options().value("type") == "imgur") {
      mUploaders[i]->cancel();
    }
  }

  QByteArray parameters;
  parameters.append(QString("refresh_token=").toUtf8());
  parameters.append(QUrl::toPercentEncoding(ScreenshotManager::instance()->settings()->value("upload/imgur/refresh_token").toString()));
  parameters.append(QString("&client_id=").toUtf8());
  parameters.append(QUrl::toPercentEncoding("3ebe94c791445c1"));
  parameters.append(QString("&client_secret=").toUtf8());
  parameters.append(QUrl::toPercentEncoding("0546b05d6a80b2092dcea86c57b792c9c9faebf0")); // TODO: TA.png
  parameters.append(QString("&grant_type=refresh_token").toUtf8());

  QNetworkRequest request(QUrl("https://api.imgur.com/oauth2/token"));
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  QNetworkReply* reply = Uploader::instance()->nam()->post(request, parameters);
  connect(reply, SIGNAL(finished()), this, SLOT(imgurToken()));
}

void Uploader::imgurToken()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

  if (reply->error() != QNetworkReply::NoError) {
    emit error(reply->errorString());
    return;
  }

  QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

  QSettings* s = ScreenshotManager::instance()->settings();
  s->setValue("upload/imgur/access_token"    , imgurResponse["access_token"].toString());
  s->setValue("upload/imgur/refresh_token"   , imgurResponse["refresh_token"].toString());
  s->setValue("upload/imgur/account_username", imgurResponse["account_username"].toString());
  s->setValue("upload/imgur/expires_in"      , imgurResponse["expires_in"].toString());

  for (int i = 0; i < mUploaders.size(); ++i) {
    if (mUploaders[i]->options().value("type") == "imgur") {
      mUploaders[i]->options().remove("access_token");
      mUploaders[i]->options().remove("refresh_token");

      mUploaders[i]->options().insert("access_token" , imgurResponse["access_token"].toString());
      mUploaders[i]->options().insert("refresh_token", imgurResponse["refresh_token"].toString());

      mUploaders[i]->retry();
    }
  }

  emit imgurAuthRefreshed();
}

void Uploader::uploaderError(ImageUploader::Error code, QString errorString, QString fileName)
{
  mUploaders.removeAll(qobject_cast<ImageUploader*>(sender()));
  sender()->deleteLater();
  mProgress = 0;

  if (code != ImageUploader::CancelError) {
    if (errorString.isEmpty()) {
      emit error(tr("Upload Error ") + code);
    }
    else {
      emit error(errorString);
    }
  }

  emit done(fileName, "", "");
}

int Uploader::uploading()
{
  return mUploaders.count();
}

void Uploader::progressChange(int p)
{
  if (mUploaders.size() <= 0) {
    mProgress = p;
    emit progress(p);
    return;
  }

  int totalProgress = 0;

  for (int i = 0; i < mUploaders.size(); ++i) {
    totalProgress += mUploaders[i]->progress();
  }

  mProgress = totalProgress / mUploaders.size();
  emit progress(mProgress);
}

