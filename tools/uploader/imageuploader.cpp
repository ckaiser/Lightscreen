#include <tools/uploader/imageuploader.h>
#include <tools/uploader/imguruploader.h>
#include <tools/uploader/pomfuploader.h>

#include <QSettings>
#include <tools/screenshotmanager.h>

ImageUploader *ImageUploader::factory(const QString &name)
{
    if (name == "imgur") {
        return new ImgurUploader;
    } else if (name == "pomf") {
        return new PomfUploader;
    }

    return nullptr;
}

QVariantHash ImageUploader::loadSettings(const QString &uploaderType)
{
    auto globalSettings = ScreenshotManager::instance()->settings();
    globalSettings->beginGroup("upload/" + uploaderType);
    auto keys = globalSettings->childKeys();

    QVariantHash settings;

    for (auto key : qAsConst(keys)) {
        settings[key] = globalSettings->value(key);
    }

    globalSettings->endGroup();
    return settings;
}

void ImageUploader::loadSettings()
{
    mSettings = loadSettings(mUploaderType);
}

void ImageUploader::saveSettings(const QString &uploaderType, const QVariantHash &settings) {
    auto globalSettings = ScreenshotManager::instance()->settings();
    globalSettings->beginGroup("upload/" + uploaderType);

    for (auto key : settings.keys()) {
        globalSettings->setValue(key, settings[key]);
    }

    globalSettings->endGroup();
}

void ImageUploader::saveSettings()
{
    saveSettings(mUploaderType, mSettings);
}

int ImageUploader::progress() const {
    return mProgress;
}

void ImageUploader::setProgress(int progress)
{
    if (mProgress != progress) {
        mProgress = progress;
        emit progressChanged(mProgress);
    }
}
