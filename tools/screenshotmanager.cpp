#include "screenshotmanager.h"
#include "screenshot.h"

#include <QSettings>
#include <QDebug>

ScreenshotManager::ScreenshotManager(QObject *parent = 0) : QObject(parent), mCount(0)
{
  mSettings = new QSettings();
}

ScreenshotManager::~ScreenshotManager()
{
  delete mSettings;
}

void ScreenshotManager::take(Screenshot::Options options)
{
  Screenshot* newScreenshot = new Screenshot(this, options);

  connect(newScreenshot, SIGNAL(askConfirmation()), this, SLOT(askConfirmation()));
  connect(newScreenshot, SIGNAL(finished())       , this, SLOT(cleanup()));

  newScreenshot->take();
}

void ScreenshotManager::askConfirmation()
{
  Screenshot* s = qobject_cast<Screenshot*>(sender());
  emit confirm(s);
}

void ScreenshotManager::cleanup()
{
  Screenshot* s = qobject_cast<Screenshot*>(sender());
  emit windowCleanup(s->options());
  s->deleteLater();
}

// Singleton
ScreenshotManager* ScreenshotManager::mInstance = 0;

ScreenshotManager *ScreenshotManager::instance()
{
  if (!mInstance)
    mInstance = new ScreenshotManager();

  return mInstance;
}
