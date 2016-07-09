#include "imguruploader.h"
#include <QNetworkAccessManager>
#include <QtNetwork>

ImgurUploader::ImgurUploader(const QVariantHash &options) : ImageUploader(options) {}

const QString ImgurUploader::clientId()
{
    return QString("3ebe94c791445c1");
}

const QString ImgurUploader::clientSecret()
{
    return QString("0546b05d6a80b2092dcea86c57b792c9c9faebf0");
}

void ImgurUploader::upload(const QString &fileName)
{
    QFile *file = new QFile(fileName);

    if (!file->open(QIODevice::ReadOnly)) {
        emit error(ImageUploader::FileError, tr("Unable to read screenshot file"), fileName);
        file->deleteLater();
        return;
    }

    QNetworkRequest request(QUrl("https://api.imgur.com/3/image"));
    request.setRawHeader("Authorization", QString("Client-ID %1").arg(clientId()).toLatin1());

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    if (!mOptions.value("anonymous", true).toBool()) {
        request.setRawHeader("Authorization", QByteArray("Bearer ") + mOptions.value("access_token").toByteArray());

        if (!mOptions.value("album").toString().isEmpty()) {
            QHttpPart albumPart;
            albumPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"album\""));
            albumPart.setBody(mOptions.value("album").toByteArray());
            multiPart->append(albumPart);
        }
    }

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QMimeDatabase().mimeTypeForFile(fileName, QMimeDatabase::MatchExtension).name());
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\""));

    imagePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(imagePart);

    QNetworkReply *reply = mOptions.value("networkManager").value<QNetworkAccessManager *>()->post(request, multiPart);
    reply->setProperty("fileName", fileName);
    this->setProperty("fileName", fileName);

    connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this, SLOT(uploadProgress(qint64, qint64)));
    connect(this , SIGNAL(cancelRequest()), reply, SLOT(abort()));
    connect(this , SIGNAL(cancelRequest()), reply, SLOT(deleteLater()));
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
}

void ImgurUploader::retry()
{
    upload(property("fileName").toString());
}

void ImgurUploader::cancel()
{
    emit cancelRequest();
}

void ImgurUploader::finished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    QString fileName = reply->property("fileName").toString();

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            emit error(ImageUploader::CancelError, "", fileName);
        } else if (reply->error() == QNetworkReply::ContentOperationNotPermittedError ||
                   reply->error() == QNetworkReply::AuthenticationRequiredError) {
            emit needAuthRefresh();
        } else {
            emit error(ImageUploader::NetworkError, reply->errorString(), fileName);
        }

        return;
    }

    if (reply->rawHeader("X-RateLimit-Remaining") == "0") {
        emit error(ImageUploader::HostError, tr("Imgur upload limit reached"), fileName);
        return;
    }

    QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

    if (imgurResponse.value("success").toBool() == true && imgurResponse.value("status").toInt() == 200) {
        QJsonObject imageData = imgurResponse.value("data").toObject();

        emit uploaded(fileName, imageData["link"].toString(), imageData["deletehash"].toString());
    } else {
        emit error(ImageUploader::HostError, tr("Imgur error"), fileName);
    }
}

void ImgurUploader::uploadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    float b = (float) bytesReceived / bytesTotal;
    int p = qRound(b * 100);
    setProgress(p);
    emit progressChange(p);
}

