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

#ifndef TASKBAR_H
#define TASKBAR_H

#include <QMutex>
#include <QObject>
#include <QCoreApplication>

#ifdef Q_OS_WIN32
namespace QW7 {

    struct TBPrivateData;

    class Taskbar : public QObject {

        Q_OBJECT

    private:
        TBPrivateData* m_private;
        unsigned int m_taskBarCreatedId;

        static QMutex m_mutex;
        static QMutex m_mutex_winevent;
        static Taskbar* m_instance;

        Taskbar(QObject* parent = NULL);
        ~Taskbar();

    public:

        static Taskbar* GetInstance();
        static void ReleaseInstance();

        bool isInitialized();
        bool winEvent(MSG* message, long* result);

    signals:
        void isReady();

        friend class TaskbarTabs;
        friend class TaskbarButton;
        friend class TaskbarToolbar;
    };
}
#endif // Q_OS_WIN32
#endif // TASKBAR_H
