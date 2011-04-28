#include "namingdialog.h"
#include "../tools/screenshot.h"
#include "../tools/os.h"
#include "../tools/screenshotmanager.h"

#include <QSettings>
#include <QKeyEvent>
#include <QUrl>
#include <QDesktopServices>

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

void NamingDialog::saveSettings()
{
  QSettings *s = ScreenshotManager::instance()->settings();
  s->setValue("options/flip"               , ui.flipNamingCheckBox->isChecked());
  s->setValue("options/naming/dateFormat"  , ui.dateFormatComboBox->currentText());
  s->setValue("options/naming/leadingZeros", ui.leadingZerosSpinBox->value());
}

void NamingDialog::openUrl(QString url)
{
  QDesktopServices::openUrl(QUrl(url));
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

