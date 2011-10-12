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
#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <QObject>
#include <QDir>
#include <QPixmap>

class Screenshot : public QObject
{
  Q_OBJECT

public:
  enum Format
  {
    PNG  = 0,
    JPEG = 1,
    BMP  = 2,
    TIFF = 3
  };

  enum Naming
  {
    Numeric = 0,
    Date = 1,
    Timestamp = 2,
    Empty = 3
  };

  enum Mode
  {
    WholeScreen  = 0,
    ActiveWindow = 1,
    SelectedArea = 2,
    SelectedWindow = 3
  };

  enum Result
  {
    Fail = 0,
    Success = 1,
    Cancel = 2
  };

  struct NamingOptions
  {
    Naming naming;
    bool flip;
    int leadingZeros;
    QString dateFormat;
  };

  struct Options
  {
    QString fileName;
    Result result;

    Format format;
    QString prefix;
    NamingOptions namingOptions;
    QDir directory;
    int mode;
    int quality;

    bool currentMonitor;
    bool clipboard;
    bool file;
    bool preview;
    bool magnify;
    bool cursor;
    bool saveAs;
    bool animations;
    bool replace;
    bool upload;
  };

  Screenshot(QObject *parent, Screenshot::Options options);
  ~Screenshot();

  Screenshot::Options &options();
  QPixmap &pixmap();
  static QString getName(NamingOptions options, QString prefix, QDir directory);

public slots:
  void confirm(bool result = true);
  void confirmation();
  void discard();
  void markUpload();
  void save();
  void setPixmap(QPixmap pixmap);
  void take();

signals:
  void askConfirmation();
  void finished();

private:
  void activeWindow();
  QString extension() const;
  void grabDesktop();
  QString newFileName() const;
  void selectedArea();
  void selectedWindow();
  void wholeScreen();

private:
  Screenshot::Options mOptions;
  QPixmap mPixmap;
  bool mPixmapDelay;
  bool mUnloaded;
  QString mUnloadFilename;

};

#endif // SCREENSHOT_H
