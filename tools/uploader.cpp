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
#include "uploader.h"
#include "qtimgur.h"
#include "screenshotmanager.h"

#include <QList>
#include <QPair>
#include <QDebug>

Uploader::Uploader(QObject *parent) : QObject(parent), mUploading(0)
{
  mImgur = new QtImgur("6920a141451d125b3e1357ce0e432409", this);
  connect(mImgur, SIGNAL(uploaded(QString, QString)), this, SLOT(uploaded(QString, QString)));
  connect(mImgur, SIGNAL(error(QString, QtImgur::Error)), this, SLOT(imgurError(QString, QtImgur::Error)));
  connect(mImgur, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(progress(qint64,qint64)));
}

void Uploader::upload(const QString &fileName)
{
  if (fileName.isEmpty()) {
    return;
  }

  // Cancel on duplicate
  for (int i = 0; i < mScreenshots.size(); ++i) {
    if (mScreenshots.at(i).first == fileName) {
      return;
    }
  }

  mImgur->upload(fileName);

  QPair<QString, QString> screenshot;
  screenshot.first  = fileName;
  screenshot.second = tr("Uploading...");

  mScreenshots.append(screenshot);

  mUploading++;
}

void Uploader::uploaded(const QString &file, const QString &url)
{
  qDebug() << "Uploader::uploaded(" << file << ", " << url << ")";

  // Modifying uploaded list, adding url.
  for (int i = 0; i < mScreenshots.size(); ++i) {
    if (mScreenshots.at(i).first == file) {
      mScreenshots[i].second = url;
      break;
    }
  }

  mUploading--;

  ScreenshotManager::instance()->saveHistory(file, url);
  emit done(file, url);
}

void Uploader::cancel(const QString &fileName)
{
  mImgur->cancel(fileName);
}

int Uploader::uploading()
{
  return mUploading;
}

void Uploader::imgurError(const QString &file, const QtImgur::Error e)
{
  qDebug() << "Uploader::imgurError(" << file << ", " << e << ")";

  mUploading--;

  // Removing the screenshot.
  for (int i = 0; i < mScreenshots.size(); ++i) {
    if (mScreenshots.at(i).first == file) {
      mScreenshots.removeAt(i);
      break;
    }
  }

  if (e == mLastError) {
    // Fail silently? Really? FINE
    return;
  }

  QString errorString;

  switch (e) {
    case QtImgur::ErrorFile:
      errorString = tr("Screenshot file not found.");
    break;
    case QtImgur::ErrorNetwork:
      errorString = tr("Could not reach imgur.com");
    break;
    case QtImgur::ErrorCredits:
      errorString = tr("You have exceeded your upload quota.");
    break;
    case QtImgur::ErrorUpload:
      errorString = tr("Upload failed.");
    break;
  }

  mLastError = e;
  emit error(errorString);
}

QString Uploader::lastUrl() const
{
  QListIterator< QPair<QString, QString> >  i(mScreenshots);
  i.toBack();

  QString url;

  while (i.hasPrevious()) {
    url = i.previous().second;

    if (!url.contains(tr("Uploading..."))) {
      return url;
    }
  }

  return url;
}

// Singleton
Uploader* Uploader::mInstance = 0;

Uploader *Uploader::instance()
{
  if (!mInstance)
    mInstance = new Uploader();

  return mInstance;
}
