/***************************************************************************
 *   Copyright (C) 2011 by Nicolae Ghimbovschi                             *
 *     nicolae.ghimbovschi@gmail.com                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 3 of the License.               *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef TASKBARTHUMBNAIL_H
#define TASKBARTHUMBNAIL_H

#include <QObject>
#include <QPixmap>
#include <QString>
#include <QWidget>

#ifdef Q_OS_WIN32
#include "taskbar.h"

namespace QW7 {

    class TaskbarThumbnail : public QObject
    {
        Q_OBJECT
    public:
        explicit TaskbarThumbnail(QObject *parent = 0);
        bool winEvent(MSG* message, long* result);

    public slots:
        void SetWindow(QObject* window);
        void SetThumbnail(QPixmap thumbnail);
        void SetThumbnailTooltip(QString tooltip);
        void EnableIconicPreview(bool enable);

    private:
        QWidget* m_widget;
        QPixmap m_thumbnail;

    };
}
#endif //Q_OS_WIN32
#endif // TASKBARTHUMBNAIL_H
