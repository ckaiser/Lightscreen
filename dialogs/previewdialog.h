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
#ifndef PREVIEWDIALOG_H
#define PREVIEWDIALOG_H

#include <QDialog>

class Screenshot;
class QStackedLayout;
class QPushButton;
class PreviewDialog : public QDialog
{
    Q_OBJECT

public:
    PreviewDialog(QWidget *parent);

    void add(Screenshot *screenshot);
    int count() const;

public slots:
    void setWidth(int w)  { resize(w, height()); }
    void setHeight(int h) { resize(width(), h);  }

signals:
    void acceptAll();
    void rejectAll();
    void uploadAll();

private slots:
    void closePreview();
    void enlargePreview();
    void indexChanged(int i);
    void next();
    void previous();
    void relocate();

protected:
    bool event(QEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void timerEvent(QTimerEvent *event);

private:
    int mAutoclose;
    int mAutocloseAction;
    int mAutocloseReset;
    int mPosition; //0: top left, 1: top right, 2: bottom left, 3: bottom rigth (default)
    int mSize;
    QPushButton    *mNextButton;
    QPushButton    *mPrevButton;
    QStackedLayout *mStack;
    QPoint mDragStartPosition;
};

#endif // PREVIEWDIALOG_H
