#ifndef IMGURUPLOADER_H
#define IMGURUPLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <tools/uploader/imageuploader.h>
#include <functional>

class ImgurUploader : public ImageUploader
{
    Q_OBJECT

public:
    typedef std::function<void(bool)> AuthorizationCallback;

    ImgurUploader(QObject *parent = 0);
    static const QString clientId();
    static const QString clientSecret();
    static void authorize(const QString &pin, AuthorizationCallback callback);
    static void refreshAuthorization(const QString &refresh_token, AuthorizationCallback callback);

public slots:
    void upload(const QString &fileName);
    void retry();
    void cancel();

private slots:
    void finished();
    void uploadProgress(qint64 bytesReceived, qint64 bytesTotal);

signals:
    void cancelRequest();

private:
    static void authorizationReply(QNetworkReply *reply, AuthorizationCallback callback);

};

#endif // IMGURUPLOADER_H
