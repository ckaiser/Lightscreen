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
#ifndef SCREENSHOTMANAGER_H
#define SCREENSHOTMANAGER_H

#include <QObject>
#include <QList>

#include <tools/screenshot.h>

class QSettings;
class ScreenshotManager : public QObject
{
    Q_OBJECT

public:
    enum State {
        Idle  = 0,
        Busy = 1,
        Disabled  = 2
    };
    Q_ENUM(State)

    ScreenshotManager(QObject *parent = 0);
    static ScreenshotManager *instance();

    void initHistory();
    int activeCount() const;
    bool portableMode();
    void saveHistory(const QString &fileName, const QString &url = "", const QString &deleteHash = "");
    void updateHistory(const QString &fileName, const QString &url, const QString &deleteHash);
    void removeHistory(const QString &fileName, qint64 time);
    void clearHistory();
    QSettings *settings() const { return mSettings; }

public slots:
    void askConfirmation();
    void cleanup();
    void finished();
    void take(Screenshot::Options &options);
    void uploadDone(const QString &fileName, const QString &url, const QString &deleteHash);

signals:
    void confirm(Screenshot *screenshot);
    void windowCleanup(const Screenshot::Options &options);
    void activeCountChange();

private:
    static ScreenshotManager *mInstance;
    QList<Screenshot*> mScreenshots;
    QSettings *mSettings;
    QString mHistoryPath;
    bool mPortableMode;
    bool mHistoryInitialized;
};

#endif // SCREENSHOTMANAGER_H
