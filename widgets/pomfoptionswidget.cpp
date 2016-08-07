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

    ui.progressIndicatorBar->setVisible(false);

    connect(ui.verifyButton, &QPushButton::clicked, this, [&]() {
        ui.verifyButton->setEnabled(false);
        ui.downloadListButton->setEnabled(false);
        ui.progressIndicatorBar->setVisible(true);

        QPointer<QWidget> guard(parentWidget());
        PomfUploader::verify(ui.pomfUrlComboBox->currentText(), [&, guard](bool result) {
            if (guard.isNull()) return;

            ui.verifyButton->setEnabled(true);
            ui.downloadListButton->setEnabled(true);
            ui.progressIndicatorBar->setVisible(false);

            if (result) {
                ui.verifyButton->setText(tr("Valid uploader!"));
                ui.verifyButton->setStyleSheet("color: green;");
                ui.verifyButton->setIcon(os::icon("yes"));
            } else {
                ui.verifyButton->setStyleSheet("color: red;");
                ui.verifyButton->setIcon(os::icon("no"));
                ui.verifyButton->setText(tr("Invalid uploader :("));
            }
        });
    });

    connect(ui.pomfUrlComboBox, &QComboBox::currentTextChanged, [&](const QString &text) {
        bool validUrl = false;
        if (text.startsWith("http://") || text.startsWith("https://")) { // TODO: Something a bit more complex
            validUrl = true;
        }

        ui.verifyButton->setEnabled(validUrl);

        if (ui.verifyButton->styleSheet().count() > 0) {
            ui.verifyButton->setStyleSheet("");
            ui.verifyButton->setIcon(QIcon());
            ui.verifyButton->setText(tr("Verify"));
        }
    });

    connect(ui.helpLabel, &QLabel::linkActivated, this, [&](const QString &url) {
        QDesktopServices::openUrl(url);
    });

    connect(ui.downloadListButton, &QPushButton::clicked, this, [&]() {
        ui.verifyButton->setEnabled(false);
        ui.downloadListButton->setEnabled(false);
        ui.progressIndicatorBar->setVisible(true);

        auto pomflistReply = Uploader::instance()->nam()->get(QNetworkRequest(QUrl("https://lightscreen.com.ar/pomf.json")));

        QPointer<QWidget> guard(parentWidget());
        connect(pomflistReply, &QNetworkReply::finished, [&, guard, pomflistReply] {
            if (guard.isNull()) return;

            ui.verifyButton->setEnabled(true);
            ui.downloadListButton->setEnabled(true);
            ui.progressIndicatorBar->setVisible(false);

            if (pomflistReply->error() != QNetworkReply::NoError) {
                QMessageBox::warning(parentWidget(), tr("Connection error"), pomflistReply->errorString());
                return;
            }

            auto pomfListData = QJsonDocument::fromJson(pomflistReply->readAll()).object();

            if (pomfListData.contains("url")) {
                auto urlList = pomfListData["url"].toArray();

                for (auto url : qAsConst(urlList)) {
                    ui.pomfUrlComboBox->addItem(url.toString());
                }

                ui.pomfUrlComboBox->showPopup();
            }
        });
    });
}
