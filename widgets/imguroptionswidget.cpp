#include <QJsonObject>
#include <QInputDialog>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QMessageBox>

#include <widgets/imguroptionswidget.h>
#include <tools/uploader/uploader.h>
#include <tools/uploader/imguruploader.h>

#include <tools/screenshotmanager.h>

ImgurOptionsWidget::ImgurOptionsWidget(QWidget *parent) : QWidget(parent)
{
    ui.setupUi(this);
    connect(ui.authButton        , &QPushButton::clicked, this, &ImgurOptionsWidget::authorize);
    connect(ui.refreshAlbumButton, &QPushButton::clicked, this, &ImgurOptionsWidget::requestAlbumList);
    connect(ui.authUserLabel     , &QLabel::linkActivated, this, [](const QString & link) {
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

        settings()->setValue("access_token", "");
        settings()->setValue("refresh_token", "");
        settings()->setValue("account_username", "");
        settings()->setValue("expires_in", 0);
    } else {
        ui.authButton->setText(tr("Deauthorize"));
        ui.authUserLabel->setText(tr("<b><a href=\"http://%1.imgur.com/all/\">%1</a></b>").arg(username));
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

    QDesktopServices::openUrl(QUrl("https://api.imgur.com/oauth2/authorize?client_id=" + ImgurUploader::clientId() + "&response_type=pin"));

    bool ok;
    QString pin = QInputDialog::getText(this, tr("Imgur Authorization"),
                                        tr("Authentication PIN:"), QLineEdit::Normal,
                                        "", &ok);
    if (ok) {
        ui.authButton->setText(tr("Authorizing.."));
        ui.authButton->setEnabled(false);

        QPointer<QWidget> guard(parentWidget());
        ImgurUploader::authorize(pin, [&, guard](bool result) {
            if (guard.isNull()) return;

            ui.authButton->setEnabled(true);

            if (result) {
                setUser(settings()->value("upload/imgur/account_username").toString());
                QTimer::singleShot(0, this, &ImgurOptionsWidget::requestAlbumList);
            } else {
                QMessageBox::critical(this, tr("Imgur Authorization Error"), tr("There's been an error authorizing your account with Imgur, please try again."));
                setUser("");
            }
        });
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

    QNetworkReply *reply = Uploader::network()->get(request);
    QPointer<QWidget> guard(parentWidget());

    connect(reply, &QNetworkReply::finished, this, [&, guard, reply] {
        if (mCurrentUser.isEmpty() || guard.isNull()) return;

        if (reply->error() != QNetworkReply::NoError)
        {
            if (reply->error() == QNetworkReply::ContentOperationNotPermittedError ||
            reply->error() == QNetworkReply::AuthenticationRequiredError) {
                ImgurUploader::refreshAuthorization(settings()->value("upload/imgur/refresh_token", "").toString(), [&](bool result) {
                    if (result) {
                        QTimer::singleShot(50, this, &ImgurOptionsWidget::requestAlbumList);
                    } else {
                        setUser("");
                    }
                });
            }

            ui.albumComboBox->addItem(tr("Loading failed :("));
            return;
        }

        const QJsonObject imgurResponse = QJsonDocument::fromJson(reply->readAll()).object();

        if (imgurResponse["success"].toBool() != true || imgurResponse["status"].toInt() != 200)
        {
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

#ifdef Q_OS_WIN
    connect(reply, &QNetworkReply::sslErrors, [reply](const QList<QSslError> &errors) {
        Q_UNUSED(errors);
        if (QSysInfo::WindowsVersion <= QSysInfo::WV_2003) {
            reply->ignoreSslErrors();
        }
    });
#endif
}
