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
#include "qtimgur.h"
#include "screenshotmanager.h"

#include <QList>
#include <QPair>
#include <QSettings>

Uploader* Uploader::mInstance = 0;

Uploader::Uploader(QObject *parent) : QObject(parent), mProgressSent(0), mProgressTotal(0)
{
  mImgur = new QtImgur("6920a141451d125b3e1357ce0e432409", this);

  connect(mImgur, SIGNAL(uploaded(QString, QString, QString)), this, SLOT(uploaded(QString, QString, QString)));
  connect(mImgur, SIGNAL(error(QString, QtImgur::Error))     , this, SLOT(imgurError(QString, QtImgur::Error)));
  connect(mImgur, SIGNAL(uploadProgress(qint64,qint64))      , this, SLOT(reportProgress(qint64, qint64)));
}

Uploader *Uploader::instance()
{
  if (!mInstance)
    mInstance = new Uploader();

  return mInstance;
}

QString Uploader::lastUrl() const
{
  return mLastUrl;
}

void Uploader::cancel()
{
  mImgur->cancelAll();
}

void Uploader::imgurError(const QString &file, const QtImgur::Error e)
{
  // Removing the screenshot.
  for (int i = 0; i < mScreenshots.size(); ++i) {
    if (mScreenshots.at(i).first == file) {
      mScreenshots.removeAt(i);
      break;
    }
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
      errorString = tr("Upload failed for %1.").arg(QFileInfo(file).fileName());
    break;
  }

  emit done(file, "", "");
  emit error(errorString);
}

void Uploader::upload(const QString &fileName)
{
  if (fileName.isEmpty()) {
    return;
  }

  mImgur->directUrl = ScreenshotManager::instance()->settings()->value("options/uploadDirectLink", false).toBool();

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
}

void Uploader::uploaded(const QString &file, const QString &url, const QString &deleteHash)
{
  // Removing the screenshot.
  for (int i = 0; i < mScreenshots.size(); ++i) {
    if (mScreenshots.at(i).first == file) {
      mScreenshots.removeAt(i);
      break;
    }
  }

  mLastUrl = url;

  emit done(file, url, deleteHash);
}

int Uploader::uploading()
{
  return mScreenshots.count();
}

void Uploader::reportProgress(qint64 sent, qint64 total)
{
  mProgressSent  = sent;
  mProgressTotal = total;

  emit progress(sent, total);
}
