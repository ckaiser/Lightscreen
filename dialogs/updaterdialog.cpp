#include <QApplication>
#include <QDesktopServices>
#include <QProgressBar>
#include <QLabel>
#include <QLayout>
#include <QUrl>

#include "updaterdialog.h"
#include "../tools/os.h"

UpdaterDialog::UpdaterDialog() :
QProgressDialog("", tr("Cancel"), 0, 0)
{
  setWindowTitle(tr("Updater - Lightscreen"));
  setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);
  setAutoClose(false);

  QProgressBar *bar = new QProgressBar(this);
  bar->setTextVisible(false);
  bar->setRange(0, 0);

  QLabel *label = new QLabel(tr("Checking for updates..."), this);
  connect(label, SIGNAL(linkActivated(QString)), this, SLOT(link(QString)));

  setLabel(label);
  setBar(bar);
}

void UpdaterDialog::updateDone(bool result)
{
  if (result) {
    setLabelText(tr("There's a new version available,<br> please see <a href=\"http://lightscreen.sourceforge.net/whatsnew/%1\">the Lighscreen website</a>.").arg(qApp->applicationVersion()));
  }
  else {
    setLabelText(tr("No new versions available."));
  }

  setMaximum(1);

  setCancelButtonText(tr("Close"));
}

void UpdaterDialog::link(QString url)
{
  QDesktopServices::openUrl(url);
}

