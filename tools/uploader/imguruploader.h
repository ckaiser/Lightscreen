#ifndef IMGURUPLOADER_H
#define IMGURUPLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include "imageuploader.h"

class ImgurUploader : public ImageUploader
{
Q_OBJECT

public:
  ImgurUploader(QVariantHash &options);

public slots:
  void upload(const QString &fileName);

private slots:
  void finished();

};

#endif // IMGURUPLOADER_H
