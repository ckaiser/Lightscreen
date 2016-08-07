#ifndef POMFOPTIONSWIDGET_H
#define POMFOPTIONSWIDGET_H

#include <QWidget>
#include "ui_pomfoptionswidget.h"

class PomfOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PomfOptionsWidget(QWidget *parent = 0);

private:
    Ui::PomfOptions ui;
    friend class OptionsDialog;
};

#endif // POMFOPTIONSWIDGET_H
