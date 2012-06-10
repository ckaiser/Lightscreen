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
#include "previewdialog.h"
#include "screenshotdialog.h"
#include "../tools/screenshot.h"
#include "../tools/screenshotmanager.h"
#include "../tools/os.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QObject>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QStackedLayout>
#include <QToolButton>

#include <QDebug>

PreviewDialog::PreviewDialog(QWidget *parent) :
    QDialog(parent), mAutoclose(0)
{
  setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
  os::aeroGlass(this);
  setWindowTitle(tr("Screenshot Preview"));

  QSettings *settings = ScreenshotManager::instance()->settings();

  mSize      = settings->value("options/previewSize", 300).toInt();
  mPosition  = settings->value("options/previewPosition", 3).toInt();

  if (settings->value("options/previewAutoclose", false).toBool()) {
    mAutoclose = settings->value("options/previewAutocloseTime").toInt();
    mAutocloseReset = mAutoclose;
    mAutocloseAction = settings->value("options/previewAutocloseAction").toInt();
  }

  QHBoxLayout *l = new QHBoxLayout;
  mStack = new QStackedLayout;
  connect(mStack, SIGNAL(currentChanged(int)), this, SLOT(indexChanged(int)));

  mPrevButton = new QPushButton(QIcon(":/icons/arrow-left"), "", this);
  connect(mPrevButton, SIGNAL(clicked()), this, SLOT(previous()));

  mNextButton = new QPushButton(QIcon(":/icons/arrow-right"), "", this);
  connect(mNextButton, SIGNAL(clicked()), this, SLOT(next()));

  mPrevButton->setCursor(Qt::PointingHandCursor);
  mPrevButton->setFlat(true);
  mPrevButton->setGraphicsEffect(os::shadow());
  mPrevButton->setIconSize(QSize(24, 24));
  mPrevButton->setVisible(false);

  mNextButton->setCursor(Qt::PointingHandCursor);
  mNextButton->setFlat(true);
  mNextButton->setGraphicsEffect(os::shadow());
  mNextButton->setIconSize(QSize(24, 24));
  mNextButton->setVisible(false);

  l->addWidget(mPrevButton);
  l->addLayout(mStack);
  l->addWidget(mNextButton);

  l->setMargin(0);
  l->setContentsMargins(6, 6, 6, 6);

  mStack->setMargin(0);

  setMaximumHeight(mSize);
  setLayout(l);

  if (mAutoclose) {
    startTimer(1000);
  }
}

void PreviewDialog::add(Screenshot *screenshot)
{
  if (!isVisible()) {
    show();
  }

  if (mAutoclose) {
    mAutoclose = mAutocloseReset;
  }

  QLabel *label = new QLabel(this);
  label->setGraphicsEffect(os::shadow());

  bool small = false;

  QSize size = screenshot->pixmap().size();

  if (size.width() > mSize || size.height() > mSize) {
    size.scale(mSize, mSize, Qt::KeepAspectRatio);
  }
  else {
    small = true;
  }

  QPixmap thumbnail = screenshot->pixmap().scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  label->setPixmap(thumbnail);

  thumbnail = QPixmap();

  label->setAlignment(Qt::AlignCenter);

  if (size.height() < 120) {
    label->setMinimumHeight(120);
  }

  if (size.width() < 140) {
    label->setMinimumWidth(140);
  }

  label->resize(size);

  QPushButton *discardPushButton = new QPushButton(QIcon(":/icons/no")   , "", label);
  QPushButton *enlargePushButton = new QPushButton(QIcon(":/icons/zoom.in"), "", label);
  QToolButton *confirmPushButton = new QToolButton(label);

  confirmPushButton->setIconSize(QSize(24, 24));
  confirmPushButton->setCursor(Qt::PointingHandCursor);
  confirmPushButton->setGraphicsEffect(os::shadow());

  if (ScreenshotManager::instance()->settings()->value("options/previewDefaultAction", 0).toInt() == 0
      || ScreenshotManager::instance()->settings()->value("options/uploadAuto").toBool()) {
    // Default button, confirm & upload.
    confirmPushButton->setIcon(QIcon(":/icons/yes"));

    if (!ScreenshotManager::instance()->settings()->value("options/uploadAuto").toBool()) {
      QMenu *confirmMenu = new QMenu(confirmPushButton);
      confirmMenu->setObjectName("confirmMenu");

      QAction *uploadAction = new QAction(QIcon(":/icons/imgur.yes"), tr("Upload"), confirmPushButton);
      connect(uploadAction, SIGNAL(triggered()), screenshot,   SLOT(markUpload()));
      connect(uploadAction, SIGNAL(triggered()), screenshot,   SLOT(confirm()));
      connect(uploadAction, SIGNAL(triggered()), this,         SLOT(closePreview()));
      connect(this,         SIGNAL(uploadAll()), uploadAction, SLOT(trigger()));

      confirmMenu->addAction(uploadAction);
      confirmPushButton->setMenu(confirmMenu);
      confirmPushButton->setPopupMode(QToolButton::MenuButtonPopup);
    }

    connect(this, SIGNAL(acceptAll()), confirmPushButton, SLOT(click()));
    connect(confirmPushButton, SIGNAL(clicked()), screenshot, SLOT(confirm()));
    connect(confirmPushButton, SIGNAL(clicked()), this, SLOT(closePreview()));
  }
  else {
    // Reversed button, upload & confirm.
    confirmPushButton->setIcon(QIcon(":/icons/imgur.yes"));

    QMenu *confirmMenu = new QMenu(confirmPushButton);
    confirmMenu->setObjectName("confirmMenu");

    QAction *confirmAction = new QAction(QIcon(":/icons/yes"), tr("Save"), confirmPushButton);
    connect(this, SIGNAL(acceptAll()), confirmAction, SLOT(trigger()));
    connect(confirmAction, SIGNAL(triggered()), screenshot, SLOT(confirm()));
    connect(confirmAction, SIGNAL(triggered()), this, SLOT(closePreview()));

    connect(confirmPushButton, SIGNAL(clicked()), screenshot,   SLOT(markUpload()));
    connect(confirmPushButton, SIGNAL(clicked()), screenshot,   SLOT(confirm()));
    connect(confirmPushButton, SIGNAL(clicked()), this,         SLOT(closePreview()));
    connect(this,         SIGNAL(uploadAll()), confirmPushButton, SLOT(click()));

    confirmMenu->addAction(confirmAction);
    confirmPushButton->setMenu(confirmMenu);
    confirmPushButton->setPopupMode(QToolButton::MenuButtonPopup);
  }

  confirmPushButton->setAutoRaise(true);
  confirmPushButton->setVisible(false);

  discardPushButton->setIconSize(QSize(24, 24));
  discardPushButton->setCursor(Qt::PointingHandCursor);
  discardPushButton->setGraphicsEffect(os::shadow());
  discardPushButton->setFlat(true);
  discardPushButton->setVisible(false);

  enlargePushButton->setIconSize(QSize(22, 22));
  enlargePushButton->setCursor(Qt::PointingHandCursor);
  enlargePushButton->setGraphicsEffect(os::shadow());
  enlargePushButton->setFlat(true);
  enlargePushButton->setVisible(false);

  enlargePushButton->setDisabled(small);

  connect(this, SIGNAL(rejectAll()), discardPushButton, SLOT(click()));

  connect(discardPushButton, SIGNAL(clicked()), screenshot, SLOT(discard()));
  connect(discardPushButton, SIGNAL(clicked()), this, SLOT(closePreview()));

  connect(enlargePushButton, SIGNAL(clicked()), this, SLOT(enlargePreview()));

  QHBoxLayout *wlayout = new QHBoxLayout;
  wlayout->addWidget(confirmPushButton);
  wlayout->addStretch();
  wlayout->addWidget(enlargePushButton);
  wlayout->addStretch();
  wlayout->addWidget(discardPushButton);
  wlayout->setMargin(0);

  QVBoxLayout *wl = new QVBoxLayout;
  wl->addStretch();
  wl->addLayout(wlayout);
  wl->setMargin(0);

  label->setLayout(wl);

  mStack->addWidget(label);
  mStack->setCurrentIndex(mStack->count()-1);

  mNextButton->setEnabled(false);

  if (mStack->count() >= 2 && !mNextButton->isVisible()) {
    mNextButton->setVisible(true);
    mPrevButton->setVisible(true);
  }

  relocate();
}

int PreviewDialog::count() const
{
  return mStack->count();
}

//

void PreviewDialog::closePreview()
{
  QWidget *widget = mStack->currentWidget();
  mStack->removeWidget(widget);
  widget->deleteLater();

  if (mStack->count() == 0) {
    close();
  }
  else {
    relocate();
  }
}

void PreviewDialog::enlargePreview()
{
  Screenshot *screenshot = qobject_cast<Screenshot*>(ScreenshotManager::instance()->children().at(mStack->currentIndex()));

  if (screenshot) {
    new ScreenshotDialog(screenshot, this);
  }
}

void PreviewDialog::indexChanged(int i)
{
  if (i == mStack->count()-1) {
    mNextButton->setEnabled(false);
    mPrevButton->setEnabled(true);
  }

  if (i == 0 && mStack->count() > 1) {
    mNextButton->setEnabled(true);
    mPrevButton->setEnabled(false);
  }

  if (i != 0 && i != mStack->count()-1) {
    mNextButton->setEnabled(true);
    mPrevButton->setEnabled(true);
  }

  if (mStack->count() < 2) {
    mNextButton->setEnabled(false);
    mPrevButton->setEnabled(false);
  }

  if (mStack->widget(i)) {
    mStack->widget(i)->setFocus();
  }

  if (mStack->count() > 1) {
    setWindowTitle(tr("Screenshot Preview (%1 of %2)").arg(mStack->currentIndex()+1).arg(mStack->count()));
  }
  else {
    setWindowTitle(tr("Screenshot Preview"));
  }
}

void PreviewDialog::next()
{
  mStack->setCurrentIndex(mStack->currentIndex()+1);
  relocate();
}

void PreviewDialog::previous()
{
  mStack->setCurrentIndex(mStack->currentIndex()-1);
  relocate();
}


void PreviewDialog::relocate()
{
  updateGeometry();
  resize(minimumSizeHint());
  QApplication::sendEvent(this, new QEvent(QEvent::Enter)); // Ensures the buttons are visible.

  QPoint where;
  switch (mPosition)
  {
  case 0:
    where = QApplication::desktop()->availableGeometry(this).topLeft();
    break;
  case 1:
    where = QApplication::desktop()->availableGeometry(this).topRight();
    where.setX(where.x() - frameGeometry().width());
    break;
  case 2:
    where = QApplication::desktop()->availableGeometry(this).bottomLeft();
    where.setY(where.y() - frameGeometry().height());
    break;
  case 3:
  default:
    where = QApplication::desktop()->availableGeometry(this).bottomRight();
    where.setX(where.x() - frameGeometry().width());
    where.setY(where.y() - frameGeometry().height());
    break;
  }

  move(where);
}

//

bool PreviewDialog::event(QEvent *event)
{
  if ((event->type() == QEvent::Enter || event->type() == QEvent::Leave)
      && mStack->currentWidget())
  {
    foreach (QObject *child, mStack->currentWidget()->children()) {
      QWidget *widget = qobject_cast<QWidget*>(child);

      if (widget) {
        // Lets avoid disappearing buttons and bail if the menu is open.
        QMenu *confirmMenu = widget->findChild<QMenu*>("confirmMenu");
        if (confirmMenu && confirmMenu->isVisible())
          return false;

        widget->setVisible((event->type() == QEvent::Enter));
      }
    }
  }
  else if (event->type() == QEvent::Close) {
    deleteLater();
  }
  else if (event->type() == QEvent::MouseButtonDblClick) {
    enlargePreview();
  }

  return QDialog::event(event);
}

void PreviewDialog::timerEvent(QTimerEvent *event)
{
  if (mAutoclose == 0) {
    if (mAutocloseAction == 0) {
      emit acceptAll();
    }
    else if (mAutocloseAction == 1) {
      emit uploadAll();
    }
    else {
      emit rejectAll();
    }
  }
  else if (mAutoclose < 0) {
    killTimer(event->timerId());
  }
  else {
    setWindowTitle(tr("Preview: Closing in %1").arg(mAutoclose));
    mAutoclose--;
  }
}
