#ifndef UPLOADDIALOG_H
#define UPLOADDIALOG_H

#include <QDialog>
#include <QItemSelection>

namespace Ui {
    class HistoryDialog;
}

class QSqlTableModel;
class QSortFilterProxyModel;
class HistoryDialog : public QDialog
{
    Q_OBJECT

public:
  explicit HistoryDialog(QWidget *parent = 0);
  ~HistoryDialog();

private slots:
  void clear();
  void contextMenu(QPoint point);
  void copy();
  void deleteImage();
  void location();
  void removeHistoryEntry();
  void refresh();
  void open(QModelIndex index);
  void selectionChanged(QItemSelection selected, QItemSelection deselected);
  void upload();
  void uploadProgress(int progress);

protected:
  bool eventFilter(QObject *object, QEvent *event);
  bool event(QEvent *event);

private:
  Ui::HistoryDialog *ui;
  QSqlTableModel *mModel;
  QSortFilterProxyModel *mFilterModel;
  QString     mSelectedScreenshot;
  QModelIndex mContextIndex;
};

#endif // UPLOADDIALOG_H
