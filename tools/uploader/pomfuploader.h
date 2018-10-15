#ifndef POMFUPLOADER_H
#define POMFUPLOADER_H

#include <tools/uploader/imageuploader.h>
#include <functional>

class PomfUploader : public ImageUploader
{
    Q_OBJECT

public:
    PomfUploader(QObject *parent = 0);

    typedef std::function<void(bool)> VerificationCallback;
    static QNetworkReply* verify(const QString &url, VerificationCallback callback);

public slots:
    void upload(const QString &fileName);
    void retry();
    void cancel();

signals:
    void cancelRequest();

};

#endif // POMFUPLOADER_H
