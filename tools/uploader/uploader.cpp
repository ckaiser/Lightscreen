/*
 * Copyright (C) 2017  Christian Kaiser
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include <tools/uploader/uploader.h>
#include <tools/screenshotmanager.h>
#include <tools/uploader/imguruploader.h>

#include <QtNetwork>
#include <QSettings>

Uploader *Uploader::mInstance = 0;

Uploader::Uploader(QObject *parent) : QObject(parent), mProgress(0)
{
    mNetworkAccessManager = new QNetworkAccessManager(this);
}

Uploader *Uploader::instance()
{
    if (!mInstance) {
        mInstance = new Uploader();
    }

    return mInstance;
}

QNetworkAccessManager *Uploader::network()
{
    return instance()->mNetworkAccessManager;
}

int Uploader::progress() const
{
    return mProgress;
}

QString Uploader::serviceName(int index)
{ // TODO: Move somewhere else? Use indexes everywhere? an enum?
    switch (index) {
        case 1:
            return "pomf";
        case 0:
        default:
            return "imgur";
    }
}

QString Uploader::lastUrl() const
{
    return mLastUrl;
}

void Uploader::cancel()
{
    mUploaders.clear();
    mProgress = 0;
    emit cancelAll();
}

void Uploader::upload(const QString &fileName, const QString &uploadService)
{
    if (fileName.isEmpty()) {
        return;
    }

    auto uploader = ImageUploader::factory(uploadService);

    connect(uploader, &ImageUploader::progressChanged, this    , &Uploader::reportProgress);
    connect(this    , &Uploader::cancelAll           , uploader, &ImageUploader::cancel);

    connect(uploader, &ImageUploader::error, [&, uploader](ImageUploader::Error errorCode, const QString &errorString, const QString &fileName) {
        mUploaders.removeAll(uploader);
        uploader->deleteLater();

        mProgress = 0; // TODO: ?

        if (errorCode != ImageUploader::CancelError) {
            if (errorString.isEmpty()) {
                emit error(tr("Upload Error %1").arg(errorCode));
            } else {
                emit error(errorString);
            }
        }

        emit done(fileName, "", "");
    });

    connect(uploader, &ImageUploader::uploaded, [&, uploader](const QString &file, const QString &url, const QString &deleteHash) {
        mLastUrl = url;
        mUploaders.removeAll(uploader);

        if (mUploaders.isEmpty()) {
            mProgress = 0;
        }

        uploader->deleteLater();
        emit done(file, url, deleteHash);
    });

    mUploaders.append(uploader);
    uploader->upload(fileName);
}

int Uploader::uploading() const
{
    return mUploaders.count();
}

void Uploader::reportProgress(int progress)
{
    if (mUploaders.size() <= 0) {
        mProgress = progress;
        emit progressChanged(progress);
        return;
    }

    int totalProgress = 0;

    for (int i = 0; i < mUploaders.size(); ++i) {
        totalProgress += mUploaders[i]->progress();
    }

    mProgress = totalProgress / mUploaders.size();
    emit progressChanged(mProgress);
}

