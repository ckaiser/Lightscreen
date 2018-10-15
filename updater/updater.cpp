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
#include <QApplication>
#include <QDate>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QVersionNumber>

#include <QJsonDocument>
#include <QJsonObject>

#include <QSettings>

#include <updater/updater.h>
#include <dialogs/updaterdialog.h>
#include <tools/screenshotmanager.h>
#include <tools/os.h>

Updater::Updater(QObject *parent) :
    QObject(parent)
{
    connect(&mNetwork, &QNetworkAccessManager::finished, this, &Updater::finished);
}

void Updater::check()
{
    QNetworkRequest request(QUrl("https://lightscreen.com.ar/version_telemetry"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject telemetryObject;
    telemetryObject.insert("version", qApp->applicationVersion());
    telemetryObject.insert("manual_check", property("withFeedback").toBool());
    telemetryObject.insert("platform", QJsonObject{
                               {"product_type", QSysInfo::productType()},
                               {"product_version", QSysInfo::productVersion()},
                               {"kernel_type", QSysInfo::kernelType()},
                               {"kernel_version", QSysInfo::kernelVersion()}
                           });

    auto settings = ScreenshotManager::instance()->settings();
    if (settings->value("options/telemetry", false).toBool()) {
        QJsonObject settingsObject;
        const auto keys = settings->allKeys();

        for (const auto& key : qAsConst(keys)) {
            if (key.contains("token") ||
                key.contains("username") ||
                key.contains("album") ||
                key.contains("lastScreenshot") ||
                key.contains("target") ||
                key.contains("geometry")) {
                continue; // Privacy/useless stuff
            }

            settingsObject.insert(key, QJsonValue::fromVariant(settings->value(key)));
        }

        telemetryObject.insert("settings", settingsObject);
    }

    mNetwork.post(request, QJsonDocument(telemetryObject).toJson());
}

void Updater::checkWithFeedback()
{
    UpdaterDialog updaterDialog;
    connect(this, &Updater::done, &updaterDialog, &UpdaterDialog::updateDone);

    setProperty("withFeedback", true);

    check();
    updaterDialog.exec();
}

void Updater::finished(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    auto currentVersion = QVersionNumber::fromString(qApp->applicationVersion()).normalized();
    auto remoteVersion  = QVersionNumber::fromString(QString(data)).normalized();

    emit done(remoteVersion > currentVersion);
}
