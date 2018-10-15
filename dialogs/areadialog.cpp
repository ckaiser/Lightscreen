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
 ******
 *
 * Based on KDE's KSnapshot regiongrabber.cpp, revision 796531, Copyright 2007 Luca Gugelmann <lucag@student.ethz.ch>
 * released under the GNU LGPL  <http://www.gnu.org/licenses/old-licenses/library.txt>
 *
 */
#include <dialogs/areadialog.h>
#include <tools/os.h>
#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QToolTip>
#include <QThread>

AreaDialog::AreaDialog(Screenshot *screenshot) :
    QDialog(0), mScreenshot(screenshot), mMouseDown(false), mMouseMagnifier(false),
    mNewSelection(false), mHandleSize(10), mMouseOverHandle(0),
    mShowHelp(true), mGrabbing(false), mOverlayAlpha(1), mAutoclose(false),
    mTLHandle(0, 0, mHandleSize, mHandleSize), mTRHandle(0, 0, mHandleSize, mHandleSize),
    mBLHandle(0, 0, mHandleSize, mHandleSize), mBRHandle(0, 0, mHandleSize, mHandleSize),
    mLHandle(0, 0, mHandleSize, mHandleSize), mTHandle(0, 0, mHandleSize, mHandleSize),
    mRHandle(0, 0, mHandleSize, mHandleSize), mBHandle(0, 0, mHandleSize, mHandleSize)
{
    mHandles << &mTLHandle << &mTRHandle << &mBLHandle << &mBRHandle
             << &mLHandle << &mTHandle << &mRHandle << &mBHandle;

    mMouseOverHandle = 0;

    setMouseTracking(true);
    setWindowTitle(tr("Lightscreen Area Mode"));
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);

    setCursor(Qt::CrossCursor);

    connect(&mIdleTimer, &QTimer::timeout, this, &AreaDialog::displayHelp);
    mIdleTimer.start(2000);

    mAutoclose = ScreenshotManager::instance()->settings()->value("options/areaAutoclose").toBool();

    if (mAutoclose) {
        return;    // Avoid creating the accept widget if it's not going to get used.
    }

    // Creating accept widget:
    mAcceptWidget = new QWidget(this);
    mAcceptWidget->resize(140, 70);
    mAcceptWidget->setWindowOpacity(0.4);
    mAcceptWidget->setStyleSheet("QWidget { background: rgba(255, 255, 255, 200); border: 4px solid #232323; padding: 0; } QPushButton { background: transparent; border: none; height: 50px; padding: 5px; } QPushButton:hover { cursor: hand; }");

    auto awAcceptButton = new QPushButton(QIcon(":/icons/yes.big"), "", this);
    connect(awAcceptButton, &QPushButton::clicked, this, &AreaDialog::grabRect);
    awAcceptButton->setCursor(Qt::PointingHandCursor);
    awAcceptButton->setIconSize(QSize(48, 48));

    auto awRejectButton = new QPushButton(QIcon(":/icons/no.big"), "", this);
    connect(awRejectButton, &QPushButton::clicked, this, &AreaDialog::cancel);
    awRejectButton->setCursor(Qt::PointingHandCursor);
    awRejectButton->setIconSize(QSize(48, 48));

    auto awLayout = new QHBoxLayout(this);
    awLayout->addWidget(awAcceptButton);
    awLayout->addWidget(awRejectButton);
    awLayout->setMargin(0);
    awLayout->setSpacing(0);

    mAcceptWidget->setLayout(awLayout);
    mAcceptWidget->setVisible(false);
}

QRect AreaDialog::resultRect() const
{
    auto devicePixelRatio = mScreenshot->pixmap().devicePixelRatio();

    return QRect(mSelection.left()   * devicePixelRatio,
                 mSelection.top()    * devicePixelRatio,
                 mSelection.width()  * devicePixelRatio,
                 mSelection.height() * devicePixelRatio);
}

void AreaDialog::animationTick(int frame)
{
    mOverlayAlpha = frame;
    update();
}

void AreaDialog::cancel()
{
    reject();
}

void AreaDialog::displayHelp()
{
    mShowHelp = true;
    update();
}

void AreaDialog::grabRect()
{
    QRect r = mSelection.normalized();
    if (!r.isNull() && r.isValid()) {
        mGrabbing = true;
        accept();
    }
}

void AreaDialog::keyboardResize()
{
    if (mKeyboardSize.contains("x")) {
        auto sizeList = mKeyboardSize.split("x");

        if (sizeList.count() == 2) {
            bool okw = false, okh = false;

            QSize size(sizeList.at(0).toInt(&okw), sizeList.at(1).toInt(&okh));

            if (okw && okh && size.width() > 0 && size.height() > 0) {
                auto oldSize = mSelection.size();

                mSelection.setSize(size);

                if (rect().contains(mSelection)) {
                    updateHandles();
                } else {
                    mSelection.setSize(oldSize); // Reverting size
                }
            }
        }
    }

    mKeyboardSize.clear();
    update();
}

void AreaDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        if (mKeyboardSize.isEmpty()) {
            cancel();
        } else {
            mKeyboardSize.clear();
            update();
        }
    } else if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
        if (mKeyboardSize.isEmpty()) {
            grabRect();
        } else {
            keyboardResize();
        }
    } else if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down || e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) {
        // Keyboard movement
        int adjustMagnitude = 1;

        if (e->modifiers() & Qt::ShiftModifier)
            adjustMagnitude += 4;

        if (e->modifiers() & Qt::ControlModifier)
            adjustMagnitude += 4;

        if (e->modifiers() & Qt::AltModifier)
            adjustMagnitude += 4;

        int xAdjust = 0;
        int yAdjust = 0;

        if (e->key() == Qt::Key_Left)
            xAdjust = -(adjustMagnitude);

        if (e->key() == Qt::Key_Right)
            xAdjust = adjustMagnitude;

        if (e->key() == Qt::Key_Up)
            yAdjust = -(adjustMagnitude);

        if (e->key() == Qt::Key_Down)
            yAdjust = adjustMagnitude;

        auto adjusted = mSelection.adjusted(xAdjust, yAdjust, xAdjust, yAdjust);

        if (rect().contains(adjusted)) {
            mSelection = adjusted;
            updateHandles();
            update();
        }
    } else if (e->key() == Qt::Key_F5) {
        setWindowOpacity(0);
        QThread::msleep(200); // Give the window system time to catch up
        mScreenshot->refresh();
        setWindowOpacity(1);
    } else {
        if (e->key() == Qt::Key_Backspace && !mKeyboardSize.isEmpty()) {
            mKeyboardSize.remove(mKeyboardSize.size()-1, 1);
            update();
            return;
        }

        if (e->key() != Qt::Key_X) {
            bool ok = false;
            int number = e->text().toInt(&ok);

            if (!ok || number < 0 || number > qMax<int>(width(), height())) {
                e->ignore();
                return;
            }
        } else {
            if (mKeyboardSize.contains("x"))
                return;
        }

        mKeyboardSize += e->text();
        update();
    }
}

void AreaDialog::mouseDoubleClickEvent(QMouseEvent *)
{
    grabRect();
}

void AreaDialog::mouseMoveEvent(QMouseEvent *e)
{
    mMouseMagnifier = false;

    if (mMouseDown) {
        mMousePos = e->pos();

        if (mNewSelection) {
            mSelection = QRect(mDragStartPoint, limitPointToRect(mMousePos, rect())).normalized();
        } else if (mMouseOverHandle == 0) {
            // Moving the whole selection
            QRect r = rect().normalized(), s = mSelectionBeforeDrag.normalized();
            QPoint p = s.topLeft() + e->pos() - mDragStartPoint;
            r.setBottomRight(r.bottomRight() - QPoint(s.width(), s.height()));

            if (!r.isNull() && r.isValid()) {
                mSelection.moveTo(limitPointToRect(p, r));
            }
        } else {
            // Dragging a handle
            QRect r = mSelectionBeforeDrag;
            QPoint offset = e->pos() - mDragStartPoint;

            bool symmetryMod = qApp->keyboardModifiers() & Qt::ShiftModifier;
            bool aspectRatioMod = qApp->keyboardModifiers() & Qt::ControlModifier;

            if (mMouseOverHandle == &mTLHandle || mMouseOverHandle == &mTHandle
                    || mMouseOverHandle == &mTRHandle) { // dragging one of the top handles
                r.setTop(r.top() + offset.y());

                if (symmetryMod) {
                    r.setBottom(r.bottom() - offset.y());
                }
            }

            if (mMouseOverHandle == &mTLHandle || mMouseOverHandle == &mLHandle
                    || mMouseOverHandle == &mBLHandle) { // dragging one of the left handles
                r.setLeft(r.left() + offset.x());

                if (symmetryMod) {
                    r.setRight(r.right() - offset.x());
                }
            }

            if (mMouseOverHandle == &mBLHandle || mMouseOverHandle == &mBHandle
                    || mMouseOverHandle == &mBRHandle) { // dragging one of the bottom handles
                r.setBottom(r.bottom() + offset.y());

                if (symmetryMod) {
                    r.setTop(r.top() - offset.y());
                }
            }

            if (mMouseOverHandle == &mTRHandle || mMouseOverHandle == &mRHandle
                    || mMouseOverHandle == &mBRHandle) { // dragging one of the right handles
                r.setRight(r.right() + offset.x());

                if (symmetryMod) {
                    r.setLeft(r.left() - offset.x());
                }
            }

            r = r.normalized();
            r.setTopLeft(limitPointToRect(r.topLeft(), rect()));
            r.setBottomRight(limitPointToRect(r.bottomRight(), rect()));

            if (aspectRatioMod) {
                if (mMouseOverHandle == &mBLHandle || mMouseOverHandle == &mBRHandle || mMouseOverHandle == &mLHandle || mMouseOverHandle == &mRHandle) {
                    r.setHeight(r.width());
                } else {
                    r.setWidth(r.height());
                }
            }

            mSelection = r;
        }

        if (mAcceptWidget) {
            QPoint acceptPos = e->pos();
            QRect  acceptRect = QRect(acceptPos, QSize(120, 70));

            // Prevent the widget from overlapping the handles
            if (acceptRect.intersects(mTLHandle)) {
                acceptPos = mTLHandle.bottomRight() + QPoint(2, 2); // Corner case
            }

            if (acceptRect.intersects(mBRHandle)) {
                acceptPos = mBRHandle.bottomRight();
            }

            if (acceptRect.intersects(mBHandle)) {
                acceptPos = mBHandle.bottomRight();
            }

            if (acceptRect.intersects(mRHandle)) {
                acceptPos = mRHandle.topRight();
            }

            if (acceptRect.intersects(mTHandle)) {
                acceptPos = mTHandle.bottomRight();
            }

            if ((acceptPos.x() + 120) > mScreenshot->pixmap().rect().width()) {
                acceptPos.setX(acceptPos.x() - 120);
            }

            if ((acceptPos.y() + 70) > mScreenshot->pixmap().rect().height()) {
                acceptPos.setY(acceptPos.y() - 70);
            }

            mAcceptWidget->move(acceptPos);
        }

        update();
    } else {
        if (mSelection.isNull()) {
            mMouseMagnifier = true;
            update();
            return;
        }

        bool found = false;
        foreach (QRect *r, mHandles) {
            if (r->contains(e->pos())) {
                mMouseOverHandle = r;
                found = true;
                break;
            }
        }

        if (!found) {
            mMouseOverHandle = 0;
            if (mSelection.contains(e->pos())) {
                setCursor(Qt::OpenHandCursor);
            } else if (mAcceptWidget && QRect(mAcceptWidget->mapToParent(mAcceptWidget->pos()), QSize(100, 60)).contains(e->pos())) {
                setCursor(Qt::PointingHandCursor);
            } else {
                setCursor(Qt::CrossCursor);
            }
        } else {
            if (mMouseOverHandle == &mTLHandle || mMouseOverHandle == &mBRHandle) {
                setCursor(Qt::SizeFDiagCursor);
            }
            if (mMouseOverHandle == &mTRHandle || mMouseOverHandle == &mBLHandle) {
                setCursor(Qt::SizeBDiagCursor);
            }
            if (mMouseOverHandle == &mLHandle || mMouseOverHandle == &mRHandle) {
                setCursor(Qt::SizeHorCursor);
            }
            if (mMouseOverHandle == &mTHandle || mMouseOverHandle == &mBHandle) {
                setCursor(Qt::SizeVerCursor);
            }
        }
    }
}

void AreaDialog::mousePressEvent(QMouseEvent *e)
{
    mShowHelp = false;
    mIdleTimer.stop();

    if (mAcceptWidget) {
        mAcceptWidget->hide();
    }

    if (e->button() == Qt::LeftButton) {
        mMouseDown = true;
        mDragStartPoint = e->pos();
        mSelectionBeforeDrag = mSelection;
        if (!mSelection.contains(e->pos())) {
            mNewSelection = true;
            mSelection = QRect();
            mShowHelp = true;
            setCursor(Qt::CrossCursor);
        } else {
            setCursor(Qt::ClosedHandCursor);
        }
    } else if (e->button() == Qt::RightButton
               || e->button() == Qt::MidButton) {
        cancel();
    }

    update();
}

void AreaDialog::mouseReleaseEvent(QMouseEvent *e)
{
    if (mAutoclose) {
        grabRect();
    }

    if (!mSelection.isNull() && mAcceptWidget) {
        mAcceptWidget->show();
    }

    mMouseDown = false;
    mNewSelection = false;
    mIdleTimer.start();

    if (mMouseOverHandle == 0 && mSelection.contains(e->pos())) {
        setCursor(Qt::OpenHandCursor);
    }

    update();
}

void AreaDialog::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    if (mGrabbing) { // grabWindow() should just get the background
        return;
    }

    QPainter painter(this);

    QPalette pal = palette();
    QFont font   = QToolTip::font();

    QColor handleColor(85, 160, 188, 220);
    QColor overlayColor(0, 0, 0, mOverlayAlpha);
    QColor textColor = pal.color(QPalette::Active, QPalette::Text);
    QColor textBackgroundColor = pal.color(QPalette::Active, QPalette::Base);
    painter.drawPixmap(0, 0, mScreenshot->pixmap());
    painter.setFont(font);

    QRect r = mSelection.normalized().adjusted(0, 0, -1, -1);

    QRegion grey(rect());
    grey = grey.subtracted(r);
    painter.setPen(handleColor);
    painter.setBrush(overlayColor);
    painter.setClipRegion(grey);
    painter.drawRect(-1, -1, rect().width() + 1, rect().height() + 1);
    painter.setClipRect(rect());
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(r);

    if (mShowHelp) {
        //Drawing the explanatory text.
        QRect helpRect = qApp->desktop()->screenGeometry(qApp->desktop()->primaryScreen());
        QString helpTxt = tr("Lightscreen area mode:\nUse your mouse to draw a rectangle to capture.\nPress any key or right click to exit.");

        helpRect.setHeight(qRound((float)(helpRect.height() / 10))); // We get a decently sized rect where the text should be drawn (centered)

        // We draw the white contrasting background for the text, using the same text and options to get the boundingRect that the text will have.
        painter.setPen(QPen(Qt::white));
        painter.setBrush(QBrush(QColor(255, 255, 255, 180), Qt::SolidPattern));
        QRectF bRect = painter.boundingRect(helpRect, Qt::AlignCenter, helpTxt);

        // These four calls provide padding for the rect
        bRect.setWidth(bRect.width() + 12);
        bRect.setHeight(bRect.height() + 10);
        bRect.setX(bRect.x() - 12);
        bRect.setY(bRect.y() - 10);

        painter.drawRoundedRect(bRect, 8, 8);

        // Draw the text:
        painter.setPen(QPen(Qt::black));
        painter.drawText(helpRect, Qt::AlignCenter, helpTxt);
    }

    if (!mKeyboardSize.isEmpty()) {
        QRect keyboardSizeRect = qApp->desktop()->screenGeometry(qApp->desktop()->primaryScreen());

        QFont originalFont = painter.font();
        QFont font = originalFont;
        font.setPixelSize(16);
        font.setBold(true);
        painter.setFont(font);

        painter.setPen(QPen(Qt::white));
        painter.setBrush(QBrush(QColor(255, 255, 255, 180), Qt::SolidPattern));
        QRectF textRect = painter.boundingRect(keyboardSizeRect, Qt::AlignLeft, mKeyboardSize);

        textRect.setX(textRect.x() + 9);
        textRect.setY(textRect.y() + 10);
        textRect.setWidth(textRect.width() + 14);
        textRect.setHeight(textRect.height() + 12);

        painter.drawRect(textRect);

        // Left-Right padding
        textRect.setX(textRect.x() + 3);
        textRect.setWidth(textRect.width() - 2);

        painter.setPen(QPen(Qt::black));
        painter.drawText(textRect, Qt::AlignCenter, mKeyboardSize, &textRect);

        painter.setFont(originalFont); // Reset the font
    }

    if (!mSelection.isNull()) {
        // The grabbed region is everything which is covered by the drawn
        // rectangles (border included). This means that there is no 0px
        // selection, since a 0px wide rectangle will always be drawn as a line.
        QString txt = QString("%1x%2").arg(mSelection.width() == 0 ? 2 : mSelection.width())
                      .arg(mSelection.height() == 0 ? 2 : mSelection.height());
        QRect textRect = painter.boundingRect(rect(), Qt::AlignLeft, txt);
        QRect boundingRect = textRect.adjusted(-4, 0, 0, 0);

        if (textRect.width() < r.width() - 2 * mHandleSize &&
                textRect.height() < r.height() - 2 * mHandleSize &&
                (r.width() > 100 && r.height() > 100)) { // center, unsuitable for small selections
            boundingRect.moveCenter(r.center());
            textRect.moveCenter(r.center());
        } else if (r.y() - 3 > textRect.height() &&
                   r.x() + textRect.width() < rect().right()) { // on top, left aligned
            boundingRect.moveBottomLeft(QPoint(r.x(), r.y() - 3));
            textRect.moveBottomLeft(QPoint(r.x() + 2, r.y() - 3));
        } else if (r.x() - 3 > textRect.width()) { // left, top aligned
            boundingRect.moveTopRight(QPoint(r.x() - 3, r.y()));
            textRect.moveTopRight(QPoint(r.x() - 5, r.y()));
        } else if (r.bottom() + 3 + textRect.height() < rect().bottom() &&
                   r.right() > textRect.width()) { // at bottom, right aligned
            boundingRect.moveTopRight(QPoint(r.right(), r.bottom() + 3));
            textRect.moveTopRight(QPoint(r.right() - 2, r.bottom() + 3));
        } else if (r.right() + textRect.width() + 3 < rect().width()) { // right, bottom aligned
            boundingRect.moveBottomLeft(QPoint(r.right() + 3, r.bottom()));
            textRect.moveBottomLeft(QPoint(r.right() + 5, r.bottom()));
        }
        // if the above didn't catch it, you are running on a very tiny screen...
        painter.setPen(textColor);
        painter.setBrush(textBackgroundColor);
        painter.drawRect(boundingRect);
        painter.drawText(textRect, txt);

        if ((r.height() > mHandleSize * 2 && r.width() > mHandleSize * 2)
                || !mMouseDown) {
            updateHandles();
            painter.setPen(handleColor);
            handleColor.setAlpha(80);
            painter.setBrush(handleColor);
            painter.drawRects(handleMask().rects());
        }
    }

    if (!mScreenshot->options().magnify) {
        return;
    }

    // Drawing the magnified version
    QPoint magStart, magEnd, drawPosition;
    QRect newRect;

    QRect pixmapRect = mScreenshot->pixmap().rect();

    if (mMouseMagnifier) {
        drawPosition = mMousePos - QPoint(100, 100);

        magStart = mMousePos - QPoint(50, 50);
        magEnd = mMousePos + QPoint(50, 50);

        newRect = QRect(magStart, magEnd);
    } else {
        // So pretty.. oh so pretty.
        if (mMouseOverHandle == &mTLHandle) {
            magStart = mSelection.topLeft();
        } else if (mMouseOverHandle == &mTRHandle) {
            magStart = mSelection.topRight();
        } else if (mMouseOverHandle == &mBLHandle) {
            magStart = mSelection.bottomLeft();
        } else if (mMouseOverHandle == &mBRHandle) {
            magStart = mSelection.bottomRight();
        } else if (mMouseOverHandle == &mLHandle) {
            magStart = QPoint(mSelection.left(), mSelection.center().y());
        } else if (mMouseOverHandle == &mTHandle) {
            magStart = QPoint(mSelection.center().x(), mSelection.top());
        } else if (mMouseOverHandle == &mRHandle) {
            magStart = QPoint(mSelection.right(), mSelection.center().y());
        } else if (mMouseOverHandle == &mBHandle) {
            magStart =  QPoint(mSelection.center().x(), mSelection.bottom());
        } else if (mMouseOverHandle == 0) {
            magStart = mMousePos;
        }

        magEnd = magStart;
        drawPosition = mSelection.bottomRight();

        magStart -= QPoint(48, 48);
        magEnd   += QPoint(48, 48);

        newRect = QRect(magStart, magEnd);

        if ((drawPosition.x() + newRect.width() * 2) > pixmapRect.width()) {
            drawPosition.setX(drawPosition.x() - newRect.width() * 2);
        }

        if ((drawPosition.y() + newRect.height() * 2) > pixmapRect.height()) {
            drawPosition.setY(drawPosition.y() - newRect.height() * 2);
        }

        if (drawPosition.y() == mSelection.bottomRight().y() - newRect.height() * 2
                && drawPosition.x() == mSelection.bottomRight().x() - newRect.width() * 2) {
            painter.setOpacity(0.7);
        }
    }

    if (!pixmapRect.contains(newRect, true) || drawPosition.x() <= 0 || drawPosition.y() <= 0)  {
        return;
    }

    QPixmap magnified = mScreenshot->pixmap().copy(newRect).scaled(QSize(newRect.width() * 2, newRect.height() * 2));

    QPainter magPainter(&magnified);
    magPainter.setPen(QPen(QBrush(QColor(35, 35, 35)), 2)); // Same border pen
    magPainter.drawRect(magnified.rect());

    if (!mMouseMagnifier) {
        magPainter.setCompositionMode(QPainter::CompositionMode_Exclusion);
        magPainter.setPen(QPen(QBrush(QColor(255, 255, 255, 180)), 1));

        QRect magRect = magnified.rect();
        magPainter.drawLine(QLine(magRect.left(), magRect.width()/2, magRect.right(), magRect.width()/2));
        magPainter.drawLine(QLine(magRect.width()/2, 0, magRect.height()/2, magnified.height()));
    }

    painter.drawPixmap(drawPosition, magnified);
}

void AreaDialog::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);

    if (mSelection.isNull()) {
        return;
    }

    QRect r = mSelection;
    r.setTopLeft(limitPointToRect(r.topLeft(), rect()));
    r.setBottomRight(limitPointToRect(r.bottomRight(), rect()));

    if (r.width() <= 1 || r.height() <= 1) { //this just results in ugly drawing...
        r = QRect();
    }

    mSelection = r;
}

void AreaDialog::showEvent(QShowEvent *e)
{
    Q_UNUSED(e)

    QRect geometry = qApp->desktop()->geometry();

    if (mScreenshot->options().currentMonitor) {
        geometry = qApp->desktop()->screenGeometry(qApp->desktop()->screenNumber(QCursor::pos()));
    }

    resize(geometry.size());
    move(geometry.topLeft());

    if (mScreenshot->options().animations) {
        os::effect(this, SLOT(animationTick(int)), 85, 300);
    } else {
        animationTick(85);
    }

    setMouseTracking(true);
}

void AreaDialog::updateHandles()
{
    QRect r = mSelection.normalized().adjusted(0, 0, -1, -1);
    int s2 = mHandleSize / 2;

    mTLHandle.moveTopLeft(r.topLeft());
    mTRHandle.moveTopRight(r.topRight());
    mBLHandle.moveBottomLeft(r.bottomLeft());
    mBRHandle.moveBottomRight(r.bottomRight());

    mLHandle.moveTopLeft(QPoint(r.x(), r.y() + r.height() / 2 - s2));
    mTHandle.moveTopLeft(QPoint(r.x() + r.width() / 2 - s2, r.y()));
    mRHandle.moveTopRight(QPoint(r.right(), r.y() + r.height() / 2 - s2));
    mBHandle.moveBottomLeft(QPoint(r.x() + r.width() / 2 - s2, r.bottom()));
}

QRegion AreaDialog::handleMask() const
{
    // note: not normalized QRects are bad here, since they will not be drawn
    QRegion mask;
    foreach(QRect * rect, mHandles) mask += QRegion(*rect);
    return mask;
}

QPoint AreaDialog::limitPointToRect(const QPoint &p, const QRect &r) const
{
    QPoint q;
    q.setX(p.x() < r.x() ? r.x() : p.x() < r.right() ? p.x() : r.right());
    q.setY(p.y() < r.y() ? r.y() : p.y() < r.bottom() ? p.y() : r.bottom());
    return q;
}
