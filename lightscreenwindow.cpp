/*
 * Qt includes
 */
#include <QDate>
#include <QDesktopServices>
#include <QFileInfo>
#include <QHttp>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QProcess>
#include <QSettings>
#include <QSound>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>
#include <QKeyEvent>

#include <QElapsedTimer>
#include <QDebug>

#if defined(Q_WS_WIN)
  #include <windows.h>
#endif

/*
 * Lightscreen includes
 */
#include "lightscreenwindow.h"
#include "dialogs/optionsdialog.h"
#include "dialogs/previewdialog.h"

#include "tools/globalshortcut/globalshortcutmanager.h"
#include "tools/os.h"
#include "tools/screenshot.h"
#include "tools/screenshotmanager.h"

#include "updater/updater.h"

LightscreenWindow::LightscreenWindow(QWidget *parent) :
  QDialog(parent),
  mDoCache(false),
  mHideTrigger(false),
  mReviveMain(false),
  mWasVisible(true),
  mIsOptimizing(false),
  mLastMode(-1),
  mLastScreenshot(QString())
{
  os::aeroGlass(this);
  os::translate(settings()->value("options/language").toString());

  ui.setupUi(this);

  setMaximumSize(size());
  setMinimumSize(size());

  setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint); // Remove the what's this button, no real use in the main window.

  // Actions
  connect(ui.optionsPushButton   , SIGNAL(clicked()), this, SLOT(showOptions()));
  connect(ui.hidePushButton      , SIGNAL(clicked()), this, SLOT(toggleVisibility()));
  connect(ui.screenshotPushButton, SIGNAL(clicked()), this, SLOT(showScreenshotMenu()));

  // Manager
  connect(ScreenshotManager::instance(), SIGNAL(confirm(Screenshot*)),               this, SLOT(preview(Screenshot*)));
  connect(ScreenshotManager::instance(), SIGNAL(windowCleanup(Screenshot::Options)), this, SLOT(cleanup(Screenshot::Options)));

  if (!settings()->contains("file/format")) {
    showOptions();  // There are no options (or the options config is invalid or incomplete)
  }
  else {
    QTimer::singleShot(0, this, SLOT(applySettings()));
    QTimer::singleShot(5000, this, SLOT(checkForUpdates()));
  }
}

LightscreenWindow::~LightscreenWindow()
{
  GlobalShortcutManager::instance()->clear();
  delete mTrayIcon;
}

/*
 * Slots
 */

void LightscreenWindow::action(int mode)
{
  if (mode == 4) {
    goToFolder();
  }
  else {
    show();
  }
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
    return false;
  }
  else if (msgBox.clickedButton() == enableAndDenotifyButton) {
    settings()->setValue("options/disableHideAlert", true);
    applySettings();
    return false;
  }
  else if (msgBox.clickedButton() == enableButton) {
    settings()->setValue("options/tray", true);
    applySettings();
    return false;
  }

  return true; // Cancel
}

void LightscreenWindow::cleanup(Screenshot::Options options)
{
  // Reversing settings
  if (settings()->value("options/hide").toBool()) {
    if (settings()->value("options/tray").toBool() && mTrayIcon) {
      mTrayIcon->show();
    }

    if (PreviewDialog::isActive()) {
      if (PreviewDialog::instance()->count() <= 1 && mWasVisible) {
        show();
      }

      PreviewDialog::instance()->show();
    }
    else {
      if (mWasVisible) {
        show();
      }
    }

    mHideTrigger = false;
  }

  if (settings()->value("options/tray").toBool() && mTrayIcon) {
    notify(options.result);

    if (settings()->value("options/message").toBool() && options.file) {
      showScreenshotMessage(options.result, options.fileName);
    }
  }

  if (settings()->value("options/playSound", false).toBool()) {
    if (options.result == Screenshot::Success) {
      QSound::play("sounds/ls.screenshot.wav");
    }
    else {
      QSound::play("afakepathtomakewindowsplaythedefaultsoundtheresprobablyabetterwaybuticantbebothered");
    }
  }

  if (options.result == Screenshot::Success
   && settings()->value("options/optipng").toBool()
   && settings()->value("file/format").toInt() == Screenshot::PNG)
  {
    compressPng(options.fileName);
  }

  if (options.result == Screenshot::Success && options.file)  {
    mLastScreenshot = options.fileName;
  }
}

void LightscreenWindow::goToFolder()
{
#ifdef Q_WS_WIN
  if (!mLastScreenshot.isEmpty()) {
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
#ifdef Q_WS_WIN
  }
#endif
}

void LightscreenWindow::messageReceived(const QString message)
{
  if (message == "-wake") {
    qApp->alert(this, 500);
    show();

    return;
  }

  if (message == "-screen")
    screenshotAction();

  if (message == "-area")
    screenshotAction(2);

  if (message == "-activewindow")
    screenshotAction(1);

  if (message == "-pickwindow")
    screenshotAction(3);

  if (message == "-folder")
    action(4);
}

void LightscreenWindow::messageClicked()
{
  goToFolder();
}

void LightscreenWindow::preview(Screenshot* screenshot)
{
  if (screenshot->options().preview) {
    PreviewDialog::instance()->add(screenshot);
  }
  else {
    screenshot->confirm(true);
  }
}

void LightscreenWindow::restoreNotification()
{
  if (mTrayIcon)
    mTrayIcon->setIcon(QIcon(":/icons/lightscreen.small"));

  setWindowTitle(tr("Lightscreen"));
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

    if (mTrayIcon)
      mTrayIcon->hide();
  }

  // Screenshot delay
  delayms = settings()->value("options/delay", 0).toInt();
  delayms = delayms * 1000; // Converting the delay to milliseconds.

  delayms += 200;

#if defined(Q_WS_WIN)
  if (optionsHide) {
    // When on Windows Vista, the window takes a little bit longer to hide
    if (QSysInfo::WindowsVersion == QSysInfo::WV_VISTA)
      delayms += 200;
  }
#endif

  if (optionsHide && PreviewDialog::isActive()) {
    if (PreviewDialog::instance()->count() >= 1)
      PreviewDialog::instance()->hide();
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
    // Populating the option object that will then be passed to the screenshot engine
    options.file           = settings()->value("file/enabled").toBool();
    options.format         = (Screenshot::Format) settings()->value("file/format").toInt();
    options.prefix         = settings()->value("file/prefix").toString();
    options.directory      = QDir(settings()->value("file/target").toString());
    options.quality        = settings()->value("options/quality", 100).toInt();
    options.currentMonitor = settings()->value("options/currentMonitor", false).toBool();
    options.clipboard      = settings()->value("options/clipboard", true).toBool();
    options.preview        = settings()->value("options/preview", false).toBool();
    options.magnify        = settings()->value("options/magnify", false).toBool();
    options.cursor         = settings()->value("options/cursor" , false).toBool();
    options.saveAs         = settings()->value("options/saveAs" , false).toBool();
    options.animations     = settings()->value("options/animations" , true).toBool();
    options.replace        = settings()->value("options/replace", false).toBool();

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

void LightscreenWindow::screenshotActionTriggered(QAction* action)
{
  screenshotAction(action->data().toInt());
}

void LightscreenWindow::showOptions()
{
  GlobalShortcutManager::clear();

  QPointer<OptionsDialog> optionsDialog = new OptionsDialog(this);

  optionsDialog->exec();
  optionsDialog->deleteLater();

  applySettings();
}

void LightscreenWindow::showScreenshotMessage(Screenshot::Result result, QString fileName)
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

  mTrayIcon->showMessage(title, message);
}

void LightscreenWindow::showScreenshotMenu()
{
  // This slot is called only on the first click
  QMenu *buttonMenu = new QMenu;

  QAction *screenAction = new QAction(QIcon(":/icons/screen"), tr("&Screen"), buttonMenu);
  screenAction->setData(QVariant(0));

  QAction *windowAction = new QAction(QIcon(":/icons/window"),tr("Active &Window"), buttonMenu);
  windowAction->setData(QVariant(1));

  QAction *windowPickerAction = new QAction(QIcon(":/icons/picker"), tr("&Pick Window"), buttonMenu);
  windowPickerAction->setData(QVariant(3));

  QAction *areaAction = new QAction(QIcon(":/icons/area"), tr("&Area"), buttonMenu);
  areaAction->setData(QVariant(2));

  QAction *goAction = new QAction(QIcon(":/icons/folder"), tr("&Go to Folder"), buttonMenu);
  connect(goAction, SIGNAL(triggered()), this, SLOT(goToFolder()));

  QActionGroup *screenshotGroup = new QActionGroup(buttonMenu);
  screenshotGroup->addAction(screenAction);
  screenshotGroup->addAction(windowAction);
  screenshotGroup->addAction(windowPickerAction);
  screenshotGroup->addAction(areaAction);

  connect(screenshotGroup, SIGNAL(triggered(QAction*)), this, SLOT(screenshotActionTriggered(QAction*)));

  buttonMenu->addAction(screenAction);
  buttonMenu->addAction(areaAction);
  buttonMenu->addAction(windowAction);
  buttonMenu->addAction(windowPickerAction);
  buttonMenu->addSeparator();
  buttonMenu->addAction(goAction);

  ui.screenshotPushButton->setMenu(buttonMenu);
  ui.screenshotPushButton->showMenu();
}

void LightscreenWindow::notify(Screenshot::Result result)
{
  switch (result)
  {
  case Screenshot::Success:
    mTrayIcon->setIcon(QIcon(":/icons/lightscreen.yes"));
    setWindowTitle(tr("Success!"));
    break;
  case Screenshot::Fail:
    mTrayIcon->setIcon(QIcon(":/icons/lightscreen.no"));
    setWindowTitle(tr("Failed!"));
    break;
  case Screenshot::Cancel:
    setWindowTitle(tr("Cancelled!"));
    break;
  }

  QTimer::singleShot(1500, this, SLOT(restoreNotification()));
}

void LightscreenWindow::showHotkeyError(QStringList hotkeys)
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
     QTimer::singleShot(10, this, SLOT(accept()));
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

void LightscreenWindow::toggleVisibility(QSystemTrayIcon::ActivationReason reason)
{
  if (reason != QSystemTrayIcon::DoubleClick)
    return;

  if (isVisible()) {
    if (settings()->value("options/tray").toBool() == false
        && closingWithoutTray())
      return;

    hide();
  }
  else {
    show();
  }
}

// Aliases
void LightscreenWindow::windowHotkey()       { screenshotAction(1); }
void LightscreenWindow::windowPickerHotkey() { screenshotAction(3); }
void LightscreenWindow::areaHotkey()         { screenshotAction(2); }

/*
 * Private
 */

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

  os::setStartup(settings()->value("options/startup").toBool(), settings()->value("options/startupHide").toBool());
}

void LightscreenWindow::compressPng(QString fileName)
{
#if defined(Q_OS_UNIX)
  QProcess::startDetached("optipng " + fileName + " -quiet");
#else
  ShellExecuteW(NULL, NULL, (LPCWSTR)QString("optipng.exe").toStdWString().data(), (LPCWSTR)fileName.toStdWString().data(), NULL, SW_HIDE);
#endif
}

void LightscreenWindow::connectHotkeys()
{
  // Set to true because if the hotkey is disabled it will show an error.
  bool screen = true, area = true, window = true, open = true, directory = true;

  if (settings()->value("actions/screen/enabled").toBool())
    screen = GlobalShortcutManager::instance()->connect(settings()->value(
        "actions/screen/hotkey").value<QKeySequence> (), this, SLOT(screenshotAction()));

  if (settings()->value("actions/area/enabled").toBool())
    area   = GlobalShortcutManager::instance()->connect(settings()->value(
        "actions/area/hotkey").value<QKeySequence> (), this, SLOT(areaHotkey()));

  if (settings()->value("actions/window/enabled").toBool())
    window = GlobalShortcutManager::instance()->connect(settings()->value(
        "actions/window/hotkey").value<QKeySequence> (), this, SLOT(windowHotkey()));

  if (settings()->value("actions/windowPicker/enabled").toBool())
    window = GlobalShortcutManager::instance()->connect(settings()->value(
        "actions/windowPicker/hotkey").value<QKeySequence> (), this, SLOT(windowPickerHotkey()));

  if (settings()->value("actions/open/enabled").toBool())
    open   = GlobalShortcutManager::instance()->connect(settings()->value(
        "actions/open/hotkey").value<QKeySequence> (), this, SLOT(show()));

  if (settings()->value("actions/directory/enabled").toBool())
    directory = GlobalShortcutManager::instance()->connect(settings()->value(
        "actions/directory/hotkey").value<QKeySequence> (), this, SLOT(goToFolder()));

  QStringList failed;
  if (!screen)    failed << "screen";
  if (!area)      failed << "area";
  if (!window)    failed << "window";
  if (!open)      failed << "open";
  if (!directory) failed << "directory";

  if (!failed.isEmpty())
    showHotkeyError(failed);
}

void LightscreenWindow::createTrayIcon()
{
  mTrayIcon = new QSystemTrayIcon(QIcon(":/icons/lightscreen.small"), this);
  mTrayIcon->setToolTip(qApp->applicationName() + " " + qApp->applicationVersion());

  connect(mTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(toggleVisibility(QSystemTrayIcon::ActivationReason)));
  connect(mTrayIcon, SIGNAL(messageClicked()), this, SLOT(messageClicked()));

  QAction *hideAction = new QAction(QIcon(":/icons/lightscreen.small"), tr("Show&/Hide"), mTrayIcon);
  connect(hideAction, SIGNAL(triggered()), this, SLOT(toggleVisibility()));

  QAction *screenAction = new QAction(QIcon(":/icons/screen"), tr("&Screen"), mTrayIcon);
  screenAction->setData(QVariant(0));

  QAction *windowAction = new QAction(QIcon(":/icons/window"), tr("Active &Window"), this);
  windowAction->setData(QVariant(1));

  QAction *windowPickerAction = new QAction(QIcon(":/icons/picker"), tr("&Pick Window"), this);
  windowPickerAction->setData(QVariant(3));

  QAction *areaAction = new QAction(QIcon(":/icons/area"), tr("&Area"), mTrayIcon);
  areaAction->setData(QVariant(2));

  QActionGroup *screenshotGroup = new QActionGroup(mTrayIcon);
  screenshotGroup->addAction(screenAction);
  screenshotGroup->addAction(areaAction);
  screenshotGroup->addAction(windowAction);
  screenshotGroup->addAction(windowPickerAction);

  connect(screenshotGroup, SIGNAL(triggered(QAction*)), this, SLOT(screenshotActionTriggered(QAction*)));

  QAction *optionsAction = new QAction(QIcon(":/icons/configure"), tr("View &Options"), mTrayIcon);
  connect(optionsAction, SIGNAL(triggered()), this, SLOT(showOptions()));

  QAction *goAction = new QAction(QIcon(":/icons/folder"), tr("&Go to Folder"), mTrayIcon);
  connect(goAction, SIGNAL(triggered()), this, SLOT(goToFolder()));

  QAction *quitAction = new QAction(tr("&Quit"), mTrayIcon);
  connect(quitAction, SIGNAL(triggered()), this, SLOT(accept()));

  QMenu* screenshotMenu = new QMenu("Screenshot");
  screenshotMenu->addAction(screenAction);  
  screenshotMenu->addAction(areaAction);
  screenshotMenu->addAction(windowAction);
  screenshotMenu->addAction(windowPickerAction);

  QMenu * trayIconMenu = new QMenu;
  trayIconMenu->addAction(hideAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addMenu(screenshotMenu);
  trayIconMenu->addAction(optionsAction);
  trayIconMenu->addAction(goAction);
  trayIconMenu->addSeparator();
  trayIconMenu->addAction(quitAction);

  mTrayIcon->setContextMenu(trayIconMenu);
}

QSettings *LightscreenWindow::settings() const
{
  return ScreenshotManager::instance()->settings();
}

void LightscreenWindow::checkForUpdates()
{
  if (settings()->value("options/disableUpdater", false).toBool())
    return;

  if (settings()->value("lastUpdateCheck").toInt() + 7
      > QDate::currentDate().dayOfYear())
    return; // If 7 days have not passed since the last update check.

  connect(Updater::instance(), SIGNAL(done(bool)), this, SLOT(updaterDone(bool)));
  Updater::instance()->check();
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
    QDesktopServices::openUrl(QUrl("http://lightscreen.sourceforge.net/new-version"));
  }
  else if (msgBox.clickedButton() == turnOffButton) {
    settings()->setValue("disableUpdater", true);
  }
}

// Event handling
bool LightscreenWindow::event(QEvent *event)
{
  if (event->type() == QEvent::Hide
   || event->type() == QEvent::Close) {
    settings()->setValue("position", pos());
  }
  else if (event->type() == QEvent::Show) {
    os::aeroGlass(this);
    
    if (!settings()->value("position").toPoint().isNull())
      move(settings()->value("position").toPoint());
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

  return QDialog::event(event);
}


