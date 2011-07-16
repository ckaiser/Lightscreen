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

#include "TaskbarTabs.h"

#ifdef Q_OS_WIN32
#include "Utils.h"
#include "Taskbar.h"
#include "TBPrivateData.h"

namespace QW7 {

    TaskbarTabs* TaskbarTabs::m_instance = NULL;
    QCoreApplication::EventFilter TaskbarTabs::m_oldEventFilter = NULL;

    TaskbarTabs::TaskbarTabs(){
    }

    TaskbarTabs::~TaskbarTabs() {
        TaskbarTab* tab;
        foreach(tab, m_tabs) {
            m_tabs.removeOne(tab);

            delete tab->m_tab_widget;
            delete tab;
        }

        if (m_oldEventFilter) {
            qApp->setEventFilter(m_oldEventFilter);
        }
    }

    TaskbarTabs* TaskbarTabs::GetInstance() {

        if (m_instance == NULL) {
            m_instance = new TaskbarTabs();
            m_oldEventFilter = qApp->setEventFilter(&TaskbarTabs::eventFilter);
        }

        return m_instance;
    }

    void TaskbarTabs::ReleaseInstance() {
        if (m_instance != NULL) {
            delete m_instance;
        }
    }

    void TaskbarTabs::SetParentWidget(QWidget* widget) {
        m_parentWidget = widget;
    }

    void TaskbarTabs::AddTab(QWidget* widget, QString title) {
        AddTab(widget, title, QIcon(), QPixmap());
    }

    void TaskbarTabs::AddTab(QWidget* widget, QString title, QIcon icon) {
        AddTab(widget, title, icon, QPixmap());
    }

    void TaskbarTabs::AddTab(QWidget* widget, QString title, QPixmap pixmap) {
        AddTab(widget, title, QIcon(), pixmap);
    }

    void TaskbarTabs::AddTab(QWidget* widget, QString title, QIcon icon, QPixmap pixmap) {
        TaskbarTab* tab = new TaskbarTab();

        tab->m_widget = widget;
        tab->m_tab_widget = new QWidget();
        tab->m_tab_widget->setWindowTitle(title);
        tab->m_tab_widget->setWindowIcon(icon.isNull() ? widget->windowIcon() : icon);
        tab->m_thumbnail = pixmap;

        m_tabs.append(tab);
        EnableWidgetIconicPreview(tab->m_tab_widget, true);

        Taskbar::GetInstance()->m_private->GetHandler()->RegisterTab(tab->m_tab_widget->winId(), m_parentWidget->winId());
        Taskbar::GetInstance()->m_private->GetHandler()->SetTabOrder(tab->m_tab_widget->winId(), NULL);
        Taskbar::GetInstance()->m_private->GetHandler()->SetTabActive(NULL, m_tabs.back()->m_tab_widget->winId(), 0);
    }

    void TaskbarTabs::UpdateTab(QWidget* widget, QString title) {
        UpdateTab(widget, title, QIcon(), QPixmap());
    }

    void TaskbarTabs::UpdateTab(QWidget* widget, QString title, QIcon icon) {
        UpdateTab(widget, title, icon, QPixmap());
    }

    void TaskbarTabs::UpdateTab(QWidget* widget, QString title, QPixmap pixmap) {
        UpdateTab(widget, title, QIcon(), pixmap);
    }

    void TaskbarTabs::UpdateTab(QWidget* widget, QString title, QIcon icon, QPixmap pixmap) {
        TaskbarTab* tab = FindTabByWId(widget->winId(), true);

        if (tab == NULL) return;

        tab->m_thumbnail = pixmap;
        tab->m_tab_widget->setWindowIcon(icon);
        tab->m_tab_widget->setWindowTitle(title);

        InvalidateIconicBitmaps(tab->m_tab_widget);
    }


    void TaskbarTabs::SetActiveTab(QWidget* widget) {
        TaskbarTab* tab = FindTabByWId(widget->winId(), true);

        if (tab == NULL) return;

         Taskbar::GetInstance()->m_private->GetHandler()->SetTabActive(tab->m_tab_widget->winId(), m_parentWidget->winId(), 0);
    }

    void TaskbarTabs::RemoveTab(QWidget* widget) {
        TaskbarTab* tab = FindTabByWId(widget->winId(), true);

        if (tab == NULL) return;

        Taskbar::GetInstance()->m_private->GetHandler()->UnregisterTab(tab->m_tab_widget->winId());

        m_tabs.removeOne(tab);
        delete tab->m_tab_widget;
        delete tab;
    }

    void TaskbarTabs::SetTabOrder(QWidget* widget, QWidget* after_widget) {
         TaskbarTab* tab1 = FindTabByWId(widget->winId(), true);
         TaskbarTab* tab2 = FindTabByWId(after_widget->winId(), true);

         if (tab1 == NULL || tab2 == NULL) return;

         Taskbar::GetInstance()->m_private->GetHandler()->SetTabOrder(tab1->m_tab_widget->winId(), tab2->m_tab_widget->winId());
    }

    void TaskbarTabs::InvalidateTabThumbnail(QWidget* widget) {
        TaskbarTab* tab = FindTabByWId(widget->winId());

        if (tab == NULL) return;

        InvalidateIconicBitmaps(tab->m_tab_widget);
    }

    TaskbarTabs::TaskbarTab* TaskbarTabs::FindTabByWId(WId id, bool inserted) {
        bool found = false;

        WId tabWId = NULL;
        TaskbarTab* tab;
        foreach(tab, m_tabs) {

            tabWId = inserted ? tab->m_widget->winId() : tab->m_tab_widget->winId();
            if (tabWId == id) {
                found = true;
                break;
            }
        }

        return found ? tab : NULL;
    }

    void TaskbarTabs::TabAction(WId id, TABEVENT action) {
        TaskbarTab* tab = FindTabByWId(id);

        if (tab != NULL) {
            switch (action) {
            case TAB_CLICK :
                emit OnTabClicked(tab->m_widget);
                break;

            case TAB_CLOSE:
                emit OnTabClose(tab->m_widget);
                break;

            case TAB_HOVER:
                emit OnTabHover(tab->m_widget);
                break;
            }
        }
    }

    void TaskbarTabs::SetPeekBitmap(WId id, QSize size, bool isLive) {
        TaskbarTab* tab = FindTabByWId(id);

        if (tab == NULL) return;

        QPixmap thumbnail = (!tab->m_thumbnail.isNull() && !isLive) ? tab->m_thumbnail :
                                                                      QPixmap::grabWidget(isLive ? m_parentWidget : tab->m_widget).scaled(size, Qt::KeepAspectRatio);

        HBITMAP hbitmap = thumbnail.toWinHBITMAP(isLive ? QPixmap::NoAlpha : QPixmap::Alpha);

        isLive ? DwmSetIconicLivePreviewBitmap(id, hbitmap, 0, 0) : DwmSetIconicThumbnail(id, hbitmap, 0);

        if (hbitmap) DeleteObject(hbitmap);
    }

    bool TaskbarTabs::eventFilter(void *message_, long *result)
    {
        MSG* message = static_cast<MSG*>(message_);

        switch(message->message)
        {
        case WM_DWMSENDICONICTHUMBNAIL : {
                if (message->hwnd == TaskbarTabs::GetInstance()->m_parentWidget->winId()) return false;

                TaskbarTabs::GetInstance()->SetPeekBitmap(message->hwnd, QSize(HIWORD(message->lParam), LOWORD(message->lParam)));
                return true;
            }
        case WM_DWMSENDICONICLIVEPREVIEWBITMAP : {
                TaskbarTabs::GetInstance()->TabAction(message->hwnd, TAB_HOVER);

                if (message->hwnd == TaskbarTabs::GetInstance()->m_parentWidget->winId()) return false;

                TaskbarTabs::GetInstance()->SetPeekBitmap(message->hwnd, TaskbarTabs::GetInstance()->m_parentWidget->size(), true);
                return true;
            }
        case WM_ACTIVATE : {
                if (LOWORD(message->wParam) == WA_ACTIVE) {
                    TaskbarTabs::GetInstance()->TabAction(message->hwnd, TAB_CLICK);
                }
                return false;
            }
        case WM_CLOSE : {
                TaskbarTabs::GetInstance()->TabAction(message->hwnd, TAB_CLOSE);
                return false;
            }
        }

        if (TaskbarTabs::m_oldEventFilter)
            return TaskbarTabs::m_oldEventFilter(message_, result);
        else
            return false;
    }
}

#endif //Q_OS_WIN32
