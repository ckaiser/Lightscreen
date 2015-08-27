#ifndef IMAGEUPLOADER_H
#define IMAGEUPLOADER_H

#include <QHash>
#include <QVariant>

class QNetworkReply;
class ImageUploader : public QObject
{
Q_OBJECT

public:
  static ImageUploader* getNewUploader(QString name, QVariantHash options = QVariantHash());

  enum Error {
    FileError,
    NetworkError,
    HostError,
    CancelError,
    OtherError
  };

public:
  ImageUploader(QVariantHash &options);
  QVariantHash &options() { return mOptions; }

public slots:
  virtual void upload(const QString &fileName) = 0;
  virtual void cancel() = 0;
  virtual void retry() = 0;
  int progress() const { return mProgress; }
  void setProgress(int progress) { mProgress = progress; }

signals:
  void uploaded(QString, QString, QString);
  void error(Error, QString, QString);
  void progressChange(int);

protected:
  QVariantHash mOptions;
  int mProgress;

};

#endif // IMAGEUPLOADER_H
