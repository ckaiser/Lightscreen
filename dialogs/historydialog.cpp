#include "historydialog.h"
#include "ui_historydialog.h"

#include "../tools/os.h"
#include "../tools/uploader/uploader.h"
#include "../tools/screenshotmanager.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlTableModel>
#include <QUrl>

HistoryDialog::HistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HistoryDialog),
    mModel(Q_NULLPTR),
    mFilterModel(Q_NULLPTR)
{
    ui->setupUi(this);

    ui->filterEdit->setText(tr("Filter.."));
    ui->filterEdit->installEventFilter(this);

    ui->cancelUploadButton->setIcon(os::icon("no"));
    ui->tableView->setEnabled(false);
    ui->clearButton->setEnabled(false);

    connect(Uploader::instance(), SIGNAL(progress(int)), this, SLOT(uploadProgress(int)));
    connect(Uploader::instance(), SIGNAL(done(QString, QString, QString)), this, SLOT(refresh()));

    connect(ui->uploadButton      , SIGNAL(clicked()), this                    , SLOT(upload()));
    connect(ui->cancelUploadButton, SIGNAL(clicked()), Uploader::instance()    , SLOT(cancel()));
    connect(ui->cancelUploadButton, SIGNAL(clicked()), ui->uploadProgressWidget, SLOT(hide()));

    connect(ui->clearButton       , SIGNAL(clicked()), this                , SLOT(clear()));

    QTimer::singleShot(0, this, &HistoryDialog::init);
}

HistoryDialog::~HistoryDialog()
{
    delete ui;
}

void HistoryDialog::clear()
{
    if (QMessageBox::question(this,
                              tr("Clearing the screenshot history"),
                              tr("Are you sure you want to clear your entire screenshot history?\nThis cannot be undone."),
                              tr("Clear History"),
                              tr("Don't Clear")) == 1) {
        return;
    }

    ScreenshotManager::instance()->clearHistory();
    mModel->select();
    ui->filterEdit->setEnabled(false);
    ui->tableView->setEnabled(false);
    ui->clearButton->setEnabled(false);
}

void HistoryDialog::contextMenu(const QPoint &point)
{
    mContextIndex = ui->tableView->indexAt(point);;

    QMenu contextMenu(ui->tableView);

    QAction copyAction((mContextIndex.column() == 0) ? tr("Copy Path") : tr("Copy URL"), &contextMenu);
    connect(&copyAction, SIGNAL(triggered()), this, SLOT(copy()));
    contextMenu.addAction(&copyAction);

    QAction deleteAction(tr("Delete from imgur.com"), &contextMenu);
    QAction locationAction(tr("Open Location"), &contextMenu);

    QAction removeAction(tr("Remove history entry"), &contextMenu);

    if (mContextIndex.data().toString().isEmpty()) {
        copyAction.setEnabled(false);
        deleteAction.setEnabled(false);
    }

    if (mContextIndex.column() == 0) {
        connect(&locationAction, SIGNAL(triggered()), this, SLOT(location()));
        contextMenu.addAction(&locationAction);
    } else {
        connect(&deleteAction, SIGNAL(triggered()), this, SLOT(deleteImage()));
        contextMenu.addAction(&deleteAction);
    }

    connect(&removeAction, SIGNAL(triggered()), this, SLOT(removeHistoryEntry()));
    contextMenu.addAction(&removeAction);
    contextMenu.exec(QCursor::pos());
}

void HistoryDialog::copy()
{
    qApp->clipboard()->setText(mContextIndex.data().toString());
}

void HistoryDialog::deleteImage()
{
    QDesktopServices::openUrl(mContextIndex.sibling(mContextIndex.row(), 2).data().toString());
}

void HistoryDialog::location()
{
    QDesktopServices::openUrl("file:///" + QFileInfo(mContextIndex.data().toString()).absolutePath());
}

void HistoryDialog::removeHistoryEntry()
{
    if (mContextIndex.column() == 0) {
        // File got right clicked:
        ScreenshotManager::instance()->removeHistory(mContextIndex.data().toString(), mContextIndex.sibling(mContextIndex.row(), 3).data().toLongLong());
    } else {
        // Screenshot URL got right clicked:
        ScreenshotManager::instance()->removeHistory(mContextIndex.sibling(mContextIndex.row(), 0).data().toString(), mContextIndex.sibling(mContextIndex.row(), 3).data().toLongLong());
    }

    refresh();
}

void HistoryDialog::refresh()
{
    mModel->select();
}

void HistoryDialog::openUrl(const QModelIndex &index)
{
    if (index.column() == 0) {
        QDesktopServices::openUrl(QUrl("file:///" + index.data().toString()));
    } else {
        QDesktopServices::openUrl(index.data().toUrl());
    }
}

void HistoryDialog::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    if (selected.indexes().count() == 0) {
        return;
    }

    QModelIndex index = selected.indexes().at(0);

    QString screenshot;

    if (index.column() == 0) {
        screenshot = index.data().toString();
    } else {
        screenshot = ui->tableView->model()->index(index.row(), 0).data().toString();
    }

    mSelectedScreenshot = screenshot;

    ui->uploadButton->setEnabled(QFile::exists(screenshot));
}

void HistoryDialog::upload()
{
    Uploader::instance()->upload(mSelectedScreenshot);
    ui->uploadProgressWidget->setVisible(true);
}

void HistoryDialog::uploadProgress(int progress)
{
    ui->uploadProgressWidget->setVisible(true);
    ui->uploadProgressBar->setValue(progress);
}

void HistoryDialog::init()
{
    ScreenshotManager::instance()->initHistory();

    if (QSqlDatabase::database().isOpen()) {
        mModel = new QSqlTableModel(this);
        mModel->setTable("history");
        mModel->setHeaderData(0, Qt::Horizontal, tr("Screenshot"));
        mModel->setHeaderData(1, Qt::Horizontal, tr("URL"));
        mModel->select();

        mFilterModel = new QSortFilterProxyModel(mModel);
        mFilterModel->setSourceModel(mModel);
        mFilterModel->setDynamicSortFilter(true);
        mFilterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        mFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        mFilterModel->setFilterKeyColumn(-1);

        while (mModel->canFetchMore()) {
            mModel->fetchMore();
        }

        ui->tableView->setWordWrap(false);
        ui->tableView->setModel(mFilterModel);

        ui->tableView->hideColumn(2); // No delete hash.
        ui->tableView->hideColumn(3); // No timestamp.

        ui->tableView->horizontalHeader()->setSectionsClickable(false);
        ui->tableView->horizontalHeader()->setSectionsMovable(false);

        ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

        ui->tableView->verticalHeader()->hide();

        ui->tableView->setTextElideMode(Qt::ElideLeft);
        ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tableView->setAlternatingRowColors(true);
        ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
        ui->tableView->setSortingEnabled(true);

        if (ui->tableView->model()->rowCount() > 0) {
            ui->tableView->setEnabled(true);
            ui->clearButton->setEnabled(true);
            ui->filterEdit->setEnabled(true);
        }

        connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(selectionChanged(QItemSelection, QItemSelection)));
        connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(openUrl(QModelIndex)));
        connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));
    }

    if (Uploader::instance()->progress() > 0) {
        ui->uploadProgressBar->setValue(Uploader::instance()->progress());
    } else {
        ui->uploadProgressWidget->setVisible(false);
    }

    ui->closeButton->setFocus();
}

bool HistoryDialog::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->filterEdit && mFilterModel) {
        if (event->type() == QEvent::FocusIn) {
            if (ui->filterEdit->text() == tr("Filter..")) {
                ui->filterEdit->setStyleSheet("");
                ui->filterEdit->setText("");
                mFilterModel->setFilterWildcard("");
                mFilterModel->sort(3, Qt::DescendingOrder);
            }
        } else if (event->type() == QEvent::FocusOut) {
            if (ui->filterEdit->text().isEmpty()) {
                ui->filterEdit->setStyleSheet("color: palette(mid);");
                ui->filterEdit->setText(tr("Filter.."));
                mFilterModel->sort(3, Qt::DescendingOrder);
            }
        } else if (event->type() == QEvent::KeyRelease) {
            if (ui->filterEdit->text() != tr("Filter..") && !ui->filterEdit->text().isEmpty()) {
                mFilterModel->setFilterWildcard(ui->filterEdit->text());
                mFilterModel->sort(3, Qt::DescendingOrder);
            } else {
                mFilterModel->setFilterWildcard("");
                mFilterModel->sort(3, Qt::DescendingOrder);
            }
        }
    }

    return QDialog::eventFilter(object, event);
}

bool HistoryDialog::event(QEvent *event)
{
    if (event->type() == QEvent::Show) {
        restoreGeometry(ScreenshotManager::instance()->settings()->value("geometry/historyDialog").toByteArray());
    } else if (event->type() == QEvent::Close) {
        ScreenshotManager::instance()->settings()->setValue("geometry/historyDialog", saveGeometry());
    }

    return QDialog::event(event);
}
