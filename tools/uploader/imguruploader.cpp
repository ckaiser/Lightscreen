#include <tools/uploader/imguruploader.h>
#include <tools/uploader/uploader.h>

#include <QNetworkAccessManager>
#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonObject>

ImgurUploader::ImgurUploader(QObject *parent) : ImageUploader(parent)
{
    mUploaderType = "imgur";
    loadSettings();
}

const QString ImgurUploader::clientId()
{
    return QString("3ebe94c791445c1");
}

const QString ImgurUploader::clientSecret()
{
    return QString("0546b05d6a80b2092dcea86c57b792c9c9faebf0");
}

void ImgurUploader::authorize(const QString &pin, AuthorizationCallback callback)
{
    if (pin.isEmpty()) {
        callback(false);
        return;
    }

    QByteArray parameters;
    parameters.append(QString("client_id=").toUtf8());
    parameters.append(QUrl::toPercentEncoding(clientId()));
    parameters.append(QString("&client_secret=").toUtf8());
    parameters.append(QUrl::toPercentEncoding(clientSecret()));
    parameters.append(QString("&grant_type=pin").toUtf8());
    parameters.append(QString("&pin=").toUtf8());
    parameters.append(QUrl::toPercentEncoding(pin));

    QNetworkRequest request(QUrl("https://api.imgur.com/oauth2/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = Uploader::network()->post(request, parameters);
    authorizationReply(reply, callback);
}

void ImgurUploader::refreshAuthorization(const QString &refresh_token, AuthorizationCallback callback)
{
    if (refresh_token.isEmpty()) {
        callback(false);
        return;
    }

    QByteArray parameters;
    parameters.append(QString("refresh_token=").toUtf8());
    parameters.append(QUrl::toPercentEncoding(refresh_token));
    parameters.append(QString("&client_id=").toUtf8());
    parameters.append(QUrl::toPercentEncoding(clientId()));
    parameters.append(QString("&client_secret=").toUtf8());
    parameters.append(QUrl::toPercentEncoding(clientSecret()));
    parameters.append(QString("&grant_type=refresh_token").toUtf8());

    QNetworkRequest request(QUrl("https://api.imgur.com/oauth2/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = Uploader::network()->post(request, parameters);
    authorizationReply(reply, callback);
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

    if (!mSettings.value("anonymous", true).toBool()) {
        request.setRawHeader("Authorization", QByteArray("Bearer ") + mSettings.value("access_token").toByteArray());

        if (!mSettings.value("album").toString().isEmpty()) {
            QHttpPart albumPart;
            albumPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"album\""));
            albumPart.setBody(mSettings.value("album").toByteArray());
            multiPart->append(albumPart);
        }
    }

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QMimeDatabase().mimeTypeForFile(fileName, QMimeDatabase::MatchExtension).name());
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\""));

    imagePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(imagePart);

    QNetworkReply *reply = Uploader::network()->post(request, multiPart);
    reply->setProperty("fileName", fileName);
    this->setProperty("fileName", fileName);
    multiPart->setParent(reply);

#ifdef Q_OS_WIN
    connect(reply, &QNetworkReply::sslErrors, [reply](const QList<QSslError> &errors) {
        Q_UNUSED(errors);
        if (QSysInfo::WindowsVersion <= QSysInfo::WV_2003) {
            reply->ignoreSslErrors();
        }
    });
#endif

    connect(reply, &QNetworkReply::uploadProgress, this, &ImgurUploader::uploadProgress);
    connect(this , &ImgurUploader::cancelRequest, reply, &QNetworkReply::abort);
    connect(this , &ImgurUploader::cancelRequest, reply, &QNetworkReply::deleteLater);

    connect(reply, &QNetworkReply::finished, this, &ImgurUploader::finished);
}

void ImgurUploader::retry()
{
    loadSettings();
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

            refreshAuthorization(mSettings["refresh_token"].toString(), [&](bool result) {
                if (result) {
                    QTimer::singleShot(50, this, &ImgurUploader::retry);
                } else {
                    cancel();
                    emit error(ImageUploader::AuthorizationError, tr("Imgur user authentication failed"), fileName);
                }
            });
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
}

void ImgurUploader::authorizationReply(QNetworkReply *reply, AuthorizationCallback callback)
{
#ifdef Q_OS_WIN
    connect(reply, &QNetworkReply::sslErrors, [reply](const QList<QSslError> &errors) {
        Q_UNUSED(errors);
        if (QSysInfo::WindowsVersion <= QSysInfo::WV_2003) {
            reply->ignoreSslErrors();
        }
    });
#endif

    connect(reply, &QNetworkReply::finished, [reply, callback] {
        reply->deleteLater();

        bool authorized = false;

        const QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

        if (!imgurResponse.isEmpty() && imgurResponse.contains("access_token")) {
            QVariantHash newSettings;
            newSettings["access_token"]     = imgurResponse.value("access_token").toString();
            newSettings["refresh_token"]    = imgurResponse.value("refresh_token").toString();
            newSettings["account_username"] = imgurResponse.value("account_username").toString();
            newSettings["expires_in"]       = imgurResponse.value("expires_in").toInt();
            ImgurUploader::saveSettings("imgur", newSettings);

            authorized = true;
        }

        callback(authorized);
    });
}

