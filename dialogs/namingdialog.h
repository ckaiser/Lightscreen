#ifndef NAMINGDIALOG_H
#define NAMINGDIALOG_H

#include "ui_namingdialog.h"
#include "tools/screenshot.h"

#include <QUrl>

class NamingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NamingDialog(Screenshot::Naming naming, QWidget *parent = 0);

protected:
  bool eventFilter(QObject *object, QEvent *event);

private slots:
  void openUrl(QString url);
  void saveSettings();

private:
    Ui::NamingDialog ui;
};

#endif // NAMINGDIALOG_H
