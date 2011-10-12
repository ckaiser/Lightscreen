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
#include "namingdialog.h"
#include "../tools/screenshot.h"
#include "../tools/os.h"
#include "../tools/screenshotmanager.h"

#include <QDesktopServices>
#include <QKeyEvent>
#include <QSettings>
#include <QUrl>

NamingDialog::NamingDialog(Screenshot::Naming naming,QWidget *parent) :
    QDialog(parent)
{
  ui.setupUi(this);
  setModal(true);
  setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);

  ui.dateFormatComboBox->installEventFilter(this);

  // Aero
  if (os::aeroGlass(this)) {
    ui.container->setStyleSheet("#container { background: palette(light); border: 1px solid palette(dark); border-radius: 4px; }");
    ui.container->setWindowOpacity(0.5);
    layout()->setMargin(2);
  }

  // Settings
  QSettings *s = ScreenshotManager::instance()->settings();
  ui.flipNamingCheckBox->setChecked(s->value("options/flip", false).toBool());

  ui.dateFormatComboBox->setCurrentIndex(
        ui.dateFormatComboBox->findText(s->value("options/naming/dateFormat", "yyyy-MM-dd").toString())
        );

  if (ui.dateFormatComboBox->currentIndex() == -1) {
    ui.dateFormatComboBox->addItem(s->value("options/naming/dateFormat", "yyyy-MM-dd").toString());
    ui.dateFormatComboBox->setCurrentIndex(ui.dateFormatComboBox->count()-1);
  }

  ui.leadingZerosSpinBox->setValue(s->value("options/naming/leadingZeros", 0).toInt());

  // Signals/Slots
  connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(saveSettings()));
  connect(ui.dateHelpLabel, SIGNAL(linkActivated(QString)), this, SLOT(openUrl(QString)));

  // Stack & window size adjustments
  ui.stack->setCurrentIndex((int)naming);
  ui.stack->currentWidget()->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

  resize(minimumSizeHint());
}

void NamingDialog::openUrl(QString url)
{
  QDesktopServices::openUrl(QUrl(url));
}

void NamingDialog::saveSettings()
{
  QSettings *s = ScreenshotManager::instance()->settings();
  s->setValue("options/flip"               , ui.flipNamingCheckBox->isChecked());
  s->setValue("options/naming/dateFormat"  , ui.dateFormatComboBox->currentText());
  s->setValue("options/naming/leadingZeros", ui.leadingZerosSpinBox->value());
}

bool NamingDialog::eventFilter(QObject *object, QEvent *event)
{
  if (event->type() == QEvent::KeyPress
      && object == ui.dateFormatComboBox)
  {
    QKeyEvent *keyEvent = (QKeyEvent*)(event);
    if (QRegExp("[?:\\\\/*\"<>|]").exactMatch(keyEvent->text())) {
      event->ignore();
      return true;
    }
  }

  return QDialog::eventFilter(object, event);
}

