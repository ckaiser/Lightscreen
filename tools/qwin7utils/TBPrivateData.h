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

#ifndef TBPRIVATEDATA_H
#define TBPRIVATEDATA_H
#include <QtGlobal>

#ifdef Q_OS_WIN32
#include "win7_include.h"

namespace QW7 {

    struct TBPrivateData {
        ITaskbarList4* m_handler;

        TBPrivateData() {
            m_handler = NULL;

            HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList4,
                                          reinterpret_cast<void**> (&(m_handler)));

            if (SUCCEEDED(hr)){

                hr = m_handler->HrInit();

                if (FAILED(hr)) {
                    m_handler->Release();
                    m_handler = NULL;
                }
            }
        }

        ITaskbarList4* GetHandler() { return m_handler;}

        ~TBPrivateData() {
            if (m_handler) {
                m_handler->Release();
            }
        }
    };

}

#endif // Q_OS_WIN32
#endif // TBPRIVATEDATA_H
