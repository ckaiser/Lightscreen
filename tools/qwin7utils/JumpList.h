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

#ifndef JUMPLIST_H
#define JUMPLIST_H

#include <QList>
#include <QString>

#ifdef Q_OS_WIN32
namespace QW7 {

    enum JLITEMTYPE {
        JL_ITEM = 0,
        JL_LINK = 1,
        JL_SEPARATOR = 2,
        JL_OTHER = 3
    };

    struct JumpListItem {
        QString m_path;
        QString m_arguments;
        QString m_description;
        QString m_title;
        QString m_icon_path;
        int     m_icon_index;
        QString m_working_path;
        JLITEMTYPE m_type;

        JumpListItem() {Reset(); m_type = JL_SEPARATOR;}

        JumpListItem(QString path): m_path(path), m_type(JL_ITEM) {}

        JumpListItem(QString path, QString arguments, QString title, QString description, \
               QString icon_path, int icon_index, QString working_path = "") : \
                m_path(path), m_arguments(arguments), m_description(description), \
                m_title(title), m_icon_path(icon_path), m_icon_index(icon_index), \
                m_working_path(working_path), m_type(JL_LINK) {}

        inline void Reset() {
            m_path.clear(); m_arguments.clear(); m_description.clear();
            m_title.clear(); m_icon_path.clear(); m_icon_index = 0;
            m_working_path.clear(); m_type = JL_OTHER;
        }

        inline bool operator==(const JumpListItem& second) const
        {
            if (m_type == second.m_type) {

                if (m_type == JL_ITEM) {
                    return m_path == second.m_path;
                } else if (m_type == JL_LINK) {
                    return (m_path == second.m_path && m_arguments == second.m_arguments);
                }
            }

            return false;
        }

    };

    typedef enum KnownListType {
        LIST_RECENT = 0,
        LIST_FREQUENT = 1
    } KnownListType;

    struct JLPrivateData;

    class JumpList
    {
    private:

        QString m_app_id;
        JLPrivateData* m_private;

        long GetKnownList(KnownListType type, QList<JumpListItem>& items);

    public:
        JumpList(QString app_id = "");
        ~JumpList();

        long SetAppID(QString app_id);

        //ICustomDestinationList
        long Begin();
        long AddRecentCategory();
        long AddFrequentCategory();
        long AddUserTasks(const QList<JumpListItem>& items);
        long AddCategory(QString title, const QList<JumpListItem>& items);
        long GetRemovedDestinations(QList<JumpListItem>& items);
        long Clear();
        long Abort();
        long Commit();

        //IApplicationDocumentLists
        long GetRecentList(QList<JumpListItem>& items);
        long GetFrequentList(QList<JumpListItem>& items);

        //IApplicationDestinations
        long ClearRecentAndFrequentList();

        //Utils
        static void AddPathToRecent(QString path);
        static void AddLinkToRecent(const JumpListItem& link);
    };
}
#endif //Q_OS_WIN32

#endif // JUMPLIST_H
