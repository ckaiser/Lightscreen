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
#include <QDate>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QProcess>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QToolTip>
#include <QUrl>
#include <QSound>

#ifdef Q_OS_WIN
  #include <windows.h>
  #include <QtWinExtras>
#endif

/*
 * Lightscreen includes
 */
#include "lightscreenwindow.h"
#include "dialogs/optionsdialog.h"
#include "dialogs/previewdialog.h"
#include "dialogs/historydialog.h"

#include "tools/os.h"
#include "tools/screenshot.h"
#include "tools/screenshotmanager.h"
#include "tools/qxtglobalshortcut/qxtglobalshortcut.h"

#include "tools/uploader/uploader.h"

#include "updater/updater.h"

LightscreenWindow::LightscreenWindow(QWidget *parent) :
  QMainWindow(parent),
  mDoCache(false),
  mHideTrigger(false),
  mReviveMain(false),
  mWasVisible(true),
  mLastMessage(0),
  mLastMode(-1),
  mLastScreenshot()
{
  ui.setupUi(this);

  if (QtWin::isCompositionEnabled()) {
    setAttribute(Qt::WA_NoSystemBackground);
    QtWin::enableBlurBehindWindow(this);
    QtWin::extendFrameIntoClientArea(this, QMargins(-1, -1, -1, -1));
  }

  ui.screenPushButton->setIcon(os::icon("screen.big"));
  ui.areaPushButton->setIcon(os::icon("area.big"));
  ui.windowPushButton->setIcon(os::icon("pickWindow.big"));

  ui.optionsPushButton->setIcon(os::icon("configure"));
  ui.folderPushButton->setIcon(os::icon("folder"));
  ui.imgurPushButton->setIcon(os::icon("imgur"));

  setMaximumSize(size());
  setMinimumSize(size());

  setWindowFlags(windowFlags() ^ Qt::WindowMaximizeButtonHint);

#ifdef Q_OS_WIN
  if (QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7) {
    mTaskbarButton = new QWinTaskbarButton(this);
  }

  if (QSysInfo::windowsVersion() < QSysInfo::WV_WINDOWS7) {
    ui.centralWidget->setStyleSheet("QPushButton { padding: 2px; border: 1px solid #acacac; background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #eaeaea, stop:1 #e5e5e5);} QPushButton:hover { border: 1px solid #7eb4ea;	background-color: #e4f0fc; }");
  }
#endif

  // Actions
  connect(ui.screenPushButton, SIGNAL(clicked()), this, SLOT(screenshotAction()));
  connect(ui.areaPushButton  , SIGNAL(clicked()), this, SLOT(areaHotkey()));
  connect(ui.windowPushButton, SIGNAL(clicked()), this, SLOT(windowPickerHotkey()));

  connect(ui.optionsPushButton, SIGNAL(clicked()), this, SLOT(showOptions()));
  connect(ui.folderPushButton , SIGNAL(clicked()), this, SLOT(goToFolder()));

  connect(ui.imgurPushButton, SIGNAL(clicked()), this, SLOT(createUploadMenu()));

  // Shortcuts
  connect(&mScreenShortcut      , &QxtGlobalShortcut::activated, this, &LightscreenWindow::screenHotkey);
  connect(&mAreaShortcut        , &QxtGlobalShortcut::activated, this, &LightscreenWindow::areaHotkey);
  connect(&mWindowShortcut      , &QxtGlobalShortcut::activated, this, &LightscreenWindow::windowHotkey);
  connect(&mWindowPickerShortcut, &QxtGlobalShortcut::activated, this, &LightscreenWindow::windowPickerHotkey);
  connect(&mOpenShortcut        , &QxtGlobalShortcut::activated, this, &LightscreenWindow::show);
  connect(&mDirectoryShortcut   , &QxtGlobalShortcut::activated, this, &LightscreenWindow::goToFolder);

  // Uploader
  connect(Uploader::instance(), SIGNAL(progress(int)),        this, SLOT(uploadProgress(int)));
  connect(Uploader::instance(), SIGNAL(done(QString, QString, QString)), this, SLOT(showUploaderMessage(QString, QString)));
  connect(Uploader::instance(), SIGNAL(error(QString)),                  this, SLOT(showUploaderError(QString)));

  // Manager
  connect(ScreenshotManager::instance(), SIGNAL(confirm(Screenshot*)),                this, SLOT(preview(Screenshot*)));
  connect(ScreenshotManager::instance(), SIGNAL(windowCleanup(Screenshot::Options&)), this, SLOT(cleanup(Screenshot::Options&)));
  connect(ScreenshotManager::instance(), SIGNAL(activeCountChange()),                 this, SLOT(updateStatus()));

  if (!settings()->contains("file/format")) {
    showOptions();  // There are no options (or the options config is invalid or incomplete)
  }
  else {
    QTimer::singleShot(0   , this, SLOT(applySettings()));
    QTimer::singleShot(5000, this, SLOT(checkForUpdates()));
  }
}

LightscreenWindow::~LightscreenWindow()
{
  settings()->setValue("lastScreenshot", mLastScreenshot);
  settings()->sync();
  delete mTrayIcon;
}

void LightscreenWindow::action(int mode)
{
  if (mode == 4) {
    goToFolder();
  }
  else {
    show();
  }
}


void LightscreenWindow::areaHotkey()
{
  screenshotAction(2);
}

void LightscreenWindow::checkForUpdates()
{
  if (settings()->value("options/disableUpdater", false).toBool())
    return;

  if (settings()->value("lastUpdateCheck").toInt() + 7
      > QDate::currentDate().dayOfYear())
    return; // If 7 days have not passed since the last update check.

  mUpdater = new Updater(this);

  connect(mUpdater, SIGNAL(done(bool)), this, SLOT(updaterDone(bool)));
  mUpdater->check();
}

void LightscreenWindow::cleanup(Screenshot::Options &options)
{
  // Reversing settings
  if (settings()->value("options/hide").toBool()) {
#ifndef Q_WS_X11 // X is not quick enough and the notification ends up everywhere but in the icon
    if (settings()->value("options/tray").toBool() && mTrayIcon) {
      mTrayIcon->show();
    }
#endif

    if (mPreviewDialog) {
      if (mPreviewDialog->count() <= 1 && mWasVisible) {
        show();
      }

      mPreviewDialog->show();
    }
    else if (mWasVisible) {
      show();
    }

    mHideTrigger = false;
  }

  if (settings()->value("options/tray").toBool() && mTrayIcon) {
    notify(options.result);

    if (settings()->value("options/message").toBool() && options.file && !options.upload) {
      // This message wll get shown only when messages are enabled and the file won't get another upload pop-up soon.
      showScreenshotMessage(options.result, options.fileName);
    }
  }

  if (settings()->value("options/playSound", false).toBool()) {
    if (options.result == Screenshot::Success) {
      QSound::play("sounds/ls.screenshot.wav");
    }
    else {
#ifdef Q_OS_WIN
      QSound::play("afakepathtomakewindowsplaythedefaultsoundtheresprobablyabetterwaybuticantbebothered");
#else
      QSound::play("sound/ls.error.wav");
#endif
    }

  }

  updateStatus();

  if (options.result != Screenshot::Success)
    return;

  mLastScreenshot = options.fileName;
}

void LightscreenWindow::closeToTrayWarning()
{
  if (!settings()->value("options/closeToTrayWarning", true).toBool())
    return;

  mLastMessage = 3;
  mTrayIcon->showMessage(tr("Closed to tray"), tr("Lightscreen will keep running, you can disable this in the options menu."));
  settings()->setValue("options/closeToTrayWarning", false);
}

bool LightscreenWindow::closingWithoutTray()
{
  if (settings()->value("options/disableHideAlert", false).toBool())
    return false;

  QMessageBox msgBox;
  msgBox.setWindowTitle(tr("Lightscreen"));
  msgBox.setText(tr("You have chosen to hide Lightscreen when there's no system tray icon, so you will not be able to access the program <b>unless you have selected a hotkey to do so</b>.<br>What do you want to do?"));
  msgBox.setIcon(QMessageBox::Warning);

  msgBox.setStyleSheet("QPushButton { padding: 4px 8px; }");

  QPushButton *enableButton = msgBox.addButton(tr("Hide but enable tray"),
      QMessageBox::ActionRole);
  QPushButton *enableAndDenotifyButton = msgBox.addButton(tr("Hide and don't warn"),
      QMessageBox::ActionRole);
  QPushButton *hideButton = msgBox.addButton(tr("Just hide"),
      QMessageBox::ActionRole);
  QPushButton *abortButton = msgBox.addButton(QMessageBox::Cancel);

  Q_UNUSED(abortButton);

  msgBox.exec();

  if (msgBox.clickedButton() == hideButton) {
    return true;
  }
  else if (msgBox.clickedButton() == enableAndDenotifyButton) {
    settings()->setValue("options/disableHideAlert", true);
    applySettings();
    return true;
  }
  else if (msgBox.clickedButton() == enableButton) {
    settings()->setValue("options/tray", true);
    applySettings();
    return true;
  }

  return false; // Cancel.
}

void LightscreenWindow::createUploadMenu()
{
  QMenu* imgurMenu = new QMenu(tr("Upload"));

  QAction *uploadAction = new QAction(os::icon("imgur"), tr("&Upload last"), imgurMenu);
  uploadAction->setToolTip(tr("Upload the last screenshot you took to imgur.com"));
  connect(uploadAction, SIGNAL(triggered()), this, SLOT(uploadLast()));

  QAction *cancelAction = new QAction(os::icon("no"), tr("&Cancel upload"), imgurMenu);
  cancelAction->setToolTip(tr("Cancel the currently uploading screenshots"));
  cancelAction->setEnabled(false);

  connect(this, SIGNAL(uploading(bool)), cancelAction, SLOT(setEnabled(bool)));
  connect(cancelAction, SIGNAL(triggered()), this, SLOT(uploadCancel()));

  QAction *historyAction = new QAction(os::icon("view-history"), tr("View &History"), imgurMenu);
  connect(historyAction, SIGNAL(triggered()), this, SLOT(showHistoryDialog()));

  imgurMenu->addAction(uploadAction);
  imgurMenu->addAction(cancelAction);
  imgurMenu->addAction(historyAction);
  imgurMenu->addSeparator();

  connect(imgurMenu, SIGNAL(aboutToShow()), this, SLOT(uploadMenuShown()));

  ui.imgurPushButton->setMenu(imgurMenu);
  ui.imgurPushButton->showMenu();
}

void LightscreenWindow::goToFolder()
{
#ifdef Q_OS_WIN
  if (!mLastScreenshot.isEmpty() && QFile::exists(mLastScreenshot)) {
    QProcess::startDetached("explorer /select, \"" + mLastScreenshot +"\"");
  }
  else {
#endif
    QString folder = settings()->value("file/target").toString();

    if (folder.isEmpty())
      folder = qApp->applicationDirPath();

    if (QDir::toNativeSeparators(folder.at(folder.size()-1)) != QDir::separator())
      folder.append(QDir::separator());

    QDesktopServices::openUrl("file:///"+folder);
#ifdef Q_OS_WIN
  }
#endif
}

void LightscreenWindow::messageClicked()
{
  if (mLastMessage == 1) {
    goToFolder();
  }
  else if (mLastMessage == 3) {
    QTimer::singleShot(0, this, SLOT(showOptions()));
  }
  else {
    QDesktopServices::openUrl(QUrl(Uploader::instance()->lastUrl()));
  }
}

void LightscreenWindow::messageReceived(const QString &message)
{
  if (message.contains(' ')) {
    foreach (QString argument, message.split(' ')) {
      messageReceived(argument);
    }
  }

  if (message == "--wake") {
    show();
    qApp->alert(this, 500);
    return;
  }

  if (message == "--screen")
    screenshotAction();
  else if (message == "--area")
    screenshotAction(2);
  else if (message == "--activewindow")
    screenshotAction(1);
  else if (message == "--pickwindow")
    screenshotAction(3);
  else if (message == "--folder")
    action(4);
  else if (message == "--uploadlast")
    uploadLast();
  else if (message == "--viewhistory")
    showHistoryDialog();
}

void LightscreenWindow::notify(const Screenshot::Result &result)
{
  switch (result)
  {
  case Screenshot::Success:
    mTrayIcon->setIcon(QIcon(":/icons/lightscreen.yes"));

#ifdef Q_OS_WIN
    if (mTaskbarButton)
      mTaskbarButton->setOverlayIcon(os::icon("yes"));
#endif

    setWindowTitle(tr("Success!"));
    break;
  case Screenshot::Fail:
    mTrayIcon->setIcon(QIcon(":/icons/lightscreen.no"));
    setWindowTitle(tr("Failed!"));
#ifdef Q_OS_WIN
    if (mTaskbarButton) {
      mTaskbarButton->setOverlayIcon(os::icon("no"));
    }
#endif
    break;
  case Screenshot::Cancel:
    setWindowTitle(tr("Cancelled!"));
    break;
  }

  QTimer::singleShot(2000, this, SLOT(restoreNotification()));
}

void LightscreenWindow::preview(Screenshot* screenshot)
{
  if (screenshot->options().preview) {
    if (!mPreviewDialog) {
      mPreviewDialog = new PreviewDialog(this);
    }

    mPreviewDialog->add(screenshot);
  }
  else {
    screenshot->confirm(true);
  }
}

void LightscreenWindow::quit()
{
  settings()->setValue("position", pos());

  int answer = 0;
  QString doing;

  if (ScreenshotManager::instance()->activeCount() > 0) {
    doing = tr("processing");
  }

  if (Uploader::instance()->uploading() > 0) {
    if (doing.isEmpty()) {
      doing = tr("uploading");
    }
    else {
      doing = tr("processing and uploading");
    }
  }

  if (!doing.isEmpty()) {
    answer = QMessageBox::question(this,
                                   tr("Are you sure you want to quit?"),
                                   tr("Lightscreen is currently %1 screenshots. Are you sure you want to quit?").arg(doing),
                                   tr("Quit"),
                                   tr("Don't Quit"));
  }

  if (answer == 0)
    emit finished();
}

void LightscreenWindow::restoreNotification()
{
  if (mTrayIcon)
    mTrayIcon->setIcon(QIcon(":/icons/lightscreen.small"));

#ifdef Q_OS_WIN
  if (mTaskbarButton)
    mTaskbarButton->clearOverlayIcon();
#endif

  updateStatus();
}

void LightscreenWindow::screenshotAction(int mode)
{
  int delayms = -1;

  bool optionsHide = settings()->value("options/hide").toBool(); // Option cache, used a couple of times.

  if (!mHideTrigger) {
    mWasVisible = isVisible();
    mHideTrigger = true;
  }

  // Applying pre-screenshot settings
  if (optionsHide) {
    hide();

#ifndef Q_WS_X11 // X is not quick enough and the notification ends up everywhere but in the icon
    if (mTrayIcon)
      mTrayIcon->hide();
#endif
  }

  // Screenshot delay
  delayms = settings()->value("options/delay", 0).toInt();
  delayms = delayms * 1000; // Converting the delay to milliseconds.

  delayms += 400;

  if (optionsHide && mPreviewDialog) {
    if (mPreviewDialog->count() >= 1) {
      mPreviewDialog->hide();
    }
  }

  // The delayed functions works using the static variable lastMode
  // which keeps the argument so a QTimer can call this function again.
  if (delayms > 0) {
    if (mLastMode < 0) {
      mLastMode = mode;

      QTimer::singleShot(delayms, this, SLOT(screenshotAction()));
      return;
    }
    else {
      mode = mLastMode;
      mLastMode = -1;
    }
  }

  static Screenshot::Options options;

  if (!mDoCache) {
    // Populating the option object that will then be passed to the screenshot engine (sounds fancy huh?)
    options.file           = settings()->value("file/enabled").toBool();
    options.format         = (Screenshot::Format) settings()->value("file/format").toInt();
    options.prefix         = settings()->value("file/prefix").toString();

    QDir dir(settings()->value("file/target").toString());
    dir.makeAbsolute();

    options.directory      = dir;

    options.quality        = settings()->value("options/quality", 100).toInt();
    options.currentMonitor = settings()->value("options/currentMonitor", false).toBool();
    options.clipboard      = settings()->value("options/clipboard",      true).toBool();
    options.imgurClipboard = settings()->value("options/imgurClipboard", false).toBool();
    options.preview        = settings()->value("options/preview",        false).toBool();
    options.magnify        = settings()->value("options/magnify",        false).toBool();
    options.cursor         = settings()->value("options/cursor",         true).toBool();
    options.saveAs         = settings()->value("options/saveAs",         false).toBool();
    options.animations     = settings()->value("options/animations",     true).toBool();
    options.replace        = settings()->value("options/replace",        false).toBool();
    options.upload         = settings()->value("options/uploadAuto",     false).toBool();
    options.optimize       = settings()->value("options/optipng",        false).toBool();

    Screenshot::NamingOptions namingOptions;
    namingOptions.naming       = (Screenshot::Naming) settings()->value("file/naming").toInt();
    namingOptions.leadingZeros = settings()->value("options/naming/leadingZeros", 0).toInt();
    namingOptions.flip         = settings()->value("options/flip", false).toBool();
    namingOptions.dateFormat   = settings()->value("options/naming/dateFormat", "yyyy-MM-dd").toString();

    options.namingOptions = namingOptions;

    mDoCache = true;
  }

  options.mode = mode;

  setStatus(tr("Processing..."));
  ScreenshotManager::instance()->take(options);
}

void LightscreenWindow::screenshotActionTriggered(QAction* action)
{
  screenshotAction(action->data().toInt());
}

void LightscreenWindow::screenHotkey()
{
  screenshotAction(0);
}

void LightscreenWindow::showHotkeyError(const QStringList &hotkeys)
{
   static bool dontShow = false;

   if (dontShow)
    return;

   QString messageText;

   messageText = tr("Some hotkeys could not be registered, they might already be in use");

   if (hotkeys.count() > 1) {
     messageText += tr("<br>The failed hotkeys are the following:") + "<ul>";

     foreach(const QString &hotkey, hotkeys) {
       messageText += QString("%1%2%3").arg("<li><b>").arg(hotkey).arg("</b></li>");
     }

    messageText += "</ul>";
   }
   else {
    messageText += tr("<br>The failed hotkey is <b>%1</b>").arg(hotkeys[0]);
   }

   messageText += tr("<br><i>What do you want to do?</i>");

   QMessageBox msgBox(this);
   msgBox.setWindowTitle(tr("Lightscreen"));
   msgBox.setText(messageText);

   QPushButton *changeButton  = msgBox.addButton(tr("Change") , QMessageBox::ActionRole);
   QPushButton *disableButton = msgBox.addButton(tr("Disable"), QMessageBox::ActionRole);
   QPushButton *exitButton    = msgBox.addButton(tr("Quit")   , QMessageBox::ActionRole);

   msgBox.exec();

   if (msgBox.clickedButton() == exitButton) {
     dontShow = true;
     QTimer::singleShot(10, this, SLOT(quit()));
   }
   else if (msgBox.clickedButton() == changeButton) {
    showOptions();
   }
   else if (msgBox.clickedButton() == disableButton) {
     foreach(const QString &hotkey, hotkeys) {
       settings()->setValue(QString("actions/%1/enabled").arg(hotkey), false);
     }
   }
}

void LightscreenWindow::showHistoryDialog()
{
  HistoryDialog historyDialog(this);
  historyDialog.exec();

  updateStatus();
}

void LightscreenWindow::showOptions()
{
  mScreenShortcut.setEnabled(false);
  mAreaShortcut.setEnabled(false);
  mWindowShortcut.setEnabled(false);
  mWindowPickerShortcut.setEnabled(false);
  mOpenShortcut.setEnabled(false);
  mDirectoryShortcut.setEnabled(false);

  QPointer<OptionsDialog> optionsDialog = new OptionsDialog(this);

  optionsDialog->exec();
  optionsDialog->deleteLater();

  applySettings();
}

void LightscreenWindow::showScreenshotMessage(const Screenshot::Result &result, const QString &fileName)
{
  if (result == Screenshot::Cancel)
    return;

  // Showing message.
  QString title;
  QString message;

  if (result == Screenshot::Success) {
    title = QFileInfo(fileName).fileName();

    if (settings()->value("file/target").toString().isEmpty()) {
      message = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
    }
    else {
      message = tr("Saved to \"%1\"").arg(settings()->value("file/target").toString());
    }
  }
  else {
    title   = tr("The screenshot was not taken");
    message = tr("An error occurred.");
  }

  mLastMessage = 1;
  mTrayIcon->showMessage(title, message);
}

void LightscreenWindow::showUploaderError(const QString &error)
{
  mLastMessage = -1;
  updateStatus();

  if (mTrayIcon && !error.isEmpty() && settings()->value("options/message").toBool()) {
    mTrayIcon->showMessage(tr("Upload error"), error);
  }

  notify(Screenshot::Fail);
}

void LightscreenWindow::showUploaderMessage(QString fileName, QString url)
{
  if (mTrayIcon && settings()->value("options/message").toBool() && !url.isEmpty()) {
    QString screenshot = QFileInfo(fileName).fileName();

    if (screenshot.startsWith(".lstemp."))
      screenshot = tr("Screenshot");

    mLastMessage = 2;
    mTrayIcon->showMessage(tr("%1 uploaded").arg(screenshot), tr("Click here to go to %1").arg(url));
  }

  updateStatus();
}

void LightscreenWindow::toggleVisibility(QSystemTrayIcon::ActivationReason reason)
{
  if (reason != QSystemTrayIcon::DoubleClick)
    return;

  if (isVisible()) {
    hide();
  }
  else {
    show();
    os::setForegroundWindow(this);
  }
}

void LightscreenWindow::updateStatus()
{
  int uploadCount = Uploader::instance()->uploading();
  int activeCount = ScreenshotManager::instance()->activeCount();

  if (uploadCount > 0) {
    setStatus(tr("%1 uploading").arg(uploadCount));
    emit uploading(true);
  }
  else {
    if (activeCount > 0) {
      setStatus(tr("%1 processing").arg(activeCount));
    }
    else {
      setStatus();
    }

    emit uploading(false);

#ifdef Q_OS_WIN
    if (mTaskbarButton) {
      mTaskbarButton->progress()->setVisible(false);
    }
#endif
  }
}

void LightscreenWindow::updaterDone(bool result)
{
  settings()->setValue("lastUpdateCheck", QDate::currentDate().dayOfYear());

  if (!result)
    return;

  QMessageBox msgBox;
  msgBox.setWindowTitle(tr("Lightscreen"));
  msgBox.setText(tr("There's a new version of Lightscreen available.<br>Would you like to see more information?<br>(<em>You can turn this notification off</em>)"));
  msgBox.setIcon(QMessageBox::Information);

  QPushButton *yesButton     = msgBox.addButton(QMessageBox::Yes);
  QPushButton *turnOffButton = msgBox.addButton(tr("Turn Off"), QMessageBox::ActionRole);
  QPushButton *remindButton  = msgBox.addButton(tr("Remind Me Later"), QMessageBox::RejectRole);

  Q_UNUSED(remindButton);

  msgBox.exec();

  if (msgBox.clickedButton() == yesButton) {
    QDesktopServices::openUrl(QUrl("http://lightscreen.com.ar/whatsnew?from=" + qApp->applicationVersion()));
  }
  else if (msgBox.clickedButton() == turnOffButton) {
    settings()->setValue("options/disableUpdater", true);
  }

  mUpdater->deleteLater();
}

void LightscreenWindow::upload(const QString &fileName)
{
  Uploader::instance()->upload(fileName);
}

void LightscreenWindow::uploadCancel()
{
  if (Uploader::instance()->uploading() <= 0) {
    return;
  }

  int confirm = QMessageBox::question(this, tr("Upload cancel"), tr("Do you want to cancel all screenshot uploads?"), tr("Cancel"), tr("Don't Cancel"));

  if (confirm == 0) {
    Uploader::instance()->cancel();
    updateStatus();
  }
}

void LightscreenWindow::uploadLast()
{
  upload(mLastScreenshot);
  updateStatus();
}

void LightscreenWindow::uploadProgress(int progress)
{
#ifdef Q_OS_WIN
  if (mTaskbarButton) {
    mTaskbarButton->progress()->setRange(0, 100);
    mTaskbarButton->progress()->setValue(progress);
  }

  if (isVisible() && progress > 0) {
    int uploadCount = Uploader::instance()->uploading();

    if (uploadCount > 1) {
      setWindowTitle(tr("%1% of %2 uploads - Lightscreen").arg(progress).arg(uploadCount));
    }
    else {
      setWindowTitle(tr("%1% - Lightscreen").arg(progress));
    }
  }
#endif
}

void LightscreenWindow::uploadMenuShown()
{
  QMenu *imgurMenu = qobject_cast<QMenu*>(sender());
  imgurMenu->actions().at(0)->setEnabled(!mLastScreenshot.isEmpty());
}

void LightscreenWindow::windowHotkey()
{
  screenshotAction(1);
}

void LightscreenWindow::windowPickerHotkey()
{
  screenshotAction(3);
}

void LightscreenWindow::applySettings()
{
  bool tray = settings()->value("options/tray").toBool();

  if (tray && !mTrayIcon) {
    createTrayIcon();
    mTrayIcon->show();
  }
  else if (!tray && mTrayIcon) {
    mTrayIcon->deleteLater();
  }

  connectHotkeys();

  mDoCache = false;

  if (settings()->value("lastScreenshot").isValid() && mLastScreenshot.isEmpty())
    mLastScreenshot = settings()->value("lastScreenshot").toString();

  os::setStartup(settings()->value("options/startup").toBool(), settings()->value("options/startupHide").toBool());
}

void LightscreenWindow::connectHotkeys()
{
  bool screen = mScreenShortcut.setShortcut(settings()->value("actions/screen/hotkey").value<QKeySequence>());
  mScreenShortcut.setEnabled(settings()->value("actions/screen/enabled").toBool());

  bool area = mAreaShortcut.setShortcut(settings()->value("actions/area/hotkey").value<QKeySequence>());
  mAreaShortcut.setEnabled(settings()->value("actions/area/enabled").toBool());

  bool window = mWindowShortcut.setShortcut(settings()->value("actions/window/hotkey").value<QKeySequence>());
  mWindowShortcut.setEnabled(settings()->value("actions/window/enabled").toBool());

  bool windowPicker = mWindowPickerShortcut.setShortcut(settings()->value("actions/windowPicker/hotkey").value<QKeySequence>());
  mWindowPickerShortcut.setEnabled(settings()->value("actions/windowPicker/enabled").toBool());

  bool open = mOpenShortcut.setShortcut(settings()->value("actions/open/hotkey").value<QKeySequence>());
  mOpenShortcut.setEnabled(settings()->value("actions/open/enabled").toBool());

  bool directory = mOpenShortcut.setShortcut(settings()->value("actions/directory/hotkey").value<QKeySequence>());
  mDirectoryShortcut.setEnabled(settings()->value("actions/directory/enabled").toBool());

  QStringList failed;
  if (!screen)       failed << "screen";
  if (!area)         failed << "area";
  if (!window)       failed << "window";
  if (!windowPicker) failed << "window picker";
  if (!open)         failed << "open";
  if (!directory)    failed << "directory";

  if (!failed.isEmpty())
    showHotkeyError(failed);
}

void LightscreenWindow::createTrayIcon()
{
  mTrayIcon = new QSystemTrayIcon(QIcon(":/icons/lightscreen.small"), this);
  updateStatus();

  connect(mTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(toggleVisibility(QSystemTrayIcon::ActivationReason)));
  connect(mTrayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));

  QAction *hideAction = new QAction(QIcon(":/icons/lightscreen.small"), tr("Show&/Hide"), mTrayIcon);
  connect(hideAction, SIGNAL(triggered()), this, SLOT(toggleVisibility()));

  QAction *screenAction = new QAction(os::icon("screen"), tr("&Screen"), mTrayIcon);
  screenAction->setData(QVariant(0));

  QAction *windowAction = new QAction(os::icon("window"), tr("Active &Window"), this);
  windowAction->setData(QVariant(1));

  QAction *windowPickerAction = new QAction(os::icon("pickWindow"), tr("&Pick Window"), this);
  windowPickerAction->setData(QVariant(3));

  QAction *areaAction = new QAction(os::icon("area"), tr("&Area"), mTrayIcon);
  areaAction->setData(QVariant(2));

  QActionGroup *screenshotGroup = new QActionGroup(mTrayIcon);
  screenshotGroup->addAction(screenAction);
  screenshotGroup->addAction(areaAction);
  screenshotGroup->addAction(windowAction);
  screenshotGroup->addAction(windowPickerAction);

  connect(screenshotGroup, SIGNAL(triggered(QAction*)), this, SLOT(screenshotActionTriggered(QAction*)));

  // Duplicated for the screenshot button :(
  QAction *uploadAction = new QAction(os::icon("imgur"), tr("&Upload last"), mTrayIcon);
  uploadAction->setToolTip(tr("Upload the last screenshot you took to imgur.com"));
  connect(uploadAction, SIGNAL(triggered()), this, SLOT(uploadLast()));

  QAction *cancelAction = new QAction(os::icon("no"), tr("&Cancel upload"), mTrayIcon);
  cancelAction->setToolTip(tr("Cancel the currently uploading screenshots"));
  cancelAction->setEnabled(false);
  connect(this, SIGNAL(uploading(bool)), cancelAction, SLOT(setEnabled(bool)));
  connect(cancelAction, SIGNAL(triggered()), this, SLOT(uploadCancel()));

  QAction *historyAction = new QAction(os::icon("view-history"), tr("View History"), mTrayIcon);
  connect(historyAction, SIGNAL(triggered()), this, SLOT(showHistoryDialog()));
  //

  QAction *optionsAction = new QAction(os::icon("configure"), tr("View &Options"), mTrayIcon);
  connect(optionsAction, SIGNAL(triggered()), this, SLOT(showOptions()));

  QAction *goAction = new QAction(os::icon("folder"), tr("&Go to Folder"), mTrayIcon);
  connect(goAction, SIGNAL(triggered()), this, SLOT(goToFolder()));

  QAction *quitAction = new QAction(tr("&Quit"), mTrayIcon);
  connect(quitAction, SIGNAL(triggered()), this, SLOT(quit()));

  QMenu* screenshotMenu = new QMenu(tr("Screenshot"));
  screenshotMenu->addAction(screenAction);
  screenshotMenu->addAction(areaAction);
  screenshotMenu->addAction(windowAction);
  screenshotMenu->addAction(windowPickerAction);

  // Duplicated for the screenshot button :(
  QMenu* imgurMenu = new QMenu(tr("Upload"));
  imgurMenu->addAction(uploadAction);
  imgurMenu->addAction(cancelAction);
  imgurMenu->addAction(historyAction);
  imgurMenu->addSeparator();

  QMenu* trayIconMenu = new QMenu;
  trayIconMenu->addAction(hideAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addMenu(imgurMenu);
  trayIconMenu->addSeparator();
  trayIconMenu->addMenu(screenshotMenu);
  trayIconMenu->addAction(optionsAction);
  trayIconMenu->addAction(goAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(quitAction);

  mTrayIcon->setContextMenu(trayIconMenu);
}

void LightscreenWindow::setStatus(QString status)
{
  if (status.isEmpty()) {
    status = tr("Lightscreen");
  }
  else {
    status += tr(" - Lightscreen");
  }

  if (mTrayIcon) {
    mTrayIcon->setToolTip(status);
  }

  setWindowTitle(status);
}

QSettings *LightscreenWindow::settings() const
{
  return ScreenshotManager::instance()->settings();
}

// Event handling
bool LightscreenWindow::event(QEvent *event)
{
  if (event->type() == QEvent::Hide) {
    settings()->setValue("position", pos());
  }
  else if (event->type() == QEvent::Close) {
    if (settings()->value("options/tray").toBool() && settings()->value("options/closeHide").toBool()) {
      closeToTrayWarning();
      hide();
    }
    else if (settings()->value("options/closeHide").toBool()) {
      if (closingWithoutTray())
        hide();
    }
    else {
      quit();
    }
  }
  else if (event->type() == QEvent::Show) {
    QPoint savedPosition = settings()->value("position").toPoint();

    if (!savedPosition.isNull() && qApp->desktop()->availableGeometry().contains(QRect(savedPosition, size())))
      move(savedPosition);
  }
  else if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
#ifdef Q_WS_MAC
    if (keyEvent->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Period) {
        keyEvent->ignore();

        if(isVisible())
            toggleVisibility();

        return false;
    }
    else
#endif
    if (!keyEvent->modifiers() && keyEvent->key() == Qt::Key_Escape) {
        keyEvent->ignore();

        if(isVisible())
            toggleVisibility();

        return false;
    }
  }
  else if (event->type() == QEvent::LanguageChange) {
    ui.retranslateUi(this);
    resize(minimumSizeHint());
  }


  return QMainWindow::event(event);
}
