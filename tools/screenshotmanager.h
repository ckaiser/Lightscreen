#ifndef SCREENSHOTMANAGER_H
#define SCREENSHOTMANAGER_H

#include <QObject>
#include <QList>

#include "screenshot.h"

class QSettings;
class ScreenshotManager : public QObject
{
  Q_OBJECT

public:
  enum State
  {
    Idle  = 0,
    Busy = 1,
    Disabled  = 2
  };

public:
  ScreenshotManager(QObject *parent);
  ~ScreenshotManager();
  static ScreenshotManager *instance();

  void setCount(const unsigned int c){ mCount = c; }
  unsigned int count() const { return mCount; }
  QSettings *settings() const { return mSettings; }

public slots:
  void take(Screenshot::Options options);
  void askConfirmation();
  void cleanup();

signals:
  void confirm(Screenshot* screenshot);
  void windowCleanup(Screenshot::Options options);

private:
  static ScreenshotManager* mInstance;
  QSettings *mSettings;
  int mCount; // Concurrent screenshot count.

};

#endif // SCREENSHOTMANAGER_H
