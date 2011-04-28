#ifndef SCREENSHOTDIALOG_H
#define SCREENSHOTDIALOG_H

#include <QDialog>

struct QPoint;
class Screenshot;
class QScrollArea;
class QLabel;
class ScreenshotDialog : public QDialog
{
public:
    ScreenshotDialog(Screenshot *screenshot, QWidget *parent = 0);

private:
    void zoom(int offset);

protected:
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);

private:
    QScrollArea *mScrollArea;
    QLabel *mLabel;
    QPoint mMousePos;
    QSize mOriginalSize;
};

#endif // SCREENSHOTDIALOG_H
