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
#ifndef LIGHTSCREENWINDOW_H
#define LIGHTSCREENWINDOW_H

#include <QtGui/QDialog>
#include <QPointer>
#include <QSystemTrayIcon>

#include "updater/updater.h"
#include "tools/screenshot.h"

#include "ui_lightscreenwindow.h"

class QHttp;
class Updater;
class QSettings;
class LightscreenWindow : public QDialog
{
    Q_OBJECT

public:
  LightscreenWindow(QWidget *parent = 0);
  ~LightscreenWindow();

public slots:
  void action(int mode = 3);
  void areaHotkey();
  void checkForUpdates();
  void cleanup(Screenshot::Options &options);
  bool closingWithoutTray();
  void goToFolder();
  void messageClicked();
  void messageReceived(const QString message);
  void notify(Screenshot::Result result);
  void optimizationDone();
  void preview(Screenshot* screenshot);
  void quit();
  void restoreNotification();
  void screenshotAction(int mode = 0);
  void screenshotActionTriggered(QAction* action);
  void showHotkeyError(QStringList hotkeys);
  void showOptions();
  void showScreenshotMenu();
  void showScreenshotMessage(Screenshot::Result result, QString fileName);
  void showUploaderError(QString error);
  void showUploaderMessage(QString fileName, QString url);
  void toggleVisibility(QSystemTrayIcon::ActivationReason reason = QSystemTrayIcon::DoubleClick);
  void updateUploadStatus();
  void updaterDone(bool result);
  void upload(QString fileName);
  void uploadAction(QAction* upload);
  void uploadLast();
  void windowHotkey();
  void windowPickerHotkey();

private slots:
  void applySettings();

private:
  void compressPng(QString fileName);
  void connectHotkeys();
  void createTrayIcon();
  bool eventFilter(QObject *object, QEvent *event);

  // Convenience function
  QSettings *settings() const;

protected:
  bool event(QEvent *event);

private:
  bool mDoCache;
  bool mHideTrigger;
  bool mReviveMain;
  bool mWasVisible;
  int  mOptimizeCount;
  int  mLastMode;
  int  mLastMessage;
  QActionGroup* mUploadHistoryActions;
  QString mLastScreenshot;
  QPointer<QSystemTrayIcon> mTrayIcon;
  Ui::LightscreenWindowClass ui;

};

#endif // LIGHTSCREENWINDOW_H

