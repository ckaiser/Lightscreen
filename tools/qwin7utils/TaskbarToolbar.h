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

#ifndef TASKBARTOOLBAR_H
#define TASKBARTOOLBAR_H

#include <QRect>
#include <QList>
#include <QAction>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QCoreApplication>

#ifdef Q_OS_WIN32

namespace QW7 {

    class TaskbarToolbar : public QObject
    {
        Q_OBJECT
    public:
        explicit TaskbarToolbar(QWidget *parent = 0);
        void AddAction(QAction* action);
        void AddActions(QList<QAction*>& actions);
        void Show();
        bool winEvent(MSG* message, long* result);
        void SetWidget(QWidget* widget);
        void SetThumbnailClip(QRect rect);
        void SetThumbnailTooltip(QString tooltip);

        ~TaskbarToolbar();

    private:
        bool m_initialized;
        QWidget* m_widget;
        QList<QAction*> m_actions;
        void RemoveActions();

    private slots:
        void OnActionChanged();

    };
}
#endif // Q_OS_WIN32

#endif // TASKBARTOOLBAR_H
