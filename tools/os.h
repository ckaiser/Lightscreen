
#ifndef OS_H_
#define OS_H_

#include <QWidget>

struct QPixmap;
struct QPoint;
struct QString;
struct QUrl;
class QGraphicsEffect;

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
}


#endif /*OS_H_*/  // Drop shadow around the preview, mind the margin.
