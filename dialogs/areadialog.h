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
 ******
 *
 * Based on KDE's KSnapshot regiongrabber.cpp, revision 796531, Copyright 2007 Luca Gugelmann <lucag@student.ethz.ch>
 * released under the GNU LGPL  <http://www.gnu.org/licenses/old-licenses/library.txt>
 *
 */
#ifndef AREADIALOG_H
#define AREADIALOG_H

#include <QDialog>
#include <QVector>
#include <QPointer>
#include <QTimer>

struct QRegion;
struct QPoint;
struct QRect;
class QPaintEvent;
class QResizeEvent;
class QMouseEvent;
class Screenshot;

class AreaDialog : public QDialog
{
    Q_OBJECT

public:
    AreaDialog(Screenshot* screenshot);
    ~AreaDialog();
    QRect resultRect();

protected slots:
    void displayHelp();
    void grabRect();
    void cancel();
    void animationTick(int frame);

signals:
    void regionGrabbed( const QPixmap & );

protected:
    void paintEvent( QPaintEvent* e );
    void resizeEvent( QResizeEvent* e );
    void mousePressEvent( QMouseEvent* e );
    void mouseMoveEvent( QMouseEvent* e );
    void mouseReleaseEvent( QMouseEvent* e );
    void mouseDoubleClickEvent( QMouseEvent* );
    void keyPressEvent( QKeyEvent* e );
    void showEvent( QShowEvent* e );

    void updateHandles();

    QRegion handleMask() const;
    QPoint limitPointToRect( const QPoint &p, const QRect &r ) const;

    Screenshot *mScreenshot;
    QPoint mMousePos;
    QRect  mSelection;
    bool   mMouseDown;
    bool   mMouseMagnifier;
    bool   mNewSelection;
    const int mHandleSize;
    QRect *mMouseOverHandle;
    QPoint mDragStartPoint;
    QRect  mSelectionBeforeDrag;
    QTimer mIdleTimer;
    bool mShowHelp;
    bool mGrabbing;
    int  mOverlayAlpha;
    bool mAutoclose;


    // naming convention for handles
    // T top, B bottom, R Right, L left
    // 2 letters: a corner
    // 1 letter: the handle on the middle of the corresponding side
    QRect mTLHandle, mTRHandle, mBLHandle, mBRHandle;
    QRect mLHandle, mTHandle, mRHandle, mBHandle;

    QVector<QRect*> mHandles;
    QPointer<QWidget> mAcceptWidget;
};

#endif
