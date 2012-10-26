/*
 * Copyright (C) 2012  Christian Kaiser
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

#include <QDebug>

#include "windowpicker.h"
#include "../dialogs/areadialog.h"
#include "uploader.h"
#include "screenshot.h"
#include "screenshotmanager.h"

#include "os.h"

#ifdef Q_WS_WIN
  #include <windows.h>
#endif

#ifdef Q_WS_X11
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
  // Here be crickets
}

Screenshot::~Screenshot()
{
  qDebug() << "Deleting Screenshot object!";

  if (!mUnloadFilename.isEmpty()) {
    QFile::remove(mUnloadFilename);
  }
}

QString Screenshot::getName(NamingOptions options, QString prefix, QDir directory)
{
  QString naming;
  int naming_largest = 0;

  if (options.flip) {
    naming = "%1" + prefix;
  }
  else {
    naming = prefix + "%1";
  }

  switch (options.naming)
  {
  case Screenshot::Numeric: // Numeric
    // Iterating through the folder to find the largest numeric naming.
    foreach(QString file, directory.entryList(QDir::Files))
    {
      if (file.contains(prefix)) {
        file.chop(file.size() - file.lastIndexOf("."));
        file.remove(prefix);

        if (file.toInt()> naming_largest) {
          naming_largest = file.toInt();
        }
      }
    }

    if (options.leadingZeros > 0) {
       //Pretty, huh?
      QString format;
      QTextStream (&format) << "%0" << (options.leadingZeros+1) << "d";

      naming = naming.arg(QString().sprintf(format.toAscii(), naming_largest + 1));
    }
    else {
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

QString& Screenshot::unloadedFileName()
{
  return mUnloadFilename;
}

Screenshot::Options &Screenshot::options()
{
  return mOptions;
}

QPixmap &Screenshot::pixmap()
{
  if (mUnloaded) {
    // A local reference.. what could go wrong? Nothing! Right guys? Guys?
    QPixmap p;
    p.load(mUnloadFilename);
    return p;
  }

  return mPixmap;
}

//

void Screenshot::confirm(bool result)
{
  qDebug() << "Screenshot::confirm(" << result << ")";

  if (result) {
    save();
  }
  else {
    mOptions.result = Screenshot::Cancel;
    emit finished();
  }

  emit cleanup();

  mPixmap = QPixmap();
}

void Screenshot::confirmation()
{
  qDebug() << "Screenshot: Asking for confirmation.";
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
  QProcess* process = new QProcess(this);

  // Delete the QProcess once it's done.
  connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this   , SLOT(optimizationDone()));
  connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), process, SLOT(deleteLater()));

  QString optiPNG;

#ifdef Q_OS_UNIX
  optiPNG = "optipng";
#else
  optiPNG = qApp->applicationDirPath() + QDir::separator() + "optipng.exe";
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
  }
  else {
    emit finished();
  }
}

void Screenshot::save()
{
  QString name = "";
  QString fileName = "";
  Screenshot::Result result = Screenshot::Fail;

  if (mOptions.file && !mOptions.saveAs)  {
    name = newFileName();
  }
  else if (mOptions.file && mOptions.saveAs) {
    name = QFileDialog::getSaveFileName(0, tr("Save as.."), newFileName(), "*" + extension());
  }

  if (!mOptions.replace && QFile::exists(name+extension())) {
    // Ugly? You should see my wife!
    int count = 0;
    int cunt  = 0;

    QString naming = QFileInfo(name).fileName();

    foreach(QString file, QFileInfo(name+extension()).dir().entryList(QDir::Files)) {
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

    name = name + " (" + QString::number(count+1) + ")";
  }

  if (mOptions.clipboard && !(mOptions.upload && mOptions.imgurClipboard)) {
    qDebug() << "Setting the clipboard pixmap, unloaded:" << mUnloaded;

    if (mUnloaded) {
      mUnloaded = false;
      mPixmap = QPixmap(mUnloadFilename);
    }

    QApplication::clipboard()->setPixmap(mPixmap, QClipboard::Clipboard);

    if (!mOptions.file) {
      result =  (Screenshot::Result)1;
    }
  }

  // In the following code I use  (Screenshot::Result)1 instead of Screenshot::Success because of some weird issue with the X11 headers. //TODO
  if (mOptions.file) {
    fileName = name + extension();

    if (name.isEmpty()) {
      result = Screenshot::Cancel;
    }
    else if (mUnloaded) {
      result = (QFile::rename(mUnloadFilename, fileName)) ? (Screenshot::Result)1: Screenshot::Fail;
    }
    else if (mPixmap.save(fileName, 0, mOptions.quality)) {
      result = (Screenshot::Result)1;
    }
    else {
      result = Screenshot::Fail;
    }

    os::addToRecentDocuments(fileName);
  }

  mOptions.fileName = fileName;
  mOptions.result   = result;

  if (!mOptions.result)
    emit finished();

  if (mOptions.format == Screenshot::PNG && mOptions.optimize && mOptions.file) {
    if (!mOptions.upload) {
      ScreenshotManager::instance()->saveHistory(mOptions.fileName);
    }

    optimize();
  }
  else if (mOptions.upload) {
    upload();
  }
  else if (mOptions.file) {
    ScreenshotManager::instance()->saveHistory(mOptions.fileName);
    emit finished();
  }
  else {
    emit finished();
  }
}

void Screenshot::setPixmap(QPixmap pixmap)
{
  mPixmap = pixmap;

  if (mPixmap.isNull()) {
    emit confirm(false);
  }
  else {
    confirmation();
  }
}

void Screenshot::take()
{
  switch (mOptions.mode)
  {
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

  if (mPixmapDelay)
    return;

  if (mPixmap.isNull()) {
    confirm(false);
  }
  else {
    confirmation();
  }
}

void Screenshot::upload()
{
  if (mOptions.file) {
    Uploader::instance()->upload(mOptions.fileName);
  }
  else if (unloadPixmap()) {
    Uploader::instance()->upload(mUnloadFilename);
  }
  else {
    emit finished();
  }
}

void Screenshot::uploadDone(QString url)
{
  if (mOptions.imgurClipboard && !url.isEmpty())
    QApplication::clipboard()->setText(url, QClipboard::Clipboard);

  qDebug() << "Screenshot: UploadDone: " << url;
  emit finished();
}

//

void Screenshot::activeWindow()
{
#ifdef Q_WS_WIN
  HWND fWindow = GetForegroundWindow();

  if (fWindow == NULL)
    return;

  if (fWindow == GetDesktopWindow()) {
   wholeScreen();
   return;
  }

  mPixmap = os::grabWindow(GetForegroundWindow());
#endif

#if defined(Q_WS_X11)
  Window focus;
  int revert;

  XGetInputFocus(QX11Info::display(), &focus, &revert);

  mPixmap = QPixmap::grabWindow(focus);
#endif
}

QString Screenshot::extension() const
{
  switch (mOptions.format) {
    case Screenshot::PNG:
      return ".png";
      break;
    case Screenshot::BMP:
      return ".bmp";
      break;
    case Screenshot::JPEG:
    default:
      return ".jpg";
      break;
  }
}

void Screenshot::grabDesktop()
{
  QRect geometry;

  if (mOptions.currentMonitor) {
    geometry = qApp->desktop()->screenGeometry(QCursor::pos());
  }
  else {
    geometry = qApp->desktop()->geometry();
  }

  mPixmap = QPixmap::grabWindow(qApp->desktop()->winId(), geometry.x(), geometry.y(), geometry.width(), geometry.height());

  if (mOptions.cursor && !mPixmap.isNull()) {
    QPainter painter(&mPixmap);
    painter.drawPixmap(QCursor::pos(), os::cursor());
  }
}

QString Screenshot::newFileName() const
{
  if (!mOptions.directory.exists()) {
    mOptions.directory.mkpath(mOptions.directory.path());
  }

  QString naming = Screenshot::getName(mOptions.namingOptions, mOptions.prefix, mOptions.directory);
  QString path   = QDir::toNativeSeparators(mOptions.directory.path());

  // Cleanup
  if (path.at(path.size()-1) != QDir::separator() && !path.isEmpty()) {
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

  if (mPixmap.isNull())
    return;

  AreaDialog selector(this);
  int result = selector.exec();

  if (result == QDialog::Accepted) {
    mPixmap = mPixmap.copy(selector.resultRect());
  }
  else {
    mPixmap = QPixmap();
  }
}

void Screenshot::selectedWindow()
{
  WindowPicker* windowPicker = new WindowPicker;
  mPixmapDelay = true;

  connect(windowPicker, SIGNAL(pixmap(QPixmap)), this, SLOT(setPixmap(QPixmap)));
}

bool Screenshot::unloadPixmap()
{
  if (mUnloaded)
    return true;

  // Unloading the pixmap to reduce memory usage during previews
  mUnloadFilename = QDir::tempPath() + QDir::separator() + QString(".lstemp.%1%2").arg(qrand() * qrand() + QDateTime::currentDateTime().toTime_t()).arg(extension());
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


