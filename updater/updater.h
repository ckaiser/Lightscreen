#ifndef UPDATER_H_
#define UPDATER_H_

#include <QObject>
#include <QHttp>

class Updater : public QObject
{
  Q_OBJECT
public:
  Updater(QObject *parent = 0);

  static Updater *instance();

signals:
  void done(bool result);

public slots:
  void checkWithFeedback();
  void check();

private slots:
  void httpDone(bool result);

private:
  QHttp mHttp;
  static Updater* mInstance;

};

#endif /*UPDATER_H_*/
