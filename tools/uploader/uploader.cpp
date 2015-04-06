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

QString Uploader::lastUrl() const
{
  return mLastUrl;
}

void Uploader::cancel()
{
  mUploaders.clear();
  emit cancelAll();
}

void Uploader::upload(const QString &fileName)
{
  if (fileName.isEmpty()) {
    return;
  }

  QVariantHash options;
  options["networkManager"].setValue(mNetworkAccessManager);
  options["directUrl"] = ScreenshotManager::instance()->settings()->value("options/uploadDirectLink", false).toBool();
  options["anonymous"] = true; //TODO: Settings

  ImgurUploader *uploader = new ImgurUploader(options);

  connect(uploader, &ImgurUploader::uploaded      , this, &Uploader::uploaded);
  connect(uploader, &ImgurUploader::error         , this, &Uploader::uploaderError);
  connect(uploader, &ImgurUploader::progressChange, this, &Uploader::progressChange);

  connect(this    , SIGNAL(cancelAll()), uploader, SLOT(cancel()));

  uploader->upload(fileName);
  mUploaders.append(uploader);
}

void Uploader::uploaded(const QString &file, const QString &url, const QString &deleteHash)
{
  mLastUrl = url;

  mUploaders.removeAll(qobject_cast<ImageUploader*>(sender()));
  sender()->deleteLater();
  emit done(file, url, deleteHash);
}

void Uploader::uploaderError(ImageUploader::Error error, QString fileName)
{
  Q_UNUSED(error)

  mUploaders.removeAll(qobject_cast<ImageUploader*>(sender()));
  sender()->deleteLater();
  emit done(fileName, "", "");
}

int Uploader::uploading()
{
  return mUploaders.count();
}

void Uploader::progressChange(int p)
{
  if (mUploaders.size() <= 0) {
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

