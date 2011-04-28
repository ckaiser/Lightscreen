#include <QApplication>
#include <QDesktopWidget>
#include <QLocale>

#include "tools/os.h"
#include <QtSingleApplication>
#include "lightscreenwindow.h"

int main(int argc, char *argv[])
{
  QtSingleApplication application(argc, argv);
  application.setOrganizationName("K");
  application.setApplicationName ("Lightscreen");
  application.setApplicationVersion("2.0");
  application.setQuitOnLastWindowClosed(false);

  if (application.isRunning()) {
    application.sendMessage("-wake");
    return 0;
  }

  LightscreenWindow lightscreen;

  if (application.arguments().size() > 1) {
    lightscreen.messageReceived(application.arguments().at(1));
  }
  else {
    lightscreen.show();
  }

  QObject::connect(&application, SIGNAL(messageReceived(const QString&)), &lightscreen, SLOT(messageReceived(const QString&)));
  QObject::connect(&lightscreen, SIGNAL(finished(int)), &application, SLOT(quit()));

  return application.exec();
}
