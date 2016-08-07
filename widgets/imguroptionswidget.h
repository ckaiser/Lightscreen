#ifndef IMGUROPTIONSWIDGET_H
#define IMGUROPTIONSWIDGET_H

#include <QWidget>

#include "ui_imguroptionswidget.h"

class QSettings;
class ImgurOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ImgurOptionsWidget(QWidget *parent = 0);
    QSettings *settings();
    void setUser(const QString &username);

private slots:
    void requestAlbumList();
    void authorize();

private:
    QString mCurrentUser;
    Ui::ImgurOptions ui;
    friend class OptionsDialog;
};

#endif // IMGUROPTIONSWIDGET_H
