#ifndef IMAGEUPLOADER_H
#define IMAGEUPLOADER_H

#include <QHash>
#include <QVariant>

class QNetworkReply;
class ImageUploader : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)

public:
    static ImageUploader *factory(const QString &type);

    enum Error {
        FileError,
        NetworkError,
        HostError,
        AuthorizationError,
        CancelError,
        OtherError
    };
    Q_ENUM(Error)

public:
    inline ImageUploader(QObject *parent = 0) : QObject(parent), mProgress(0) {}

    static QVariantHash loadSettings(const QString &uploaderType);
    void loadSettings();

    static void saveSettings(const QString &uploaderType, const QVariantHash &settings);
    void saveSettings();

public slots:
    virtual void upload(const QString &fileName) = 0;
    virtual void cancel() = 0;
    virtual void retry() = 0;
    int progress() const;
    void setProgress(int progress);

signals:
    void uploaded(const QString &fileName, const QString &url, const QString &deleteHash); // TODO: Make last param generic
    void error(ImageUploader::Error errorCode, const QString &errorString, const QString &fileName);
    void progressChanged(int progress);

protected:
    int mProgress;
    QString mUploaderType;
    QVariantHash mSettings;

};

#endif // IMAGEUPLOADER_H
