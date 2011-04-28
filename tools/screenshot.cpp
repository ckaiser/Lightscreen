#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

#if defined(Q_WS_WIN)
  #include <windows.h>
#endif

#include "../dialogs/AreaDialog.h"
#include "windowpicker.h"
#include "screenshot.h"

#include "os.h"

Screenshot::Screenshot(QObject *parent, Screenshot::Options options): QObject(parent), mOptions(options), mPixmapDelay(false) {
}

Screenshot::Options Screenshot::options()
{
  return mOptions;
}

QPixmap &Screenshot::pixmap()
{
  return mPixmap;
}

void Screenshot::activeWindow()
{
#if defined(Q_WS_WIN)
  HWND fWindow = GetForegroundWindow();

  if (fWindow == NULL)
    return;

  if (fWindow == GetDesktopWindow()) {
   wholeScreen();
   return;
  }

  mPixmap = os::grabWindow(GetForegroundWindow());
#else
  wholeScreen(); //TODO
#endif
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
    naming = naming.arg(QDateTime::currentDateTime().toString(options.dateFormat));
    break;
  case  Screenshot::Timestamp: // Timestamp
    naming = naming.arg(QDateTime::currentDateTime().toTime_t());
    break;
  case  Screenshot::None:
    naming = naming.arg("");
    break;
  }

  return naming;
}

QString Screenshot::newFileName()
{
  if (!mOptions.directory.exists())
    mOptions.directory.mkpath(mOptions.directory.path());

  QString naming = Screenshot::getName(mOptions.namingOptions, mOptions.prefix, mOptions.directory);

  QString fileName;

  QString path = QDir::toNativeSeparators(mOptions.directory.path());

  // Cleanup
  if (path.at(path.size()-1) != QDir::separator() && !path.isEmpty())
    path.append(QDir::separator());

  fileName.append(path);
  fileName.append(naming);

  return fileName;
}

QString Screenshot::extension()
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

void Screenshot::selectedArea()
{
  static bool alreadySelecting = false; // Prevents multiple AreaDialog instances -- [Is this even possible anymore?]

  if (alreadySelecting)
    return;

  alreadySelecting = true;

  grabDesktop();

  if (mPixmap.isNull())
    return;

  AreaDialog selector(this);
  int result = selector.exec();

  alreadySelecting = false;

  if (result == QDialog::Accepted) {
    mPixmap = mPixmap.copy(selector.resultRect());
  }
  else {
    mPixmap = QPixmap();
  }
}


void Screenshot::selectedWindow()
{
#if defined(Q_WS_WIN)
  WindowPicker* windowPicker = new WindowPicker;
  mPixmapDelay = true;

  connect(windowPicker, SIGNAL(pixmap(QPixmap)), this, SLOT(setPixmap(QPixmap)));
#else
  wholeScreen();
#endif
}

void Screenshot::wholeScreen()
{
  grabDesktop();
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
    emit askConfirmation();
  }
}

void Screenshot::confirm(bool result)
{
  if (result) {
    save();
  }
  else {
    mOptions.result = Screenshot::Cancel;
  }

  mPixmap = QPixmap(); // Cleanup just in case.
  emit finished();
}

void Screenshot::discard()
{
  confirm(false);
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

  fileName = name + extension();

  if (fileName.isEmpty()) {
    result = Screenshot::Fail;
  }
  else if (mPixmap.save(fileName, 0, mOptions.quality)) {
    result = Screenshot::Success;
  }

  if (mOptions.file) { // Windows only
    os::addToRecentDocuments(fileName);
  }

  if (mOptions.clipboard) {
    QApplication::clipboard()->setPixmap(mPixmap, QClipboard::Clipboard);

    if (!mOptions.file) {
      result = Screenshot::Success;
    }
  }

  mPixmap = QPixmap();
  mOptions.fileName = fileName;
  mOptions.result   = result;
}

void Screenshot::setPixmap(QPixmap pixmap)
{
  mPixmap = pixmap;
  pixmap = QPixmap(); // ??

  if (mPixmap.isNull()) {
    emit confirm(false);
  }
  else {
    emit askConfirmation();
  }
}
