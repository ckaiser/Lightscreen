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

#include "Taskbar.h"

#ifdef Q_OS_WIN32
#include <QMutexLocker>
#include "TBPrivateData.h"

namespace QW7 {

    QMutex Taskbar::m_mutex;
    QMutex Taskbar::m_mutex_winevent;

    Taskbar* Taskbar::m_instance = NULL;


    Taskbar::Taskbar(QObject* parent) : QObject(parent) {
        m_private = NULL;
        m_taskBarCreatedId = WM_NULL;
    }

    Taskbar* Taskbar::GetInstance() {
        QMutexLocker locker(&m_mutex);

        if (m_instance == NULL) {
            m_instance = new Taskbar();
        }

        return m_instance;
    }

    Taskbar::~Taskbar() {
        if (m_private) {
            delete m_private;
            m_private = NULL;
        }
    }

    void Taskbar::ReleaseInstance() {
        QMutexLocker locker(&m_mutex);

        if (m_instance != NULL) {
            delete m_instance;
        }
    }


    bool Taskbar::isInitialized() {
        if (m_private != NULL) {
            return (m_private->GetHandler() != NULL);
        } else {
            return false;
        }
    }

    bool Taskbar::winEvent(MSG* message, long* result) {
        if (m_taskBarCreatedId == WM_NULL) {
            m_taskBarCreatedId = RegisterWindowMessage(L"TaskbarButtonCreated");
        }

        if (message->message == m_taskBarCreatedId) {
            QMutexLocker locker(&m_mutex_winevent);

            if (!m_private) {
                m_private = new TBPrivateData();

                if (m_private) {
                    emit isReady();
                    return true;
                }
            }

        }

        return false;
    }
}

#endif //Q_OS_WIN32
