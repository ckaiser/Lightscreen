#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QtGui/QDialog>
#include "../updater/updater.h"
#include "ui_optionsdialog.h"

class QSettings;
class QAbstractButton;
class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    OptionsDialog(QWidget *parent = 0);

public slots:
  void accepted();
  void checkUpdatesNow();
  void languageChange(QString language);
  void openUrl(QString url);
  void rejected();
  void saveSettings();
  void updatePreview();
  void loadSettings();

protected:
  bool event(QEvent *event);

#if defined(Q_WS_WIN)
  bool winEvent(MSG *message, long *result);
#endif

private slots:
  void namingOptions();
  void flipToggled(bool checked);
  void dialogButtonClicked(QAbstractButton *button);
  void browse();
  void init();

private:
  bool hotkeyCollision();
  QSettings *settings() const;

private:
    Ui::OptionsDialog ui;
};

#endif // OPTIONSDIALOG_H
