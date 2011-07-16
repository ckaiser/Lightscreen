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

#ifndef TASKBARTABS_H
#define TASKBARTABS_H

#include <QList>
#include <QIcon>
#include <QString>
#include <QPixmap>
#include <QWidget>
#include <QCoreApplication>

#ifdef Q_OS_WIN32

namespace QW7 {

    class TaskbarTabs : public QObject
    {
        Q_OBJECT
    public:

        static TaskbarTabs* GetInstance();
        static void ReleaseInstance();
        static bool eventFilter(void *message_, long *result);

        void SetParentWidget(QWidget* widget);

        void AddTab(QWidget* widget, QString title = "");
        void AddTab(QWidget* widget, QString title, QIcon icon);
        void AddTab(QWidget* widget, QString title, QPixmap pixmap);
        void AddTab(QWidget* widget, QString title, QIcon icon, QPixmap pixmap);
        void SetActiveTab(QWidget* widget);
        void RemoveTab(QWidget* widget);
        void SetTabOrder(QWidget* widget, QWidget* after_widget);

        void UpdateTab(QWidget* widget, QString title = "");
        void UpdateTab(QWidget* widget, QString title, QIcon icon);
        void UpdateTab(QWidget* widget, QString title, QPixmap pixmap);
        void UpdateTab(QWidget* widget, QString title, QIcon icon, QPixmap pixmap);
        void InvalidateTabThumbnail(QWidget* widget);

    private:
        struct TaskbarTab {
            TaskbarTab() : m_widget(NULL), m_tab_widget(NULL) {}

            QPixmap  m_thumbnail;
            QWidget* m_widget;
            QWidget* m_tab_widget;
        };

        enum TABEVENT {
            TAB_CLICK = 0,
            TAB_CLOSE = 1,
            TAB_HOVER = 2
        };

        QWidget* m_parentWidget;
        QList<TaskbarTab*> m_tabs;

        static TaskbarTabs* m_instance;
        static QCoreApplication::EventFilter m_oldEventFilter;

        TaskbarTabs();
        ~TaskbarTabs();

        void TabAction(WId id, TABEVENT action);
        TaskbarTab* FindTabByWId(WId id, bool inserted = false);
        void SetPeekBitmap(WId id, QSize size, bool isLive = false);

    signals:
        void OnTabClicked(QWidget* widget);
        void OnTabClose(QWidget* widget);
        void OnTabHover(QWidget* widget);
    };

}
#endif // Q_OS_WIN32
#endif // TASKBARTABS_H
