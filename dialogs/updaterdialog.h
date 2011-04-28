#ifndef UPDATERDIALOG_H
#define UPDATERDIALOG_H

#include <QProgressDialog>

class UpdaterDialog : public QProgressDialog
{
  Q_OBJECT

public:
  UpdaterDialog();

public slots:
  void updateDone(bool result);

private slots:
  void link(QString url);

};

#endif // UPDATERDIALOG_H
