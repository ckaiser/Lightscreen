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
#include <QObject>
#include <QList>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QPalette>
#include <QDesktopWidget>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QStackedLayout>
#include <QSettings>

#include <QDebug>

PreviewDialog::PreviewDialog(QWidget *parent) :
    QDialog(parent), mAutoclose(0)
{
  setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
  os::aeroGlass(this);
  setWindowTitle("Screenshot Preview");

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

  QLabel *widget = new QLabel(this);
  widget->setGraphicsEffect(os::shadow());

  bool small = false;

  connect(widget, SIGNAL(destroyed()), screenshot, SLOT(discard()));

  QSize size = screenshot->pixmap().size();

  if (size.width() > mSize || size.height() > mSize) {
    size.scale(mSize, mSize, Qt::KeepAspectRatio);
  }
  else {
    small = true;
  }

  QPixmap thumbnail = screenshot->pixmap().scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

  widget->setPixmap(thumbnail);

  thumbnail = QPixmap();

  widget->setAlignment(Qt::AlignCenter);

  if (size.height() < 80) {
    widget->setMinimumHeight(80);
  }

  if (size.width() < 80) {
    widget->setMinimumWidth(80);
  }

  widget->resize(size);

  QPushButton *confirmPushButton = new QPushButton(QIcon(":/icons/yes")  , "", widget);
  QPushButton *discardPushButton = new QPushButton(QIcon(":/icons/no")   , "", widget);
  QPushButton *enlargePushButton = new QPushButton(QIcon(":/icons/zoom.in"), "", widget);

  confirmPushButton->setFlat(true);
  confirmPushButton->setIconSize(QSize(24, 24));
  confirmPushButton->setCursor(Qt::PointingHandCursor);
  confirmPushButton->setGraphicsEffect(os::shadow());
  confirmPushButton->setDefault(true);

  discardPushButton->setFlat(true);
  discardPushButton->setIconSize(QSize(24, 24));
  discardPushButton->setCursor(Qt::PointingHandCursor);
  discardPushButton->setGraphicsEffect(os::shadow());

  enlargePushButton->setFlat(true);
  enlargePushButton->setIconSize(QSize(22, 22));
  enlargePushButton->setCursor(Qt::PointingHandCursor);
  enlargePushButton->setGraphicsEffect(os::shadow());

  enlargePushButton->setDisabled(small);

  connect(this, SIGNAL(acceptAll()), confirmPushButton, SLOT(click()));
  connect(this, SIGNAL(rejectAll()), discardPushButton, SLOT(click()));

  connect(confirmPushButton, SIGNAL(clicked()), screenshot, SLOT(confirm()));
  connect(confirmPushButton, SIGNAL(clicked()), this, SLOT(closePreview()));

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

  widget->setLayout(wl);

  mStack->addWidget(widget);
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

void PreviewDialog::relocate()
{
  updateGeometry();
  resize(minimumSizeHint());

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

void PreviewDialog::closePreview()
{
  QLabel *widget = qobject_cast<QLabel*>(sender()->parent());
  mStack->removeWidget(widget);
  widget->hide();
  widget->deleteLater();

  if (mStack->count() == 0) {
    close();
  }
  else {
    relocate();
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
}

void PreviewDialog::previous()
{
  mStack->setCurrentIndex(mStack->currentIndex()-1);
  relocate();
}

void PreviewDialog::next()
{
  mStack->setCurrentIndex(mStack->currentIndex()+1);
  relocate();
}

void PreviewDialog::enlargePreview()
{
  Screenshot *screenshot = qobject_cast<Screenshot*>(ScreenshotManager::instance()->children().at(mStack->currentIndex()));

  if (screenshot == 0)
    return;

  new ScreenshotDialog(screenshot);
}

void PreviewDialog::closeEvent(QCloseEvent *event)
{
  Q_UNUSED(event)

  mInstance = 0;
  deleteLater();
}

void PreviewDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
  Q_UNUSED(event)
  enlargePreview();
}

void PreviewDialog::timerEvent(QTimerEvent *event)
{
  if (mAutoclose == 0) {
    if (mAutocloseAction == 0) {
      emit acceptAll();
    }
    else {
      emit rejectAll();
    }
  }
  else if (mAutoclose < 0) {
    killTimer(event->timerId());
  }
  else {
    setWindowTitle(tr("Screenshot Preview: Closing in %1").arg(mAutoclose));
    mAutoclose--;
  }
}

// Singleton

PreviewDialog* PreviewDialog::mInstance = 0;

PreviewDialog *PreviewDialog::instance()
{
  if (!mInstance) {
    mInstance = new PreviewDialog(0);
  }

  return mInstance;
}

bool PreviewDialog::isActive()
{
  if (mInstance) {
    return true;
  }

  return false;
}
