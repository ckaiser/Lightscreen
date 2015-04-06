#include "imguruploader.h"
#include <QNetworkAccessManager>
#include <QtNetwork>

ImgurUploader::ImgurUploader(QVariantHash &options, QString fileName) : ImageUploader(options)
{
  upload(fileName);
}

void ImgurUploader::upload(const QString &fileName)
{
  QFile file(fileName);

  if (!file.open(QIODevice::ReadOnly)) {
    emit error(ImageUploader::FileError, fileName);
    return;
  }

  QByteArray data;
  data.append(QString("key=").toUtf8());
  data.append(QUrl::toPercentEncoding(mOptions.value("APIKey", "6920a141451d125b3e1357ce0e432409").toString()));
  data.append(QString("&image=").toUtf8());
  data.append(QUrl::toPercentEncoding(file.readAll().toBase64()));
  file.close();

  QNetworkRequest request(QUrl("http://api.imgur.com/2/upload"));
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  QNetworkReply *reply = mOptions.value("networkManager").value<QNetworkAccessManager*>()->post(request, data);
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
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  QString fileName = reply->property("fileName").toString();
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    if (reply->error() == QNetworkReply::OperationCanceledError) {
      emit error(ImageUploader::CancelError, fileName);
    }
    else {
      emit error(ImageUploader::NetworkError, fileName);
    }

    return;
  }

  if (reply->rawHeader("X-RateLimit-Remaining") == "0") {
    emit error(ImageUploader::HostError, fileName);
    return;
  }

  QXmlStreamReader reader(reply->readAll());

  QString url_option = "imgur_page";

  if (mOptions.value("directUrl", false).toBool())
      url_option = "original";

  QString url;
  QString deleteHash;

  bool hasError = false;

  while (!reader.atEnd()) {
    reader.readNext();

    if (reader.isStartElement()) {
      if (reader.name() == "error") {
        hasError = true;
      }

      if (reader.name() == "deletehash") {
        deleteHash = reader.readElementText();
      }

      if (reader.name() == url_option) {
        url = reader.readElementText();
      }
    }
  }

  if (deleteHash.isEmpty() || url.isEmpty() || hasError) {
    emit error(ImageUploader::HostError, fileName);
  }
  else {
    emit uploaded(fileName, url, deleteHash);
  }
}

void ImgurUploader::uploadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
  float b = (float) bytesReceived/bytesTotal;
  int p = qRound(b*100);
  setProgress(p);
  emit progressChange(p);
}

