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
#ifndef NAMINGDIALOG_H
#define NAMINGDIALOG_H

#include <tools/screenshot.h>
#include "ui_namingdialog.h"

#include <QUrl>

class NamingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NamingDialog(Screenshot::Naming naming, QWidget *parent = 0);

protected:
    bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::NamingDialog ui;
};

#endif // NAMINGDIALOG_H
