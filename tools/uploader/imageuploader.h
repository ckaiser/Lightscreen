#ifndef IMAGEUPLOADER_H
#define IMAGEUPLOADER_H

#include <QHash>
#include <QVariant>

class QNetworkReply;
class ImageUploader : public QObject
{
Q_OBJECT

public:
  static ImageUploader* getNewUploader(QString name, QVariantHash &options = QVariantHash());

  enum Error {
    FileError,
    NetworkError,
    HostError,
    CancelError,
    OtherError
  };

public:
  ImageUploader(QVariantHash &options);

public slots:
  virtual void upload(const QString &fileName) = 0;

signals:
  void uploaded(QString, QString, QString);
  void error(Error, QString);
  void progress(int);

protected:
  QVariantHash mOptions;

};

#endif // IMAGEUPLOADER_H
