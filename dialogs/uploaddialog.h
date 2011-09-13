#ifndef UPLOADDIALOG_H
#define UPLOADDIALOG_H

#include <QDialog>
#include <QItemSelection>


namespace Ui {
    class UploadDialog;
}

class QSortFilterProxyModel;
class UploadDialog : public QDialog
{
    Q_OBJECT

public:
  explicit UploadDialog(QWidget *parent = 0);
  ~UploadDialog();

private slots:
  void copy();
  void location();
  void contextMenu(QPoint point);
  void selectionChanged(QItemSelection selected, QItemSelection deselected);
  void open(QModelIndex index);
  void reloadHistory();
  void upload();
  void clear();

protected:
  bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::UploadDialog *ui;
    QSortFilterProxyModel *mFilterModel;
    QString mSelectedScreenshot;
    QModelIndex mContextIndex;
};

#endif // UPLOADDIALOG_H
