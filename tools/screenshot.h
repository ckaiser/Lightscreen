#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <QObject>
#include <QDir>
#include <QPixmap>

class Screenshot : public QObject
{
  Q_OBJECT

public:
  enum Format
  {
    PNG  = 0,
    JPEG = 1,
    BMP  = 2,
    TIFF = 3
  };

  enum Naming
  {
    Numeric = 0,
    Date = 1,
    Timestamp = 2,
    None = 3
  };

  enum Mode
  {
    WholeScreen  = 0,
    ActiveWindow = 1,
    SelectedArea = 2,
    SelectedWindow = 3
  };

  enum Result
  {
    Fail = 0,
    Success = 1,
    Cancel = 2
  };

  struct NamingOptions
  {
    Naming naming;
    bool flip;
    int leadingZeros;
    QString dateFormat;
  };

  struct Options
  {
    QString fileName;
    Result result;

    Format format;
    QString prefix;
    NamingOptions namingOptions;
    QDir directory;
    int mode;
    int quality;

    bool currentMonitor;
    bool clipboard;
    bool file;
    bool preview;
    bool magnify;
    bool cursor;
    bool saveAs;
    bool animations;
    bool replace;
  };

  Screenshot(QObject *parent, Screenshot::Options options);

  Screenshot::Options options();
  QPixmap &pixmap();
  static QString getName(NamingOptions options, QString prefix, QDir directory);

public slots:
  void take();
  void confirm(bool result = true);
  void discard();
  void save();
  void setPixmap(QPixmap pixmap);

signals:
  void askConfirmation();
  void finished();

private:
  void    activeWindow();
  QString newFileName();
  QString extension();
  void    selectedArea();
  void    selectedWindow();
  void    wholeScreen();
  void    grabDesktop();

private:
  Screenshot::Options mOptions;
  QPixmap mPixmap;
  bool mPixmapDelay;

};

#endif // SCREENSHOT_H
