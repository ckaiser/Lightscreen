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
#ifndef OS_H_
#define OS_H_

#include <QWidget>

class QPixmap;
class QPoint;
class QString;
class QUrl;
class QGraphicsEffect;
class QIcon;

#if defined(Q_OS_LINUX)
    typedef unsigned long XID;
    typedef XID Window;
#endif

namespace os {
// Returns the cursor pixmap in Windows
QPair<QPixmap, QPoint> cursor();

// A QTimeLine based effect for a slot (TODO: look at the new effect classes)
void effect(QObject *target, const char *slot, int frames, int duration = 400, const char *cleanup = 0);

// Returns the current users's Documents/My Documents folder
QString getDocumentsPath();

// Returns the pixmap of the given window id.
QPixmap grabWindow(WId winId);

// Set the target window as the foreground window (Windows only)
void setForegroundWindow(QWidget *window);

// Adds lightscreen to the startup list in Windows & Linux (KDE, Gnome and Xfce for now).
void setStartup(bool startup, bool hide);

// Creates a new QGraphicsDropShadowEffect to apply to widgets.
QGraphicsEffect *shadow(const QColor &color = Qt::black, int blurRadius = 6, int offset = 1);

// Returns a QIcon for the given icon name (taking into account color schemes and whatnot).
QIcon icon(const QString &name, QColor backgroundColor = QColor());

// X11-specific functions for the Window Picker
#if defined(Q_OS_LINUX)
    Window findRealWindow(Window w, int depth = 0);
    Window windowUnderCursor(bool includeDecorations = true);
#endif
}

// qAsConst backport
#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
    namespace QtPrivate {
    template <typename T> struct QAddConst { typedef const T Type; };
    }

    // this adds const to non-const objects (like std::as_const)
    template <typename T>
    Q_DECL_CONSTEXPR typename QtPrivate::QAddConst<T>::Type &qAsConst(T &t) Q_DECL_NOTHROW { return t; }
    // prevent rvalue arguments:
    template <typename T>
    void qAsConst(const T &&) Q_DECL_EQ_DELETE;
#endif

#endif /*OS_H_*/
