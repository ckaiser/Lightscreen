/*
 * Copyright (C) 2012  Christian Kaiser
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

#include <QMainWindow>
#include <QPointer>
#include <QSystemTrayIcon>

#include "updater/updater.h"
#include "tools/screenshot.h"
#include "dialogs/previewdialog.h"

#include "ui_lightscreenwindow.h"

#ifdef Q_WS_WIN
  #include "tools/qwin7utils/TaskbarButton.h"
#endif

class QHttp;
class Updater;
class QSettings;
class LightscreenWindow : public QMainWindow
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
  void closeToTrayWarning();
  bool closingWithoutTray();
  void createUploadMenu();
  void goToFolder();
  void messageClicked();
  void messageReceived(const QString &message);
  void notify(const Screenshot::Result &result);
  void preview(Screenshot* screenshot);
  void quit();
  void restoreNotification();
  void screenshotAction(int mode = 0);
  void screenshotActionTriggered(QAction* action);
  void showHotkeyError(const QStringList &hotkeys);
  void showHistoryDialog();
  void showOptions();
  void showScreenshotMessage(const Screenshot::Result &result, const QString &fileName);
  void showUploaderError(const QString &error);
  void showUploaderMessage(QString fileName, QString url);
  void toggleVisibility(QSystemTrayIcon::ActivationReason reason = QSystemTrayIcon::DoubleClick);
  void updateUploadStatus();
  void updaterDone(bool result);
  void upload(const QString &fileName);
  void uploadCancel();
  void uploadLast();
  void uploadProgress(qint64 sent, qint64 total);
  void uploadMenuShown();
  void windowHotkey();
  void windowPickerHotkey();

private slots:
  void applySettings();

signals:
  void uploading(bool uploading);
  void finished();

private:
  void connectHotkeys();
  void createTrayIcon();

#ifdef Q_WS_WIN
  bool winEvent(MSG *message, long *result);
#endif
  // Convenience function
  QSettings *settings() const;

protected:
  bool event(QEvent *event);

private:
  bool mDoCache;
  bool mHideTrigger;
  bool mReviveMain;
  bool mWasVisible;
  int  mLastMessage;
  int  mLastMode;
  QString mLastScreenshot;
  QPointer<QSystemTrayIcon> mTrayIcon;
  QPointer<PreviewDialog> mPreviewDialog;
  Ui::LightscreenWindowClass ui;

#ifdef Q_WS_WIN
  QW7::TaskbarButton *mTaskbarButton;
#endif
};

#endif // LIGHTSCREENWINDOW_H

