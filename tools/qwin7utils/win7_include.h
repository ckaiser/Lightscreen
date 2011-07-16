#ifndef WIN7UTILS_H
#define WIN7UTILS_H
#include <QtGlobal>

#ifdef Q_OS_WIN32
#include <shlobj.h>
#include <shlwapi.h>
#include <initguid.h>
#include <objidl.h>
#include <shellapi.h>
//#include <ShObjIdl.h>

//Definitions for Windows 7 Taskbar and JumpList

#define WM_DWMSENDICONICTHUMBNAIL         0x0323
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP 0x0326

typedef enum STPFLAG
{
    STPF_NONE = 0,
    STPF_USEAPPTHUMBNAILALWAYS = 0x1,
    STPF_USEAPPTHUMBNAILWHENACTIVE = 0x2,
    STPF_USEAPPPEEKALWAYS = 0x4,
    STPF_USEAPPPEEKWHENACTIVE = 0x8
} STPFLAG;

typedef enum THUMBBUTTONMASK
{
    THB_BITMAP = 0x1,
    THB_ICON = 0x2,
    THB_TOOLTIP	= 0x4,
    THB_FLAGS = 0x8
} THUMBBUTTONMASK;

typedef enum THUMBBUTTONFLAGS
{
    THBF_ENABLED = 0,
    THBF_DISABLED = 0x1,
    THBF_DISMISSONCLICK	= 0x2,
    THBF_NOBACKGROUND = 0x4,
    THBF_HIDDEN	= 0x8,
    THBF_NONINTERACTIVE	= 0x10
} THUMBBUTTONFLAGS;

typedef struct THUMBBUTTON
{
    THUMBBUTTONMASK dwMask;
    UINT iId;
    UINT iBitmap;
    HICON hIcon;
    WCHAR szTip[260];
    THUMBBUTTONFLAGS dwFlags;
} THUMBBUTTON;
typedef struct THUMBBUTTON *LPTHUMBBUTTON;

typedef enum TBPFLAG
{
    TBPF_NOPROGRESS = 0,
    TBPF_INDETERMINATE = 0x1,
    TBPF_NORMAL = 0x2,
    TBPF_ERROR = 0x4,
    TBPF_PAUSED = 0x8
} TBPFLAG;

typedef enum KNOWNDESTCATEGORY
{
    KDC_FREQUENT = 1,
    KDC_RECENT = (KDC_FREQUENT + 1)
} KNOWNDESTCATEGORY;

typedef enum _SIGDN
{
    SIGDN_NORMALDISPLAY	= 0,
    SIGDN_PARENTRELATIVEPARSING	= (int) 0x80018001,
    SIGDN_DESKTOPABSOLUTEPARSING = (int) 0x80028000,
    SIGDN_PARENTRELATIVEEDITING	= (int) 0x80031001,
    SIGDN_DESKTOPABSOLUTEEDITING = (int) 0x8004c000,
    SIGDN_FILESYSPATH = (int) 0x80058000,
    SIGDN_URL = (int) 0x80068000,
    SIGDN_PARENTRELATIVEFORADDRESSBAR = (int) 0x8007c001,
    SIGDN_PARENTRELATIVE = (int) 0x80080001
} SIGDN;

enum _SICHINTF
{
    SICHINT_DISPLAY = 0,
    SICHINT_ALLFIELDS = (int) 0x80000000,
    SICHINT_CANONICAL = 0x10000000,
    SICHINT_TEST_FILESYSPATH_IF_NOT_EQUAL = 0x20000000
};
typedef DWORD SICHINTF;

typedef enum APPDOCLISTTYPE
{
    ADLT_RECENT = 0,
    ADLT_FREQUENT = (ADLT_RECENT + 1)
} APPDOCLISTTYPE;

typedef struct _tagpropertykey
{
    GUID fmtid;
    DWORD pid;
} PROPERTYKEY;

#define REFPROPERTYKEY const PROPERTYKEY &

typedef struct tagPROPVARIANT PROPVARIANT;
#define REFPROPVARIANT const PROPVARIANT &


#define DEFINE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) const PROPERTYKEY name \
                = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }

#define DEFINE_KNOWN_FOLDER(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        extern const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define DEFINE_GUID_(n,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) const GUID n GUID_SECT = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

typedef enum
{
    KF_FLAG_DEFAULT         = 0x00000000,
    KF_FLAG_CREATE          = 0x00008000,
    KF_FLAG_DONT_VERIFY     = 0x00004000,
    KF_FLAG_DONT_UNEXPAND   = 0x00002000,
    KF_FLAG_NO_ALIAS        = 0x00001000,
    KF_FLAG_INIT            = 0x00000800,
    KF_FLAG_DEFAULT_PATH    = 0x00000400,
    KF_FLAG_NOT_PARENT_RELATIVE = 0x00000200,
    KF_FLAG_SIMPLE_IDLIST   = 0x00000100,
    KF_FLAG_ALIAS_ONLY      = 0x80000000,
} KNOWN_FOLDER_FLAG;

typedef GUID KNOWNFOLDERID;
#define REFKNOWNFOLDERID const KNOWNFOLDERID &

//MIDL_INTERFACE("56FDF342-FD6D-11d0-958A-006097C9A090")
DECLARE_INTERFACE_(ITaskbarList, IUnknown)
{
    STDMETHOD (HrInit) (THIS) PURE;
    STDMETHOD (AddTab) (THIS_ HWND hwnd) PURE;
    STDMETHOD (DeleteTab) (THIS_ HWND hwnd) PURE;
    STDMETHOD (ActivateTab) (THIS_ HWND hwnd) PURE;
    STDMETHOD (SetActiveAlt) (THIS_ HWND hwnd) PURE;
};
typedef ITaskbarList *LPITaskbarList;

//MIDL_INTERFACE("602D4995-B13A-429b-A66E-1935E44F4317")
DECLARE_INTERFACE_(ITaskbarList2, ITaskbarList)
{
    STDMETHOD (MarkFullscreenWindow) (THIS_ HWND hwnd, int fFullscreen) PURE;
};
typedef ITaskbarList2 *LPITaskbarList2;

//MIDL_INTERFACE("ea1afb91-9e28-4b86-90e9-9e9f8a5eefaf")
DECLARE_INTERFACE_(ITaskbarList3, ITaskbarList2)
{
    STDMETHOD (SetProgressValue) (THIS_ HWND hwnd, ULONGLONG ullCompleted, ULONGLONG ullTotal) PURE;
    STDMETHOD (SetProgressState) (THIS_ HWND hwnd, TBPFLAG tbpFlags) PURE;
    STDMETHOD (RegisterTab) (THIS_ HWND hwndTab,HWND hwndMDI) PURE;
    STDMETHOD (UnregisterTab) (THIS_ HWND hwndTab) PURE;
    STDMETHOD (SetTabOrder) (THIS_ HWND hwndTab, HWND hwndInsertBefore) PURE;
    STDMETHOD (SetTabActive) (THIS_ HWND hwndTab, HWND hwndMDI, DWORD dwReserved) PURE;
    STDMETHOD (ThumbBarAddButtons) (THIS_ HWND hwnd, UINT cButtons, LPTHUMBBUTTON pButton) PURE;
    STDMETHOD (ThumbBarUpdateButtons) (THIS_ HWND hwnd, UINT cButtons, LPTHUMBBUTTON pButton) PURE;
    STDMETHOD (ThumbBarSetImageList) (THIS_ HWND hwnd, HIMAGELIST himl) PURE;
    STDMETHOD (SetOverlayIcon) (THIS_ HWND hwnd, HICON hIcon, LPCWSTR pszDescription) PURE;
    STDMETHOD (SetThumbnailTooltip) (THIS_ HWND hwnd, LPCWSTR pszTip) PURE;
    STDMETHOD (SetThumbnailClip) (THIS_ HWND hwnd, RECT *prcClip) PURE;
};
typedef ITaskbarList3 *LPITaskbarList3;

//MIDL_INTERFACE("c43dc798-95d1-4bea-9030-bb99e2983a1a")
DECLARE_INTERFACE_(ITaskbarList4, ITaskbarList3)
{
    STDMETHOD (SetTabProperties) (HWND hwndTab, STPFLAG stpFlags) PURE;
};
typedef ITaskbarList4 *LPITaskbarList4;

//MIDL_INTERFACE("92CA9DCD-5622-4bba-A805-5E9F541BD8C9")
DECLARE_INTERFACE_(IObjectArray, IUnknown)
{
    STDMETHOD (GetCount) (UINT *pcObjects) PURE;
    STDMETHOD (GetAt) (UINT uiIndex, REFIID riid, void **ppv) PURE;
};
typedef IObjectArray *LPIObjectArray;

//MIDL_INTERFACE("6332debf-87b5-4670-90c0-5e57b408a49e")
DECLARE_INTERFACE_(ICustomDestinationList, IUnknown)
{
    STDMETHOD (SetAppID) (LPCWSTR pszAppID);
    STDMETHOD (BeginList) (UINT *pcMinSlots, REFIID riid, void **ppv) PURE;
    STDMETHOD (AppendCategory) (LPCWSTR pszCategory, IObjectArray *poa) PURE;
    STDMETHOD (AppendKnownCategory) (KNOWNDESTCATEGORY category) PURE;
    STDMETHOD (AddUserTasks) (IObjectArray *poa) PURE;
    STDMETHOD (CommitList) (void) PURE;
    STDMETHOD (GetRemovedDestinations) (REFIID riid, void **ppv) PURE;
    STDMETHOD (DeleteList) (LPCWSTR pszAppID) PURE;
    STDMETHOD (AbortList)  (void) PURE;

};
typedef ICustomDestinationList *LPICustomDestinationList;

//MIDL_INTERFACE("5632b1a4-e38a-400a-928a-d4cd63230295")
DECLARE_INTERFACE_(IObjectCollection, IObjectArray)
{
    STDMETHOD (AddObject) (IUnknown *punk) PURE;
    STDMETHOD (AddFromArray) (IObjectArray *poaSource)PURE;
    STDMETHOD (RemoveObjectAt) (UINT uiIndex) PURE;
    STDMETHOD (Clear) (void) PURE;
};
typedef IObjectCollection *LPIObjectCollection;

//MIDL_INTERFACE("43826d1e-e718-42ee-bc55-a1e261c37bfe")
DECLARE_INTERFACE_(IShellItem, IUnknown)
{
    STDMETHOD (BindToHandler) (IBindCtx *pbc, REFGUID bhid, REFIID riid,void **ppv) PURE;
    STDMETHOD (GetParent) (IShellItem **ppsi) PURE;
    STDMETHOD (GetDisplayName) (SIGDN sigdnName, LPWSTR *ppszName) PURE;
    STDMETHOD (GetAttributes) (SFGAOF sfgaoMask, SFGAOF *psfgaoAttribs) PURE;
    STDMETHOD (Compare) (IShellItem *psi, SICHINTF hint, int *piOrder) PURE;
};
typedef IShellItem *LPIShellItem;

//MIDL_INTERFACE("886d8eeb-8cf2-4446-8d02-cdba1dbdcf99")
DECLARE_INTERFACE_(IPropertyStore, IUnknown)
{
    STDMETHOD (GetCount) (DWORD *cProps) PURE;
    STDMETHOD (GetAt) (DWORD iProp, PROPERTYKEY *pkey) PURE;
    STDMETHOD (GetValue) (REFPROPERTYKEY key, PROPVARIANT *pv) PURE;
    STDMETHOD (SetValue) (REFPROPERTYKEY key, REFPROPVARIANT propvar) PURE;
    STDMETHOD (Commit) (void) PURE;
};
typedef IPropertyStore *LPIPropertyStore;

//MIDL_INTERFACE("3c594f9f-9f30-47a1-979a-c9e83d3d0a06")
DECLARE_INTERFACE_(IApplicationDocumentLists, IUnknown)
{
    STDMETHOD (SetAppID) (LPCWSTR pszAppID) PURE;
    STDMETHOD (GetList) (APPDOCLISTTYPE listtype, UINT cItemsDesired, REFIID riid, void **ppv) PURE;
};
typedef IApplicationDocumentLists *LPIApplicationDocumentLists;

//MIDL_INTERFACE("12337d35-94c6-48a0-bce7-6a9c69d4d600")
DECLARE_INTERFACE_(IApplicationDestinations, IUnknown)
{
    STDMETHOD (SetAppID) (LPCWSTR pszAppID) PURE;
    STDMETHOD (RemoveDestination) (IUnknown *punk) PURE;
    STDMETHOD (RemoveAllDestinations) (void) PURE;
};
typedef IApplicationDestinations *LPIApplicationDestinations;
DEFINE_KNOWN_FOLDER(FOLDERID_Documents, 0xFDD39AD0, 0x238F, 0x46AF, 0xAD, 0xB4, 0x6C, 0x85, 0x48, 0x03, 0x69, 0xC7);

DEFINE_PROPERTYKEY(PKEY_Title, 0xF29F85E0, 0x4FF9, 0x1068, 0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9, 2);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_IsDestListSeparator, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 6);

DEFINE_GUID_(CLSID_DestinationList,0x77f10cf0,0x3db5,0x4966,0xb5,0x20,0xb7,0xc5,0x4f,0xd3,0x5e,0xd6);
DEFINE_GUID_(CLSID_TaskbarList,0x56fdf344,0xfd6d,0x11d0,0x95,0x8a,0x0,0x60,0x97,0xc9,0xa0,0x90);
DEFINE_GUID_(CLSID_EnumerableObjectCollection,0x2d3468c1,0x36a7,0x43b6,0xac,0x24,0xd3,0xf0,0x2f,0xd9,0x60,0x7a);
DEFINE_GUID_(CLSID_ShellItem,0x9ac9fbe1,0xe0a2,0x4ad6,0xb4,0xee,0xe2,0x12,0x01,0x3e,0xa9,0x17);
DEFINE_GUID_(CLSID_ApplicationDocumentLists,0x86bec222,0x30f2,0x47e0,0x9f,0x25,0x60,0xd1,0x1c,0xd7,0x5c,0x28);
DEFINE_GUID_(CLSID_ApplicationDestinations,0x86c14003,0x4d6b,0x4ef3,0xa7,0xb4,0x05,0x06,0x66,0x3b,0x2e,0x68);

DEFINE_GUID_(IID_ITaskbarList,0x56FDF342,0xFD6D,0x11d0,0x95,0x8A,0x00,0x60,0x97,0xC9,0xA0,0x90);
DEFINE_GUID_(IID_ITaskbarList2,0x602D4995,0xB13A,0x429b,0xA6,0x6E,0x19,0x35,0xE4,0x4F,0x43,0x17);
DEFINE_GUID_(IID_ITaskbarList3,0xea1afb91,0x9e28,0x4b86,0x90,0xE9,0x9e,0x9f,0x8a,0x5e,0xef,0xaf);
DEFINE_GUID_(IID_ITaskbarList4,0xc43dc798,0x95d1,0x4bea,0x90,0x30,0xbb,0x99,0xe2,0x98,0x3a,0x1a);
DEFINE_GUID_(IID_IObjectArray,0x92ca9dcd,0x5622,0x4bba,0xa8,0x05,0x5e,0x9f,0x54,0x1b,0xd8,0xc9);
DEFINE_GUID_(IID_ICustomDestinationList,0x6332debf,0x87b5,0x4670,0x90,0xc0,0x5e,0x57,0xb4,0x08,0xa4,0x9e);
DEFINE_GUID_(IID_IObjectCollection,0x5632b1a4,0xe38a,0x400a,0x92,0x8a,0xd4,0xcd,0x63,0x23,0x02,0x95);
DEFINE_GUID_(IID_IShellItem,0x43826d1e,0xe718,0x42ee,0xbc,0x55,0xa1,0xe2,0x61,0xc3,0x7b,0xfe);
DEFINE_GUID_(IID_IPropertyStore,0x886d8eeb,0x8cf2,0x4446,0x8d,0x02,0xcd,0xba,0x1d,0xbd,0xcf,0x99);
DEFINE_GUID_(IID_IApplicationDocumentLists,0x3c594f9f,0x9f30,0x47a1,0x97,0x9a,0xc9,0xe8,0x3d,0x3d,0x0a,0x06);
DEFINE_GUID_(IID_IApplicationDestinations,0x12337d35,0x94c6,0x48a0,0xbc,0xe7,0x6a,0x9c,0x69,0xd4,0xd6,0x00);

extern "C"
{
    typedef HRESULT (WINAPI *t_GetCurrentProcessExplicitAppUserModelID)(PWSTR* AppID);
    typedef HRESULT (WINAPI *t_SetCurrentProcessExplicitAppUserModelID)(PCWSTR AppID);
    typedef HRESULT (WINAPI *t_SHCreateItemInKnownFolder)(REFKNOWNFOLDERID kfid, DWORD dwKFFlags, PCWSTR pszItem, REFIID riid, void **ppv);
    typedef HRESULT (WINAPI *t_SHCreateItemFromParsingName)(PCWSTR pszPath, IBindCtx *pbc, REFIID riid, void **ppv);
}

WINOLEAPI PropVariantClear(PROPVARIANT* pvar);

inline HRESULT InitPropVariantFromString(PCWSTR psz, PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_LPWSTR;
    HRESULT hr = SHStrDupW(psz, &ppropvar->pwszVal);
    return hr;
}

inline HRESULT InitPropVariantFromBoolean(BOOL fVal, PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_BOOL;
    ppropvar->boolVal = fVal ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

#endif //Q_OS_WIN32

#endif // WIN7UTILS_H
