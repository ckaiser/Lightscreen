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
