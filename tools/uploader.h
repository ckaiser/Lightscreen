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
  QString lastUrl();
  QList< QPair<QString, QString> > &screenshots() { return mScreenshots; }

public slots:
  void upload(QString fileName);
  void uploaded(QString fileName, QString url);
  int  uploading();
  void imgurError(QString file, QtImgur::Error e);

signals:
  void done(QString, QString);
  void error(QString);

private:
  static Uploader* mInstance;

  // Filename, url
  QList< QPair<QString, QString> > mScreenshots;
  QtImgur *mImgur;
  QtImgur::Error mLastError;

  int mUploading;

};

#endif // UPLOADER_H
