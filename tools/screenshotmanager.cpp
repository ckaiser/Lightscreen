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
#include <tools/screenshotmanager.h>
#include <tools/screenshot.h>
#include <tools/uploader/uploader.h>
#include <tools/os.h>

#include <QApplication>
#include <QDateTime>
#include <QStandardPaths>
#include <QFile>
#include <QSettings>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>

ScreenshotManager::ScreenshotManager(QObject *parent) : QObject(parent), mHistoryInitialized(false)
{
    if (QFile::exists(qApp->applicationDirPath() + QDir::separator() + "config.ini")) {
        mSettings     = new QSettings(qApp->applicationDirPath() + QDir::separator() + "config.ini", QSettings::IniFormat, this);
        mPortableMode = true;
        mHistoryPath  = qApp->applicationDirPath() + QDir::separator();
    } else {
        mSettings     = new QSettings(this);
        mPortableMode = false;
        mHistoryPath  = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QDir::separator();
    }

    connect(Uploader::instance(), &Uploader::done, this, &ScreenshotManager::uploadDone);
}

void ScreenshotManager::initHistory()
{
    if (mHistoryInitialized) {
        return;
    }

    // Creating the SQLite database.
    QSqlDatabase history = QSqlDatabase::addDatabase("QSQLITE");

    QDir historyPath(mHistoryPath);

    if (!historyPath.exists()) {
        historyPath.mkpath(mHistoryPath);
    }

    history.setHostName("localhost");
    history.setDatabaseName(mHistoryPath + "history.sqlite");

    if (history.open()) {
        QSqlQuery tableQuery;
        mHistoryInitialized = tableQuery.exec("CREATE TABLE IF NOT EXISTS history (fileName text, URL text, deleteURL text, time integer)");

        history.exec("CREATE INDEX IF NOT EXISTS fileName_index ON history(fileName)");
    } else {
        qCritical() << "Could not open SQLite DB.";
        mHistoryInitialized = false;
    }
}

int ScreenshotManager::activeCount() const
{
    return mScreenshots.count();
}

bool ScreenshotManager::portableMode()
{
    return mPortableMode;
}

void ScreenshotManager::saveHistory(const QString &fileName, const QString &url, const QString &deleteHash)
{
    if (!mSettings->value("/options/history", true).toBool()) {
        return;
    }

    if (!mHistoryInitialized) {
        initHistory();
    }

    QString deleteUrl;

    if (!deleteHash.isEmpty()) {
        deleteUrl = "https://imgur.com/delete/" + deleteHash;
    }

    QSqlQuery saveHistoryQuery;
    saveHistoryQuery.prepare("INSERT INTO history (fileName, URL, deleteURL, time) VALUES(?, ?, ?, ?)");
    saveHistoryQuery.addBindValue(fileName);
    saveHistoryQuery.addBindValue(url);
    saveHistoryQuery.addBindValue(deleteUrl);
    saveHistoryQuery.addBindValue(QDateTime::currentMSecsSinceEpoch());
    saveHistoryQuery.exec();
}

void ScreenshotManager::updateHistory(const QString &fileName, const QString &url, const QString &deleteHash)
{
    if (!mSettings->value("/options/history", true).toBool() || url.isEmpty()) {
        return;
    }

    if (!mHistoryInitialized) {
        initHistory();
    }

    QSqlQuery query;
    query.prepare("SELECT fileName FROM history WHERE URL IS NOT EMPTY AND fileName = ?");
    query.addBindValue(fileName);
    query.exec();

    if (query.record().count() > 0) {
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE history SET URL = ?, deleteURL = ?, time = ? WHERE fileName = ?");
        updateQuery.addBindValue(url);
        updateQuery.addBindValue("https://imgur.com/delete/" + deleteHash);
        updateQuery.addBindValue(QDateTime::currentMSecsSinceEpoch());
        updateQuery.addBindValue(fileName);

        updateQuery.exec();
    } else {
        saveHistory(fileName, url, deleteHash);
    }
}

void ScreenshotManager::removeHistory(const QString &fileName, qint64 time)
{
    if (!mHistoryInitialized) {
        initHistory();
    }

    QSqlQuery removeQuery;
    removeQuery.prepare("DELETE FROM history WHERE fileName = ? AND time = ?");
    removeQuery.addBindValue(fileName);
    removeQuery.addBindValue(time);

    removeQuery.exec();
}

void ScreenshotManager::clearHistory()
{
    if (!mHistoryInitialized) {
        initHistory();
    }

    QSqlQuery deleteQuery("DELETE FROM history");
    deleteQuery.exec();

    QSqlQuery vacQuery("VACUUM");
    vacQuery.exec();
}

//

void ScreenshotManager::askConfirmation()
{
    Screenshot *s = qobject_cast<Screenshot *>(sender());
    emit confirm(s);
}

void ScreenshotManager::cleanup()
{
    Screenshot *screenshot = qobject_cast<Screenshot *>(sender());
    emit windowCleanup(screenshot->options());
}

void ScreenshotManager::finished()
{
    Screenshot *screenshot = qobject_cast<Screenshot *>(sender());
    mScreenshots.removeOne(screenshot);
    emit activeCountChange();
    screenshot->deleteLater();
}

void ScreenshotManager::take(Screenshot::Options &options)
{
    Screenshot *newScreenshot = new Screenshot(this, options);
    mScreenshots.append(newScreenshot);

    connect(newScreenshot, &Screenshot::askConfirmation, this, &ScreenshotManager::askConfirmation);
    connect(newScreenshot, &Screenshot::cleanup        , this, &ScreenshotManager::cleanup);
    connect(newScreenshot, &Screenshot::finished       , this, &ScreenshotManager::finished);

    newScreenshot->take();
}

void ScreenshotManager::uploadDone(const QString &fileName, const QString &url, const QString &deleteHash)
{
    for (Screenshot *screenshot : qAsConst(mScreenshots)) {
        if (screenshot->options().fileName == fileName
                || screenshot->unloadedFileName() == fileName) {
            screenshot->uploadDone(url);

            if (screenshot->options().file) {
                updateHistory(fileName, url, deleteHash);
            } else {
                saveHistory("", url, deleteHash);
            }

            return;
        }
    }

    // If we get here, it's because the screenshot upload wasn't on the current screenshot list, which means it's a View History/Upload Later upload.
    updateHistory(fileName, url, deleteHash);
}

// Singleton
ScreenshotManager *ScreenshotManager::mInstance = nullptr;

ScreenshotManager *ScreenshotManager::instance()
{
    if (!mInstance) {
        mInstance = new ScreenshotManager();
    }

    return mInstance;
}
