#ifndef PREVIEWDIALOG_H
#define PREVIEWDIALOG_H

#include <QDialog>

class Screenshot;
class QStackedLayout;
class QPushButton;
class PreviewDialog : public QDialog
{
    Q_OBJECT

public:
  PreviewDialog(QWidget *parent);

  void add(Screenshot* screenshot);
  int count();

  static PreviewDialog *instance();
  static bool isActive();

public slots:
  void setWidth(int w)  { resize(w, height()); }
  void setHeight(int h) { resize(width(), h);  }

signals:
  void acceptAll();
  void rejectAll();

private slots:
  void closePreview();
  void relocate();
  void previous();
  void next();
  void indexChanged(int i);
  void enlargePreview();

protected:
  void closeEvent(QCloseEvent* event);
  void mouseDoubleClickEvent(QMouseEvent *event);
  void timerEvent(QTimerEvent *event);

private:
  static PreviewDialog* mInstance;
  int mSize;
  int mPosition; //0: top left, 1: top right, 2: bottom left, 3: bottom rigth (default)
  int mAutoclose;
  int mAutocloseReset;
  int mAutocloseAction;
  QStackedLayout* mStack;
  QPushButton*    mNextButton;
  QPushButton*    mPrevButton;
};

#endif // PREVIEWDIALOG_H
