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

#include <QDebug>

#ifdef Q_WS_WIN
  #include <windows.h>
#endif

#include "optionsdialog.h"
#include "namingdialog.h"
#include "historydialog.h"
#include "../tools/os.h"
#include "../tools/screenshot.h"
#include "../tools/screenshotmanager.h"
#include "../updater/updater.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
  QDialog(parent)
{
  ui.setupUi(this);
  setModal(true);

  if (os::aeroGlass(this)) {
    layout()->setMargin(2);
    resize(minimumSizeHint());
  }

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
  Updater::instance()->checkWithFeedback();
}

void OptionsDialog::languageChange(QString language)
{
  os::translate(language);
}

void OptionsDialog::loadSettings()
{
  settings()->sync();
  os::translate(settings()->value("options/language").toString()); // Why? Don't ask me, I'm just the programmer.

  if (!settings()->contains("file/format")) {
    // If there are no settings, get rid of the cancel button so that the user is forced to save them
    ui.buttonBox->clear();
    ui.buttonBox->addButton(QDialogButtonBox::Ok);

    // Move the first option window to the center of the screen, since Windows usually positions it in a random location since it has no visible parent.
    if (!(static_cast<QWidget*>(parent())->isVisible()))
      move(qApp->desktop()->screen(qApp->desktop()->primaryScreen())->rect().center()-QPoint(height()/2, width()/2));
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
    ui.magnifyCheckBox->setChecked(settings()->value("magnify", true).toBool());
    ui.cursorCheckBox->setChecked(settings()->value("cursor", false).toBool());
    ui.saveAsCheckBox->setChecked(settings()->value("saveAs", false).toBool());
    ui.previewGroupBox->setChecked(settings()->value("preview", false).toBool());
    ui.previewSizeSpinBox->setValue(settings()->value("previewSize", 300).toInt());
    ui.previewPositionComboBox->setCurrentIndex(settings()->value("previewPosition", 3).toInt());
    ui.previewAutocloseCheckBox->setChecked(settings()->value("previewAutoclose", false).toBool());
    ui.previewAutocloseTimeSpinBox->setValue(settings()->value("previewAutocloseTime", 15).toInt());
    ui.previewAutocloseActionComboBox->setCurrentIndex(settings()->value("previewAutocloseAction", 0).toInt());
    ui.previewDefaultActionComboBox->setCurrentIndex(settings()->value("previewDefaultAction", 0).toInt());
    ui.areaAutocloseCheckBox->setChecked(settings()->value("areaAutoclose", false).toBool());
    ui.historyCheckBox->setChecked(settings()->value("history", true).toBool());

    // Advanced
    ui.clipboardCheckBox->setChecked(settings()->value("clipboard", true).toBool());
    ui.optiPngCheckBox->setChecked(settings()->value("optipng", true).toBool());
    ui.warnHideCheckBox->setChecked(!settings()->value("disableHideAlert", false).toBool());
    ui.currentMonitorCheckBox->setChecked(settings()->value("currentMonitor", false).toBool());
    ui.replaceCheckBox->setChecked(settings()->value("replace", false).toBool());
    ui.uploadCheckBox->setChecked(settings()->value("uploadAuto", false).toBool());

#ifdef Q_WS_WIN
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

  //TODO: Must replace with not-stupid system
  QString lang = settings()->value("language").toString();
  int index = ui.languageComboBox->findText(lang);

  if (index == -1)
    index = ui.languageComboBox->findText("English");

  ui.languageComboBox->setCurrentIndex(index);

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
    ui.windowHotkeyWidget->setHotkey(settings()->value("hotkey", "Alt + Print").toString());
  settings()->endGroup();

  settings()->beginGroup("windowPicker");
    ui.windowPickerCheckBox->setChecked(settings()->value("enabled").toBool());
    ui.windowPickerHotkeyWidget->setHotkey(settings()->value("hotkey", "Ctrl + Alt + Print").toString());
  settings()->endGroup();

  settings()->beginGroup("open");
    ui.openCheckBox->setChecked(settings()->value("enabled").toBool());
    ui.openHotkeyWidget->setHotkey(settings()->value("hotkey", "Ctrl + PgUp").toString());
  settings()->endGroup();

  settings()->beginGroup("directory");
    ui.directoryCheckBox->setChecked(settings()->value("enabled").toBool());
    ui.directoryHotkeyWidget->setHotkey(settings()->value("hotkey", "Ctrl + PgDown").toString());
  settings()->endGroup();

  settings()->endGroup();

  QTimer::singleShot(0, this, SLOT(updatePreview()));

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

void OptionsDialog::rejected()
{
  languageChange(settings()->value("options/language").toString()); // Revert language to default.
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
    // We save the explicit string because addition/removal of language files can cause it to change
    settings()->setValue("language", ui.languageComboBox->currentText());
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
    settings()->setValue("disableHideAlert", !ui.warnHideCheckBox->isChecked());
    settings()->setValue("clipboard", ui.clipboardCheckBox->isChecked());
    settings()->setValue("optipng", ui.optiPngCheckBox->isChecked());
    settings()->setValue("currentMonitor", ui.currentMonitorCheckBox->isChecked());
    settings()->setValue("replace", ui.replaceCheckBox->isChecked());

    //Upload
    settings()->setValue("uploadAuto", ui.uploadCheckBox->isChecked());
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
}

void OptionsDialog::updatePreview()
{
  Screenshot::NamingOptions options;

  options.naming       = (Screenshot::Naming)ui.namingComboBox->currentIndex();
  options.flip         = settings()->value("options/flip").toBool();
  options.leadingZeros = settings()->value("options/naming/leadingZeros").toInt();
  options.dateFormat   = settings()->value("options/naming/dateFormat").toString();

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
  if (event->type() == QEvent::LanguageChange) {

    // ComboBoxes revert to the first index when translated:
    int naming = ui.namingComboBox->currentIndex();
    int format = ui.formatComboBox->currentIndex();
    int previewPosition  = ui.previewPositionComboBox->currentIndex();
    int previewAutoclose = ui.previewAutocloseActionComboBox->currentIndex();
    int previewDefault   = ui.previewDefaultActionComboBox->currentIndex();

    ui.retranslateUi(this);

    // Restoring comboboxes
    ui.namingComboBox->setCurrentIndex(naming);
    ui.formatComboBox->setCurrentIndex(format);
    ui.previewPositionComboBox->setCurrentIndex(previewPosition);
    ui.previewAutocloseActionComboBox->setCurrentIndex(previewAutoclose);
    ui.previewDefaultActionComboBox->setCurrentIndex(previewDefault);

    updatePreview();
    resize(minimumSizeHint());
  }
  else if (event->type() == QEvent::Close) {
    if (!settings()->contains("file/format")) {
      // I'm afraid I can't let you do that, Dave.
      event->ignore();
      return false;
    }
  }

  return QDialog::event(event);
}

#ifdef Q_WS_WIN
// Qt does not send the print screen key as a regular QKeyPress event, so we must use the Windows API
bool OptionsDialog::winEvent(MSG *message, long *result)
{
  if ((message->message == WM_KEYUP || message->message == WM_SYSKEYUP)
      && message->wParam == VK_SNAPSHOT) {
        qApp->postEvent(qApp->focusWidget(), new QKeyEvent(QEvent::KeyPress, Qt::Key_Print,  qApp->keyboardModifiers()));
  }

  return QDialog::winEvent(message, result);
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
}

void OptionsDialog::dialogButtonClicked(QAbstractButton *button)
{
  if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole) {
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Lightscreen - Restore Default Options"));
    msgBox.setText(tr("Restoring the default options will cause you to lose all of your current configuration."));
    msgBox.setIcon(QMessageBox::Warning);

    QPushButton *restoreButton     = msgBox.addButton(tr("Restore"), QMessageBox::ActionRole);
    QPushButton *dontRestoreButton = msgBox.addButton(tr("Don't Restore"), QMessageBox::ActionRole);

    msgBox.setDefaultButton(dontRestoreButton);
    msgBox.exec();

    Q_UNUSED(restoreButton)

    if (msgBox.clickedButton() == dontRestoreButton)
      return;

    QString language = settings()->value("options/language").toString(); // Only mantain language.

    settings()->clear();
    settings()->setValue("options/language", language);

    loadSettings();
  }
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

  ui.buttonBox->addButton(new QPushButton(" " + tr("Restore Defaults") + " ", this), QDialogButtonBox::ResetRole);

  // Set up the autocomplete for the directory.
  QCompleter *completer = new QCompleter(this);
  completer->setModel(new QDirModel(QStringList(), QDir::Dirs, QDir::Name, completer));
  ui.targetLineEdit->setCompleter(completer);

  // HotkeyWidget icons.
  ui.screenHotkeyWidget->setIcon      (QIcon(":/icons/screen"));
  ui.windowHotkeyWidget->setIcon      (QIcon(":/icons/window"));
  ui.windowPickerHotkeyWidget->setIcon(QIcon(":/icons/picker"));
  ui.areaHotkeyWidget->setIcon        (QIcon(":/icons/area"));
  ui.openHotkeyWidget->setIcon        (QIcon(":/icons/lightscreen.small"));
  ui.directoryHotkeyWidget->setIcon   (QIcon(":/icons/folder"));

  // Version
  ui.versionLabel->setText(tr("Version %1").arg(qApp->applicationVersion()));

  setEnabled(false); // We disable the widgets to prevent any user interaction until the settings have loaded.
  setUpdatesEnabled(false);

  //
  // Connections
  //

  connect(ui.buttonBox              , SIGNAL(clicked(QAbstractButton*)), this    , SLOT(dialogButtonClicked(QAbstractButton*)));
  connect(ui.buttonBox              , SIGNAL(accepted())               , this    , SLOT(accepted()));
  connect(ui.buttonBox              , SIGNAL(rejected())               , this    , SLOT(rejected()));
  connect(ui.namingOptionsButton    , SIGNAL(clicked())                , this    , SLOT(namingOptions()));

  connect(ui.prefixLineEdit         , SIGNAL(textChanged(QString))     , this    , SLOT(updatePreview()));
  connect(ui.formatComboBox         , SIGNAL(currentIndexChanged(int)) , this    , SLOT(updatePreview()));
  connect(ui.namingComboBox         , SIGNAL(currentIndexChanged(int)) , this    , SLOT(updatePreview()));

  connect(ui.browsePushButton       , SIGNAL(clicked())                , this    , SLOT(browse()));
  connect(ui.checkUpdatesPushButton , SIGNAL(clicked())                , this    , SLOT(checkUpdatesNow()));
  connect(ui.historyPushButton      , SIGNAL(clicked())                , this    , SLOT(viewHistory()));

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

  connect(ui.moreInformationLabel, SIGNAL(linkActivated(QString))      , this, SLOT(openUrl(QString)));

  connect(ui.languageComboBox    , SIGNAL(currentIndexChanged(QString)), this, SLOT(languageChange(QString)));

  connect(ui.mainLabel ,        SIGNAL(linkActivated(QString)), this, SLOT(openUrl(QString)));
  connect(ui.licenseAboutLabel, SIGNAL(linkActivated(QString)), this, SLOT(openUrl(QString)));
  connect(ui.linksLabel,        SIGNAL(linkActivated(QString)), this, SLOT(openUrl(QString)));

  //
  // Languages
  //

  QDir languages(":/translations");

  ui.languageComboBox->addItem("English");

  foreach (QString language, languages.entryList()) {
    ui.languageComboBox->addItem(language);
  }
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
