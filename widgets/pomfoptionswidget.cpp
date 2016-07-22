#include <QJsonObject>
#include <QInputDialog>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QMessageBox>

#include <QRegExpValidator>
#include "pomfoptionswidget.h"
#include "../uploader/uploader.h"
#include "../uploader/pomfuploader.h"

#include "../screenshotmanager.h"
#include "../os.h"

PomfOptionsWidget::PomfOptionsWidget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.verifyButton, &QPushButton::clicked, this, [&]() {
        ui.verifyButton->setEnabled(false);
        ui.pomfUrlComboBox->setEnabled(false);

        QPointer<QWidget> guard(parentWidget());
        PomfUploader::verify(ui.pomfUrlComboBox->currentText(), [&, guard](bool result) {
            if (guard.isNull()) return;

            ui.verifyButton->setEnabled(true);
            ui.pomfUrlComboBox->setEnabled(true);

            if (result) {
                ui.verifyButton->setText(tr("Valid uploader!"));
                ui.verifyButton->setStyleSheet("color: green;");
                ui.verifyButton->setIcon(os::icon("yes"));
            } else {
                ui.verifyButton->setStyleSheet("color: red;");
                ui.verifyButton->setIcon(os::icon("no"));
                ui.verifyButton->setText(tr("Invalid :("));
            }
        });
    });

    connect(ui.pomfUrlComboBox, &QComboBox::currentTextChanged, [&](const QString &text) {
        bool validUrl = false;
        if (text.startsWith("http://") || text.startsWith("https://")) { // TODO: Something a bit more complex
            validUrl = true;
        }

        ui.visitButton->setEnabled(validUrl);
        ui.verifyButton->setEnabled(validUrl);

        if (ui.verifyButton->styleSheet().count() > 0) {
            ui.verifyButton->setStyleSheet("");
            ui.verifyButton->setIcon(QIcon());
            ui.verifyButton->setText(tr("Verify"));
        }
    });

    connect(ui.visitButton, &QPushButton::clicked, this, [&]() {
        QDesktopServices::openUrl(ui.pomfUrlComboBox->currentText());
    });
}
