#include <QJsonObject>
#include <QInputDialog>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QMessageBox>

#include "imguroptions.h"
#include "../uploader/uploader.h"
#include "../screenshotmanager.h"

ImgurOptions::ImgurOptions(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    connect(Uploader::instance(), &Uploader::imgurAuthRefreshed, this, &ImgurOptions::imgurRequestAlbumList);
    connect(ui.imgurAuthButton        , SIGNAL(clicked()), this    , SLOT(imgurAuthorize()));
    connect(ui.imgurRefreshAlbumButton, SIGNAL(clicked()), this    , SLOT(imgurRequestAlbumList()));
}

QSettings *ImgurOptions::settings()
{
    return ScreenshotManager::instance()->settings();
}

void ImgurOptions::imgurAuthorize()
{
    if (ui.imgurAuthButton->text() == tr("Deauthorize")) {
        ui.imgurAuthUserLabel->setText(tr("<i>none</i>"));
        ui.imgurAuthButton->setText(tr("Authorize"));

        ui.imgurRefreshAlbumButton->setEnabled(false);
        ui.imgurAlbumComboBox->setEnabled(false);
        ui.imgurAlbumComboBox->clear();
        ui.imgurAlbumComboBox->addItem(tr("- None -"));

        settings()->setValue("upload/imgur/access_token", "");
        settings()->setValue("upload/imgur/refresh_token", "");
        settings()->setValue("upload/imgur/account_username", "");
        settings()->setValue("upload/imgur/expires_in", 0);
        return;
    }

    QDesktopServices::openUrl(QUrl("https://api.imgur.com/oauth2/authorize?client_id=3ebe94c791445c1&response_type=pin")); //TODO: get the client-id from somewhere?

    bool ok;
    QString pin = QInputDialog::getText(this, tr("Imgur Authorization"),
                                        tr("PIN:"), QLineEdit::Normal,
                                        "", &ok);
    if (ok) {
        QByteArray parameters;
        parameters.append(QString("client_id=").toUtf8());
        parameters.append(QUrl::toPercentEncoding("3ebe94c791445c1"));
        parameters.append(QString("&client_secret=").toUtf8());
        parameters.append(QUrl::toPercentEncoding("0546b05d6a80b2092dcea86c57b792c9c9faebf0")); // TODO: TA.png
        parameters.append(QString("&grant_type=pin").toUtf8());
        parameters.append(QString("&pin=").toUtf8());
        parameters.append(QUrl::toPercentEncoding(pin));

        QNetworkRequest request(QUrl("https://api.imgur.com/oauth2/token"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QNetworkReply *reply = Uploader::instance()->nam()->post(request, parameters);
        connect(reply, SIGNAL(finished()), this, SLOT(imgurToken()));

        ui.imgurAuthButton->setText(tr("Authorizing.."));
        ui.imgurAuthButton->setEnabled(false);
    }
}

void ImgurOptions::imgurToken()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    ui.imgurAuthButton->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, tr("Imgur Authorization Error"), tr("There's been an error authorizing your account with Imgur, please try again."));
        ui.imgurAuthButton->setText(tr("Authorize"));
        return;
    }

    QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

    settings()->setValue("upload/imgur/access_token"    , imgurResponse.value("access_token").toString());
    settings()->setValue("upload/imgur/refresh_token"   , imgurResponse.value("refresh_token").toString());
    settings()->setValue("upload/imgur/account_username", imgurResponse.value("account_username").toString());
    settings()->setValue("upload/imgur/expires_in"      , imgurResponse.value("expires_in").toInt());
    settings()->sync();

    ui.imgurAuthUserLabel->setText("<b>" + imgurResponse.value("account_username").toString() + "</b>");
    ui.imgurAuthButton->setText(tr("Deauthorize"));

    QTimer::singleShot(0, this, &ImgurOptions::imgurRequestAlbumList);
}

void ImgurOptions::imgurRequestAlbumList()
{
    QString username = settings()->value("upload/imgur/account_username").toString();

    if (username.isEmpty()) {
        return;
    }

    ui.imgurRefreshAlbumButton->setEnabled(true);
    ui.imgurAlbumComboBox->clear();
    ui.imgurAlbumComboBox->setEnabled(false);
    ui.imgurAlbumComboBox->addItem(tr("Loading album data..."));

    QNetworkRequest request(QUrl("https://api.imgur.com/3/account/" + username + "/albums/"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + settings()->value("upload/imgur/access_token").toByteArray());

    QNetworkReply *reply = Uploader::instance()->nam()->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(imgurAlbumList()));
}

void ImgurOptions::imgurAlbumList()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());

    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() == QNetworkReply::ContentOperationNotPermittedError ||
                reply->error() == QNetworkReply::AuthenticationRequiredError) {
            Uploader::instance()->imgurAuthRefresh();
        }

        ui.imgurAlbumComboBox->addItem(tr("Loading failed :("));
        return;
    }

    QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

    if (imgurResponse["success"].toBool() != true || imgurResponse["status"].toInt() != 200) {
        return;
    }

    const QJsonArray albumList = imgurResponse["data"].toArray();

    ui.imgurAlbumComboBox->clear();
    ui.imgurAlbumComboBox->setEnabled(true);
    ui.imgurAlbumComboBox->addItem(tr("- None -"), "");
    ui.imgurRefreshAlbumButton->setEnabled(true);

    int settingsIndex = 0;

    for (auto albumValue : albumList) {
        const QJsonObject album = albumValue.toObject();

        QString albumVisibleTitle = album["title"].toString();

        if (albumVisibleTitle.isEmpty()) {
            albumVisibleTitle = tr("untitled");
        }

        ui.imgurAlbumComboBox->addItem(albumVisibleTitle, album["id"].toString());

        if (album["id"].toString() == settings()->value("upload/imgur/album").toString()) {
            settingsIndex = ui.imgurAlbumComboBox->count() - 1;
        }
    }

    ui.imgurAlbumComboBox->setCurrentIndex(settingsIndex);
}
