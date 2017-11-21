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
#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <QObject>
#include <QDir>
#include <QPixmap>

class Screenshot : public QObject
{
    Q_OBJECT

public:
    enum Format {
        PNG  = 0,
        JPEG = 1,
        BMP  = 2,
        WEBP = 3
    };
    Q_ENUM(Format)

    enum Naming {
        Numeric = 0,
        Date = 1,
        Timestamp = 2,
        Empty = 3
    };
    Q_ENUM(Naming)

    enum Mode {
        None         = 0,
        WholeScreen  = 1,
        ActiveWindow = 2,
        SelectedArea = 3,
        SelectedWindow = 4
    };
    Q_ENUM(Mode)

    enum Result {
        Failure = 0,
        Success = 1,
        Cancel = 2
    };
    Q_ENUM(Result)

    struct NamingOptions {
        Naming naming;
        bool flip;
        int leadingZeros;
        QString dateFormat;
    };

    struct Options {
        QString fileName;
        Result result;

        Format format;
        NamingOptions namingOptions;
        QDir directory;
        QString prefix;
        QString uploadService;

        int mode;
        int quality;

        bool animations;
        bool clipboard;
        bool urlClipboard;
        bool currentMonitor;
        bool cursor;
        bool file;
        bool magnify;
        bool optimize;
        bool preview;
        bool replace;
        bool saveAs;
        bool upload;
    };

    Screenshot(QObject *parent, Screenshot::Options options);
    ~Screenshot();

    const Screenshot::Options &options();
    QPixmap &pixmap();
    static QString getName(const NamingOptions &options, const QString &prefix, const QDir &directory);
    const QString &unloadedFileName();

public slots:
    void confirm(bool result = true);
    void confirmation();
    void discard();
    void markUpload();
    void optimize();
    void optimizationDone();
    void save();
    void setPixmap(const QPixmap &pixmap);
    void take();
    void upload();
    void uploadDone(const QString &url);
    void refresh();

signals:
    void askConfirmation();
    void cleanup();
    void finished();

private:
    void activeWindow();
    const QString extension() const;
    void grabDesktop();
    const QString newFileName() const;
    void selectedArea();
    void selectedWindow();
    bool unloadPixmap();
    void wholeScreen();

private:
    Screenshot::Options mOptions;
    QPixmap mPixmap;
    bool mPixmapDelay;
    bool mUnloaded;
    QString mUnloadFilename;

};

#endif // SCREENSHOT_H
