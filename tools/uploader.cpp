#include "uploader.h"
#include "qtimgur.h"

#include <QDebug>

Uploader::Uploader(QObject *parent) : QObject(parent)
{
  mImgur = new QtImgur("6920a141451d125b3e1357ce0e432409", this);
  connect(mImgur, SIGNAL(uploaded(QString, QString)), this, SLOT(uploaded(QString, QString)));
  connect(mImgur, SIGNAL(error(QString, QtImgur::Error)), this, SLOT(imgurError(QString, QtImgur::Error)));
}

void Uploader::upload(QString fileName)
{
  if (!fileName.isEmpty() && !mScreenshots.contains(fileName)) {
    mImgur->upload(fileName);
    mScreenshots.insert(fileName, tr("Uploading..."));
    mUploading++;
  }
}

void Uploader::uploaded(QString file, QString url)
{
  mScreenshots[file] = url;
  mUploading--;
  emit done(file, url);
}

int Uploader::uploading()
{
  return mUploading;
}

void Uploader::imgurError(QString file, QtImgur::Error e)
{
  mUploading--;
  mScreenshots.remove(file);

  if (e == mLastError) {
    // Fail silently? Really? FINE
    return;
  }

  QString errorString;

  switch (e) {
    case QtImgur::ErrorFile:
      errorString = tr("Screenshot file not found.");
    break;
    case QtImgur::ErrorNetwork:
      errorString = tr("Could not reach imgur.com");
    break;
    case QtImgur::ErrorCredits:
      errorString = tr("You have exceeded your upload quota.");
    break;
    case QtImgur::ErrorUpload:
      errorString = tr("Upload failed.");
    break;
  }

  mLastError = e;
  emit error(errorString);
}

QString Uploader::lastUrl()
{
  QMapIterator<QString, QString> i(mScreenshots);
  i.toBack();

  QString url;

  while (i.hasPrevious()) {
    url = i.previous().value();

    if (!url.contains(tr("Uploading..."))) {
      return url;
    }
  }

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
