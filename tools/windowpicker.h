#ifndef WINDOWPICKER_H
#define WINDOWPICKER_H

#include <QWidget>

class QLabel;
class QRect;
class QRubberBand;
class WindowPicker : public QWidget
{
  Q_OBJECT

public:
    WindowPicker();
    ~WindowPicker();

signals:
    void pixmap(QPixmap pixmap);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void closeEvent(QCloseEvent*);

private:
    void cancel();

private:
    QPixmap mCrosshair;
    QLabel *mWindowLabel;
    QLabel *mWindowIcon;
    QLabel *mCrosshairLabel;
    QRubberBand *mWindowIndicator;
    HWND mCurrentWindow;
    bool mTaken;

};

#endif // WINDOWPICKER_H
