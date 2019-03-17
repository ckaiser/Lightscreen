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
#include <QToolTip>
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

#include <QFutureWatcher>
#include <QtConcurrent>

#ifdef Q_OS_WIN
    #include <windows.h>
#endif

#include <dialogs/optionsdialog.h>
#include <dialogs/namingdialog.h>
#include <dialogs/historydialog.h>

#include <tools/os.h>
#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/uploader/uploader.h>

#include <updater/updater.h>

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent)
{
    ui.setupUi(this);
    setModal(true);

#if defined(Q_OS_LINUX)
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

    setEnabled(false); // We disable the widgets to prevent any user interaction until the settings have loaded.
    QMetaObject::invokeMethod(this, "init"        , Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "loadSettings", Qt::QueuedConnection);
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

    if (exportFileName.isEmpty()) {
        return;
    }

    if (QFile::exists(exportFileName)) {
        QFile::remove(exportFileName);
    }

    QSettings exportedSettings(exportFileName, QSettings::IniFormat);

    for (auto key : settings()->allKeys()) {
        exportedSettings.setValue(key, settings()->value(key));
    }
}

void OptionsDialog::importSettings()
{
    QString importFileName = QFileDialog::getOpenFileName(this,
                             tr("Import Settings"),
                             QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                             tr("Lightscreen Settings (*.ini)"));

    QSettings importedSettings(importFileName, QSettings::IniFormat);

    saveSettings();

    for (auto key : importedSettings.allKeys()) {
        if (settings()->contains(key)) {
            settings()->setValue(key, importedSettings.value(key));
        }
    }

    loadSettings();
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
    } else {
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
    ui.urlClipboardCheckBox->setChecked(settings()->value("urlClipboard", false).toBool());
    ui.optiPngCheckBox->setChecked(settings()->value("optimize", false).toBool());
    ui.closeHideCheckBox->setChecked(settings()->value("closeHide", true).toBool());
    ui.currentMonitorCheckBox->setChecked(settings()->value("currentMonitor", false).toBool());
    ui.replaceCheckBox->setChecked(settings()->value("replace", false).toBool());
    ui.uploadCheckBox->setChecked(settings()->value("uploadAuto", false).toBool());

#ifdef Q_OS_WIN
    if (!QFile::exists(qApp->applicationDirPath() + QDir::separator() + "optipng.exe")) {
        ui.optiPngCheckBox->setEnabled(false);
        ui.optiPngLabel->setText("optipng.exe not found");
    }
#elif defined(Q_OS_LINUX)
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
    ui.uploadServiceComboBox->setCurrentIndex(settings()->value("service").toInt());

    settings()->beginGroup("imgur");
    ui.imgurOptions->setUser(settings()->value("account_username", "").toString());
    settings()->endGroup();

    settings()->beginGroup("pomf");

    // TODO: Move to pomfuploader in a more generic way.
    QString pomf_url = settings()->value("pomf_url", "").toString();

    if (!pomf_url.isEmpty()) {
        if (ui.pomfOptions->ui.pomfUrlComboBox->findText(pomf_url, Qt::MatchFixedString) == -1) {
            ui.pomfOptions->ui.pomfUrlComboBox->addItem(pomf_url);
        }

        ui.pomfOptions->ui.pomfUrlComboBox->setCurrentText(settings()->value("pomf_url", "").toString());
        ui.pomfOptions->ui.verifyButton->setEnabled(!settings()->value("pomf_url", "").toString().isEmpty());
    }

    settings()->endGroup();
    settings()->endGroup();

    QTimer::singleShot(0, this, &OptionsDialog::updatePreview);

    setEnabled(true);
    setUpdatesEnabled(true);
}

void OptionsDialog::openUrl(QString url)
{
    if (url == "#aboutqt") {
        qApp->aboutQt();
    } else {
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
    settings()->setValue("urlClipboard", ui.urlClipboardCheckBox->isChecked());
    settings()->setValue("optimize", ui.optiPngCheckBox->isChecked());
    settings()->setValue("currentMonitor", ui.currentMonitorCheckBox->isChecked());
    settings()->setValue("replace", ui.replaceCheckBox->isChecked());

    //Upload
    settings()->setValue("uploadAuto", ui.uploadCheckBox->isChecked());
    settings()->endGroup();

    settings()->beginGroup("actions");
    settings()->beginGroup("screen");
    settings()->setValue("enabled", ui.screenCheckBox->isChecked());
    settings()->setValue("hotkey" , ui.screenHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->beginGroup("area");
    settings()->setValue("enabled", ui.areaCheckBox->isChecked());
    settings()->setValue("hotkey" , ui.areaHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->beginGroup("window");
    settings()->setValue("enabled", ui.windowCheckBox->isChecked());
    settings()->setValue("hotkey" , ui.windowHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->beginGroup("windowPicker");
    settings()->setValue("enabled", ui.windowPickerCheckBox->isChecked());
    settings()->setValue("hotkey" , ui.windowPickerHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->beginGroup("open");
    settings()->setValue("enabled", ui.openCheckBox->isChecked());
    settings()->setValue("hotkey" , ui.openHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->beginGroup("directory");
    settings()->setValue("enabled", ui.directoryCheckBox->isChecked());
    settings()->setValue("hotkey" , ui.directoryHotkeyWidget->hotkey());
    settings()->endGroup();

    settings()->endGroup();

    settings()->beginGroup("upload");
    settings()->setValue("service", ui.uploadServiceComboBox->currentIndex());

    settings()->beginGroup("imgur");
        settings()->setValue("anonymous", settings()->value("account_username").toString().isEmpty());
        settings()->setValue("album"    , ui.imgurOptions->ui.albumComboBox->property("currentData").toString());
    settings()->endGroup();

    settings()->beginGroup("pomf");
        settings()->setValue("pomf_url", ui.pomfOptions->ui.pomfUrlComboBox->currentText());
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

    if (ui.fileGroupBox->isChecked()) { // Only change the naming button when file options are enabled.
        ui.namingOptionsButton->setDisabled((options.naming == Screenshot::Empty));
    }

    QString preview = Screenshot::getName(options,
                                          ui.prefixLineEdit->text(),
                                          QDir(ui.targetLineEdit->text()));

    preview = QString("%1.%2").arg(preview).arg(ui.formatComboBox->currentText().toLower());

    if (preview.length() >= 40) {
        preview.truncate(37);
        preview.append("...");
    }

    ui.previewLabel->setText(preview);

    ui.qualitySlider->setEnabled(ui.formatComboBox->currentText() != "PNG");
    ui.qualityLabel->setEnabled(ui.qualitySlider->isEnabled());
}

void OptionsDialog::viewHistory()
{
    HistoryDialog historyDialog(this);
    historyDialog.exec();
}

//

bool OptionsDialog::event(QEvent *event)
{
    if (event->type() == QEvent::Close || event->type() == QEvent::Hide) {
        settings()->setValue("geometry/optionsDialog", saveGeometry());

        if (!settings()->contains("file/format")) {
            // I'm afraid I can't let you do that, Dave.
            event->ignore();
            return false;
        }
    } else if (event->type() == QEvent::Show) {
        if (settings()->contains("geometry/optionsDialog")) {
            restoreGeometry(settings()->value("geometry/optionsDialog").toByteArray());
        } else {
            move(QApplication::desktop()->screenGeometry(this).center() - rect().center());
        }
    }

    return QDialog::event(event);
}

#ifdef Q_OS_WIN
// Qt does not send the print screen key as a regular QKeyPress event, so we must use the Windows API
bool OptionsDialog::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    if (eventType == "windows_generic_MSG") {
        MSG *m = static_cast<MSG *>(message);

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

    if (fileName.isEmpty()) {
        return;
    }

    ui.targetLineEdit->setText(fileName);
    updatePreview();
}

void OptionsDialog::dialogButtonClicked(QAbstractButton *button)
{
    if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole) {
        QPushButton *pb = qobject_cast<QPushButton *>(button);
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

    if (msgBox.clickedButton() == dontRestoreButton) {
        return;
    }

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
    } else {
        ui.filenameLayout->addWidget(ui.prefixLineEdit);
        ui.filenameLayout->addWidget(ui.namingComboBox);
    }

    if (ui.prefixLineEdit->text() == tr("screenshot.")
            && checked) {
        ui.prefixLineEdit->setText(tr(".screenshot"));
    }

    if (ui.prefixLineEdit->text() == tr(".screenshot")
            && !checked) {
        ui.prefixLineEdit->setText(tr("screenshot."));
    }

    setUpdatesEnabled(true); // Avoids flicker
}

void OptionsDialog::init()
{
    // Make the scroll area share the Tab Widget background color
    ui.optionsScrollArea->setStyleSheet("QScrollArea { background-color: transparent; } #scrollAreaWidgetContents { background-color: transparent; }");

    ui.browsePushButton->setIcon(os::icon("folder"));
    ui.namingOptionsButton->setIcon(os::icon("configure"));

    // Export/Import menu .
    QMenu *optionsMenu = new QMenu(tr("Options"));

    QAction *exportAction = new QAction(tr("&Export.."), optionsMenu);
    connect(exportAction, &QAction::triggered, this, &OptionsDialog::exportSettings);

    QAction *importAction = new QAction(tr("&Import.."), optionsMenu);
    connect(importAction, &QAction::triggered, this, &OptionsDialog::importSettings);

    QAction *restoreAction = new QAction(tr("&Restore Defaults"), optionsMenu);
    connect(restoreAction, &QAction::triggered, this, &OptionsDialog::restoreDefaults);

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
    ui.screenHotkeyWidget->setIcon(os::icon("screen"));
    ui.windowHotkeyWidget->setIcon(os::icon("window"));
    ui.windowPickerHotkeyWidget->setIcon(os::icon("pickWindow"));
    ui.areaHotkeyWidget->setIcon(os::icon("area"));
    ui.openHotkeyWidget->setIcon(QIcon(":/icons/lightscreen.small"));
    ui.directoryHotkeyWidget->setIcon(os::icon("folder"));

    // Version
    ui.versionLabel->setText(tr("Version %1").arg(qApp->applicationVersion()));

    ui.uploadSslWarningLabel->setVisible(false);

    // Run the SSL check in another thread (slows down dialog startup considerably).
    auto futureWatcher = new QFutureWatcher<bool>(this);
    connect(futureWatcher, &QFutureWatcher<bool>::finished, futureWatcher, &QFutureWatcher<bool>::deleteLater);
    connect(futureWatcher, &QFutureWatcher<bool>::finished, this, [&, futureWatcher] {
        ui.uploadSslWarningLabel->setVisible(!futureWatcher->future().result());
    });

    futureWatcher->setFuture(QtConcurrent::run([]() -> bool { return QSslSocket::supportsSsl(); }));

    //
    // Connections
    //

    connect(ui.buttonBox, &QDialogButtonBox::clicked     , this, &OptionsDialog::dialogButtonClicked);
    connect(ui.buttonBox, &QDialogButtonBox::accepted    , this, &OptionsDialog::accepted);
    connect(ui.namingOptionsButton, &QPushButton::clicked, this, &OptionsDialog::namingOptions);

    connect(ui.prefixLineEdit, &QLineEdit::textEdited, this, &OptionsDialog::updatePreview);
    connect(ui.formatComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptionsDialog::updatePreview);
    connect(ui.namingComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &OptionsDialog::updatePreview);

    connect(ui.browsePushButton       , &QPushButton::clicked, this, &OptionsDialog::browse);
    connect(ui.checkUpdatesPushButton , &QPushButton::clicked, this, &OptionsDialog::checkUpdatesNow);
    connect(ui.historyPushButton      , &QPushButton::clicked, this, &OptionsDialog::viewHistory);

    connect(ui.windowPickerCheckBox, &QCheckBox::toggled, ui.windowPickerHotkeyWidget, &HotkeyWidget::setEnabled);

    connect(ui.screenCheckBox      , &QCheckBox::toggled, ui.screenHotkeyWidget   , &HotkeyWidget::setEnabled);
    connect(ui.areaCheckBox        , &QCheckBox::toggled, ui.areaHotkeyWidget     , &HotkeyWidget::setEnabled);
    connect(ui.windowCheckBox      , &QCheckBox::toggled, ui.windowHotkeyWidget   , &HotkeyWidget::setEnabled);
    connect(ui.openCheckBox        , &QCheckBox::toggled, ui.openHotkeyWidget     , &HotkeyWidget::setEnabled);
    connect(ui.directoryCheckBox   , &QCheckBox::toggled, ui.directoryHotkeyWidget, &HotkeyWidget::setEnabled);

    // "Save as" disables the file target input field.
    connect(ui.saveAsCheckBox, &QCheckBox::toggled, [&](bool checked) {
        ui.targetLineEdit->setDisabled(checked);
        ui.browsePushButton->setDisabled(checked);
        ui.directoryLabel->setDisabled(checked);
    });

    connect(ui.qualitySlider, static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged), ui.qualityValueLabel, static_cast<void (QLabel::*)(int)>(&QLabel::setNum));
    connect(ui.startupCheckBox, &QCheckBox::toggled   , ui.startupHideCheckBox, &QCheckBox::setEnabled);
    connect(ui.trayCheckBox   , &QCheckBox::toggled   , ui.messageCheckBox    , &QCheckBox::setEnabled);

    // Auto-upload disables the default action button in the previews.
    connect(ui.uploadCheckBox, &QCheckBox::toggled, [&](bool checked) {
        ui.previewDefaultActionLabel->setDisabled(checked);
        ui.previewDefaultActionComboBox->setDisabled(checked);
        ui.directoryHotkeyWidget->setEnabled(checked);
    });

    auto conflictWarning = [](bool fullConflict, QWidget *w) {
        if (fullConflict) {
            QToolTip::showText(QCursor::pos(), tr("<font color=\"darkRed\">This setting conflicts with the Screenshot Clipboard setting, which has been disabled.</font>"), w);
        } else {
            QToolTip::showText(QCursor::pos(), tr("<b>This setting might conflict with the Screenshot Clipboard setting!</b>"), w);
        }
    };

    connect(ui.uploadCheckBox, &QCheckBox::toggled, [&](bool checked) {
        if (ui.urlClipboardCheckBox->isChecked() && ui.clipboardCheckBox->isChecked()) {
            ui.clipboardGroupBox->setDisabled(checked);

            if (checked) {
                conflictWarning(true, ui.uploadCheckBox);
            }
        }
    });

    connect(ui.urlClipboardCheckBox, &QCheckBox::toggled, [&](bool checked) {
        if (ui.uploadCheckBox->isChecked() && ui.clipboardCheckBox->isChecked()) {
            ui.clipboardGroupBox->setDisabled(checked);

            if (checked) {
                conflictWarning(true, ui.urlClipboardCheckBox);
            }
        } else if (ui.clipboardCheckBox->isChecked()) {
            conflictWarning(false, ui.urlClipboardCheckBox);
        }
    });

    connect(ui.mainLabel ,           &QLabel::linkActivated, this, &OptionsDialog::openUrl);
    connect(ui.licenseAboutLabel,    &QLabel::linkActivated, this, &OptionsDialog::openUrl);
    connect(ui.linksLabel,           &QLabel::linkActivated, this, &OptionsDialog::openUrl);
    connect(ui.uploadSslWarningLabel,&QLabel::linkActivated, this, &OptionsDialog::openUrl);

    connect(ui.tabWidget, &QTabWidget::currentChanged, [&](int index) {
        if (index == 2 && ui.uploadServiceStackWidget->currentIndex() == 0 && !ui.imgurOptions->mCurrentUser.isEmpty() && ui.imgurOptions->ui.albumComboBox->count() == 1) {
            QTimer::singleShot(20, ui.imgurOptions, &ImgurOptionsWidget::requestAlbumList);
        }
    });
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
                && ui.areaCheckBox->isChecked()) {
            return true;
        }

        if (ui.screenHotkeyWidget->hotkey() == ui.windowHotkeyWidget->hotkey()
                && ui.windowCheckBox->isChecked()) {
            return true;
        }

        if (ui.screenHotkeyWidget->hotkey() == ui.windowPickerHotkeyWidget->hotkey()
                && ui.windowPickerCheckBox->isChecked()) {
            return true;
        }

        if (ui.screenHotkeyWidget->hotkey() == ui.openHotkeyWidget->hotkey()
                && ui.openCheckBox->isChecked()) {
            return true;
        }

        if (ui.screenHotkeyWidget->hotkey() == ui.directoryHotkeyWidget->hotkey()
                && ui.directoryCheckBox->isChecked()) {
            return true;
        }
    }

    if (ui.areaCheckBox->isChecked()) {
        if (ui.areaHotkeyWidget->hotkey() == ui.screenHotkeyWidget->hotkey()
                && ui.screenCheckBox->isChecked()) {
            return true;
        }

        if (ui.areaHotkeyWidget->hotkey() == ui.windowHotkeyWidget->hotkey()
                && ui.windowCheckBox->isChecked()) {
            return true;
        }

        if (ui.areaHotkeyWidget->hotkey() == ui.windowPickerHotkeyWidget->hotkey()
                && ui.windowPickerCheckBox->isChecked()) {
            return true;
        }

        if (ui.areaHotkeyWidget->hotkey() == ui.openHotkeyWidget->hotkey()
                && ui.openCheckBox->isChecked()) {
            return true;
        }

        if (ui.areaHotkeyWidget->hotkey() == ui.directoryHotkeyWidget->hotkey()
                && ui.directoryCheckBox->isChecked()) {
            return true;
        }
    }

    if (ui.windowCheckBox->isChecked()) {
        if (ui.windowHotkeyWidget->hotkey() == ui.screenHotkeyWidget->hotkey()
                && ui.screenCheckBox->isChecked()) {
            return true;
        }

        if (ui.windowHotkeyWidget->hotkey() == ui.areaHotkeyWidget->hotkey()
                && ui.areaCheckBox->isChecked()) {
            return true;
        }

        if (ui.windowHotkeyWidget->hotkey() == ui.windowPickerHotkeyWidget->hotkey()
                && ui.windowPickerCheckBox->isChecked()) {
            return true;
        }

        if (ui.windowHotkeyWidget->hotkey() == ui.openHotkeyWidget->hotkey()
                && ui.openCheckBox->isChecked()) {
            return true;
        }

        if (ui.windowHotkeyWidget->hotkey() == ui.directoryHotkeyWidget->hotkey()
                && ui.directoryCheckBox->isChecked()) {
            return true;
        }
    }

    if (ui.openCheckBox->isChecked()) {
        if (ui.openHotkeyWidget->hotkey() == ui.screenHotkeyWidget->hotkey()
                && ui.screenCheckBox->isChecked()) {
            return true;
        }

        if (ui.openHotkeyWidget->hotkey() == ui.areaHotkeyWidget->hotkey()
                && ui.areaCheckBox->isChecked()) {
            return true;
        }

        if (ui.openHotkeyWidget->hotkey() == ui.windowPickerHotkeyWidget->hotkey()
                && ui.windowPickerCheckBox->isChecked()) {
            return true;
        }

        if (ui.openHotkeyWidget->hotkey() == ui.windowHotkeyWidget->hotkey()
                && ui.windowCheckBox->isChecked()) {
            return true;
        }

        if (ui.openHotkeyWidget->hotkey() == ui.directoryHotkeyWidget->hotkey()
                && ui.directoryCheckBox->isChecked()) {
            return true;
        }
    }

    if (ui.directoryCheckBox->isChecked()) {
        if (ui.directoryHotkeyWidget->hotkey() == ui.screenHotkeyWidget->hotkey()
                && ui.screenCheckBox->isChecked()) {
            return true;
        }

        if (ui.directoryHotkeyWidget->hotkey() == ui.areaHotkeyWidget->hotkey()
                && ui.areaCheckBox->isChecked()) {
            return true;
        }

        if (ui.directoryHotkeyWidget->hotkey() == ui.windowPickerHotkeyWidget->hotkey()
                && ui.windowPickerCheckBox->isChecked()) {
            return true;
        }

        if (ui.directoryHotkeyWidget->hotkey() == ui.windowHotkeyWidget->hotkey()
                && ui.windowCheckBox->isChecked()) {
            return true;
        }

        if (ui.directoryHotkeyWidget->hotkey() == ui.openHotkeyWidget->hotkey()
                && ui.openCheckBox->isChecked()) {
            return true;
        }
    }

    return false;
}

QSettings *OptionsDialog::settings() const
{
    return ScreenshotManager::instance()->settings();
}
