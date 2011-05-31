#include "uploader.h"
#include "qtimgur.h"

#include <QDebug>

Uploader::Uploader(QObject *parent) : QObject(parent)
{
  mImgur = new QtImgur("6920a141451d125b3e1357ce0e432409", this);
  connect(mImgur, SIGNAL(uploaded(QString, QString)), this, SLOT(uploaded(QString, QString)));
  connect(mImgur, SIGNAL(error(QtImgur::Error)), this, SLOT(imgurError(QtImgur::Error)));
}

void Uploader::upload(QString fileName)
{
  qDebug() << "Uplader: upload me this: " << fileName;
  if (!fileName.isEmpty()) {
    mImgur->upload(fileName);
    mUploading++;
  }
}

void Uploader::uploaded(QString file, QString url)
{
  qDebug() << "Uploader: uploaded(" << file << ", " << url << ")";

  mUploading--;
  mScreenshots.insert(file, url);
  emit done(file, url);
}

int Uploader::uploading()
{
  return mUploading;
}

void Uploader::imgurError(QtImgur::Error e)
{
  mUploading--;
  emit error();
}

QString Uploader::lastUrl()
{
  QMapIterator<QString, QString> i(mScreenshots);
  i.toBack();

  QString url;

  while (i.hasPrevious()) {
    url = i.previous().value();

    if (!url.isEmpty()) {
      qDebug() << "Uploader: lastUrl is " << url;
      return url;
    }
  }

  qDebug() << "Uploader: no go on lastUrl";
  return url;
}

// Singleton
Uploader* Uploader::mInstance = 0;

Uploader *Uploader::instance()
{
  if (!mInstance)
    mInstance = new Uploader();

  return mInstance;
}
