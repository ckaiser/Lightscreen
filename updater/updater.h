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
#ifndef UPDATER_H_
#define UPDATER_H_

#include <QObject>
#include <QNetworkAccessManager>

class Updater : public QObject
{
    Q_OBJECT
public:
    Updater(QObject *parent = 0);

signals:
    void done(bool result);

public slots:
    void check();
    void checkWithFeedback();

private slots:
    void finished(QNetworkReply *reply);

private:
    QNetworkAccessManager mNetwork;

};

#endif /*UPDATER_H_*/
