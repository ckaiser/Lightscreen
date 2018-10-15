/*
 * Copyright (C) 2017  Christian Kaiser
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <QApplication>
#include <QBitmap>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDialog>
#include <QDir>
#include <QGraphicsDropShadowEffect>
#include <QIcon>
#include <QLibrary>
#include <QLocale>
#include <QMessageBox>
#include <QPixmap>
#include <QPointer>
#include <QProcess>
#include <QSettings>
#include <QTextEdit>
#include <QTimeLine>
#include <QTimer>
#include <QUrl>
#include <QWidget>
#include <string>
#include <QMessageBox>
#include <QPainter>
#include <QPair>

#ifdef Q_OS_WIN
    #include <QtWin>
    #include <qt_windows.h>
    #include <ShlObj.h>

    // Define for MinGW
    #ifndef SM_CXPADDEDBORDER
        #define SM_CXPADDEDBORDER 92
    #endif
#elif defined(Q_OS_LINUX)
    #include <QX11Info>
    #include <X11/X.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/Xatom.h>
#endif

#include <tools/os.h>

QPair<QPixmap, QPoint> os::cursor()
{
#ifdef Q_OS_WIN
    /*
    * Taken from: git://github.com/arrai/mumble-record.git > src > mumble > Overlay.cpp
    * BSD License.
    */

    QPixmap pixmap;
    QPoint hotspot;

    CURSORINFO cursorInfo;
    cursorInfo.cbSize = sizeof(cursorInfo);
    ::GetCursorInfo(&cursorInfo);

    HICON cursor = cursorInfo.hCursor;

    ICONINFO iconInfo;
    ::GetIconInfo(cursor, &iconInfo);

    ICONINFO info;
    ZeroMemory(&info, sizeof(info));

    if (::GetIconInfo(cursor, &info)) {
        if (info.hbmColor) {
            pixmap = QtWin::fromHBITMAP(info.hbmColor, QtWin::HBitmapAlpha);
        } else {
            QBitmap orig(QtWin::fromHBITMAP(info.hbmMask));
            QImage img = orig.toImage();

            int h = img.height() / 2;
            int w = static_cast<uint>(img.bytesPerLine()) / sizeof(quint32);

            QImage out(img.width(), h, QImage::Format_MonoLSB);
            QImage outmask(img.width(), h, QImage::Format_MonoLSB);

            for (int i = 0; i < h; ++i) {
                const quint32 *srcimg = reinterpret_cast<const quint32 *>(img.scanLine(i + h));
                const quint32 *srcmask = reinterpret_cast<const quint32 *>(img.scanLine(i));

                quint32 *dstimg = reinterpret_cast<quint32 *>(out.scanLine(i));
                quint32 *dstmask = reinterpret_cast<quint32 *>(outmask.scanLine(i));

                for (int j = 0; j < w; ++j) {
                    dstmask[j] = srcmask[j];
                    dstimg[j] = srcimg[j];
                }
            }

            pixmap = QBitmap::fromImage(out, Qt::ColorOnly);
        }

        hotspot.setX(static_cast<int>(info.xHotspot));
        hotspot.setY(static_cast<int>(info.yHotspot));

        if (info.hbmMask) {
            ::DeleteObject(info.hbmMask);
        }

        if (info.hbmColor) {
            ::DeleteObject(info.hbmColor);
        }

        ::DeleteObject(cursor);
    }

    return QPair<QPixmap, QPoint>(pixmap, hotspot);
#else
    return QPair<QPixmap, QPoint>(QPixmap(), QPoint());
#endif
}

void os::effect(QObject *target, const char *slot, int frames, int duration, const char *cleanup)
{
    QTimeLine *timeLine = new QTimeLine(duration);
    timeLine->setFrameRange(0, frames);

    timeLine->connect(timeLine, SIGNAL(frameChanged(int)), target, slot);

    if (cleanup != 0) {
        timeLine->connect(timeLine, SIGNAL(finished()), target, SLOT(cleanup()));
    }

    timeLine->connect(timeLine, SIGNAL(finished()), timeLine, SLOT(deleteLater()));


    timeLine->start();
}

QString os::getDocumentsPath()
{
#ifdef Q_OS_WIN
    TCHAR szPath[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPath(NULL,
                                  CSIDL_PERSONAL | CSIDL_FLAG_CREATE,
                                  NULL,
                                  0,
                                  szPath))) {
        return QString::fromWCharArray(szPath);
    }

    return QDir::homePath() + QDir::separator() + "My Documents";
#else
    return QDir::homePath() + QDir::separator() + "Documents";
#endif
}

QPixmap os::grabWindow(WId winId)
{
#ifdef Q_OS_WIN
    RECT rcWindow;

    HWND hwndId = (HWND)winId;

    GetWindowRect(hwndId, &rcWindow);

    int margin = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER) / 2;

    rcWindow.right -= margin;
    rcWindow.left += margin;

    if (IsZoomed(hwndId)) {
        rcWindow.top += margin;
    } else {
        rcWindow.top += GetSystemMetrics(SM_CXPADDEDBORDER);
    }

    rcWindow.bottom -= margin;

    int width, height;
    width = rcWindow.right - rcWindow.left;
    height = rcWindow.bottom - rcWindow.top;

    RECT rcScreen;
    GetWindowRect(GetDesktopWindow(), &rcScreen);

    RECT rcResult;
    UnionRect(&rcResult, &rcWindow, &rcScreen);

    QPixmap pixmap;

    // Comparing the rects to determine if the window is outside the boundaries of the screen,
    // the window DC method has the disadvantage that it does not show Aero glass transparency,
    // so we'll avoid it for the screenshots that don't need it.

    HDC hdcMem;
    HBITMAP hbmCapture;

    if (EqualRect(&rcScreen, &rcResult)) {
        // Grabbing the window from the Screen DC.
        HDC hdcScreen = GetDC(NULL);

        BringWindowToTop(hwndId);

        hdcMem = CreateCompatibleDC(hdcScreen);
        hbmCapture = CreateCompatibleBitmap(hdcScreen, width, height);
        SelectObject(hdcMem, hbmCapture);

        BitBlt(hdcMem, 0, 0, width, height, hdcScreen, rcWindow.left, rcWindow.top, SRCCOPY);
    } else {
        // Grabbing the window by its own DC
        HDC hdcWindow = GetWindowDC(hwndId);

        hdcMem = CreateCompatibleDC(hdcWindow);
        hbmCapture = CreateCompatibleBitmap(hdcWindow, width, height);
        SelectObject(hdcMem, hbmCapture);

        BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);
    }

    ReleaseDC(hwndId, hdcMem);
    DeleteDC(hdcMem);

    pixmap = QtWin::fromHBITMAP(hbmCapture);

    DeleteObject(hbmCapture);

    return pixmap;
#else
    return QPixmap::grabWindow(winId);
#endif
}

void os::setForegroundWindow(QWidget *window)
{
#ifdef Q_OS_WIN
    ShowWindow((HWND)window->winId(), SW_RESTORE);
    SetForegroundWindow((HWND)window->winId());
#else
    Q_UNUSED(window)
#endif
}

void os::setStartup(bool startup, bool hide)
{
    QString lightscreen = QDir::toNativeSeparators(qApp->applicationFilePath());

    if (hide) {
        lightscreen.append(" -h");
    }

#ifdef Q_OS_WIN
    // Windows startup settings
    QSettings init("Microsoft", "Windows");
    init.beginGroup("CurrentVersion");
    init.beginGroup("Run");

    if (startup) {
        init.setValue("Lightscreen", lightscreen);
    } else {
        init.remove("Lightscreen");
    }

    init.endGroup();
    init.endGroup();
#endif

#if defined(Q_OS_LINUX)
    QFile desktopFile(QDir::homePath() + "/.config/autostart/lightscreen.desktop");

    desktopFile.remove();

    if (startup) {
        desktopFile.open(QIODevice::WriteOnly);
        desktopFile.write(QString("[Desktop Entry]\nExec=%1\nType=Application").arg(lightscreen).toLatin1());
    }
#endif
}

QGraphicsEffect *os::shadow(const QColor &color, int blurRadius, int offset)
{
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect;
    shadowEffect->setBlurRadius(blurRadius);
    shadowEffect->setOffset(offset);
    shadowEffect->setColor(color);

    return shadowEffect;
}

QIcon os::icon(const QString &name, QColor backgroundColor)
{
    if (!backgroundColor.isValid()) {
        backgroundColor = qApp->palette().color(QPalette::Button);
    }

    if (backgroundColor.value() > 125) {
        return QIcon(":/icons/" + name);
    } else {
        return QIcon(":/icons/inv/" + name);
    }
}

#ifdef Q_OS_LINUX
// Taken from KSnapshot. Oh KDE, what would I do without you :D
Window os::findRealWindow(Window w, int depth)
{
    if (depth > 5) {
        return None;
    }

    static Atom wm_state = XInternAtom(QX11Info::display(), "WM_STATE", False);
    Atom type;
    int format;
    unsigned long nitems, after;
    unsigned char *prop;

    if (XGetWindowProperty(QX11Info::display(), w, wm_state, 0, 0, False, AnyPropertyType,
                           &type, &format, &nitems, &after, &prop) == Success) {
        if (prop != NULL) {
            XFree(prop);
        }

        if (type != None) {
            return w;
        }
    }

    Window root, parent;
    Window *children;
    unsigned int nchildren;
    Window ret = None;

    if (XQueryTree(QX11Info::display(), w, &root, &parent, &children, &nchildren) != 0) {
        for (unsigned int i = 0;
                i < nchildren && ret == None;
                ++i) {
            ret = os::findRealWindow(children[ i ], depth + 1);
        }

        if (children != NULL) {
            XFree(children);
        }
    }

    return ret;
}

Window os::windowUnderCursor(bool includeDecorations)
{
    Window root;
    Window child;
    uint mask;
    int rootX, rootY, winX, winY;

    XQueryPointer(QX11Info::display(), QX11Info::appRootWindow(), &root, &child,
                  &rootX, &rootY, &winX, &winY, &mask);

    if (child == None) {
        child = QX11Info::appRootWindow();
    }

    if (!includeDecorations) {
        Window real_child = os::findRealWindow(child);

        if (real_child != None) {  // test just in case
            child = real_child;
        }
    }

    return child;
}
#endif
