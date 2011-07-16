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

#include "taskbarthumbnail.h"

#ifdef Q_OS_WIN32
#include <QWidget>

#include "utils.h"
#include "win7_include.h"
#include "tbprivatedata.h"
#include "taskbar.h"

namespace QW7 {

    TaskbarThumbnail::TaskbarThumbnail(QObject *parent) : QObject(parent) {
        SetWindow(parent);
    }

    void TaskbarThumbnail::SetWindow(QObject* window) {
        m_widget = dynamic_cast<QWidget*>(window);
    }

    void TaskbarThumbnail::EnableIconicPreview(bool enable) {
        EnableWidgetIconicPreview(m_widget, enable);
    }

    void TaskbarThumbnail::SetThumbnail(QPixmap thumbnail) {
        m_thumbnail = thumbnail;
    }

    void TaskbarThumbnail::SetThumbnailTooltip(QString tooltip) {
        if (Taskbar::GetInstance()->m_private) {
            Taskbar::GetInstance()->m_private->GetHandler()->SetThumbnailTooltip(m_widget->winId(), tooltip.toStdWString().c_str());
        }
    }

    bool TaskbarThumbnail::winEvent(MSG* message, long* result) {

        switch (message->message)
        {
        case WM_DWMSENDICONICTHUMBNAIL: {
                HBITMAP hbitmap = m_thumbnail.toWinHBITMAP(QPixmap::Alpha);
                DwmSetIconicThumbnail(m_widget->winId(), hbitmap, 0);

                if (hbitmap) DeleteObject(hbitmap);
                return true;
            }
            break;


        case WM_DWMSENDICONICLIVEPREVIEWBITMAP: {
                HBITMAP hbitmap = QPixmap::grabWidget(m_widget).scaled(m_widget->size(), Qt::KeepAspectRatio).toWinHBITMAP();

                DwmSetIconicLivePreviewBitmap(m_widget->winId(), hbitmap, 0, 0);
                if (hbitmap) DeleteObject(hbitmap);
                return true;
            }
            break;

        }

        return false;//Taskbar::winEvent(message, result);
    }


}

#endif //Q_OS_WIN32
