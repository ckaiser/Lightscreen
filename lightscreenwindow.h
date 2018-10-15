/*
 * Copyright (C) 2017  Christian Kaiser
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

#include <updater/updater.h>
#include <tools/screenshot.h>

#include <dialogs/previewdialog.h>

#include "ui_lightscreenwindow.h"

class Updater;
class QSettings;
class QProgressBar;
class QWinTaskbarButton;
class UGlobalHotkeys;
class LightscreenWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum Action {
        ShowMainWindow = 5,
        OpenScreenshotFolder = 6
    };
    Q_ENUM(Action)

    LightscreenWindow(QWidget *parent = 0);
    ~LightscreenWindow();

public slots:
    void action(int mode = 3);
    void areaHotkey();
    void checkForUpdates();
    void cleanup(const Screenshot::Options &options);
    void closeToTrayWarning();
    bool closingWithoutTray();
    void createUploadMenu();
    void goToFolder();
    void messageClicked();
    void executeArgument(const QString &message);
    void executeArguments(const QStringList &arguments);
    void notify(const Screenshot::Result &result);
    void preview(Screenshot *screenshot);
    void quit();
    void restoreNotification();
    void setStatus(QString status = "");
    void screenshotAction(Screenshot::Mode mode = Screenshot::None);
    void screenshotActionTriggered(QAction *action);
    void screenHotkey();
    void showHotkeyError(const QStringList &hotkeys);
    void showHistoryDialog();
    void showOptions();
    void showScreenshotMessage(const Screenshot::Result &result, const QString &fileName);
    void showUploaderError(const QString &error);
    void showUploaderMessage(const QString &fileName, const QString &url);
    void toggleVisibility();
    void updateStatus();
    void updaterDone(bool result);
    void upload(const QString &fileName);
    void uploadCancel();
    void uploadLast();
    void uploadProgress(int progress);
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

#ifdef Q_OS_WIN
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
    Screenshot::Mode  mLastMode;
    QString mLastScreenshot;
    QPointer<QSystemTrayIcon> mTrayIcon;
    QPointer<PreviewDialog> mPreviewDialog;
    QPointer<Updater> mUpdater;
    Ui::LightscreenWindowClass ui;

    QPointer<UGlobalHotkeys> mGlobalHotkeys;

    bool mHasTaskbarButton;

#ifdef Q_OS_WIN
    QPointer<QWinTaskbarButton> mTaskbarButton;
#else
    class QWinTaskbarProgressDummy
    {
    public:
        void setVisible(bool v) { Q_UNUSED(v) }
        void setPaused(bool p)  { Q_UNUSED(p) }
        void resume() {}
        void stop() {}
        void reset() {}
        void setRange(int m, int m2)  { Q_UNUSED(m) Q_UNUSED(m2) }
        void setValue(int v)  { Q_UNUSED(v) }

    };

    class QWinTaskbarDummy : public QObject
    {
    public:
        void setOverlayIcon(QIcon i) { Q_UNUSED(i) }
        void clearOverlayIcon() {}
        QWinTaskbarProgressDummy *progress() { return 0; }
        void setWindow(QWindow *w) { Q_UNUSED(w) }
    };

    QWinTaskbarDummy *mTaskbarButton;
#endif
};

#endif // LIGHTSCREENWINDOW_H

