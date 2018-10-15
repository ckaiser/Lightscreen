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
#include <QKeyEvent>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <QtWinExtras>
#endif

//
//Lightscreen includes
//
#include <lightscreenwindow.h>
#include <dialogs/optionsdialog.h>
#include <dialogs/previewdialog.h>
#include <dialogs/historydialog.h>

#include <tools/os.h>
#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/UGlobalHotkey/uglobalhotkeys.h>
#include <tools/uploader/uploader.h>

#include <updater/updater.h>

LightscreenWindow::LightscreenWindow(QWidget *parent) :
    QMainWindow(parent),
    mDoCache(false),
    mHideTrigger(false),
    mReviveMain(false),
    mWasVisible(true),
    mLastMessage(0),
    mLastMode(Screenshot::None),
    mLastScreenshot(),
    mHasTaskbarButton(false)
{
    ui.setupUi(this);

    ui.screenPushButton->setIcon(os::icon("screen.big"));
    ui.areaPushButton->setIcon(os::icon("area.big"));
    ui.windowPushButton->setIcon(os::icon("pickWindow.big"));

    ui.optionsPushButton->setIcon(os::icon("configure"));
    ui.folderPushButton->setIcon(os::icon("folder"));
    ui.imgurPushButton->setIcon(os::icon("imgur"));

    createUploadMenu();

#ifdef Q_OS_WIN
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_WINDOWS7) {
        mTaskbarButton = new QWinTaskbarButton(this);
        mHasTaskbarButton = true;

        if (QtWin::isCompositionEnabled()) {
            setAttribute(Qt::WA_NoSystemBackground);
            QtWin::enableBlurBehindWindow(this);
            QtWin::extendFrameIntoClientArea(this, QMargins(-1, -1, -1, -1));
        }
    }

    if (QSysInfo::WindowsVersion < QSysInfo::WV_WINDOWS7) {
        ui.centralWidget->setStyleSheet("QPushButton { padding: 2px; border: 1px solid #acacac; background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #eaeaea, stop:1 #e5e5e5);} QPushButton:hover { border: 1px solid #7eb4ea;	background-color: #e4f0fc; }");
    }
#endif

    setMinimumSize(size());
    setWindowFlags(windowFlags() ^ Qt::WindowMaximizeButtonHint);

    // Actions
    connect(ui.screenPushButton, &QPushButton::clicked, this, &LightscreenWindow::screenHotkey);
    connect(ui.areaPushButton  , &QPushButton::clicked, this, &LightscreenWindow::areaHotkey);
    connect(ui.windowPushButton, &QPushButton::clicked, this, &LightscreenWindow::windowPickerHotkey);

    connect(ui.optionsPushButton, &QPushButton::clicked, this, &LightscreenWindow::showOptions);
    connect(ui.folderPushButton , &QPushButton::clicked, this, &LightscreenWindow::goToFolder);

    // Shortcuts
    mGlobalHotkeys = new UGlobalHotkeys(this);

    connect(mGlobalHotkeys, &UGlobalHotkeys::activated, [&](size_t id) {
        action(id);
    });

    // Uploader
    connect(Uploader::instance(), &Uploader::progressChanged, this, &LightscreenWindow::uploadProgress);
    connect(Uploader::instance(), &Uploader::done           , this, &LightscreenWindow::showUploaderMessage);
    connect(Uploader::instance(), &Uploader::error          , this, &LightscreenWindow::showUploaderError);

    // Manager
    connect(ScreenshotManager::instance(), &ScreenshotManager::confirm,           this, &LightscreenWindow::preview);
    connect(ScreenshotManager::instance(), &ScreenshotManager::windowCleanup,     this, &LightscreenWindow::cleanup);
    connect(ScreenshotManager::instance(), &ScreenshotManager::activeCountChange, this, &LightscreenWindow::updateStatus);

    if (!settings()->contains("file/format")) {
        showOptions();  // There are no options (or the options config is invalid or incomplete)
    } else {
        QTimer::singleShot(0   , this, &LightscreenWindow::applySettings);
        QTimer::singleShot(5000, this, &LightscreenWindow::checkForUpdates);
    }
}

LightscreenWindow::~LightscreenWindow()
{
    settings()->setValue("lastScreenshot", mLastScreenshot);
    settings()->sync();
    mGlobalHotkeys->unregisterAllHotkeys();
}

void LightscreenWindow::action(int mode)
{
    if (mode <= Screenshot::SelectedWindow) {
        screenshotAction((Screenshot::Mode)mode);
    } else if (mode == ShowMainWindow) {
        show();
    } else if (mode == OpenScreenshotFolder) {
        goToFolder();
    } else {
        qWarning() << "Unknown hotkey ID: " << mode;
    }
}

void LightscreenWindow::areaHotkey()
{
    screenshotAction(Screenshot::SelectedArea);
}

void LightscreenWindow::checkForUpdates()
{
    if (settings()->value("options/disableUpdater", false).toBool()) {
        return;
    }

    if (settings()->value("lastUpdateCheck").toInt() + 7
            > QDate::currentDate().dayOfYear()) {
        return;    // If 7 days have not passed since the last update check.
    }

    mUpdater = new Updater(this);

    connect(mUpdater, &Updater::done, this, &LightscreenWindow::updaterDone);
    mUpdater->check();
}

void LightscreenWindow::cleanup(const Screenshot::Options &options)
{
    // Reversing settings
    if (settings()->value("options/hide").toBool()) {
#ifndef Q_OS_LINUX // X is not quick enough and the notification ends up everywhere but in the icon
        if (settings()->value("options/tray").toBool() && mTrayIcon) {
            mTrayIcon->show();
        }
#endif

        if (mPreviewDialog) {
            if (mPreviewDialog->count() <= 1 && mWasVisible) {
                show();
            }

            mPreviewDialog->show();
        } else if (mWasVisible) {
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
        } else {
#ifdef Q_OS_WIN
            QSound::play("afakepathtomakewindowsplaythedefaultsoundtheresprobablyabetterwaybuticantbebothered");
#else
            QSound::play("sound/ls.error.wav");
#endif
        }

    }

    updateStatus();

    if (options.result != Screenshot::Success) {
        return;
    }

    mLastScreenshot = options.fileName;
}

void LightscreenWindow::closeToTrayWarning()
{
    if (!settings()->value("options/closeToTrayWarning", true).toBool()) {
        return;
    }

    mLastMessage = 3;
    mTrayIcon->showMessage(tr("Closed to tray"), tr("Lightscreen will keep running, you can disable this in the options menu."));
    settings()->setValue("options/closeToTrayWarning", false);
}

bool LightscreenWindow::closingWithoutTray()
{
    if (settings()->value("options/disableHideAlert", false).toBool()) {
        return false;
    }

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Lightscreen"));
    msgBox.setText(tr("You have chosen to hide Lightscreen when there's no system tray icon, so you will not be able to access the program <b>unless you have selected a hotkey to do so</b>.<br>What do you want to do?"));
    msgBox.setIcon(QMessageBox::Warning);

    msgBox.setStyleSheet("QPushButton { padding: 4px 8px; }");

    auto enableButton      = msgBox.addButton(tr("Hide but enable tray"), QMessageBox::ActionRole);
    auto enableQuietButton = msgBox.addButton(tr("Hide and don't warn"), QMessageBox::ActionRole);
    auto hideButton        = msgBox.addButton(tr("Just hide"), QMessageBox::ActionRole);
    auto abortButton       = msgBox.addButton(QMessageBox::Cancel);

    Q_UNUSED(abortButton);

    msgBox.exec();

    if (msgBox.clickedButton() == hideButton) {
        return true;
    } else if (msgBox.clickedButton() == enableQuietButton) {
        settings()->setValue("options/disableHideAlert", true);
        applySettings();
        return true;
    } else if (msgBox.clickedButton() == enableButton) {
        settings()->setValue("options/tray", true);
        applySettings();
        return true;
    }

    return false; // Cancel.
}

void LightscreenWindow::createUploadMenu()
{
    auto imgurMenu = new QMenu(tr("Upload"));

    auto uploadAction = new QAction(os::icon("imgur"), tr("&Upload last"), imgurMenu);
    uploadAction->setToolTip(tr("Upload the last screenshot you took to imgur.com"));
    connect(uploadAction, &QAction::triggered, this, &LightscreenWindow::uploadLast);

    auto cancelAction = new QAction(os::icon("no"), tr("&Cancel upload"), imgurMenu);
    cancelAction->setToolTip(tr("Cancel the currently uploading screenshots"));
    cancelAction->setEnabled(false);

    connect(this, &LightscreenWindow::uploading, cancelAction, &QAction::setEnabled);
    connect(cancelAction, &QAction::triggered, this, &LightscreenWindow::uploadCancel);

    auto historyAction = new QAction(os::icon("view-history"), tr("View &History"), imgurMenu);
    connect(historyAction, &QAction::triggered, this, &LightscreenWindow::showHistoryDialog);

    imgurMenu->addAction(uploadAction);
    imgurMenu->addAction(cancelAction);
    imgurMenu->addAction(historyAction);
    imgurMenu->addSeparator();

    connect(imgurMenu, &QMenu::aboutToShow, this, [&, imgurMenu] {
        imgurMenu->actions().at(0)->setEnabled(!mLastScreenshot.isEmpty());
    });

    ui.imgurPushButton->setMenu(imgurMenu);
}

void LightscreenWindow::goToFolder()
{
#ifdef Q_OS_WIN
    if (!mLastScreenshot.isEmpty() && QFile::exists(mLastScreenshot)) {
        QProcess::startDetached("explorer /select, \"" + mLastScreenshot + "\"");
    } else {
#endif
        QDir path(settings()->value("file/target").toString());

        // We might want to go to the folder without it having been created by taking a screenshot yet.
        if (!path.exists()) {
            path.mkpath(path.absolutePath());
        }

        QDesktopServices::openUrl(QUrl::fromLocalFile(path.absolutePath() + QDir::separator()));
#ifdef Q_OS_WIN
    }
#endif
}

void LightscreenWindow::messageClicked()
{
    if (mLastMessage == 1) {
        goToFolder();
    } else if (mLastMessage == 3) {
        QTimer::singleShot(0, this, &LightscreenWindow::showOptions);
    } else {
        QDesktopServices::openUrl(QUrl(Uploader::instance()->lastUrl()));
    }
}

void LightscreenWindow::executeArgument(const QString &message)
{
    if (message == "--wake") {
        show();
        os::setForegroundWindow(this);
        qApp->alert(this, 2000);
    } else if (message == "--screen") {
        screenshotAction(Screenshot::WholeScreen);
    } else if (message == "--area") {
        screenshotAction(Screenshot::SelectedArea);
    } else if (message == "--activewindow") {
        screenshotAction(Screenshot::ActiveWindow);
    } else if (message == "--pickwindow") {
        screenshotAction(Screenshot::SelectedWindow);
    } else if (message == "--folder") {
        action(OpenScreenshotFolder);
    } else if (message == "--uploadlast") {
        uploadLast();
    } else if (message == "--viewhistory") {
        showHistoryDialog();
    } else if (message == "--options") {
        showOptions();
    } else if (message == "--quit") {
        qApp->quit();
    }
}

void LightscreenWindow::executeArguments(const QStringList &arguments)
{
    // If we just have the default argument, call "--wake"
    if (arguments.count() == 1 && (arguments.at(0) == qApp->arguments().at(0) || arguments.at(0).contains(QFileInfo(qApp->applicationFilePath()).fileName()))) {
        executeArgument("--wake");
        return;
    }

    for (auto argument : arguments) {
        executeArgument(argument);
    }
}

void LightscreenWindow::notify(const Screenshot::Result &result)
{
    switch (result) {
    case Screenshot::Success:
        mTrayIcon->setIcon(QIcon(":/icons/lightscreen.yes"));

        if (mHasTaskbarButton) {
            mTaskbarButton->setOverlayIcon(os::icon("yes"));
        }

        setWindowTitle(tr("Success!"));
        break;
    case Screenshot::Failure:
        mTrayIcon->setIcon(QIcon(":/icons/lightscreen.no"));
        setWindowTitle(tr("Failed!"));

        if (mHasTaskbarButton) {
            mTaskbarButton->setOverlayIcon(os::icon("no"));
        }

        break;
    case Screenshot::Cancel:
        setWindowTitle(tr("Cancelled!"));
        break;
    }

    QTimer::singleShot(2000, this, &LightscreenWindow::restoreNotification);
}

void LightscreenWindow::preview(Screenshot *screenshot)
{
    if (screenshot->options().preview) {
        if (!mPreviewDialog) {
            mPreviewDialog = new PreviewDialog(this);
        }

        mPreviewDialog->add(screenshot);
    } else {
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
        } else {
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

    if (answer == 0) {
        emit finished();
    }
}

void LightscreenWindow::restoreNotification()
{
    if (mTrayIcon) {
        mTrayIcon->setIcon(QIcon(":/icons/lightscreen.small"));
    }

    if (mHasTaskbarButton) {
        mTaskbarButton->clearOverlayIcon();
        mTaskbarButton->progress()->setVisible(false);
        mTaskbarButton->progress()->stop();
        mTaskbarButton->progress()->reset();
    }

    updateStatus();
}

void LightscreenWindow::screenshotAction(Screenshot::Mode mode)
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

#ifndef Q_OS_LINUX // X is not quick enough and the notification ends up everywhere but in the icon
        if (mTrayIcon) {
            mTrayIcon->hide();
        }
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
        if (mLastMode == Screenshot::None) {
            mLastMode = mode;

            QTimer::singleShot(delayms, this, [&] {
                screenshotAction(mLastMode);
            });
            return;
        } else {
            mode = mLastMode;
            mLastMode = Screenshot::None;
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
        options.urlClipboard   = settings()->value("options/urlClipboard",   false).toBool();
        options.preview        = settings()->value("options/preview",        false).toBool();
        options.magnify        = settings()->value("options/magnify",        false).toBool();
        options.cursor         = settings()->value("options/cursor",         true).toBool();
        options.saveAs         = settings()->value("options/saveAs",         false).toBool();
        options.animations     = settings()->value("options/animations",     true)  .toBool();
        options.replace        = settings()->value("options/replace",        false).toBool();
        options.upload         = settings()->value("options/uploadAuto",     false).toBool();
        options.optimize       = settings()->value("options/optimize",       false).toBool();

        options.uploadService  = Uploader::serviceName(settings()->value("upload/service", 0).toInt());

        Screenshot::NamingOptions namingOptions;
        namingOptions.naming       = (Screenshot::Naming) settings()->value("file/naming").toInt();
        namingOptions.leadingZeros = settings()->value("options/naming/leadingZeros", 0).toInt();
        namingOptions.flip         = settings()->value("options/flip", false).toBool();
        namingOptions.dateFormat   = settings()->value("options/naming/dateFormat", "yyyy-MM-dd").toString();

        options.namingOptions = namingOptions;

        mDoCache = true;
    }

    options.mode = mode;

    ScreenshotManager::instance()->take(options);
}

void LightscreenWindow::screenshotActionTriggered(QAction *action)
{
    screenshotAction(action->data().value<Screenshot::Mode>());
}

void LightscreenWindow::screenHotkey()
{
    screenshotAction(Screenshot::WholeScreen);
}

void LightscreenWindow::showHotkeyError(const QStringList &hotkeys)
{
    static bool dontShow = false;

    if (dontShow) {
        return;
    }

    QString messageText;

    messageText = tr("Some hotkeys could not be registered, they might already be in use");

    if (hotkeys.count() > 1) {
        messageText += tr("<br>The failed hotkeys are the following:") + "<ul>";

        for (auto hotkey : hotkeys) {
            messageText += QString("%1%2%3").arg("<li><b>").arg(hotkey).arg("</b></li>");
        }

        messageText += "</ul>";
    } else {
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
        QTimer::singleShot(10, this, &LightscreenWindow::quit);
    } else if (msgBox.clickedButton() == changeButton) {
        showOptions();
    } else if (msgBox.clickedButton() == disableButton) {
        for (auto hotkey : hotkeys) {
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
    mGlobalHotkeys->unregisterAllHotkeys();
    QPointer<OptionsDialog> optionsDialog = new OptionsDialog(this);

    optionsDialog->exec();
    optionsDialog->deleteLater();

    applySettings();
}

void LightscreenWindow::showScreenshotMessage(const Screenshot::Result &result, const QString &fileName)
{
    if (result == Screenshot::Cancel) {
        return;
    }

    // Showing message.
    QString title;
    QString message;

    if (result == Screenshot::Success) {
        title = QFileInfo(fileName).fileName();

        if (settings()->value("file/target").toString().isEmpty()) {
            message = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
        } else {
            message = tr("Saved to \"%1\"").arg(settings()->value("file/target").toString());
        }
    } else {
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

    notify(Screenshot::Failure);
}

void LightscreenWindow::showUploaderMessage(const QString &fileName, const QString &url)
{
    if (mTrayIcon && settings()->value("options/message").toBool() && !url.isEmpty()) {
        QString screenshot = QFileInfo(fileName).fileName();

        if (screenshot.startsWith(".lstemp.")) {
            screenshot = tr("Screenshot");
        }

        mLastMessage = 2;
        mTrayIcon->showMessage(tr("%1 uploaded").arg(screenshot), tr("Click here to go to %1").arg(url));
    }

    updateStatus();
}

void LightscreenWindow::toggleVisibility()
{
    if (isVisible()) {
        hide();
    } else {
        show();
        os::setForegroundWindow(this);
    }
}

void LightscreenWindow::updateStatus()
{
    int uploadCount = Uploader::instance()->uploading();
    int activeCount = ScreenshotManager::instance()->activeCount();

    if (mHasTaskbarButton) {
        mTaskbarButton->progress()->setPaused(true);
        mTaskbarButton->progress()->setVisible(true);
    }

    if (uploadCount > 0) {
        setStatus(tr("%1 uploading").arg(uploadCount));

        if (mHasTaskbarButton) {
            mTaskbarButton->progress()->setRange(0, 100);
            mTaskbarButton->progress()->resume();
        }

        emit uploading(true);
    } else {
        if (activeCount > 1) {
            setStatus(tr("%1 processing").arg(activeCount));
        } else if (activeCount == 1) {
            setStatus(tr("processing"));
        } else {
            setStatus();

            if (mHasTaskbarButton) {
                mTaskbarButton->progress()->setVisible(false);
            }
        }

        emit uploading(false);
    }
}

void LightscreenWindow::updaterDone(bool result)
{
    mUpdater->deleteLater();

    settings()->setValue("lastUpdateCheck", QDate::currentDate().dayOfYear());

    if (!result) {
        return;
    }

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
        QDesktopServices::openUrl(QUrl("https://lightscreen.com.ar/whatsnew?from=" + qApp->applicationVersion()));
    } else if (msgBox.clickedButton() == turnOffButton) {
        settings()->setValue("options/disableUpdater", true);
    }
}

void LightscreenWindow::upload(const QString &fileName)
{
    Uploader::instance()->upload(fileName, Uploader::serviceName(settings()->value("upload/service", 0).toInt()));
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
    if (mHasTaskbarButton) {
        mTaskbarButton->progress()->setVisible(true);
        mTaskbarButton->progress()->setValue(progress);
    }

    if (isVisible() && progress > 0) {
        int uploadCount = Uploader::instance()->uploading();

        if (uploadCount > 1) {
            setWindowTitle(tr("%1% of %2 uploads - Lightscreen").arg(progress).arg(uploadCount));
        } else {
            setWindowTitle(tr("%1% - Lightscreen").arg(progress));
        }
    }
}

void LightscreenWindow::windowHotkey()
{
    screenshotAction(Screenshot::ActiveWindow);
}

void LightscreenWindow::windowPickerHotkey()
{
    screenshotAction(Screenshot::SelectedWindow);
}

void LightscreenWindow::applySettings()
{
    bool tray = settings()->value("options/tray", true).toBool();

    if (tray && !mTrayIcon) {
        createTrayIcon();
        mTrayIcon->setVisible(true);
    } else if (!tray && mTrayIcon) {
        mTrayIcon->setVisible(false);
    }

    connectHotkeys();

    mDoCache = false;

    if (settings()->value("lastScreenshot").isValid() && mLastScreenshot.isEmpty()) {
        mLastScreenshot = settings()->value("lastScreenshot").toString();
    }

    os::setStartup(settings()->value("options/startup").toBool(), settings()->value("options/startupHide").toBool());
}

void LightscreenWindow::connectHotkeys()
{
    const QStringList actions = {"screen", "window", "area", "windowPicker", "open", "directory"};
    QStringList failed;
    size_t id = Screenshot::WholeScreen;

    for (auto action : actions) {
        if (settings()->value("actions/" + action + "/enabled").toBool()) {
            if (!mGlobalHotkeys->registerHotkey(settings()->value("actions/" + action + "/hotkey").toString(), id)) {
                failed << action;
            }
        }

        id++;
    }

    if (!failed.isEmpty()) {
        showHotkeyError(failed);
    }
}

void LightscreenWindow::createTrayIcon()
{
    mTrayIcon = new QSystemTrayIcon(QIcon(":/icons/lightscreen.small"), this);
    updateStatus();

    connect(mTrayIcon, &QSystemTrayIcon::messageClicked, this, &LightscreenWindow::messageClicked);
    connect(mTrayIcon, &QSystemTrayIcon::activated     , this, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason != QSystemTrayIcon::DoubleClick) return;
        toggleVisibility();
    });

    auto hideAction = new QAction(QIcon(":/icons/lightscreen.small"), tr("Show&/Hide"), mTrayIcon);
    connect(hideAction, &QAction::triggered, this, &LightscreenWindow::toggleVisibility);

    auto screenAction = new QAction(os::icon("screen"), tr("&Screen"), mTrayIcon);
    screenAction->setData(QVariant::fromValue<Screenshot::Mode>(Screenshot::WholeScreen));

    auto windowAction = new QAction(os::icon("window"), tr("Active &Window"), this);
    windowAction->setData(QVariant::fromValue<Screenshot::Mode>(Screenshot::ActiveWindow));

    auto windowPickerAction = new QAction(os::icon("pickWindow"), tr("&Pick Window"), this);
    windowPickerAction->setData(QVariant::fromValue<Screenshot::Mode>(Screenshot::SelectedWindow));

    auto areaAction = new QAction(os::icon("area"), tr("&Area"), mTrayIcon);
    areaAction->setData(QVariant::fromValue<Screenshot::Mode>(Screenshot::SelectedArea));

    auto screenshotGroup = new QActionGroup(mTrayIcon);
    screenshotGroup->addAction(screenAction);
    screenshotGroup->addAction(areaAction);
    screenshotGroup->addAction(windowAction);
    screenshotGroup->addAction(windowPickerAction);

    connect(screenshotGroup, &QActionGroup::triggered, this, &LightscreenWindow::screenshotActionTriggered);

    // Duplicated for the screenshot button :(
    auto uploadAction = new QAction(os::icon("imgur"), tr("&Upload last"), mTrayIcon);
    uploadAction->setToolTip(tr("Upload the last screenshot you took to imgur.com"));
    connect(uploadAction, &QAction::triggered, this, &LightscreenWindow::uploadLast);

    auto cancelAction = new QAction(os::icon("no"), tr("&Cancel upload"), mTrayIcon);
    cancelAction->setToolTip(tr("Cancel the currently uploading screenshots"));
    cancelAction->setEnabled(false);
    connect(this, &LightscreenWindow::uploading, cancelAction, &QAction::setEnabled);
    connect(cancelAction, &QAction::triggered, this, &LightscreenWindow::uploadCancel);

    auto historyAction = new QAction(os::icon("view-history"), tr("View History"), mTrayIcon);
    connect(historyAction, &QAction::triggered, this, &LightscreenWindow::showHistoryDialog);
    //

    auto optionsAction = new QAction(os::icon("configure"), tr("View &Options"), mTrayIcon);
    connect(optionsAction, &QAction::triggered, this, &LightscreenWindow::showOptions);

    auto goAction = new QAction(os::icon("folder"), tr("&Go to Folder"), mTrayIcon);
    connect(goAction, &QAction::triggered, this, &LightscreenWindow::goToFolder);

    auto quitAction = new QAction(tr("&Quit"), mTrayIcon);
    connect(quitAction, &QAction::triggered, this, &LightscreenWindow::quit);

    auto screenshotMenu = new QMenu(tr("Screenshot"));
    screenshotMenu->addAction(screenAction);
    screenshotMenu->addAction(areaAction);
    screenshotMenu->addAction(windowAction);
    screenshotMenu->addAction(windowPickerAction);

    // Duplicated for the screenshot button :(
    auto imgurMenu = new QMenu(tr("Upload"));
    imgurMenu->addAction(uploadAction);
    imgurMenu->addAction(cancelAction);
    imgurMenu->addAction(historyAction);
    imgurMenu->addSeparator();

    auto trayIconMenu = new QMenu;
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
    } else {
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
    if (event->type() == QEvent::Show) {
        QPoint savedPosition = settings()->value("position").toPoint();

        if (!savedPosition.isNull() && qApp->desktop()->availableGeometry().contains(QRect(savedPosition, size()))) {
            move(savedPosition);
        }

        if (mHasTaskbarButton) {
            mTaskbarButton->setWindow(windowHandle());
        }
    } else if (event->type() == QEvent::Hide) {
        settings()->setValue("position", pos());
    } else if (event->type() == QEvent::Close) {
        if (settings()->value("options/tray").toBool() && settings()->value("options/closeHide").toBool()) {
            closeToTrayWarning();
            hide();
        } else if (settings()->value("options/closeHide").toBool()) {
            if (closingWithoutTray()) {
                hide();
            }
        } else {
            quit();
        }
    } else if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
#ifdef Q_WS_MAC
        if (keyEvent->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Period) {
            keyEvent->ignore();

            if (isVisible()) {
                toggleVisibility();
            }

            return false;
        } else
#endif
            if (!keyEvent->modifiers() && keyEvent->key() == Qt::Key_Escape) {
                keyEvent->ignore();

                if (isVisible()) {
                    toggleVisibility();
                }

                return false;
            }
    } else if (event->type() == QEvent::LanguageChange) {
        ui.retranslateUi(this);
        resize(minimumSizeHint());
    }


    return QMainWindow::event(event);
}
