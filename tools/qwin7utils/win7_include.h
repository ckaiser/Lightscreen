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

DEFINE_PROPERTYKEY(PKEY_Title, 0xF29F85E0, 0x4FF9, 0x1068, 0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9, 2);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_IsDestListSeparator, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 6);

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
