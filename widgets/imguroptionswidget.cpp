#include <QJsonObject>
#include <QInputDialog>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QMessageBox>

#include "imguroptionswidget.h"
#include "../uploader/uploader.h"
#include "../uploader/imguruploader.h"

#include "../screenshotmanager.h"

ImgurOptionsWidget::ImgurOptionsWidget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    connect(Uploader::instance(), &Uploader::imgurAuthRefreshed, this, &ImgurOptionsWidget::requestAlbumList);

    connect(ui.authButton        , &QPushButton::clicked, this, &ImgurOptionsWidget::authorize);
    connect(ui.refreshAlbumButton, &QPushButton::clicked, this, &ImgurOptionsWidget::requestAlbumList);
    connect(ui.authUserLabel     , &QLabel::linkActivated, this, [](const QString &link) {
        QDesktopServices::openUrl(link);
    });
}

QSettings *ImgurOptionsWidget::settings()
{
    return ScreenshotManager::instance()->settings();
}

void ImgurOptionsWidget::setUser(const QString &username)
{
    mCurrentUser = username;

    setUpdatesEnabled(false);

    if (mCurrentUser.isEmpty()) {
        ui.authUserLabel->setText(tr("<i>none</i>"));
        ui.albumComboBox->setEnabled(false);
        ui.refreshAlbumButton->setEnabled(false);
        ui.albumComboBox->clear();
        ui.albumComboBox->addItem(tr("- None -"));
        ui.authButton->setText(tr("Authorize"));
        ui.helpLabel->setEnabled(true);

        settings()->setValue("upload/imgur/access_token", "");
        settings()->setValue("upload/imgur/refresh_token", "");
        settings()->setValue("upload/imgur/account_username", "");
        settings()->setValue("upload/imgur/expires_in", 0);
    } else {
        ui.authButton->setText(tr("Deauthorize"));
        ui.authUserLabel->setText("<b><a href=\"http://"+ username +".imgur.com/all/\">" + username + "</a></b>");
        ui.refreshAlbumButton->setEnabled(true);
        ui.helpLabel->setEnabled(false);
    }

    setUpdatesEnabled(true);
}

void ImgurOptionsWidget::authorize()
{
    if (!mCurrentUser.isEmpty()) {
        setUser("");
        return;
    }

    QDesktopServices::openUrl(QUrl("https://api.imgur.com/oauth2/authorize?client_id=" + ImgurUploader::clientId() + "&response_type=pin")); //TODO: get the client-id from somewhere?

    bool ok;
    QString pin = QInputDialog::getText(this, tr("Imgur Authorization"),
                                        tr("Authentication PIN:"), QLineEdit::Normal,
                                        "", &ok);
    if (ok) {
        QByteArray parameters;
        parameters.append(QString("client_id=").toUtf8());
        parameters.append(QUrl::toPercentEncoding(ImgurUploader::clientId()));
        parameters.append(QString("&client_secret=").toUtf8());
        parameters.append(QUrl::toPercentEncoding(ImgurUploader::clientSecret()));
        parameters.append(QString("&grant_type=pin").toUtf8());
        parameters.append(QString("&pin=").toUtf8());
        parameters.append(QUrl::toPercentEncoding(pin));

        QNetworkRequest request(QUrl("https://api.imgur.com/oauth2/token"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QNetworkReply *reply = Uploader::instance()->nam()->post(request, parameters);
        connect(reply, &QNetworkReply::finished, this, [&, reply] {
            ui.authButton->setEnabled(true);

            if (reply->error() != QNetworkReply::NoError) {
                QMessageBox::critical(this, tr("Imgur Authorization Error"), tr("There's been an error authorizing your account with Imgur, please try again."));
                setUser("");
                return;
            }

            QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

            settings()->setValue("upload/imgur/access_token"    , imgurResponse.value("access_token").toString());
            settings()->setValue("upload/imgur/refresh_token"   , imgurResponse.value("refresh_token").toString());
            settings()->setValue("upload/imgur/account_username", imgurResponse.value("account_username").toString());
            settings()->setValue("upload/imgur/expires_in"      , imgurResponse.value("expires_in").toInt());
            settings()->sync();

            setUser(imgurResponse.value("account_username").toString());

            QTimer::singleShot(0, this, &ImgurOptionsWidget::requestAlbumList);
        });

        ui.authButton->setText(tr("Authorizing.."));
        ui.authButton->setEnabled(false);
    }
}

void ImgurOptionsWidget::requestAlbumList()
{
    if (mCurrentUser.isEmpty()) {
        return;
    }

    ui.refreshAlbumButton->setEnabled(true);
    ui.albumComboBox->clear();
    ui.albumComboBox->setEnabled(false);
    ui.albumComboBox->addItem(tr("Loading album data..."));

    QNetworkRequest request(QUrl::fromUserInput("https://api.imgur.com/3/account/" + mCurrentUser + "/albums/"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + settings()->value("upload/imgur/access_token").toByteArray());

    QNetworkReply *reply = Uploader::instance()->nam()->get(request);

    connect(reply, &QNetworkReply::finished, this, [&, reply] {
        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() == QNetworkReply::ContentOperationNotPermittedError ||
                    reply->error() == QNetworkReply::AuthenticationRequiredError) {
                Uploader::instance()->imgurAuthRefresh();
                qDebug() << "Attempting imgur auth refresh";
            }

            ui.albumComboBox->addItem(tr("Loading failed :("));
            return;
        }

        const QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

        if (imgurResponse["success"].toBool() != true || imgurResponse["status"].toInt() != 200) {
            return;
        }

        const QJsonArray albumList = imgurResponse["data"].toArray();

        setUpdatesEnabled(false);

        ui.albumComboBox->clear();
        ui.albumComboBox->setEnabled(true);
        ui.albumComboBox->addItem(tr("- None -"), "");
        ui.refreshAlbumButton->setEnabled(true);

        int settingsIndex = 0;

        for (auto albumValue : albumList) {
            const QJsonObject album = albumValue.toObject();

            QString albumVisibleTitle = album["title"].toString();

            if (albumVisibleTitle.isEmpty()) {
                albumVisibleTitle = tr("untitled");
            }

            ui.albumComboBox->addItem(albumVisibleTitle, album["id"].toString());

            if (album["id"].toString() == settings()->value("upload/imgur/album").toString()) {
                settingsIndex = ui.albumComboBox->count() - 1;
            }
        }

        ui.albumComboBox->setCurrentIndex(settingsIndex);

        setUpdatesEnabled(true);
    });
}
