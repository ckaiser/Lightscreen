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

#include "AppUserModel.h"

#ifdef Q_OS_WIN32
#include "win7_include.h"

namespace QW7 {
    void AppUserModel::SetCurrentProcessExplicitAppUserModelID(QString app_id) {
        HMODULE shell;

        shell = LoadLibrary(L"shell32.dll");
        if (shell) {
            t_SetCurrentProcessExplicitAppUserModelID set_user_model = reinterpret_cast<t_SetCurrentProcessExplicitAppUserModelID>(GetProcAddress (shell, "SetCurrentProcessExplicitAppUserModelID"));
            set_user_model(app_id.toStdWString().c_str());

            FreeLibrary (shell);
        }

    }

    QString AppUserModel::GetCurrentProcessExplicitAppUserModelID() {
        HMODULE shell;
        QString appid;

        shell = LoadLibrary(L"shell32.dll");
        if (shell) {
            t_GetCurrentProcessExplicitAppUserModelID get_user_model = reinterpret_cast<t_GetCurrentProcessExplicitAppUserModelID>(GetProcAddress (shell, "GetCurrentProcessExplicitAppUserModelID"));
            wchar_t* _app_id = NULL;
            if (SUCCEEDED(get_user_model(&_app_id))) {
                appid.fromStdWString(_app_id);
            }

            FreeLibrary (shell);
        }
        return appid;
    }
}

#endif //Q_OS_WIN32
