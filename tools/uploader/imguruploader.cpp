#include "imguruploader.h"
#include <QNetworkAccessManager>
#include <QtNetwork>

ImgurUploader::ImgurUploader(QVariantHash &options) : ImageUploader(options) {}

void ImgurUploader::upload(const QString &fileName)
{

  if (mOptions.value("anonymous", true).toBool()) {
    uploadAnonymous(fileName);
  }
  else {
    //uploadOAuth(fileName);
  }
}

void ImgurUploader::uploadAnonymous(const QString &fileName)
{
  QFile *file = new QFile(fileName);

  if (!file->open(QIODevice::ReadOnly)) {
    emit error(ImageUploader::FileError, fileName);
    file->deleteLater();
    return;
  }

  QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

  QHttpPart imagePart;
  imagePart.setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
  imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\""));

  imagePart.setBodyDevice(file);
  file->setParent(multiPart);
  multiPart->append(imagePart);

  QNetworkRequest request(QUrl("https://api.imgur.com/3/upload"));
  request.setRawHeader("Authorization", "Client-ID 3ebe94c791445c1");

  QNetworkReply *reply = mOptions.value("networkManager").value<QNetworkAccessManager*>()->post(request, multiPart);

  reply->setProperty("fileName", fileName);
  this->setProperty("fileName", fileName);

  connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(uploadProgress(qint64, qint64)));
  connect(this , SIGNAL(cancelRequest()), reply, SLOT(abort()));
  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(networkError(QNetworkReply::NetworkError)));
  connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void ImgurUploader::cancel()
{
  emit cancelRequest();
  deleteLater();
}

void ImgurUploader::networkError(QNetworkReply::NetworkError code)
{
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  QString fileName = reply->property("fileName").toString();
  reply->deleteLater();

  switch (code)
  {
  case QNetworkReply::OperationCanceledError:
      emit error(ImageUploader::CancelError, fileName);
      break;
  default:
    emit error(ImageUploader::NetworkError, fileName);
  }
}

void ImgurUploader::finished()
{
  qDebug() << "ImgurUploader::finished()";

  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  QString fileName = reply->property("fileName").toString();

  if (reply->error() != QNetworkReply::NoError) {
    if (reply->error() == QNetworkReply::OperationCanceledError) {
      emit error(ImageUploader::CancelError, fileName);
    }
    else {
      qDebug() << reply->errorString();

      emit error(ImageUploader::NetworkError, fileName);
    }

    return;
  }

  if (reply->rawHeader("X-RateLimit-Remaining") == "0") {
    emit error(ImageUploader::HostError, fileName);
    return;
  }

  QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

  if (imgurResponse.value("success").toBool() == true) {
    QJsonObject imageData = imgurResponse.value("data").toObject();

    emit uploaded(fileName, imageData["link"].toString(), imageData["deletehash"].toString());
  }
  else {
    emit error(ImageUploader::HostError, fileName);
  }

  reply->deleteLater();
}

void ImgurUploader::uploadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
  float b = (float) bytesReceived/bytesTotal;
  int p = qRound(b*100);
  setProgress(p);
  emit progressChange(p);
}

