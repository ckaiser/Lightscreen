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
#ifndef SCREENSHOTDIALOG_H
#define SCREENSHOTDIALOG_H

#include <QDialog>

struct QPoint;
class Screenshot;
class QScrollArea;
class QLabel;
class ScreenshotDialog : public QDialog
{
public:
    ScreenshotDialog(Screenshot *screenshot, QWidget *parent = 0);

private:
  void zoom(int offset);

protected:
  bool eventFilter(QObject *obj, QEvent *event);
  void closeEvent(QCloseEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);

private:
  QLabel *mLabel;
  QPoint mMousePos;
  QScrollArea *mScrollArea;
  QSize mOriginalSize;
};

#endif // SCREENSHOTDIALOG_H
