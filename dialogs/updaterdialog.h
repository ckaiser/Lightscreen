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
#ifndef UPDATERDIALOG_H
#define UPDATERDIALOG_H

#include <QProgressDialog>

class UpdaterDialog : public QProgressDialog
{
    Q_OBJECT

public:
    UpdaterDialog(QWidget *parent = 0);

public slots:
    void updateDone(bool result);

private slots:
    void link(QString url);

};

#endif // UPDATERDIALOG_H
