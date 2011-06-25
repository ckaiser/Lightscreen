/*
 * Copyright (C) 2011  Christian Kaiser
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

struct QPixmap;
struct QPoint;
struct QString;
struct QUrl;
class QGraphicsEffect;
typedef unsigned long XID;
typedef XID Window;

namespace os
{
  // Gives the target widget a full aero glass background
  bool aeroGlass(QWidget *target);
  // Adds the filename to the Windows recent document list (useful for Windows 7 jump lists)
  void addToRecentDocuments(QString fileName);
  // Returns the cursor pixmap in Windows
  QPixmap cursor();
  // Adds lightscreen to the startup list in Windows & Linux (KDE, Gnome and Xfce for now).
  void setStartup(bool startup, bool hide);
  // Set the target window as the foreground window (Windows only)
  void setForegroundWindow(QWidget *window);
  // Returns the current users's Documents/My Documents folder
  QString getDocumentsPath();
  // Returns the pixmap of the given window id.
  QPixmap grabWindow(WId winId);
  // Translates the ui to the given language name.
  void translate(QString language);
  // A QTimeLine based effect for a slot (TODO: look at the new effect classes)
  void effect(QObject* target, const char* slot, int frames, int duration = 400, const char* cleanup = 0);
  // Creates a new QGraphicsDropShadowEffect to apply to widgets.
  QGraphicsEffect* shadow(QColor color = Qt::black, int blurRadius = 6, int offset = 1);
  // X11-specific functions for the Window Picker
#if defined(Q_WS_X11)
  Window findRealWindow(Window w, int depth = 0);
  Window windowUnderCursor(bool includeDecorations = true);
#endif
}


#endif /*OS_H_*/
