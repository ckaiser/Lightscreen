#include <QJsonObject>
#include <QInputDialog>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QMessageBox>

#include <QRegExpValidator>
#include <widgets/pomfoptionswidget.h>
#include <tools/uploader/uploader.h>
#include <tools/uploader/pomfuploader.h>

#include <tools/screenshotmanager.h>
#include <tools/os.h>

PomfOptionsWidget::PomfOptionsWidget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);

    ui.progressIndicatorBar->setVisible(false);
    ui.cancelButton->setVisible(false);

    connect(ui.verifyButton, &QPushButton::clicked, this, [&]() {
        ui.verifyButton->setEnabled(false);
        ui.downloadListButton->setEnabled(false);
        ui.progressIndicatorBar->setVisible(true);
        ui.cancelButton->setVisible(true);
        disconnect(ui.cancelButton);
        ui.cancelButton->disconnect();

        QPointer<QWidget> guard(parentWidget());
        QPointer<QNetworkReply> reply = PomfUploader::verify(ui.pomfUrlComboBox->currentText(), [&, guard](bool result) {
            if (guard.isNull()) return;

            ui.verifyButton->setEnabled(true);
            ui.downloadListButton->setEnabled(true);
            ui.progressIndicatorBar->setVisible(false);

            if (result) {
                ui.verifyButton->setText(tr("Valid uploader!"));
                ui.verifyButton->setStyleSheet("color: green;");
                ui.verifyButton->setIcon(os::icon("yes"));
            } else if (ui.cancelButton->isVisible() == true) { // Not cancelled
                ui.verifyButton->setStyleSheet("color: red;");
                ui.verifyButton->setIcon(os::icon("no"));
                ui.verifyButton->setText(tr("Invalid uploader :("));
            }

            ui.cancelButton->setVisible(false);
        });

        if (reply) {
            connect(ui.cancelButton, &QPushButton::clicked, [&, reply] {
                if (reply) {
                    ui.cancelButton->setVisible(false);
                    reply->abort();
                }
            });
        }
    });

    connect(ui.pomfUrlComboBox, &QComboBox::currentTextChanged, [&](const QString &text) {
        bool validUrl = false;

        if (!text.isEmpty() && (text.startsWith("http://") || text.startsWith("https://"))) { // TODO: Something a bit more complex
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
        ui.cancelButton->setVisible(true);
        disconnect(ui.cancelButton);
        ui.cancelButton->disconnect();

        QUrl pomfRepoURL = QUrl(ScreenshotManager::instance()->settings()->value("options/upload/pomfRepo").toString());

        if (pomfRepoURL.isEmpty()) {
            pomfRepoURL = QUrl("https://lightscreen.com.ar/pomf.json");
        }

        auto pomflistReply = QPointer<QNetworkReply>(Uploader::network()->get(QNetworkRequest(pomfRepoURL)));

        QPointer<QWidget> guard(parentWidget());
        connect(pomflistReply, &QNetworkReply::finished, [&, guard, pomflistReply] {
            if (guard.isNull()) return;
            if (pomflistReply.isNull()) return;

            ui.verifyButton->setEnabled(true);
            ui.downloadListButton->setEnabled(true);
            ui.progressIndicatorBar->setVisible(false);
            ui.cancelButton->setVisible(false);

            if (pomflistReply->error() != QNetworkReply::NoError) {
                QMessageBox::warning(parentWidget(), tr("Connection error"), pomflistReply->errorString());
                return;
            }

            auto pomfListData = QJsonDocument::fromJson(pomflistReply->readAll()).object();

            if (pomfListData.contains("url")) {
                auto urlList = pomfListData["url"].toArray();

                for (auto url : qAsConst(urlList)) {
                    if (ui.pomfUrlComboBox->findText(url.toString(), Qt::MatchExactly) < 0) {
                        ui.pomfUrlComboBox->addItem(url.toString());
                    }
                }

                ui.pomfUrlComboBox->showPopup();
            }
        });

#ifdef Q_OS_WIN
        connect(pomflistReply, &QNetworkReply::sslErrors, [pomflistReply](const QList<QSslError> &errors) {
            Q_UNUSED(errors);
            if (!pomflistReply.isNull() && QSysInfo::WindowsVersion <= QSysInfo::WV_2003) {
                pomflistReply->ignoreSslErrors();
            }
        });
#endif

        connect(ui.cancelButton, &QPushButton::clicked, [&, guard, pomflistReply] {
            if (guard.isNull()) return;
            if (pomflistReply.isNull()) return;

            pomflistReply->abort();
        });

    });
}
