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

#ifndef JLPRIVATEDATA_H
#define JLPRIVATEDATA_H

#include <QtGlobal>

#ifdef Q_OS_WIN32
#include "win7_include.h"

namespace QW7 {

    struct JLPrivateData {
        bool m_list_is_initialized;
        UINT m_max_count;
        IObjectArray* m_array;
        ICustomDestinationList* m_handler;

        JLPrivateData(QString app_id) {
            m_handler = NULL;
            m_array = NULL;
            m_list_is_initialized = false;

            HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_ICustomDestinationList,
                                          reinterpret_cast<void**> (&(m_handler)));

            if (SUCCEEDED(hr)){
                if(app_id.length() > 0) {
                    m_handler->SetAppID(app_id.toStdWString().c_str());
                }
            }
        }

        inline ICustomDestinationList* ListHandler() { return m_handler;}

        void InitList() {
            ReleaseList();

            m_max_count = 0;
            m_list_is_initialized = SUCCEEDED(m_handler->BeginList(&m_max_count, IID_IObjectArray, reinterpret_cast<void**> (&(m_array))));
        }

        void ReleaseList() {
            if (m_array) {
                m_array->Release();
                m_array = NULL;
                m_list_is_initialized = false;
            }
        }

        ~JLPrivateData() {
            if (m_handler) {
                m_handler->Release();
            }

            ReleaseList();
        }
    };

}

#endif // Q_OS_WIN32

#endif // JLPRIVATEDATA_H
