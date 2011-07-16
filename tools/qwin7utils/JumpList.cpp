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

#include "JumpList.h"

#ifdef Q_OS_WIN32

#include "win7_include.h"
#include "JLPrivateData.h"

namespace QW7 {


    namespace LOCAL_UTILS {

        IShellItem* JLItem2ShellItem(const JumpListItem& item) {
            HMODULE shell;
            IShellItem *shell_item = NULL;
            t_SHCreateItemFromParsingName SHCreateItemFromParsingName = NULL;

            shell = LoadLibrary(L"shell32.dll");

            if (shell) {
                SHCreateItemFromParsingName = reinterpret_cast<t_SHCreateItemFromParsingName>
                                              (GetProcAddress (shell, "SHCreateItemFromParsingName"));

                if (SHCreateItemFromParsingName != NULL) {
                    SHCreateItemFromParsingName(item.m_path.toStdWString().c_str(), NULL, IID_IShellItem,
                                                reinterpret_cast<void**> (&(shell_item)));
                }

                FreeLibrary (shell);
            }

            return shell_item;
        }

        IShellLink* JLItem2ShellLink(const JumpListItem& item) {
            IShellLink* shell_link = NULL;
            IPropertyStore* prop_store = NULL;
            bool is_not_separator = (item.m_path.length() > 0);

            HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink,
                                          reinterpret_cast<void**> (&(shell_link)));

            if(SUCCEEDED(hr)) {

                if (is_not_separator) {
                    shell_link->SetPath(item.m_path.toStdWString().c_str());
                    shell_link->SetArguments(item.m_arguments.toStdWString().c_str());
                    shell_link->SetIconLocation(item.m_icon_path.toStdWString().c_str(), item.m_icon_index);
                    shell_link->SetDescription(item.m_description.toStdWString().c_str());
                }

                hr = shell_link->QueryInterface(IID_IPropertyStore, reinterpret_cast<void**> (&(prop_store)));

                if (SUCCEEDED(hr)) {
                    PROPVARIANT pv;

                    if (is_not_separator) {
                        hr = InitPropVariantFromString(item.m_title.toStdWString().c_str(), &pv);

                        if (SUCCEEDED(hr)) {
                            hr = prop_store->SetValue(PKEY_Title, pv);
                        }
                    } else {
                        hr = InitPropVariantFromBoolean(TRUE, &pv);

                        if (SUCCEEDED(hr)) {
                            hr = prop_store->SetValue(PKEY_AppUserModel_IsDestListSeparator, pv);
                        }
                    }

                    //Save the changes we made to the property store
                    prop_store->Commit();
                    prop_store->Release();

                    PropVariantClear(&pv);
                }
            }

            return shell_link;
        }

        long List2ObjectCollection(const QList<JumpListItem>& items, IObjectCollection* obj_collection) {
            HRESULT hr = 0;

            if (obj_collection == NULL) {
                return -1;
            }

            for (int index = 0; index < items.size(); ++index) {
                JumpListItem item = items.at(index);

                if (item.m_type == JL_ITEM) {
                    IShellItem* s_item = JLItem2ShellItem(item);

                    if (s_item) {
                        hr = obj_collection->AddObject(s_item);
                    }
                } else if (item.m_type != JL_OTHER){
                    IShellLink* s_link = JLItem2ShellLink(item);

                    if (s_link) {
                        hr = obj_collection->AddObject(s_link);
                    }
                }
            }

            return hr;
        }

        void ObjectArray2List(IObjectArray *object_array, QList<JumpListItem>& items) {
            bool fRet = false;
            UINT cItems;

            if (SUCCEEDED(object_array->GetCount(&cItems)))
            {
                IShellItem *shell_item;
                IShellLink *shell_link;
                wchar_t buffer[MAX_PATH];

                for (UINT i = 0; !fRet && i < cItems; i++)
                {
                    JumpListItem item;

                    if (SUCCEEDED(object_array->GetAt(i, IID_IShellItem, reinterpret_cast<void**> (&(shell_item)))))
                    {
                        LPWSTR file_path = NULL;
                        HRESULT hr = shell_item->GetDisplayName(SIGDN_FILESYSPATH, &file_path);

                        if (SUCCEEDED(hr))
                        {
                            item.m_type = JL_ITEM;
                            item.m_path = QString::fromStdWString(file_path);
                            items.append(item);
                            CoTaskMemFree(file_path);
                        }

                        shell_item->Release();
                    } else if (SUCCEEDED(object_array->GetAt(i, IID_IShellLink, reinterpret_cast<void**> (&(shell_link))))) {
                        int index = 0;

                        item.m_type = JL_LINK;
                        shell_link->GetDescription(buffer, MAX_PATH);
                        item.m_description.fromStdWString(buffer);

                        shell_link->GetArguments(buffer, MAX_PATH);
                        item.m_arguments.fromStdWString(buffer);

                        shell_link->GetIconLocation(buffer, MAX_PATH, &index);
                        item.m_icon_path.fromStdWString(buffer);
                        item.m_icon_index = index;

                        shell_link->GetPath(buffer, MAX_PATH, NULL, SLGP_UNCPRIORITY);
                        item.m_path.fromStdWString(buffer);

                        shell_link->GetWorkingDirectory(buffer, MAX_PATH);
                        item.m_working_path.fromStdWString(buffer);

                        items.append(item);

                        shell_link->Release();
                    }
                }
            }
        }
    }


    JumpList::JumpList(QString app_id) {
        m_app_id = app_id;
        m_private = new JLPrivateData(app_id);
    }

    long JumpList::SetAppID(QString app_id) {
        if (m_private->m_list_is_initialized) {
            return -1;
        }

        m_app_id = app_id;

        return m_private->ListHandler()->SetAppID(app_id.toStdWString().c_str());
    }

    //ICustomDestinationList
    long JumpList::Begin() {
        if (!m_private->m_list_is_initialized) {
            m_private->InitList();
        }

        return m_private->m_list_is_initialized ? 0 : -1;
    }

    long JumpList::AddRecentCategory() {
        if (!m_private->m_list_is_initialized) {
            return -1;
        }

        return m_private->ListHandler()->AppendKnownCategory(KDC_RECENT);
    }

    long JumpList::AddFrequentCategory() {
        if (!m_private->m_list_is_initialized) {
            return -1;
        }

        return m_private->ListHandler()->AppendKnownCategory(KDC_FREQUENT);
    }

    long JumpList::AddCategory(QString title, const QList<JumpListItem>& items) {
        if (!m_private->m_list_is_initialized) {
            return -1;
        }

        IObjectCollection* obj_collection;

        HRESULT hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL,
                                      CLSCTX_INPROC, IID_IObjectCollection, reinterpret_cast<void**> (&(obj_collection)));

        if (SUCCEEDED(hr)) {
            IObjectArray* object_array;

            LOCAL_UTILS::List2ObjectCollection(items, obj_collection);
            hr = obj_collection->QueryInterface(IID_IObjectArray,reinterpret_cast<void**> (&(object_array)));

            if (SUCCEEDED(hr)) {
                hr = m_private->ListHandler()->AppendCategory(title.toStdWString().c_str(), object_array);
                object_array->Release();
            }

            obj_collection->Release();
        }

        return hr;
    }

    long JumpList::AddUserTasks(const QList<JumpListItem>& items) {
        if (!m_private->m_list_is_initialized) {
            return -1;
        }

        IObjectCollection* obj_collection;

        HRESULT hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL,
                                      CLSCTX_INPROC, IID_IObjectCollection, reinterpret_cast<void**> (&(obj_collection)));

        if (SUCCEEDED(hr)) {
            LOCAL_UTILS::List2ObjectCollection(items, obj_collection);
            hr = m_private->ListHandler()->AddUserTasks(obj_collection);

            obj_collection->Release();
        }

        return hr;
    }

    long JumpList::GetRemovedDestinations(QList<JumpListItem>& items) {
        IObjectArray* object_array = NULL;

        HRESULT hr = m_private->ListHandler()->GetRemovedDestinations(IID_IObjectArray,
                                                                  reinterpret_cast<void**> (&(object_array)));

        if (SUCCEEDED(hr)) {
            LOCAL_UTILS::ObjectArray2List(object_array, items);
            object_array->Release();
        }

        return hr;
    }

    long JumpList::Clear() {
        return m_private->ListHandler()->DeleteList(m_app_id.isEmpty() ? NULL : m_app_id.toStdWString().c_str());
    }

    long JumpList::Commit() {
        if (!m_private->m_list_is_initialized) {
            return -1;
        }

        HRESULT hr = m_private->ListHandler()->CommitList();
        m_private->ReleaseList();

        return hr;
    }

    long JumpList::Abort() {
        if (!m_private->m_list_is_initialized) {
            return -1;
        }

        HRESULT hr = m_private->ListHandler()->AbortList();
        m_private->ReleaseList();

        return hr;
    }

    //IApplicationDocumentLists
    long JumpList::GetKnownList(KnownListType type, QList<JumpListItem>& items) {
        IObjectArray* object_array = NULL;
        IApplicationDocumentLists* app_documents = NULL;

        HRESULT hr = CoCreateInstance(CLSID_ApplicationDocumentLists, NULL, CLSCTX_INPROC_SERVER, IID_IApplicationDocumentLists,
                         reinterpret_cast<void**> (&(app_documents)));

        if (SUCCEEDED(hr)) {
            if (m_app_id.length() > 0) {
                app_documents->SetAppID(m_app_id.toStdWString().c_str());
            }

            hr = app_documents->GetList(type == LIST_RECENT ? ADLT_RECENT : ADLT_FREQUENT, 0, IID_IObjectArray,
                                        reinterpret_cast<void**> (&(object_array)));

            if (SUCCEEDED(hr)) {
                LOCAL_UTILS::ObjectArray2List(object_array, items);
                object_array->Release();
            }

            app_documents->Release();
        }

        return hr;
    }

    long JumpList::GetRecentList(QList<JumpListItem>& items) {
        return GetKnownList(LIST_RECENT, items);
    }

    long JumpList::GetFrequentList(QList<JumpListItem>& items) {
        return GetKnownList(LIST_FREQUENT, items);
    }

    //IApplicationDestinations
    long JumpList::ClearRecentAndFrequentList() {
        IApplicationDestinations* app_destinations = NULL;

        HRESULT hr = CoCreateInstance(CLSID_ApplicationDestinations, NULL ,CLSCTX_INPROC_SERVER, IID_IApplicationDestinations,
                         reinterpret_cast<void**> (&(app_destinations)));

        if (SUCCEEDED(hr)) {
            if (m_app_id.length() > 0) {
                app_destinations->SetAppID(m_app_id.toStdWString().c_str());
            }

            hr = app_destinations->RemoveAllDestinations();

            app_destinations->Release();
        }

        return hr;
    }

    void JumpList::AddPathToRecent(QString path) {
        SHAddToRecentDocs(0x00000003, path.toStdWString().c_str());
    }

    void JumpList::AddLinkToRecent(const JumpListItem& link) {
        SHAddToRecentDocs(0x00000006, LOCAL_UTILS::JLItem2ShellLink(link));
    }

    JumpList::~JumpList() {
        delete m_private;
    }
}
#endif //Q_OS_WIN32
