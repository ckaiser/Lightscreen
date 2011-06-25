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

#include <QSettings>
#include <QApplication>
#include <QFile>
#include <QDebug>

ScreenshotManager::ScreenshotManager(QObject *parent = 0) : QObject(parent), mCount(0)
{
  if (QFile::exists(qApp->applicationDirPath() + "/config.ini")) {
    // Portable edition :]
    mSettings = new QSettings(qApp->applicationDirPath() + "/config.ini", QSettings::IniFormat);
  }
  else {
    mSettings = new QSettings();
  }
}

ScreenshotManager::~ScreenshotManager()
{
  delete mSettings;
}

void ScreenshotManager::take(Screenshot::Options &options)
{
  Screenshot* newScreenshot = new Screenshot(this, options);

  connect(newScreenshot, SIGNAL(askConfirmation()), this, SLOT(askConfirmation()));
  connect(newScreenshot, SIGNAL(finished())       , this, SLOT(cleanup()));

  newScreenshot->take();
}

void ScreenshotManager::askConfirmation()
{
  Screenshot* s = qobject_cast<Screenshot*>(sender());
  emit confirm(s);
}

void ScreenshotManager::cleanup()
{
  Screenshot* s = qobject_cast<Screenshot*>(sender());
  emit windowCleanup(s->options());
  s->deleteLater();
}

// Singleton
ScreenshotManager* ScreenshotManager::mInstance = 0;

ScreenshotManager *ScreenshotManager::instance()
{
  if (!mInstance)
    mInstance = new ScreenshotManager();

  return mInstance;
}
