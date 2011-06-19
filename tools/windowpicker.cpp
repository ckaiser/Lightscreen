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
#include <QApplication>
#include <QWidget>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QRubberBand>
#include <QPushButton>
#include <QRubberBand>

#include "windowpicker.h"
#include "os.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif


WindowPicker::WindowPicker() : QWidget(0), mCrosshair(":/icons/picker"), mWindowLabel(0), mTaken(false)
{
  setWindowFlags(Qt::SplashScreen | Qt::WindowStaysOnTopHint);
  setWindowTitle(tr("Lightscreen Window Picker"));
  setStyleSheet("QWidget { color: #000; } #frame { padding: 7px 10px; border: 1px solid #898c95; background-color: qlineargradient(spread:pad, x1:1, y1:1, x2:0.988636, y2:0.608, stop:0 rgba(235, 235, 235, 255), stop:1 rgba(255, 255, 255, 255)); border-radius: 8px; }");

  QLabel *helpLabel = new QLabel(tr("Grab the window picker by clicking and holding down the mouse button, then drag it to the window of your choice and release it to capture."), this);
  helpLabel->setMinimumWidth(400);
  helpLabel->setMaximumWidth(400);
  helpLabel->setWordWrap(true);

  mWindowIndicator = new QRubberBand(QRubberBand::Rectangle, 0);
  mWindowIndicator->setWindowFlags(mWindowIndicator->windowFlags() & Qt::WindowStaysOnTopHint);
  mWindowIndicator->setGeometry(0, 0, 0, 0);
  mWindowIndicator->show();

  mWindowIcon = new QLabel(this);
  mWindowIcon->setMinimumSize(22, 22);
  mWindowIcon->setMaximumSize(22, 22);
  mWindowIcon->setScaledContents(true);

  mWindowLabel = new QLabel(tr(" - Start dragging to select windows"), this);
  mWindowLabel->setStyleSheet("font-weight: bold");

  mCrosshairLabel = new QLabel(this);
  mCrosshairLabel->setAlignment(Qt::AlignHCenter);
  mCrosshairLabel->setPixmap(mCrosshair);

  QPushButton *closeButton = new QPushButton(tr("Close"));
  connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

  QHBoxLayout *windowLayout = new QHBoxLayout;
  windowLayout->addWidget(mWindowIcon);
  windowLayout->addWidget(mWindowLabel);
  windowLayout->setMargin(0);

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  buttonLayout->addStretch(0);
  buttonLayout->addWidget(closeButton);
  buttonLayout->setMargin(0);

  QHBoxLayout *crosshairLayout = new QHBoxLayout;
  crosshairLayout->addStretch(0);
  crosshairLayout->addWidget(mCrosshairLabel);
  crosshairLayout->addStretch(0);
  crosshairLayout->setMargin(0);

  QVBoxLayout *fl = new QVBoxLayout;
  fl->addWidget(helpLabel);
  fl->addLayout(windowLayout);
  fl->addLayout(crosshairLayout);
  fl->addLayout(buttonLayout);
  fl->setMargin(0);

  QFrame *frame = new QFrame(this);
  frame->setObjectName("frame");
  frame->setLayout(fl);

  QVBoxLayout *l = new QVBoxLayout;
  l->setMargin(0);
  l->addWidget(frame);

  setLayout(l);

  resize(sizeHint());
  move(QApplication::desktop()->screenGeometry(QApplication::desktop()->screenNumber(QCursor::pos())).center()-QPoint(width()/2, height()/2));
  show();
}

WindowPicker::~WindowPicker() {
  qApp->restoreOverrideCursor();
  delete mWindowIndicator;
}

void WindowPicker::cancel() {
  mWindowIcon->setPixmap(QPixmap());
  mCrosshairLabel->setPixmap(mCrosshair);
  mWindowIndicator->setGeometry(0, 0, 0, 0);
  qApp->restoreOverrideCursor();
}

void WindowPicker::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton) {
     POINT mousePos;
     mousePos.x = event->globalX();
     mousePos.y = event->globalY();

     HWND window = GetAncestor(WindowFromPoint(mousePos), GA_ROOT);

     if (window == winId()) {
       cancel();
       return;
     }

     mWindowIndicator->hide();
     mTaken = true;

     setWindowFlags(windowFlags() ^ Qt::WindowStaysOnTopHint);
     close();

     emit pixmap(os::grabWindow(window));

     return;
  }

  mWindowIndicator->setGeometry(0, 0, 0, 0);

  close();
}

void WindowPicker::mousePressEvent(QMouseEvent *event)
{
    qApp->setOverrideCursor(QCursor(mCrosshair));
    mCrosshairLabel->setMinimumWidth(mCrosshairLabel->width());
    mCrosshairLabel->setMinimumHeight(mCrosshairLabel->height());
    mCrosshairLabel->setPixmap(QPixmap());
    QWidget::mousePressEvent(event);
}

void WindowPicker::mouseMoveEvent(QMouseEvent *event)
{
  POINT mousePos;
  mousePos.x = event->globalX();
  mousePos.y = event->globalY();

  HWND cWindow = GetAncestor(WindowFromPoint(mousePos), GA_ROOT);

  if (cWindow == mWindowIndicator->winId()) {
    return;
  }

  if (mCurrentWindow != cWindow) {
    SetForegroundWindow(cWindow);
    RECT rc;
    GetWindowRect(cWindow, &rc);
    mWindowIndicator->setGeometry(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  }

  mCurrentWindow = cWindow;

  if (mCurrentWindow == winId()) {
    mWindowIcon->setPixmap(QPixmap());
    mWindowLabel->setText("");
    return;
  }

  // Text
  WCHAR str[60];
  HICON icon;

  ::GetWindowText(mCurrentWindow, str, 60);
  ///

  // Retrieving the application icon
  icon = (HICON)::GetClassLong(mCurrentWindow, GCL_HICON);

  if (icon != NULL) {
    mWindowIcon->setPixmap(QPixmap::fromWinHICON(icon));
  }
  else {
    mWindowIcon->setPixmap(QPixmap());
  }

  QString windowText = QString(" - %1").arg(QString::fromWCharArray(str));

  if (windowText == " - ") {
    mWindowLabel->setText("");
    return;
  }

  if (windowText.length() == 62) {
    mWindowLabel->setText(windowText + "...");
  }
  else {
    mWindowLabel->setText(windowText);
  }
}

void WindowPicker::closeEvent(QCloseEvent*)
{
  if (!mTaken)
    emit pixmap(QPixmap());

  qApp->restoreOverrideCursor();
  deleteLater();
}
