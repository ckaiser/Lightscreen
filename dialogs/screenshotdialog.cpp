#include "screenshotdialog.h"
#include "../tools/screenshot.h"


#include <QApplication>
#include <QDesktopWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>

#include <QDebug>

ScreenshotDialog::ScreenshotDialog(Screenshot *screenshot, QWidget *parent) :
    QDialog(parent)
{
  setWindowTitle(tr("Lightscreen Screenshot Viewer"));
  setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint | Qt::WindowContextHelpButtonHint);
  setWhatsThis(tr("You can zoom using the mouse wheel while holding the CTRL key. To return to the default zoom press \"Ctrl-0\"."));

  mScrollArea = new QScrollArea(this);
  mScrollArea->verticalScrollBar()->installEventFilter(this);

  QPalette newPalette = mScrollArea->palette();
  newPalette.setBrush(QPalette::Background, QBrush(QPixmap(":/backgrounds/checkerboard")));
  mScrollArea->setPalette(newPalette);

  mLabel = new QLabel(this);

  QHBoxLayout *layout = new QHBoxLayout(this);
  mLabel->setPixmap(screenshot->pixmap());
  mLabel->setScaledContents(true);

  mScrollArea->setAlignment(Qt::AlignCenter);
  mScrollArea->setWidget(mLabel);

  layout->addWidget(mScrollArea);
  layout->setMargin(0);

  setLayout(layout);
  setCursor(Qt::OpenHandCursor);

  mOriginalSize = mLabel->size();

  QSize size = (screenshot->pixmap().size() + QSize(2, 2)); // WTF: The 2x2 avoids the scrollbars..

  if (size.width() >= qApp->desktop()->availableGeometry().width()) {
    size.setWidth(qApp->desktop()->availableGeometry().size().width() - 300);
  }

  if (size.height() >= qApp->desktop()->availableGeometry().height()) {
    size.setHeight(qApp->desktop()->availableGeometry().size().height() - 300);
  }

  resize(size);  
  show();
}

void ScreenshotDialog::zoom(int offset)
{
  if (offset == 0) {
    mLabel->resize(mOriginalSize);
  }
  else {
    QSize newSize = mLabel->size();
    newSize.scale(mLabel->size() + QSize(offset, offset),  Qt::KeepAspectRatio);

    if (offset < 0 && (newSize.width() < 200 || newSize.height() < 200)) {
      return;
    }

    mLabel->resize(newSize);
  }
}

void ScreenshotDialog::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_0 && event->modifiers() & Qt::ControlModifier) {
    zoom(0);
  }
}

void ScreenshotDialog::mousePressEvent(QMouseEvent *event)
{
  mMousePos = event->pos();
  setCursor(Qt::ClosedHandCursor);
}

void ScreenshotDialog::mouseReleaseEvent(QMouseEvent *event)
{
  Q_UNUSED(event)
  setCursor(Qt::OpenHandCursor);
}

void ScreenshotDialog::mouseMoveEvent(QMouseEvent *event)
{
  QPoint diff = event->pos() - mMousePos;
  mMousePos = event->pos();

  mScrollArea->verticalScrollBar()->setValue(mScrollArea->verticalScrollBar()->value() - diff.y());
  mScrollArea->horizontalScrollBar()->setValue(mScrollArea->horizontalScrollBar()->value() - diff.x());
}

void ScreenshotDialog::closeEvent(QCloseEvent *event)
{
  Q_UNUSED(event)
  deleteLater();
}

bool ScreenshotDialog::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Wheel
   && qApp->keyboardModifiers() & Qt::ControlModifier) {
    QWheelEvent *wheelEvent = static_cast<QWheelEvent *>(event);
    zoom(wheelEvent->delta());
    return true;
  }

  return QObject::eventFilter(obj, event);
}
