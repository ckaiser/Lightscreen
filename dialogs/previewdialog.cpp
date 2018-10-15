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
#include <dialogs/previewdialog.h>
#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/os.h>

#include <QApplication>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDrag>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QObject>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QStackedLayout>
#include <QToolButton>
#include <QUrl>

PreviewDialog::PreviewDialog(QWidget *parent) :
    QDialog(parent), mAutoclose(0), mAutocloseAction(0), mAutocloseReset(0), mPosition(0), mSize(0)
{
    setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Screenshot Preview"));
    setContextMenuPolicy(Qt::CustomContextMenu);

    auto settings = ScreenshotManager::instance()->settings();

    mSize     = settings->value("options/previewSize", 300).toInt();
    mPosition = settings->value("options/previewPosition", 3).toInt();

    if (settings->value("options/previewAutoclose", false).toBool()) {
        mAutoclose = settings->value("options/previewAutocloseTime").toInt();
        mAutocloseReset = mAutoclose;
        mAutocloseAction = settings->value("options/previewAutocloseAction").toInt();
    }

    auto layout = new QHBoxLayout;
    mStack = new QStackedLayout;
    connect(mStack, &QStackedLayout::currentChanged, this, &PreviewDialog::indexChanged);

    mPrevButton = new QPushButton(os::icon("arrow-left"), "", this);
    connect(mPrevButton, &QPushButton::clicked, this, &PreviewDialog::previous);

    mNextButton = new QPushButton(os::icon("arrow-right"), "", this);
    connect(mNextButton, &QPushButton::clicked, this, &PreviewDialog::next);

    mPrevButton->setCursor(Qt::PointingHandCursor);
    mPrevButton->setFlat(true);
    mPrevButton->setGraphicsEffect(os::shadow(Qt::white));
    mPrevButton->setIconSize(QSize(24, 24));
    mPrevButton->setVisible(false);

    mNextButton->setCursor(Qt::PointingHandCursor);
    mNextButton->setFlat(true);
    mNextButton->setGraphicsEffect(os::shadow(Qt::white));
    mNextButton->setIconSize(QSize(24, 24));
    mNextButton->setVisible(false);

    layout->addWidget(mPrevButton);
    layout->addLayout(mStack);
    layout->addWidget(mNextButton);

    layout->setMargin(0);
    layout->setContentsMargins(6, 6, 6, 6);

    mStack->setMargin(0);

    setMaximumHeight(mSize);
    setLayout(layout);

    if (mAutoclose) {
        startTimer(1000);
    }

    auto contextMenu = new QMenu(this);

    contextMenu->setTitle(tr("Global Preview Actions"));
    contextMenu->addAction(os::icon("yes")  , tr("&Save All")  , this, &PreviewDialog::acceptAll);
    contextMenu->addAction(os::icon("imgur"), tr("&Upload All"), this, &PreviewDialog::uploadAll);
    contextMenu->addSeparator();
    contextMenu->addAction(os::icon("no")   , tr("&Cancel All"), this, &PreviewDialog::rejectAll);
    contextMenu->addSeparator();
    contextMenu->addAction(os::icon("folder"), tr("Open &Folder"), this, [parent] {
        QMetaObject::invokeMethod(parent, "goToFolder");
    });

    connect(this, &PreviewDialog::customContextMenuRequested, contextMenu, [contextMenu](const QPoint &pos) {
        Q_UNUSED(pos)
        contextMenu->popup(QCursor::pos());
    });
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

    bool smallShot = false;

    QSize size = screenshot->pixmap().size();

    if (size.width() > mSize || size.height() > mSize) {
        size.scale(mSize, mSize, Qt::KeepAspectRatio);
    } else {
        smallShot = true;
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

    QPushButton *discardPushButton = new QPushButton(os::icon("no")   , "", label);
    QPushButton *enlargePushButton = new QPushButton(os::icon("preview"), "", label);
    QToolButton *confirmPushButton = new QToolButton(label);

    confirmPushButton->setIconSize(QSize(24, 24));
    confirmPushButton->setCursor(Qt::PointingHandCursor);
    confirmPushButton->setGraphicsEffect(os::shadow(Qt::white));

    if (ScreenshotManager::instance()->settings()->value("options/previewDefaultAction", 0).toInt() == 0
            || ScreenshotManager::instance()->settings()->value("options/uploadAuto").toBool()) {
        // Default button, confirm & upload.
        confirmPushButton->setIcon(os::icon("yes"));

        if (!ScreenshotManager::instance()->settings()->value("options/uploadAuto").toBool()) {
            QMenu *confirmMenu = new QMenu(confirmPushButton);
            confirmMenu->setObjectName("confirmMenu");

            QAction *uploadAction = new QAction(os::icon("imgur"), tr("Upload"), confirmPushButton);
            connect(uploadAction, &QAction::triggered, screenshot, &Screenshot::markUpload);
            connect(uploadAction, &QAction::triggered, screenshot, [screenshot] { screenshot->confirm(true); });
            connect(uploadAction, &QAction::triggered, this,       &PreviewDialog::closePreview);
            connect(this,         &PreviewDialog::uploadAll, uploadAction, &QAction::trigger);

            confirmMenu->addAction(uploadAction);
            confirmPushButton->setMenu(confirmMenu);
            confirmPushButton->setPopupMode(QToolButton::MenuButtonPopup);
        }

        connect(this, &PreviewDialog::acceptAll, confirmPushButton, &QPushButton::click);
        connect(confirmPushButton, &QPushButton::clicked, screenshot, [screenshot] { screenshot->confirm(true); });
        connect(confirmPushButton, &QPushButton::clicked, this, &PreviewDialog::closePreview);
    } else {
        // Reversed button, upload & confirm.
        confirmPushButton->setIcon(os::icon("imgur"));

        QMenu *confirmMenu = new QMenu(confirmPushButton);
        confirmMenu->setObjectName("confirmMenu");

        QAction *confirmAction = new QAction(os::icon("yes"), tr("Save"), confirmPushButton);
        connect(this, &PreviewDialog::acceptAll, confirmAction, &QAction::trigger);
        connect(confirmAction, &QAction::triggered, screenshot, [screenshot] { screenshot->confirm(true); });
        connect(confirmAction, &QAction::triggered, this, &PreviewDialog::closePreview);

        connect(confirmPushButton, &QPushButton::clicked, screenshot, &Screenshot::markUpload);
        connect(confirmPushButton, &QPushButton::clicked, screenshot, [screenshot] { screenshot->confirm(true); });
        connect(confirmPushButton, &QPushButton::clicked, this,       &PreviewDialog::closePreview);
        connect(this, &PreviewDialog::uploadAll, confirmPushButton, &QPushButton::click);

        confirmMenu->addAction(confirmAction);
        confirmPushButton->setMenu(confirmMenu);
        confirmPushButton->setPopupMode(QToolButton::MenuButtonPopup);
    }

    confirmPushButton->setAutoRaise(true);
    confirmPushButton->setVisible(false);

    discardPushButton->setIconSize(QSize(24, 24));
    discardPushButton->setCursor(Qt::PointingHandCursor);
    discardPushButton->setGraphicsEffect(os::shadow(Qt::white));
    discardPushButton->setFlat(true);
    discardPushButton->setVisible(false);

    enlargePushButton->setIconSize(QSize(22, 22));
    enlargePushButton->setCursor(Qt::PointingHandCursor);
    enlargePushButton->setGraphicsEffect(os::shadow(Qt::white));
    enlargePushButton->setFlat(true);
    enlargePushButton->setVisible(false);

    enlargePushButton->setDisabled(smallShot);

    connect(this, &PreviewDialog::rejectAll, discardPushButton, &QPushButton::click);

    connect(discardPushButton, &QPushButton::clicked, screenshot, &Screenshot::discard);
    connect(discardPushButton, &QPushButton::clicked, this, &PreviewDialog::closePreview);

    connect(enlargePushButton, &QPushButton::clicked, this, &PreviewDialog::enlargePreview);

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
    label->setProperty("screenshotObject", QVariant::fromValue<Screenshot *>(screenshot));

    mStack->addWidget(label);
    mStack->setCurrentIndex(mStack->count() - 1);

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
    } else {
        relocate();
    }
}

void PreviewDialog::enlargePreview()
{
    Screenshot *screenshot = mStack->currentWidget()->property("screenshotObject").value<Screenshot *>();

    if (screenshot) {
        QFileInfo info(screenshot->unloadedFileName());
        QDesktopServices::openUrl(QUrl(info.absoluteFilePath()));
    }
}

void PreviewDialog::indexChanged(int i)
{
    if (i == mStack->count() - 1) {
        mNextButton->setEnabled(false);
        mPrevButton->setEnabled(true);
    }

    if (i == 0 && mStack->count() > 1) {
        mNextButton->setEnabled(true);
        mPrevButton->setEnabled(false);
    }

    if (i != 0 && i != mStack->count() - 1) {
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
        setWindowTitle(tr("Screenshot Preview (%1 of %2)").arg(mStack->currentIndex() + 1).arg(mStack->count()));
    } else {
        setWindowTitle(tr("Screenshot Preview"));
    }
}

void PreviewDialog::next()
{
    mStack->setCurrentIndex(mStack->currentIndex() + 1);
    relocate();
}

void PreviewDialog::previous()
{
    mStack->setCurrentIndex(mStack->currentIndex() - 1);
    relocate();
}


void PreviewDialog::relocate()
{
    updateGeometry();
    resize(minimumSizeHint());
    QApplication::sendEvent(this, new QEvent(QEvent::Enter)); // Ensures the buttons are visible.

    QPoint where;
    switch (mPosition) {
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
            && mStack->currentWidget()) {
        for (QObject *child : mStack->currentWidget()->children()) {
            QWidget *widget = qobject_cast<QWidget *>(child);

            if (widget) {
                // Lets avoid disappearing buttons and bail if the menu is open.
                QMenu *confirmMenu = widget->findChild<QMenu *>("confirmMenu");
                if (confirmMenu && confirmMenu->isVisible()) {
                    return false;
                }

                widget->setVisible((event->type() == QEvent::Enter));
            }
        }
    } else if (event->type() == QEvent::Close) {
        if (mStack->count() != 0) {
            emit rejectAll();
        }

        deleteLater();
    }

    return QDialog::event(event);
}

void PreviewDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    enlargePreview();
}

void PreviewDialog::mousePressEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if ((event->pos() - mDragStartPosition).manhattanLength()
            < QApplication::startDragDistance())
        return;

    Screenshot *screenshot = mStack->currentWidget()->property("screenshotObject").value<Screenshot *>();

    if (screenshot) {
        QFileInfo info(screenshot->unloadedFileName());

        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;

        mimeData->setUrls(QList<QUrl>() << QUrl::fromLocalFile(info.absoluteFilePath()));
        drag->setMimeData(mimeData);

        drag->exec(Qt::CopyAction | Qt::MoveAction);
    }
}

void PreviewDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mDragStartPosition = event->pos();
    }
}

void PreviewDialog::timerEvent(QTimerEvent *event)
{
    if (mAutoclose == 0) {
        if (mAutocloseAction == 0) {
            emit acceptAll();
        } else if (mAutocloseAction == 1) {
            emit uploadAll();
        } else {
            emit rejectAll();
        }
    } else if (mAutoclose < 0) {
        killTimer(event->timerId());
    } else {
        setWindowTitle(tr("Preview: Closing in %1").arg(mAutoclose));
        mAutoclose--;
    }
}

