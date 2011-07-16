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

#include "TaskbarToolbar.h"

#ifdef Q_OS_WIN32
#include <QBitmap>
#include <QPixmap>

#include "Taskbar.h"
#include "TBPrivateData.h"

#define IDTB_FIRST 3000

namespace QW7 {

    TaskbarToolbar::TaskbarToolbar(QWidget *parent) :
            QObject(parent)
    {
        m_initialized = false;
        SetWidget(parent);
    }

    void TaskbarToolbar::SetWidget(QWidget* widget) {
        m_widget = widget;
    }

    void TaskbarToolbar::SetThumbnailClip(QRect rect) {
        RECT wrect;
        wrect.left = rect.left(); wrect.right = rect.right();
        wrect.top = rect.top(); wrect.bottom = rect.bottom();
        Taskbar::GetInstance()->m_private->GetHandler()->SetThumbnailClip(m_widget->winId(), &wrect);
    }

    void TaskbarToolbar::SetThumbnailTooltip(QString tooltip) {
        Taskbar::GetInstance()->m_private->GetHandler()->SetThumbnailTooltip(m_widget->winId(), tooltip.toStdWString().c_str());
    }

    void TaskbarToolbar::AddAction(QAction* action) {
        if (m_initialized) return;

        m_actions.append(action);

        connect(action, SIGNAL(changed()), this, SLOT(OnActionChanged()));
    }

    void TaskbarToolbar::AddActions(QList<QAction*>& actions) {
        if (m_initialized) return;

        RemoveActions();

        m_actions.append(actions);

        QAction* action;
        foreach(action, m_actions) {
            connect(action, SIGNAL(changed()), this, SLOT(OnActionChanged()));
        }
    }

    void TaskbarToolbar::RemoveActions() {
        QAction* action;

        foreach(action, m_actions) {
            disconnect(action, SIGNAL(changed()), this, SLOT(OnActionChanged()));
        }

        m_actions.clear();
    }

    void TaskbarToolbar::OnActionChanged() {
        Show();
    }

    void TaskbarToolbar::Show() {
        HIMAGELIST himl = ImageList_Create(20, 20, ILC_COLOR32, m_actions.size(), 0);

        THUMBBUTTON* thbButtons = new THUMBBUTTON[m_actions.size()];

        if (!himl) return;
        if (!thbButtons) return;

        int index = 0;
        QAction* action;

        foreach(action, m_actions) {
            QPixmap img = action->icon().pixmap(20);
            QBitmap mask  = img.createMaskFromColor(Qt::transparent);

            ImageList_Add(himl, img.toWinHBITMAP(QPixmap::PremultipliedAlpha), mask.toWinHBITMAP());

            wcscpy(thbButtons[index].szTip, action->text().toStdWString().c_str());

            thbButtons[index].iId = IDTB_FIRST + index;
            thbButtons[index].iBitmap = index;
            thbButtons[index].dwMask = (THUMBBUTTONMASK)(THB_BITMAP | THB_FLAGS | THB_TOOLTIP);
            thbButtons[index].dwFlags = (THUMBBUTTONFLAGS)(action->isEnabled() ? THBF_ENABLED : THBF_DISABLED);

            if (!action->isVisible()) thbButtons[index].dwFlags = (THUMBBUTTONFLAGS)(thbButtons[index].dwFlags | THBF_HIDDEN);
            if (action->data().toBool()) thbButtons[index].dwFlags = (THUMBBUTTONFLAGS)(thbButtons[index].dwFlags | THBF_DISMISSONCLICK);

            ++index;
        }

        HRESULT hr = Taskbar::GetInstance()->m_private->GetHandler()->ThumbBarSetImageList(m_widget->winId(), himl);

        if (S_OK == hr) {
            if (!m_initialized) {
                Taskbar::GetInstance()->m_private->GetHandler()->ThumbBarAddButtons(m_widget->winId(), m_actions.size(), thbButtons);
            } else {
                Taskbar::GetInstance()->m_private->GetHandler()->ThumbBarUpdateButtons(m_widget->winId(), m_actions.size(), thbButtons);
            }
        }

        m_initialized = true;

        delete[] thbButtons;
        ImageList_Destroy(himl);
    }

    bool TaskbarToolbar::winEvent(MSG* message, long* result) {

        switch (message->message)
        {
        case WM_COMMAND:
            {
                int buttonId = LOWORD(message->wParam) - IDTB_FIRST;

                if ((buttonId >= 0) && (buttonId < m_actions.size())) {
                    QAction* action = m_actions.at(buttonId);

                    if (action) action->trigger();
                    return true;
                }


                break;
            }
        }

        return false;
    }


    TaskbarToolbar::~TaskbarToolbar() {
        RemoveActions();
    }


}

#endif //Q_OS_WIN32
