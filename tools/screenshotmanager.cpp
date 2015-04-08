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
#include "screenshotmanager.h"
#include "screenshot.h"
#include "uploader/uploader.h"

#include <QApplication>
#include <QDateTime>
#include <QStandardPaths>
#include <QFile>
#include <QSettings>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

ScreenshotManager::ScreenshotManager(QObject *parent = 0) : QObject(parent)
{
  if (QFile::exists(qApp->applicationDirPath() + QDir::separator() + "config.ini")) {
    mSettings     = new QSettings(qApp->applicationDirPath() + QDir::separator() + "config.ini", QSettings::IniFormat);
    mPortableMode = true;
    mHistoryPath  = qApp->applicationDirPath() + QDir::separator();
  }
  else {
    mSettings     = new QSettings();
    mPortableMode = false;
    mHistoryPath  = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator();
  }

  initHistory();

  connect(Uploader::instance(), SIGNAL(done(QString, QString, QString)), this, SLOT(uploadDone(QString, QString, QString)));
}

ScreenshotManager::~ScreenshotManager()
{
  delete mSettings;
}

void ScreenshotManager::initHistory()
{
  // Creating the SQLite database.
  QSqlDatabase history = QSqlDatabase::addDatabase("QSQLITE");

  QDir hp(mHistoryPath);

  if (!hp.exists())
    hp.mkpath(mHistoryPath);

  history.setHostName("localhost");
  history.setDatabaseName(mHistoryPath + "history.sqlite");

  if (history.open()) {
    history.exec("CREATE TABLE IF NOT EXISTS history (fileName text, URL text, deleteURL text, time integer)");
  }
  else {
    qCritical() << "Could not open SQLite DB.";
  }
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

  QString deleteUrl;

  if (!deleteHash.isEmpty())
    deleteUrl = "http://imgur.com/delete/" + deleteHash;

  QSqlQuery query;
  query.prepare("INSERT INTO history (fileName, URL, deleteURL, time) VALUES(?, ?, ?, ?)");
  query.addBindValue(fileName);
  query.addBindValue(url);
  query.addBindValue(deleteUrl);
  query.addBindValue(QDateTime::currentMSecsSinceEpoch());
  query.exec();
}

void ScreenshotManager::updateHistory(QString fileName, QString url, QString deleteHash)
{
  if (!mSettings->value("/options/history", true).toBool())
    return;

  QSqlQuery query;
  query.prepare("SELECT fileName FROM history WHERE URL IS NOT EMPTY AND fileName = ?");
  query.addBindValue(fileName);
  query.exec();

  if (query.record().count() > 0 && !url.isEmpty()) {
    // Makes sure to only update the latest file, in case something weird happened with the files (deleted screenshots, etc). Though that might still happen in some edge cases that I'm too lazy to account for.
    QSqlQuery updateQuery;
    updateQuery.prepare("UPDATE history SET URL = ?, deleteURL = ?, time = ? WHERE fileName = ?");
    updateQuery.addBindValue(url);
    updateQuery.addBindValue("http://imgur.com/delete/" + deleteHash);
    updateQuery.addBindValue(QDateTime::currentMSecsSinceEpoch());
    updateQuery.addBindValue(fileName);

    updateQuery.exec();
  }
}

void ScreenshotManager::removeHistory(QString fileName, qint64 time)
{
    QSqlQuery removeQuery;
    removeQuery.prepare("DELETE FROM history WHERE fileName = ? AND time = ?");
    removeQuery.addBindValue(fileName);
    removeQuery.addBindValue(time);

    removeQuery.exec();
}

void ScreenshotManager::clearHistory()
{
  QSqlQuery clearQuery("DROP TABLE history");
  clearQuery.exec();

  initHistory();
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
  emit activeCountChange();
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
        updateHistory(fileName, url, deleteHash);
      }
      else {
        saveHistory("", url, deleteHash);
      }

      return;
    }
  }

  // If we get here, it's because the screenshot upload wasn't on the current screenshot list, which means it's a View History/Upload Later upload.
  updateHistory(fileName, url, deleteHash);
}

// Singleton
ScreenshotManager* ScreenshotManager::mInstance = 0;

ScreenshotManager *ScreenshotManager::instance()
{
  if (!mInstance)
    mInstance = new ScreenshotManager();

  return mInstance;
}
