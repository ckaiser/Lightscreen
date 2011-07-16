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

#include <QtGlobal>

#ifdef Q_OS_WIN32

#include "Utils.h"
namespace QW7 {

#define DWM_BB_ENABLE                 0x00000001  // fEnable has been specified

    typedef struct _DWM_BLURBEHIND
    {
        DWORD dwFlags;
        BOOL fEnable;
        HRGN hRgnBlur;
        BOOL fTransitionOnMaximized;
    } DWM_BLURBEHIND, *PDWM_BLURBEHIND;

    typedef struct _MARGINS
    {
        int cxLeftWidth;      // width of left border that retains its size
        int cxRightWidth;     // width of right border that retains its size
        int cyTopHeight;      // height of top border that retains its size
        int cyBottomHeight;   // height of bottom border that retains its size
    } MARGINS, *PMARGINS;

    // Window attributes
    enum DWMWINDOWATTRIBUTE
    {
        DWMWA_NCRENDERING_ENABLED = 1,      // [get] Is non-client rendering enabled/disabled
        DWMWA_NCRENDERING_POLICY,           // [set] Non-client rendering policy
        DWMWA_TRANSITIONS_FORCEDISABLED,    // [set] Potentially enable/forcibly disable transitions
        DWMWA_ALLOW_NCPAINT,                // [set] Allow contents rendered in the non-client area to be visible on the DWM-drawn frame.
        DWMWA_CAPTION_BUTTON_BOUNDS,        // [get] Bounds of the caption button area in window-relative space.
        DWMWA_NONCLIENT_RTL_LAYOUT,         // [set] Is non-client content RTL mirrored
        DWMWA_FORCE_ICONIC_REPRESENTATION,  // [set] Force this window to display iconic thumbnails.
        DWMWA_FLIP3D_POLICY,                // [set] Designates how Flip3D will treat the window.
        DWMWA_EXTENDED_FRAME_BOUNDS,        // [get] Gets the extended frame bounds rectangle in screen space
        DWMWA_HAS_ICONIC_BITMAP,            // [set] Indicates an available bitmap when there is no better thumbnail representation.
        DWMWA_DISALLOW_PEEK,                // [set] Don't invoke Peek on the window.
        DWMWA_EXCLUDED_FROM_PEEK,           // [set] LivePreview exclusion information
        DWMWA_LAST
    };

    // Values designating how Flip3D treats a given window.
    enum DWMFLIP3DWINDOWPOLICY
    {
        DWMFLIP3D_DEFAULT,      // Hide or include the window in Flip3D based on window style and visibility.
        DWMFLIP3D_EXCLUDEBELOW, // Display the window under Flip3D and disabled.
        DWMFLIP3D_EXCLUDEABOVE, // Display the window above Flip3D and enabled.
        DWMFLIP3D_LAST
    };

    typedef enum _DWMNCRENDERINGPOLICY {
      DWMNCRP_USEWINDOWSTYLE,
      DWMNCRP_DISABLED,
      DWMNCRP_ENABLED,
      DWMNCRP_LAST
    } DWMNCRENDERINGPOLICY;

    extern "C"
    {
        typedef HRESULT (WINAPI *t_DwmSetIconicThumbnail)(HWND hwnd, HBITMAP hbmp, DWORD dwSITFlags);
        typedef HRESULT (WINAPI *t_DwmSetWindowAttribute)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
        typedef HRESULT (WINAPI *t_DwmSetIconicLivePreviewBitmap)(HWND hwnd, HBITMAP hbmp, POINT *pptClient, DWORD dwSITFlags);
        typedef HRESULT (WINAPI *t_DwmEnableBlurBehindWindow)(HWND hWnd, const DWM_BLURBEHIND* pBlurBehind);
        typedef HRESULT (WINAPI *t_DwmExtendFrameIntoClientArea)(HWND hwnd, const MARGINS *pMarInset);
        typedef HRESULT (WINAPI *t_DwmInvalidateIconicBitmaps)(HWND hwnd);
    }

    void DwmSetIconicThumbnail(HWND hwnd, HBITMAP hbmp, DWORD dwSITFlags) {
        HMODULE shell;

        shell = LoadLibrary(L"dwmapi.dll");
        if (shell) {
            t_DwmSetIconicThumbnail set_iconic_thumbnail = reinterpret_cast<t_DwmSetIconicThumbnail>(GetProcAddress (shell, "DwmSetIconicThumbnail"));
            set_iconic_thumbnail(hwnd, hbmp, dwSITFlags);

            FreeLibrary (shell);
        }
    }

    void DwmSetWindowAttribute(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute) {
        HMODULE shell;

        shell = LoadLibrary(L"dwmapi.dll");
        if (shell) {
            t_DwmSetWindowAttribute set_window_attribute = reinterpret_cast<t_DwmSetWindowAttribute>(GetProcAddress (shell, "DwmSetWindowAttribute"));
            set_window_attribute(hwnd, dwAttribute, pvAttribute, cbAttribute);

            FreeLibrary (shell);
        }
    }

    void DwmSetIconicLivePreviewBitmap(HWND hwnd, HBITMAP hbmp, POINT *pptClient, DWORD dwSITFlags) {
        HMODULE shell;

        shell = LoadLibrary(L"dwmapi.dll");
        if (shell) {
            t_DwmSetIconicLivePreviewBitmap set_live_preview = reinterpret_cast<t_DwmSetIconicLivePreviewBitmap>(GetProcAddress (shell, "DwmSetIconicLivePreviewBitmap"));
            set_live_preview(hwnd, hbmp, pptClient, dwSITFlags);

            FreeLibrary (shell);
        }
    }


    void DwmEnableBlurBehindWindow(HWND hwnd, const DWM_BLURBEHIND* pBlurBehind) {
        HMODULE shell;

        shell = LoadLibrary(L"dwmapi.dll");
        if (shell) {
            t_DwmEnableBlurBehindWindow set_window_blur = reinterpret_cast<t_DwmEnableBlurBehindWindow>(GetProcAddress (shell, "DwmEnableBlurBehindWindow"));
            set_window_blur(hwnd, pBlurBehind);

            FreeLibrary (shell);
        }
    }

    void DwmExtendFrameIntoClientArea(HWND hwnd, const MARGINS *pMarInset) {
        HMODULE shell;

        shell = LoadLibrary(L"dwmapi.dll");
        if (shell) {
            t_DwmExtendFrameIntoClientArea set_window_frame_into_client_area = reinterpret_cast<t_DwmExtendFrameIntoClientArea>(GetProcAddress (shell, "DwmExtendFrameIntoClientArea"));
            set_window_frame_into_client_area(hwnd, pMarInset);

            FreeLibrary (shell);
        }

    }

    void DwmInvalidateIconicBitmaps(HWND hwnd) {
        HMODULE shell;

        shell = LoadLibrary(L"dwmapi.dll");
        if (shell) {
            t_DwmInvalidateIconicBitmaps invalidate_icon_bitmap = reinterpret_cast<t_DwmInvalidateIconicBitmaps>(GetProcAddress (shell, "DwmInvalidateIconicBitmaps"));
            invalidate_icon_bitmap(hwnd);

            FreeLibrary (shell);
        }

    }

    void ExtendFrameIntoClientArea(QWidget* widget) {
        MARGINS margins = {-1};

        DwmExtendFrameIntoClientArea(widget->winId(), &margins);
    }

    long EnableBlurBehindWidget(QWidget* widget, bool enable)
    {
        HWND hwnd = widget->winId();
        HRESULT hr = S_OK;

        widget->setAttribute(Qt::WA_TranslucentBackground, enable);
        widget->setAttribute(Qt::WA_NoSystemBackground, enable);

        // Create and populate the Blur Behind structure
        DWM_BLURBEHIND bb = {0};

        bb.dwFlags = DWM_BB_ENABLE;
        bb.fEnable = enable;
        bb.hRgnBlur = NULL;

        DwmEnableBlurBehindWindow(hwnd, &bb);
        return hr;
    }

    void EnableWidgetIconicPreview(QWidget* widget, bool enable) {
        BOOL _enable = enable ? TRUE : FALSE;

        DwmSetWindowAttribute(
            widget->winId(),
            DWMWA_FORCE_ICONIC_REPRESENTATION,
            &_enable,
            sizeof(_enable));

        DwmSetWindowAttribute(
            widget->winId(),
            DWMWA_HAS_ICONIC_BITMAP,
            &_enable,
            sizeof(_enable));

    }

    void InvalidateIconicBitmaps(QWidget* widget) {
        DwmInvalidateIconicBitmaps(widget->winId());
    }

}

#endif //Q_OS_WIN32
