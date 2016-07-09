#ifndef IMGUROPTIONS_H
#define IMGUROPTIONS_H

#include <QWidget>
#include "ui_imguroptions.h"

class QSettings;
class ImgurOptions : public QWidget
{
    Q_OBJECT
public:
    explicit ImgurOptions(QWidget *parent = 0);
    QSettings *settings();

private slots:
    void requestAlbumList();
    void authorize();

private:
    Ui::ImgurOptions ui;
    friend class OptionsDialog;
};

#endif // IMGUROPTIONS_H
