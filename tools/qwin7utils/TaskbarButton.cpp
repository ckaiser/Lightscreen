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

#include <QBitmap>
#include <QPixmap>
#include <QPainter>

#include "TaskbarButton.h"

#ifdef Q_OS_WIN32
#include "Taskbar.h"
#include "TBPrivateData.h"

namespace QW7 {

    TaskbarButton::TaskbarButton(QWidget* parent) : QObject(parent) {
        SetWindow(parent);
    }

    void TaskbarButton::SetWindow(QWidget* window) {
        m_widget = window;
    }

    long TaskbarButton::SetOverlayIcon(const QIcon& icon, QString description) {
        if (Taskbar::GetInstance()->m_private) {
            HICON overlay_icon = icon.isNull() ? NULL : icon.pixmap(48).toWinHICON();
            long result = Taskbar::GetInstance()->m_private->GetHandler()->SetOverlayIcon(m_widget->winId(), overlay_icon, description.toStdWString().c_str());

            if (overlay_icon) {
                DestroyIcon(overlay_icon);
                return result;
            }
        }

        return -1;
    }

    long TaskbarButton::SetState(ProgressBarState state) {
        if (Taskbar::GetInstance()->m_private) {
            return Taskbar::GetInstance()->m_private->GetHandler()->SetProgressState(m_widget->winId(), (TBPFLAG)state);
        }

        return -1;
    }

    long TaskbarButton::SetProgresValue(unsigned long long done, unsigned long long total) {
        if (Taskbar::GetInstance()->m_private) {
            return Taskbar::GetInstance()->m_private->GetHandler()->SetProgressValue(m_widget->winId(), done, total);
        }

        return -1;
    }

}

#endif //Q_OS_WIN32
