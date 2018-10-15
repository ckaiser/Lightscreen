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
#include <dialogs/namingdialog.h>
#include <tools/screenshot.h>
#include <tools/os.h>
#include <tools/screenshotmanager.h>

#include <QDesktopServices>
#include <QKeyEvent>
#include <QSettings>
#include <QUrl>

NamingDialog::NamingDialog(Screenshot::Naming naming, QWidget *parent) :
    QDialog(parent)
{
    ui.setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);

    ui.dateFormatComboBox->installEventFilter(this);

    // Settings
    QSettings *settings = ScreenshotManager::instance()->settings();
    ui.flipNamingCheckBox->setChecked(settings->value("options/flip", false).toBool());

    ui.dateFormatComboBox->setCurrentIndex(
        ui.dateFormatComboBox->findText(settings->value("options/naming/dateFormat", "yyyy-MM-dd").toString())
    );

    if (ui.dateFormatComboBox->currentIndex() == -1) {
        ui.dateFormatComboBox->addItem(settings->value("options/naming/dateFormat", "yyyy-MM-dd").toString());
        ui.dateFormatComboBox->setCurrentIndex(ui.dateFormatComboBox->count() - 1);
    }

    ui.leadingZerosSpinBox->setValue(settings->value("options/naming/leadingZeros", 0).toInt());

    // Signals/Slots
    connect(ui.buttonBox    , &QDialogButtonBox::accepted, this, [&] {
        settings->setValue("options/flip"               , ui.flipNamingCheckBox->isChecked());
        settings->setValue("options/naming/dateFormat"  , ui.dateFormatComboBox->currentText());
        settings->setValue("options/naming/leadingZeros", ui.leadingZerosSpinBox->value());
    });

    connect(ui.dateHelpLabel, &QLabel::linkActivated, this, [](const QUrl &url) {
        QDesktopServices::openUrl(QUrl(url));
    });

    // Stack & window size adjustments
    ui.stack->setCurrentIndex((int)naming);
    ui.stack->currentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    resize(minimumSizeHint());
}

bool NamingDialog::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::KeyPress
            && object == ui.dateFormatComboBox) {
        QKeyEvent *keyEvent = (QKeyEvent *)(event);
        if (QRegExp("[?:\\\\/*\"<>|]").exactMatch(keyEvent->text())) {
            event->ignore();
            return true;
        }
    }

    return QDialog::eventFilter(object, event);
}

