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
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QPainter>
#include <QPixmap>
#include <QProcess>
#include <QTextStream>
#include <QScreen>
#include <QStringBuilder>

#include <tools/screenshot.h>
#include <tools/screenshotmanager.h>
#include <tools/windowpicker.h>
#include <tools/uploader/uploader.h>
#include <dialogs/areadialog.h>

#include <tools/os.h>

#ifdef Q_OS_WIN
    #include <windows.h>
#endif

#ifdef Q_OS_LINUX
    #include <QX11Info>
    #include <X11/X.h>
    #include <X11/Xlib.h>
#endif

Screenshot::Screenshot(QObject *parent, Screenshot::Options options):
    QObject(parent),
    mOptions(options),
    mPixmapDelay(false),
    mUnloaded(false),
    mUnloadFilename()
{
    if (mOptions.format == Screenshot::PNG) {
        mOptions.quality = 80;
    }
}

Screenshot::~Screenshot()
{
    if (!mUnloadFilename.isEmpty()) {
        QFile::remove(mUnloadFilename);
    }
}

QString Screenshot::getName(const NamingOptions &options, const QString &prefix, const QDir &directory)
{
    QString naming;
    int naming_largest = 0;

    if (options.flip) {
        naming = "%1" % prefix;
    } else {
        naming = prefix % "%1";
    }

    switch (options.naming) {
    case Screenshot::Numeric: // Numeric
        // Iterating through the folder to find the largest numeric naming.
        for (auto file : directory.entryList(QDir::Files)) {
            if (file.contains(prefix)) {
                file.chop(file.size() - file.lastIndexOf("."));
                file.remove(prefix);

                if (file.toInt() > naming_largest) {
                    naming_largest = file.toInt();
                }
            }
        }

        if (options.leadingZeros > 0) {
            //Pretty, huh?
            QString format;
            QTextStream(&format) << "%0" << (options.leadingZeros + 1) << "d";

            naming = naming.arg(QString().sprintf(format.toLatin1(), naming_largest + 1));
        } else {
            naming = naming.arg(naming_largest + 1);
        }
        break;
    case  Screenshot::Date: // Date
        naming = naming.arg(QLocale().toString(QDateTime::currentDateTime(), options.dateFormat));
        break;
    case  Screenshot::Timestamp: // Timestamp
        naming = naming.arg(QDateTime::currentDateTime().toTime_t());
        break;
    case  Screenshot::Empty:
        naming = naming.arg("");
        break;
    }

    return naming;
}

const QString &Screenshot::unloadedFileName()
{
    return mUnloadFilename;
}

const Screenshot::Options &Screenshot::options()
{
    return mOptions;
}

QPixmap &Screenshot::pixmap()
{
    return mPixmap;
}

//

void Screenshot::confirm(bool result)
{
    if (result) {
        save();
    } else {
        mOptions.result = Screenshot::Cancel;
        emit finished();
    }

    emit cleanup();

    mPixmap = QPixmap();
}

void Screenshot::confirmation()
{
    emit askConfirmation();

    if (mOptions.file) {
        unloadPixmap();
    }
}

void Screenshot::discard()
{
    confirm(false);
}

void Screenshot::markUpload()
{
    mOptions.upload = true;
}

void Screenshot::optimize()
{
    QProcess *process = new QProcess(this);

    // Delete the QProcess once it's done.
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this   , SLOT(optimizationDone()));
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), process, SLOT(deleteLater()));

    QString optiPNG;

#ifdef Q_OS_UNIX
    optiPNG = "optipng";
#else
    optiPNG = qApp->applicationDirPath() % QDir::separator() % "optipng.exe";
#endif

    if (!QFile::exists(optiPNG)) {
        emit optimizationDone();
    }

    process->start(optiPNG, QStringList() << mOptions.fileName);

    if (process->state() == QProcess::NotRunning) {
        emit optimizationDone();
        process->deleteLater();
    }
}

void Screenshot::optimizationDone()
{
    if (mOptions.upload) {
        upload();
    } else {
        emit finished();
    }
}

void Screenshot::save()
{
    QString name = "";
    QString fileName = "";
    Screenshot::Result result = Screenshot::Failure;

    if (mOptions.file && !mOptions.saveAs)  {
        name = newFileName();
    } else if (mOptions.file && mOptions.saveAs) {
        name = QFileDialog::getSaveFileName(0, tr("Save as.."), newFileName(), "*" % extension());
    }

    if (!mOptions.replace && QFile::exists(name % extension())) {
        // Ugly? You should see my wife!
        int count = 0;
        int cunt  = 0;

        QString naming = QFileInfo(name).fileName();

        for (auto file : QFileInfo(name % extension()).dir().entryList(QDir::Files)) {
            if (file.contains(naming)) {
                file.remove(naming);
                file.remove(" (");
                file.remove(")");
                file.remove(extension());

                cunt = file.toInt();

                if (cunt > count) {
                    count = cunt;
                }
            }
        }

        name = name % " (" % QString::number(count % 1) % ")";
    }

    if (mOptions.clipboard && !(mOptions.upload && mOptions.urlClipboard)) {
        if (mUnloaded) {
            mUnloaded = false;
            mPixmap = QPixmap(mUnloadFilename);
        }

        QApplication::clipboard()->setPixmap(mPixmap, QClipboard::Clipboard);

        if (!mOptions.file) {
            result = Screenshot::Success;
        }
    }

    if (mOptions.file) {
        fileName = name % extension();

        if (name.isEmpty()) {
            result = Screenshot::Cancel;
        } else if (mUnloaded) {
            result = (QFile::rename(mUnloadFilename, fileName)) ? Screenshot::Success : Screenshot::Failure;
        } else if (mPixmap.save(fileName, 0, mOptions.quality)) {
            result = Screenshot::Success;
        } else {
            result = Screenshot::Failure;
        }
    }

    mOptions.fileName = fileName;
    mOptions.result   = result;

    if (!mOptions.result) {
        emit finished();
    }

    if (mOptions.format == Screenshot::PNG && mOptions.optimize && mOptions.file) {
        if (!mOptions.upload) {
            ScreenshotManager::instance()->saveHistory(mOptions.fileName);
        }

        optimize();
    } else if (mOptions.upload) {
        upload();
    } else if (mOptions.file) {
        ScreenshotManager::instance()->saveHistory(mOptions.fileName);
        emit finished();
    } else {
        emit finished();
    }
}

void Screenshot::setPixmap(const QPixmap &pixmap)
{
    mPixmap = pixmap;

    if (mPixmap.isNull()) {
        emit confirm(false);
    } else {
        confirmation();
    }
}

void Screenshot::take()
{
    switch (mOptions.mode) {
    case Screenshot::WholeScreen:
        wholeScreen();
        break;

    case Screenshot::SelectedArea:
        selectedArea();
        break;

    case Screenshot::ActiveWindow:
        activeWindow();
        break;

    case Screenshot::SelectedWindow:
        selectedWindow();
        break;
    }

    if (mPixmapDelay) {
        return;
    }

    if (mPixmap.isNull()) {
        confirm(false);
    } else {
        confirmation();
    }
}

void Screenshot::upload()
{
    if (mOptions.file) {
        Uploader::instance()->upload(mOptions.fileName, mOptions.uploadService);
    } else if (unloadPixmap()) {
        Uploader::instance()->upload(mUnloadFilename, mOptions.uploadService);
    } else {
        emit finished();
    }
}

void Screenshot::uploadDone(const QString &url)
{
    if (mOptions.urlClipboard && !url.isEmpty()) {
        QApplication::clipboard()->setText(url, QClipboard::Clipboard);
    }

    emit finished();
}

void Screenshot::refresh()
{
    grabDesktop();
}

//

void Screenshot::activeWindow()
{
#ifdef Q_OS_WIN
    HWND fWindow = GetForegroundWindow();

    if (fWindow == NULL) {
        return;
    }

    if (fWindow == GetDesktopWindow()) {
        wholeScreen();
        return;
    }

    mPixmap = os::grabWindow((WId)GetForegroundWindow());
#endif

#if defined(Q_OS_LINUX)
    Window focus;
    int revert;

    XGetInputFocus(QX11Info::display(), &focus, &revert);

    mPixmap = QPixmap::grabWindow(focus);
#endif
}

const QString Screenshot::extension() const
{
    switch (mOptions.format) {
    case Screenshot::PNG:
        return QStringLiteral(".png");
        break;
    case Screenshot::BMP:
        return QStringLiteral(".bmp");
        break;
    case Screenshot::WEBP:
        return QStringLiteral(".webp");
        break;
    case Screenshot::JPEG:
        return QStringLiteral(".jpg");
        break;
    }
}

void Screenshot::grabDesktop()
{
    QRect geometry;
    QPoint cursorPosition = QCursor::pos();

    if (mOptions.currentMonitor) {
        int currentScreen = qApp->desktop()->screenNumber(cursorPosition);

        geometry = qApp->desktop()->screen(currentScreen)->geometry();
        cursorPosition = cursorPosition - geometry.topLeft();
    } else {
        int top = 0;

        for (QScreen *screen : QGuiApplication::screens()) {
            auto screenRect = screen->geometry();

            if (screenRect.top() < 0) {
                top += screenRect.top() * -1;
            }

            if (screenRect.left() < 0) {
                cursorPosition.setX(cursorPosition.x() + screenRect.width()); //= localCursorPos + screenRect.normalized().topLeft();
            }

            geometry = geometry.united(screenRect);
        }

        cursorPosition.setY(cursorPosition.y() + top);
    }

    mPixmap = QApplication::primaryScreen()->grabWindow(QApplication::desktop()->winId(), geometry.x(), geometry.y(), geometry.width(), geometry.height());
    mPixmap.setDevicePixelRatio(QApplication::desktop()->devicePixelRatio());

    if (mOptions.cursor && !mPixmap.isNull()) {
        QPainter painter(&mPixmap);
        auto cursorInfo = os::cursor();
        auto cursorPixmap = cursorInfo.first;
        cursorPixmap.setDevicePixelRatio(QApplication::desktop()->devicePixelRatio()); 

#if 0 // Debug cursor position helper
        painter.setBrush(QBrush(Qt::darkRed));
        painter.setPen(QPen(QBrush(Qt::red), 5));
        QRectF rect;
        rect.setSize(QSizeF(100, 100));
        rect.moveCenter(cursorPosition);
        painter.drawRoundRect(rect, rect.size().height()*2, rect.size().height()*2);
#endif

        painter.drawPixmap(cursorPosition-cursorInfo.second, cursorPixmap);
    }
}

const QString Screenshot::newFileName() const
{
    if (!mOptions.directory.exists()) {
        mOptions.directory.mkpath(mOptions.directory.path());
    }

    QString naming = Screenshot::getName(mOptions.namingOptions, mOptions.prefix, mOptions.directory);
    QString path   = QDir::toNativeSeparators(mOptions.directory.path());

    // Cleanup
    if (path.at(path.size() - 1) != QDir::separator() && !path.isEmpty()) {
        path.append(QDir::separator());
    }

    QString fileName;
    fileName.append(path);
    fileName.append(naming);

    return fileName;
}

void Screenshot::selectedArea()
{
    grabDesktop();

    if (mPixmap.isNull()) {
        return;
    }

    AreaDialog selector(this);
    int result = selector.exec();

    if (result == QDialog::Accepted) {
        mPixmap = mPixmap.copy(selector.resultRect());
    } else {
        mPixmap = QPixmap();
    }
}

void Screenshot::selectedWindow()
{
    WindowPicker *windowPicker = new WindowPicker;
    mPixmapDelay = true;

    connect(windowPicker, SIGNAL(pixmap(QPixmap)), this, SLOT(setPixmap(QPixmap)));
}

bool Screenshot::unloadPixmap()
{
    if (mUnloaded) {
        return true;
    }

    // Unloading the pixmap to reduce memory usage during previews
    mUnloadFilename = QString("%1/.screenshot.%2%3").arg(mOptions.directory.path()).arg(qrand() * qrand() + QDateTime::currentDateTime().toTime_t()).arg(extension());
    mUnloaded       = mPixmap.save(mUnloadFilename, 0, mOptions.quality);

    if (mUnloaded) {
        mPixmap = QPixmap();
    }

    return mUnloaded;
}

void Screenshot::wholeScreen()
{
    grabDesktop();
}
