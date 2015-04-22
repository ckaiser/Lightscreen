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
#include <QCompleter>
#include <QDate>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDirModel>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QInputDialog>
#include <QMenu>
#include <QAction>

#ifdef Q_OS_WIN
  #include <windows.h>
#endif

#include "optionsdialog.h"
#include "namingdialog.h"
#include "historydialog.h"
#include "../tools/os.h"
#include "../tools/screenshot.h"
#include "../tools/screenshotmanager.h"
#include "../tools/uploader/uploader.h"
#include "../updater/updater.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
  QDialog(parent)
{
  ui.setupUi(this);
  setModal(true);

#if defined(Q_WS_X11)
  // KDE-specific style tweaks.
  if (qApp->style()->objectName() == "oxygen") {
      ui.browsePushButton->setMaximumWidth(30);
      ui.namingOptionsButton->setMaximumWidth(30);

      ui.fileGroupBox->setFlat(false);
      ui.startupGroupBox->setFlat(false);
      ui.capturesGroupBox->setFlat(false);
      ui.controlGroupBox->setFlat(false);
      ui.interfaceGroupBox->setFlat(false);
      ui.screenshotsGroupBox->setFlat(false);
      ui.previewGroupBox->setFlat(false);
      ui.updaterGroupBox->setFlat(false);
      ui.historyGroupBox->setFlat(false);
      ui.clipboardGroupBox->setFlat(false);

      ui.optionsTab->layout()->setContentsMargins(0, 0, 6, 0);
      ui.aboutTab->layout()->setMargin(8);
  }
#endif

  QTimer::singleShot(0, this, SLOT(init()));
  QTimer::singleShot(1, this, SLOT(loadSettings()));
}

void OptionsDialog::accepted()
{
  if (hotkeyCollision()) {
    QMessageBox::critical(this, tr("Hotkey conflict"), tr("You have assigned the same hotkeys to more than one action."));
    return;
  }

  if (ui.prefixLineEdit->text().contains(QRegExp("[?:\\\\/*\"<>|]"))) {
    QMessageBox::critical(this, tr("Filename character error"), tr("The filename can't contain any of the following characters: ? : \\ / * \" < > |"));
    return;
  }

  if (!ui.fileGroupBox->isChecked() && !ui.clipboardCheckBox->isChecked()) {
    QMessageBox::critical(this, tr("Final Destination"), tr("You can't take screenshots unless you enable either file saving or the clipboard."));
    return;
  }

  saveSettings();
  accept();
}

void OptionsDialog::checkUpdatesNow()
{
  Updater updater;
  updater.checkWithFeedback();
}

void OptionsDialog::exportSettings()
{
  QString exportFileName = QFileDialog::getSaveFileName(this,
                                                    tr("Export Settings"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + "config.ini",
                                                    tr("Lightscreen Settings (*.ini)"));

  if (exportFileName.isEmpty())
    return;

  if (QFile::exists(exportFileName)) {
    QFile::remove(exportFileName);
  }

  QSettings exportedSettings(exportFileName, QSettings::IniFormat);

  foreach(const QString &key, settings()->allKeys()) {
    exportedSettings.setValue(key, settings()->value(key));
  }
}

void OptionsDialog::imgurAuthorize()
{ // TODO: Tuck this into Uploader
  if (ui.imgurAuthButton->text() == tr("Deauthorize"))
  {
    ui.imgurAuthUserLabel->setText(tr("<i>none</i>"));
    ui.imgurAuthButton->setText(tr("Authorize"));

    ui.imgurAlbumComboBox->setEnabled(false);
    ui.imgurAlbumComboBox->clear();
    ui.imgurAlbumComboBox->addItem(tr("- None -"));

    settings()->setValue("upload/imgur/access_token", "");
    settings()->setValue("upload/imgur/refresh_token", "");
    settings()->setValue("upload/imgur/account_username", "");
    settings()->setValue("upload/imgur/expires_in", 0);
    return;
  }

  openUrl("https://api.imgur.com/oauth2/authorize?client_id=3ebe94c791445c1&response_type=pin"); //TODO: get the client-id from somewhere?

  bool ok;
  QString pin = QInputDialog::getText(this, tr("Imgur Authorization"),
                                           tr("PIN:"), QLineEdit::Normal,
                                           "", &ok);
  if (ok) {
    QByteArray parameters;
    parameters.append(QString("client_id=").toUtf8());
    parameters.append(QUrl::toPercentEncoding("3ebe94c791445c1"));
    parameters.append(QString("&client_secret=").toUtf8());
    parameters.append(QUrl::toPercentEncoding("0546b05d6a80b2092dcea86c57b792c9c9faebf0")); // TODO: TA.png
    parameters.append(QString("&grant_type=pin").toUtf8());
    parameters.append(QString("&pin=").toUtf8());
    parameters.append(QUrl::toPercentEncoding(pin));

    QNetworkRequest request(QUrl("https://api.imgur.com/oauth2/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* reply = Uploader::instance()->nam()->post(request, parameters);
    connect(reply, SIGNAL(finished()), this, SLOT(imgurToken()));

    ui.imgurAuthButton->setText(tr("Authorizing.."));
    ui.imgurAuthButton->setEnabled(false);
  }
}

void OptionsDialog::imgurToken()
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  ui.imgurAuthButton->setEnabled(true);

  if (reply->error() != QNetworkReply::NoError) {
    QMessageBox::critical(this, tr("Imgur Authorization Error"), tr("There's been an error authorizing your account with Imgur, please try again."));
    ui.imgurAuthButton->setText(tr("Authorize"));
    return;
  }

  QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

  settings()->setValue("upload/imgur/access_token"    , imgurResponse.value("access_token").toString());
  settings()->setValue("upload/imgur/refresh_token"   , imgurResponse.value("refresh_token").toString());
  settings()->setValue("upload/imgur/account_username", imgurResponse.value("account_username").toString());
  settings()->setValue("upload/imgur/expires_in"      , imgurResponse.value("expires_in").toInt());

  ui.imgurAuthUserLabel->setText("<b>" + imgurResponse.value("account_username").toString() + "</b>");
  ui.imgurAuthButton->setText(tr("Deauthorize"));

  imgurRequestAlbumList();
}

void OptionsDialog::importSettings()
{
  QString importFileName = QFileDialog::getOpenFileName(this,
                                                        tr("Import Settings"),
                                                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                        tr("Lightscreen Settings (*.ini)"));

  QSettings importedSettings(importFileName, QSettings::IniFormat);

  saveSettings();

  foreach(const QString &key, importedSettings.allKeys()) {
    if (settings()->contains(key))
      settings()->setValue(key, importedSettings.value(key));
  }

  loadSettings();
}

void OptionsDialog::imgurRequestAlbumList() {

  QString username = settings()->value("upload/imgur/account_username").toString();

  if (username.isEmpty()) {
    return;
  }

  ui.imgurAlbumComboBox->clear();
  ui.imgurAlbumComboBox->setEnabled(false);
  ui.imgurAlbumComboBox->addItem(tr("Loading album data..."));

  QNetworkRequest request(QUrl("https://api.imgur.com/3/account/" + username + "/albums/"));
  request.setRawHeader("Authorization", QByteArray("Bearer ") + settings()->value("upload/imgur/access_token").toByteArray());

  QNetworkReply* reply = Uploader::instance()->nam()->get(request);
  connect(reply, SIGNAL(finished()), this, SLOT(imgurAlbumList()));
}

void OptionsDialog::imgurAlbumList() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

  if (reply->error() != QNetworkReply::NoError) {
    if (reply->error() == QNetworkReply::ContentOperationNotPermittedError ||
        reply->error() == QNetworkReply::AuthenticationRequiredError) {
      Uploader::instance()->imgurAuthRefresh();
      connect(Uploader::instance(), &Uploader::imgurAuthRefreshed, this, &OptionsDialog::imgurRequestAlbumList);
    }

    ui.imgurAlbumComboBox->addItem(tr("Loading failed :("));
    return;
  }

  QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

  if (imgurResponse["success"].toBool() != true || imgurResponse["status"].toInt() != 200)
    return;

  QJsonArray albumList = imgurResponse["data"].toArray();

  ui.imgurAlbumComboBox->clear();
  ui.imgurAlbumComboBox->setEnabled(true);
  ui.imgurAlbumComboBox->addItem(tr("- None -"), "");

  int settingsIndex = 0;

  foreach (const QJsonValue &albumValue, albumList) {
      QJsonObject album = albumValue.toObject();
      ui.imgurAlbumComboBox->addItem(album["title"].toString(), album["id"].toString());

      if (album["id"].toString() == settings()->value("upload/imgur/album").toString())
        settingsIndex = ui.imgurAlbumComboBox->count() - 1;
  }

  ui.imgurAlbumComboBox->setCurrentIndex(settingsIndex);
}

void OptionsDialog::loadSettings()
{
  settings()->sync();
  setUpdatesEnabled(false);

  if (!settings()->contains("file/format")) {
    // If there are no settings, get rid of the cancel button so that the user is forced to save them
    ui.buttonBox->clear();
    ui.buttonBox->addButton(QDialogButtonBox::Ok);
  }

  ui.startupCheckBox->toggle();
  ui.trayCheckBox->toggle();
  ui.previewAutocloseCheckBox->toggle();

  QString targetDefault;

  if (ScreenshotManager::instance()->portableMode()) {
    targetDefault = tr("Screenshots");
  }
  else {
    targetDefault = os::getDocumentsPath() + QDir::separator() + tr("Screenshots");
  }

  settings()->beginGroup("file");
    ui.formatComboBox->setCurrentIndex(settings()->value("format", 1).toInt());
    ui.prefixLineEdit->setText(settings()->value("prefix", tr("screenshot.")).toString());
    ui.namingComboBox->setCurrentIndex(settings()->value("naming", 0).toInt());
    ui.targetLineEdit->setText(settings()->value("target", targetDefault).toString());
    ui.fileGroupBox->setChecked(settings()->value("enabled", true).toBool());
  settings()->endGroup();

  settings()->beginGroup("options");
    ui.startupCheckBox->setChecked(settings()->value("startup", false).toBool());
    ui.startupHideCheckBox->setChecked(settings()->value("startupHide", true).toBool());
    ui.hideCheckBox->setChecked(settings()->value("hide", true).toBool());
    ui.delaySpinBox->setValue(settings()->value("delay", 0).toInt());
    flipToggled(settings()->value("flip", false).toBool());
    ui.trayCheckBox->setChecked(settings()->value("tray", true).toBool());
    ui.messageCheckBox->setChecked(settings()->value("message", true).toBool());
    ui.qualitySlider->setValue(settings()->value("quality", 100).toInt());
    ui.playSoundCheckBox->setChecked(settings()->value("playSound", false).toBool());
    ui.updaterCheckBox->setChecked(!settings()->value("disableUpdater", false).toBool());
    ui.magnifyCheckBox->setChecked(settings()->value("magnify", false).toBool());
    ui.cursorCheckBox->setChecked(settings()->value("cursor", true).toBool());
    ui.saveAsCheckBox->setChecked(settings()->value("saveAs", false).toBool());
    ui.previewGroupBox->setChecked(settings()->value("preview", false).toBool());
    ui.previewSizeSpinBox->setValue(settings()->value("previewSize", 300).toInt());
    ui.previewPositionComboBox->setCurrentIndex(settings()->value("previewPosition", 3).toInt());
    ui.previewAutocloseCheckBox->setChecked(settings()->value("previewAutoclose", false).toBool());
    ui.previewAutocloseTimeSpinBox->setValue(settings()->value("previewAutocloseTime", 15).toInt());
    ui.previewAutocloseActionComboBox->setCurrentIndex(settings()->value("previewAutocloseAction", 0).toInt());
    ui.previewDefaultActionComboBox->setCurrentIndex(settings()->value("previewDefaultAction", 0).toInt());
    ui.areaAutocloseCheckBox->setChecked(settings()->value("areaAutoclose", false).toBool());

    // History mode is neat for normal operation but I'll keep it disabled by default on portable mode.
    ui.historyCheckBox->setChecked(settings()->value("history", (ScreenshotManager::instance()->portableMode()) ? false : true).toBool());

    // Advanced
    ui.clipboardCheckBox->setChecked(settings()->value("clipboard", true).toBool());
    ui.imgurClipboardCheckBox->setChecked(settings()->value("imgurClipboard", false).toBool());
    ui.optiPngCheckBox->setChecked(settings()->value("optipng", true).toBool());
    ui.closeHideCheckBox->setChecked(settings()->value("closeHide", true).toBool());
    ui.currentMonitorCheckBox->setChecked(settings()->value("currentMonitor", false).toBool());
    ui.replaceCheckBox->setChecked(settings()->value("replace", false).toBool());
    ui.uploadCheckBox->setChecked(settings()->value("uploadAuto", false).toBool());
    ui.uploadDirectLinkCheckBox->setChecked(settings()->value("uploadDirectLink", false).toBool());

#ifdef Q_OS_WIN
    if (!QFile::exists(qApp->applicationDirPath() + QDir::separator() + "optipng.exe")) {
      ui.optiPngCheckBox->setEnabled(false);
      ui.optiPngLabel->setText("optipng.exe not found");
    }
#elif defined(Q_WS_X11)
    if (!QProcess::startDetached("optipng")) {
      ui.optiPngCheckBox->setChecked(false);
      ui.optiPngCheckBox->setEnabled(false);
      ui.optiPngLabel->setText(tr("Install 'OptiPNG'"));
    }

    //TODO: Sound cue support on Linux
    ui.playSoundCheckBox->setVisible(false);
    ui.playSoundCheckBox->setChecked(false);

    //TODO: Cursor support on X11
    ui.cursorCheckBox->setVisible(false);
    ui.cursorCheckBox->setChecked(false);
#endif
  settings()->endGroup();

  settings()->beginGroup("actions");
    // This toggle is for the first run
    ui.screenCheckBox->toggle();
    ui.areaCheckBox->toggle();
    ui.windowCheckBox->toggle();
    ui.windowPickerCheckBox->toggle();
    ui.openCheckBox->toggle();
    ui.directoryCheckBox->toggle();

    settings()->beginGroup("screen");
      ui.screenCheckBox->setChecked(settings()->value("enabled", true).toBool());
      ui.screenHotkeyWidget->setHotkey(settings()->value("hotkey", "Print").toString());
    settings()->endGroup();

    settings()->beginGroup("area");
      ui.areaCheckBox->setChecked(settings()->value("enabled").toBool());
      ui.areaHotkeyWidget->setHotkey(settings()->value("hotkey", "Ctrl+Print").toString());
    settings()->endGroup();

    settings()->beginGroup("window");
      ui.windowCheckBox->setChecked(settings()->value("enabled").toBool());
      ui.windowHotkeyWidget->setHotkey(settings()->value("hotkey", "Alt+Print").toString());
    settings()->endGroup();

    settings()->beginGroup("windowPicker");
      ui.windowPickerCheckBox->setChecked(settings()->value("enabled").toBool());
      ui.windowPickerHotkeyWidget->setHotkey(settings()->value("hotkey", "Ctrl+Alt+Print").toString());
    settings()->endGroup();

    settings()->beginGroup("open");
      ui.openCheckBox->setChecked(settings()->value("enabled").toBool());
      ui.openHotkeyWidget->setHotkey(settings()->value("hotkey", "Ctrl+PgUp").toString());
    settings()->endGroup();

    settings()->beginGroup("directory");
      ui.directoryCheckBox->setChecked(settings()->value("enabled").toBool());
      ui.directoryHotkeyWidget->setHotkey(settings()->value("hotkey", "Ctrl+PgDown").toString());
    settings()->endGroup();
  settings()->endGroup();

  settings()->beginGroup("upload");
    //ui.serviceComboBox->setCurrentIndex(settings()->value("service").toInt());

    settings()->beginGroup("imgur");
      ui.imgurAuthGroupBox->setChecked(!settings()->value("anonymous", false).toBool());
      ui.imgurAuthUserLabel->setText(settings()->value("account_username", tr("<i>none</i>")).toString());

      if (settings()->value("account_username").toString().isEmpty()) {
        ui.imgurAuthUserLabel->setText(tr("<i>none</i>"));
        ui.imgurAlbumComboBox->setEnabled(false);
      }
      else {
        ui.imgurAuthButton->setText(tr("Deauthorize"));
        ui.imgurAuthUserLabel->setText("<b>" + ui.imgurAuthUserLabel->text() + "</b>");
      }
    settings()->endGroup();
  settings()->endGroup();

  QTimer::singleShot(0, this, SLOT(updatePreview()));
  QTimer::singleShot(1, this, SLOT(imgurRequestAlbumList()));

  setEnabled(true);
  setUpdatesEnabled(true);
}

void OptionsDialog::openUrl(QString url)
{
  if (url == "#aboutqt") {
    qApp->aboutQt();
  }
  else {
    QDesktopServices::openUrl(QUrl(url));
  }
}

void OptionsDialog::saveSettings()
{
  settings()->beginGroup("file");
    settings()->setValue("format", ui.formatComboBox->currentIndex());
    settings()->setValue("prefix", ui.prefixLineEdit->text());
    settings()->setValue("naming", ui.namingComboBox->currentIndex());
    settings()->setValue("target", ui.targetLineEdit->text());
    settings()->setValue("enabled", ui.fileGroupBox->isChecked());
  settings()->endGroup();

  settings()->beginGroup("options");
    settings()->setValue("startup", ui.startupCheckBox->isChecked());
    settings()->setValue("startupHide", ui.startupHideCheckBox->isChecked());
    settings()->setValue("hide", ui.hideCheckBox->isChecked());
    settings()->setValue("delay", ui.delaySpinBox->value());
    settings()->setValue("tray", ui.trayCheckBox->isChecked());
    settings()->setValue("message", ui.messageCheckBox->isChecked());
    settings()->setValue("quality", ui.qualitySlider->value());
    settings()->setValue("playSound", ui.playSoundCheckBox->isChecked());
    // This settings is inverted because the first iteration of the Updater did not have a settings but instead relied on the messagebox choice of the user.
    settings()->setValue("disableUpdater", !ui.updaterCheckBox->isChecked());
    settings()->setValue("magnify", ui.magnifyCheckBox->isChecked());
    settings()->setValue("cursor", ui.cursorCheckBox->isChecked());
    settings()->setValue("saveAs", ui.saveAsCheckBox->isChecked());
    settings()->setValue("preview", ui.previewGroupBox->isChecked());
    settings()->setValue("previewSize", ui.previewSizeSpinBox->value());
    settings()->setValue("previewPosition", ui.previewPositionComboBox->currentIndex());
    settings()->setValue("previewAutoclose", ui.previewAutocloseCheckBox->isChecked());
    settings()->setValue("previewAutocloseTime", ui.previewAutocloseTimeSpinBox->value());
    settings()->setValue("previewAutocloseAction", ui.previewAutocloseActionComboBox->currentIndex());
    settings()->setValue("previewDefaultAction", ui.previewDefaultActionComboBox->currentIndex());
    settings()->setValue("areaAutoclose", ui.areaAutocloseCheckBox->isChecked());
    settings()->setValue("history", ui.historyCheckBox->isChecked());

    // Advanced
    settings()->setValue("closeHide", ui.closeHideCheckBox->isChecked());
    settings()->setValue("clipboard", ui.clipboardCheckBox->isChecked());
    settings()->setValue("imgurClipboard", ui.imgurClipboardCheckBox->isChecked());
    settings()->setValue("optipng", ui.optiPngCheckBox->isChecked());
    settings()->setValue("currentMonitor", ui.currentMonitorCheckBox->isChecked());
    settings()->setValue("replace", ui.replaceCheckBox->isChecked());

    //Upload
    settings()->setValue("uploadAuto", ui.uploadCheckBox->isChecked());
    settings()->setValue("uploadDirectLink", ui.uploadDirectLinkCheckBox->isChecked());

  settings()->endGroup();

  settings()->beginGroup("actions");

    settings()->beginGroup("screen");
      settings()->setValue("enabled", ui.screenCheckBox->isChecked());
      settings()->setValue("hotkey", ui.screenHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->beginGroup("area");
      settings()->setValue("enabled", ui.areaCheckBox->isChecked());
      settings()->setValue("hotkey", ui.areaHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->beginGroup("window");
      settings()->setValue("enabled", ui.windowCheckBox->isChecked());
      settings()->setValue("hotkey", ui.windowHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->beginGroup("windowPicker");
      settings()->setValue("enabled", ui.windowPickerCheckBox->isChecked());
      settings()->setValue("hotkey", ui.windowPickerHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->beginGroup("open");
      settings()->setValue("enabled", ui.openCheckBox->isChecked());
      settings()->setValue("hotkey", ui.openHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->beginGroup("directory");
      settings()->setValue("enabled", ui.directoryCheckBox->isChecked());
      settings()->setValue("hotkey", ui.directoryHotkeyWidget->hotkey());
    settings()->endGroup();

  settings()->endGroup();

  settings()->beginGroup("upload");
    //settings()->setValue("service", ui.serviceComboBox->currentIndex());

    settings()->beginGroup("imgur");
      settings()->setValue("anonymous", !ui.imgurAuthGroupBox->isChecked());
      settings()->setValue("album", ui.imgurAlbumComboBox->property("currentData").toString()); // TODO
    settings()->endGroup();

  settings()->endGroup();
}

void OptionsDialog::updatePreview()
{
  Screenshot::NamingOptions options;

  options.naming       = (Screenshot::Naming) ui.namingComboBox->currentIndex();
  options.flip         = settings()->value("options/flip", false).toBool();
  options.leadingZeros = settings()->value("options/naming/leadingZeros", 0).toInt();
  options.dateFormat   = settings()->value("options/naming/dateFormat", "yyyy-MM-dd").toString();

  if (ui.fileGroupBox->isChecked()) // Only change the naming button when file options are enabled.
    ui.namingOptionsButton->setDisabled((options.naming == Screenshot::Empty));

  QString preview = Screenshot::getName(options,
                                        ui.prefixLineEdit->text(),
                                        QDir(ui.targetLineEdit->text()));

  preview = QString("%1.%2").arg(preview).arg(ui.formatComboBox->currentText().toLower());

  if (preview.length() >= 40) {
    preview.truncate(37);
    preview.append("...");
  }

  ui.previewLabel->setText(preview);
}

void OptionsDialog::viewHistory()
{
    HistoryDialog historyDialog(this);
    historyDialog.exec();
}

//

bool OptionsDialog::event(QEvent* event)
{
  if (event->type() == QEvent::Close || event->type() == QEvent::Hide) {
    settings()->setValue("geometry/optionsDialog", saveGeometry());

    if (!settings()->contains("file/format")) {
      // I'm afraid I can't let you do that, Dave.
      event->ignore();
      return false;
    }
  }
  else if (event->type() == QEvent::Show)
  {
    restoreGeometry(settings()->value("geometry/optionsDialog").toByteArray());
  }

  return QDialog::event(event);
}

#ifdef Q_OS_WIN
// Qt does not send the print screen key as a regular QKeyPress event, so we must use the Windows API
bool OptionsDialog::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
  if (eventType == "windows_generic_MSG") {
    MSG* m = static_cast<MSG*>(message);

    if ((m->message == WM_KEYUP || m->message == WM_SYSKEYUP) && m->wParam == VK_SNAPSHOT) {
        qApp->postEvent(qApp->focusWidget(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Print,  qApp->queryKeyboardModifiers()));
    }
  }

  return QDialog::nativeEvent(eventType, message, result);
}
#endif

//

void OptionsDialog::browse()
{
  QString fileName = QFileDialog::getExistingDirectory(this,
                                                       tr("Select where you want to save the screenshots"),
                                                       ui.targetLineEdit->text());

  if (fileName.isEmpty())
    return;

  ui.targetLineEdit->setText(fileName);
  updatePreview();
}

void OptionsDialog::dialogButtonClicked(QAbstractButton *button)
{
  if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole) {
    QPushButton *pb = qobject_cast<QPushButton*>(button);
    pb->showMenu();
  }
}

void OptionsDialog::restoreDefaults()
{
  QMessageBox msgBox;
  msgBox.setWindowTitle(tr("Lightscreen - Restore Default Options"));
  msgBox.setText(tr("Restoring the default options will cause you to lose all of your current configuration."));
  msgBox.setIcon(QMessageBox::Warning);

  msgBox.addButton(tr("Restore"), QMessageBox::ActionRole);
  QPushButton *dontRestoreButton = msgBox.addButton(tr("Don't Restore"), QMessageBox::ActionRole);

  msgBox.setDefaultButton(dontRestoreButton);
  msgBox.exec();

  if (msgBox.clickedButton() == dontRestoreButton)
    return;

  settings()->clear();
  loadSettings();
}

void OptionsDialog::flipToggled(bool checked)
{
  setUpdatesEnabled(false);

  ui.filenameLayout->removeWidget(ui.prefixLineEdit);
  ui.filenameLayout->removeWidget(ui.namingComboBox);

  if (checked) {
    ui.filenameLayout->addWidget(ui.namingComboBox);
    ui.filenameLayout->addWidget(ui.prefixLineEdit);
  }
  else {
    ui.filenameLayout->addWidget(ui.prefixLineEdit);
    ui.filenameLayout->addWidget(ui.namingComboBox);
  }

  if (ui.prefixLineEdit->text() == tr("screenshot.")
   && checked)
    ui.prefixLineEdit->setText(tr(".screenshot"));

  if (ui.prefixLineEdit->text() == tr(".screenshot")
   && !checked)
    ui.prefixLineEdit->setText(tr("screenshot."));

  setUpdatesEnabled(true); // Avoids flicker
}

void OptionsDialog::init()
{
  // Make the scroll area share the Tab Widget background color
  QPalette optionsPalette = ui.optionsScrollArea->palette();
  optionsPalette.setColor(QPalette::Window,  ui.tabWidget->palette().color(QPalette::Base));
  ui.optionsScrollArea->setPalette(optionsPalette);

  ui.browsePushButton->setIcon(os::icon("folder"));
  ui.namingOptionsButton->setIcon(os::icon("configure"));

  // Export/Import menu .
  QMenu* optionsMenu = new QMenu(tr("Options"));

  QAction *exportAction = new QAction(tr("&Export.."), optionsMenu);
  connect(exportAction, SIGNAL(triggered()), this, SLOT(exportSettings()));

  QAction *importAction = new QAction(tr("&Import.."), optionsMenu);
  connect(importAction, SIGNAL(triggered()), this, SLOT(importSettings()));

  QAction *restoreAction = new QAction(tr("&Restore Defaults"), optionsMenu);
  connect(restoreAction, SIGNAL(triggered()), this, SLOT(restoreDefaults()));

  optionsMenu->addAction(exportAction);
  optionsMenu->addAction(importAction);
  optionsMenu->addSeparator();
  optionsMenu->addAction(restoreAction);

  QPushButton *optionsButton = new QPushButton(tr("Options"), this);
  optionsButton->setMenu(optionsMenu);
  ui.buttonBox->addButton(optionsButton, QDialogButtonBox::ResetRole);

  // Set up the autocomplete for the directory.
  QCompleter *completer = new QCompleter(this);
  completer->setModel(new QDirModel(QStringList(), QDir::Dirs, QDir::Name, completer));
  ui.targetLineEdit->setCompleter(completer);

  // HotkeyWidget icons.
  ui.screenHotkeyWidget->setIcon      (os::icon("screen"));
  ui.windowHotkeyWidget->setIcon      (os::icon("window"));
  ui.windowPickerHotkeyWidget->setIcon(os::icon("pickWindow"));
  ui.areaHotkeyWidget->setIcon        (os::icon("area"));
  ui.openHotkeyWidget->setIcon        (QIcon(":/icons/lightscreen.small"));
  ui.directoryHotkeyWidget->setIcon   (os::icon("folder"));

  // Version
  ui.versionLabel->setText(tr("Version %1").arg(qApp->applicationVersion()));

  setEnabled(false); // We disable the widgets to prevent any user interaction until the settings have loaded.

  //
  // Connections
  //

  connect(ui.buttonBox              , SIGNAL(clicked(QAbstractButton*)), this    , SLOT(dialogButtonClicked(QAbstractButton*)));
  connect(ui.buttonBox              , SIGNAL(accepted())               , this    , SLOT(accepted()));
  connect(ui.namingOptionsButton    , SIGNAL(clicked())                , this    , SLOT(namingOptions()));

  connect(ui.prefixLineEdit         , SIGNAL(textEdited(QString))      , this    , SLOT(updatePreview()));
  connect(ui.formatComboBox         , SIGNAL(currentIndexChanged(int)) , this    , SLOT(updatePreview()));
  connect(ui.namingComboBox         , SIGNAL(currentIndexChanged(int)) , this    , SLOT(updatePreview()));

  connect(ui.browsePushButton       , SIGNAL(clicked())                , this    , SLOT(browse()));
  connect(ui.checkUpdatesPushButton , SIGNAL(clicked())                , this    , SLOT(checkUpdatesNow()));
  connect(ui.historyPushButton      , SIGNAL(clicked())                , this    , SLOT(viewHistory()));
  connect(ui.imgurAuthButton        , SIGNAL(clicked())                , this    , SLOT(imgurAuthorize()));

  connect(ui.screenCheckBox      , SIGNAL(toggled(bool)), ui.screenHotkeyWidget   , SLOT(setEnabled(bool)));
  connect(ui.areaCheckBox        , SIGNAL(toggled(bool)), ui.areaHotkeyWidget     , SLOT(setEnabled(bool)));
  connect(ui.windowCheckBox      , SIGNAL(toggled(bool)), ui.windowHotkeyWidget   , SLOT(setEnabled(bool)));
  connect(ui.windowPickerCheckBox, SIGNAL(toggled(bool)), ui.windowPickerHotkeyWidget, SLOT(setEnabled(bool)));
  connect(ui.openCheckBox        , SIGNAL(toggled(bool)), ui.openHotkeyWidget     , SLOT(setEnabled(bool)));
  connect(ui.directoryCheckBox   , SIGNAL(toggled(bool)), ui.directoryHotkeyWidget, SLOT(setEnabled(bool)));

  // "Save as" disables the file target input field.
  connect(ui.saveAsCheckBox      , SIGNAL(toggled(bool)), ui.targetLineEdit       , SLOT(setDisabled(bool)));
  connect(ui.saveAsCheckBox      , SIGNAL(toggled(bool)), ui.browsePushButton     , SLOT(setDisabled(bool)));
  connect(ui.saveAsCheckBox      , SIGNAL(toggled(bool)), ui.directoryLabel       , SLOT(setDisabled(bool)));

  connect(ui.startupCheckBox     , SIGNAL(toggled(bool)), ui.startupHideCheckBox  , SLOT(setEnabled(bool)));
  connect(ui.qualitySlider       , SIGNAL(valueChanged(int)), ui.qualityValueLabel, SLOT(setNum(int)));
  connect(ui.trayCheckBox        , SIGNAL(toggled(bool)), ui.messageCheckBox      , SLOT(setEnabled(bool)));

  // Auto-upload disables the default action button in the previews.
  connect(ui.uploadCheckBox      , SIGNAL(toggled(bool)), ui.previewDefaultActionLabel   , SLOT(setDisabled(bool)));
  connect(ui.uploadCheckBox      , SIGNAL(toggled(bool)), ui.previewDefaultActionComboBox, SLOT(setDisabled(bool)));
  connect(ui.directoryCheckBox   , SIGNAL(toggled(bool)), ui.directoryHotkeyWidget, SLOT(setEnabled(bool)));

  connect(ui.mainLabel ,        SIGNAL(linkActivated(QString)), this, SLOT(openUrl(QString)));
  connect(ui.licenseAboutLabel, SIGNAL(linkActivated(QString)), this, SLOT(openUrl(QString)));
  connect(ui.linksLabel,        SIGNAL(linkActivated(QString)), this, SLOT(openUrl(QString)));
}

void OptionsDialog::namingOptions()
{
  NamingDialog namingDialog((Screenshot::Naming) ui.namingComboBox->currentIndex());

  namingDialog.exec();
  flipToggled(settings()->value("options/flip").toBool());
  updatePreview();
}

//

bool OptionsDialog::hotkeyCollision()
{
  // Check for hotkey collision (there's probably a better way to do this...=)

  if (ui.screenCheckBox->isChecked()) {
    if (ui.screenHotkeyWidget->hotkey() == ui.areaHotkeyWidget->hotkey()
        && ui.areaCheckBox->isChecked())
      return true;

    if (ui.screenHotkeyWidget->hotkey() == ui.windowHotkeyWidget->hotkey()
        && ui.windowCheckBox->isChecked())
      return true;

    if (ui.screenHotkeyWidget->hotkey() == ui.windowPickerHotkeyWidget->hotkey()
        && ui.windowPickerCheckBox->isChecked())
      return true;

    if (ui.screenHotkeyWidget->hotkey() == ui.openHotkeyWidget->hotkey()
        && ui.openCheckBox->isChecked())
      return true;

    if (ui.screenHotkeyWidget->hotkey() == ui.directoryHotkeyWidget->hotkey()
        && ui.directoryCheckBox->isChecked())
      return true;
  }

  if (ui.areaCheckBox->isChecked()) {
    if (ui.areaHotkeyWidget->hotkey() == ui.screenHotkeyWidget->hotkey()
        && ui.screenCheckBox->isChecked())
      return true;

    if (ui.areaHotkeyWidget->hotkey() == ui.windowHotkeyWidget->hotkey()
        && ui.windowCheckBox->isChecked())
      return true;

    if (ui.areaHotkeyWidget->hotkey() == ui.windowPickerHotkeyWidget->hotkey()
        && ui.windowPickerCheckBox->isChecked())
      return true;

    if (ui.areaHotkeyWidget->hotkey() == ui.openHotkeyWidget->hotkey()
        && ui.openCheckBox->isChecked())
      return true;

    if (ui.areaHotkeyWidget->hotkey() == ui.directoryHotkeyWidget->hotkey()
        && ui.directoryCheckBox->isChecked())
      return true;
  }

  if (ui.windowCheckBox->isChecked()) {
    if (ui.windowHotkeyWidget->hotkey() == ui.screenHotkeyWidget->hotkey()
        && ui.screenCheckBox->isChecked())
      return true;

    if (ui.windowHotkeyWidget->hotkey() == ui.areaHotkeyWidget->hotkey()
        && ui.areaCheckBox->isChecked())
      return true;

    if (ui.windowHotkeyWidget->hotkey() == ui.windowPickerHotkeyWidget->hotkey()
        && ui.windowPickerCheckBox->isChecked())
      return true;

    if (ui.windowHotkeyWidget->hotkey() == ui.openHotkeyWidget->hotkey()
        && ui.openCheckBox->isChecked())
      return true;

    if (ui.windowHotkeyWidget->hotkey() == ui.directoryHotkeyWidget->hotkey()
        && ui.directoryCheckBox->isChecked())
      return true;
  }

  if (ui.openCheckBox->isChecked()) {
    if (ui.openHotkeyWidget->hotkey() == ui.screenHotkeyWidget->hotkey()
        && ui.screenCheckBox->isChecked())
      return true;

    if (ui.openHotkeyWidget->hotkey() == ui.areaHotkeyWidget->hotkey()
        && ui.areaCheckBox->isChecked())
      return true;

    if (ui.openHotkeyWidget->hotkey() == ui.windowPickerHotkeyWidget->hotkey()
        && ui.windowPickerCheckBox->isChecked())
      return true;

    if (ui.openHotkeyWidget->hotkey() == ui.windowHotkeyWidget->hotkey()
        && ui.windowCheckBox->isChecked())
      return true;

    if (ui.openHotkeyWidget->hotkey() == ui.directoryHotkeyWidget->hotkey()
        && ui.directoryCheckBox->isChecked())
      return true;
  }

  if (ui.directoryCheckBox->isChecked()) {
    if (ui.directoryHotkeyWidget->hotkey() == ui.screenHotkeyWidget->hotkey()
        && ui.screenCheckBox->isChecked())
      return true;

    if (ui.directoryHotkeyWidget->hotkey() == ui.areaHotkeyWidget->hotkey()
        && ui.areaCheckBox->isChecked())
      return true;

    if (ui.directoryHotkeyWidget->hotkey() == ui.windowPickerHotkeyWidget->hotkey()
        && ui.windowPickerCheckBox->isChecked())
      return true;

    if (ui.directoryHotkeyWidget->hotkey() == ui.windowHotkeyWidget->hotkey()
        && ui.windowCheckBox->isChecked())
      return true;

    if (ui.directoryHotkeyWidget->hotkey() == ui.openHotkeyWidget->hotkey()
        && ui.openCheckBox->isChecked())
      return true;
  }

  return false;
}

QSettings *OptionsDialog::settings() const
{
  return ScreenshotManager::instance()->settings();
}
