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
#include <QApplication>
#include <QDesktopServices>
#include <QLabel>
#include <QLayout>
#include <QProgressBar>
#include <QUrl>

#include <dialogs/updaterdialog.h>
#include <tools/os.h>

UpdaterDialog::UpdaterDialog(QWidget *parent) :
    QProgressDialog("", tr("Cancel"), 0, 0, parent)
{
    setWindowTitle(tr("Updater - Lightscreen"));
    setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);
    setAutoClose(false);

    QProgressBar *bar = new QProgressBar(this);
    bar->setTextVisible(false);
    bar->setRange(0, 0);

    QLabel *label = new QLabel(tr("Checking for updates..."), this);
    connect(label, &QLabel::linkActivated, this, &UpdaterDialog::link);

    setLabel(label);
    setBar(bar);
}

void UpdaterDialog::updateDone(bool result)
{
    if (result) {
        setLabelText(tr("There's a new version available,<br> please see <a href=\"https://lightscreen.com.ar/whatsnew/%1\">the Lighscreen website</a>.").arg(qApp->applicationVersion()));
    } else {
        setLabelText(tr("No new versions available."));
    }

    setMaximum(1);

    setCancelButtonText(tr("Close"));
}

void UpdaterDialog::link(QString url)
{
    QDesktopServices::openUrl(url);
}

