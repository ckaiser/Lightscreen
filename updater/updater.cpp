#include <QDate>
#include <QHttp>
#include <QApplication>

#include "updater.h"
#include "../dialogs/updaterdialog.cpp"

Updater::Updater(QObject *parent) :
  QObject(parent)
{
  connect(&mHttp, SIGNAL(done(bool)), this, SLOT(httpDone(bool)));
}

void Updater::check()
{
  if (mHttp.hasPendingRequests())
    return;

  mHttp.setHost("lightscreen.sourceforge.net");
  mHttp.get("/version");
}

void Updater::checkWithFeedback()
{
  UpdaterDialog updaterDialog;
  connect(this, SIGNAL(done(bool)), &updaterDialog, SLOT(updateDone(bool)));

  check();
  updaterDialog.exec();
}

void Updater::httpDone(bool result)
{
  if (result) {
    QByteArray data = mHttp.readAll();
    double version  = QString(data).toDouble();

    emit done((version > qApp->applicationVersion().toDouble()));
  }
  else {
    emit done(false);
  }
}

Updater* Updater::mInstance = 0;

Updater *Updater::instance()
{
  if (!mInstance)
    mInstance = new Updater();

  return mInstance;
}
