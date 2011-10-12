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
#ifndef UPLOADER_H
#define UPLOADER_H

#include <QObject>
#include <QList>
#include <QPair>
#include "qtimgur.h"

class Uploader : public QObject
{
  Q_OBJECT

public:
  Uploader(QObject *parent = 0);
  static Uploader* instance();
  QString lastUrl() const;
  QList< QPair<QString, QString> > &screenshots() { return mScreenshots; }

public slots:
  void cancel(const QString &fileName);
  void imgurError(const QString &file, const QtImgur::Error e);
  void upload(const QString &fileName);
  void uploaded(const QString &fileName, const QString &url);
  int  uploading();

signals:
  void done(QString, QString);
  void error(QString);
  void progress(qint64, qint64);

private:
  static Uploader* mInstance;

  // Filename, url
  QList< QPair<QString, QString> > mScreenshots;
  QtImgur *mImgur;
  QtImgur::Error mLastError;

  int mUploading;
};

#endif // UPLOADER_H
