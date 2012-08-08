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
#include "screenshotmanager.h"
#include "screenshot.h"
#include "uploader.h"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QSettings>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

ScreenshotManager::ScreenshotManager(QObject *parent = 0) : QObject(parent)
{
  QString historyPath;

  if (QFile::exists(qApp->applicationDirPath() + "/config.ini")) {
    mSettings     = new QSettings(qApp->applicationDirPath() + QDir::separator() + "config.ini", QSettings::IniFormat);
    mPortableMode = true;
    historyPath   = qApp->applicationDirPath() + QDir::separator() + "history.sqlite";
  }
  else {
    mSettings     = new QSettings();
    mPortableMode = false;
    historyPath   = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + QDir::separator() + "history.sqlite";
  }

  // Creating the SQLite database.
  mHistory = QSqlDatabase::addDatabase("QSQLITE");
  mHistory.setDatabaseName(historyPath);

  if (mHistory.open()) {
    mHistory.exec("CREATE TABLE IF NOT EXISTS history (fileName text, URL text, deleteURL text, time integer)");
  }

  connect(Uploader::instance(), SIGNAL(done(QString, QString, QString)), this, SLOT(uploadDone(QString, QString, QString)));
}

ScreenshotManager::~ScreenshotManager()
{
  delete mSettings;
}

int ScreenshotManager::activeCount() const
{
  return mScreenshots.count();
}

bool ScreenshotManager::portableMode()
{
  return mPortableMode;
}

void ScreenshotManager::saveHistory(QString fileName, QString url, QString deleteHash)
{
  if (!mSettings->value("/options/history", true).toBool())
    return;

  if (!mHistory.isOpen())
    return;

  mHistory.exec(QString("INSERT INTO history (fileName, URL, deleteURL, time) VALUES('%1', '%2', '%3', %4)")
                 .arg(fileName)
                 .arg(url)
                 .arg("http://imgur.com/delete/" + deleteHash)
                 .arg(QDateTime::currentMSecsSinceEpoch())
                 );
}

void ScreenshotManager::clearHistory()
{
  if (mHistory.isOpen())
    mHistory.exec("DROP TABLE history");
}

//

void ScreenshotManager::askConfirmation()
{
  Screenshot* s = qobject_cast<Screenshot*>(sender());
  emit confirm(s);
}

void ScreenshotManager::cleanup()
{
  Screenshot* screenshot = qobject_cast<Screenshot*>(sender());
  emit windowCleanup(screenshot->options());
}

void ScreenshotManager::finished()
{
  Screenshot* screenshot = qobject_cast<Screenshot*>(sender());
  mScreenshots.removeOne(screenshot);
  screenshot->deleteLater();
}

void ScreenshotManager::take(Screenshot::Options &options)
{
  Screenshot* newScreenshot = new Screenshot(this, options);
  mScreenshots.append(newScreenshot);

  connect(newScreenshot, SIGNAL(askConfirmation()), this, SLOT(askConfirmation()));
  connect(newScreenshot, SIGNAL(cleanup())        , this, SLOT(cleanup()));
  connect(newScreenshot, SIGNAL(finished())       , this, SLOT(finished()));

  newScreenshot->take();
}

void ScreenshotManager::uploadDone(QString fileName, QString url, QString deleteHash)
{
  foreach (Screenshot* screenshot, mScreenshots) {
    if (screenshot->options().fileName == fileName
        || screenshot->unloadedFileName() == fileName) {
      screenshot->uploadDone(url);

      if (screenshot->options().file) {
        saveHistory(fileName, url, deleteHash);
      }
      else {
        saveHistory("", url, deleteHash);
      }
    }
  }
}

// Singleton
ScreenshotManager* ScreenshotManager::mInstance = 0;

ScreenshotManager *ScreenshotManager::instance()
{
  if (!mInstance)
    mInstance = new ScreenshotManager();

  return mInstance;
}
