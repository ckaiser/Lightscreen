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
#ifndef SCREENSHOTMANAGER_H
#define SCREENSHOTMANAGER_H

#include <QObject>
#include <QList>
#include <QtSql/QSqlDatabase>

#include "screenshot.h"

class QSettings;
class ScreenshotManager : public QObject
{
  Q_OBJECT

public:
  enum State
  {
    Idle  = 0,
    Busy = 1,
    Disabled  = 2
  };

public:
  ScreenshotManager(QObject *parent);
  ~ScreenshotManager();
  static ScreenshotManager *instance();

  int activeCount() const;
  bool portableMode();
  void saveHistory(QString fileName, QString url = "", QString deleteHash = "");
  void clearHistory();
  QSettings *settings() const { return mSettings; }
  QSqlDatabase &history() { return mHistory; }

public slots:
  void askConfirmation();
  void cleanup();
  void finished();
  void take(Screenshot::Options &options);
  void uploadDone(QString fileName, QString url, QString deleteHash);

signals:
  void confirm(Screenshot* screenshot);
  void windowCleanup(Screenshot::Options &options);

private:
  static ScreenshotManager* mInstance;
  QList<Screenshot*> mScreenshots;
  QSettings *mSettings;
  QSqlDatabase mHistory;
  bool mPortableMode;
};

#endif // SCREENSHOTMANAGER_H
