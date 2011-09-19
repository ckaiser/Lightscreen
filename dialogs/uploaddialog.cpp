#include "uploaddialog.h"
#include "ui_uploaddialog.h"

#include "../tools/os.h"
#include "../tools/qxtcsvmodel.h"
#include "../tools/uploader.h"
#include "../tools/screenshotmanager.h"

#include <QMessageBox>
#include <QFile>
#include <QDesktopServices>
#include <QFileSystemWatcher>
#include <QUrl>
#include <QMenu>
#include <QFileInfo>
#include <QClipboard>
#include <QDir>
#include <QSortFilterProxyModel>
#include <QDebug>

UploadDialog::UploadDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UploadDialog)
{
    ui->setupUi(this);

    if (os::aeroGlass(this)) {
      layout()->setMargin(2);
    }

    ui->filterEdit->setText(tr("Filter.."));
    ui->filterEdit->installEventFilter(this);

    QFile historyFile(ScreenshotManager::instance()->historyPath());

    if (!historyFile.exists())
    {
      ui->tableView->setEnabled(false);
      return;
    }

    QFileSystemWatcher *watcher = new QFileSystemWatcher(QStringList() << historyFile.fileName(), this);
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(reloadHistory()));

    reloadHistory();

    ui->tableView->setTextElideMode(Qt::ElideLeft);
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView->setAlternatingRowColors(true);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableView->setSortingEnabled(true);

    if (ui->tableView->model()->rowCount() > 0)
    {
      ui->clearButton->setEnabled(true);
      ui->filterEdit->setEnabled(true);
    }

    connect(ui->uploadButton, SIGNAL(clicked()), this, SLOT(upload()));
    connect(ui->clearButton , SIGNAL(clicked()), this, SLOT(clear()));
    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(selectionChanged(QItemSelection,QItemSelection)));
    connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(open(QModelIndex)));
    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenu(QPoint)));

}

UploadDialog::~UploadDialog()
{
    delete ui;
}

void UploadDialog::contextMenu(QPoint point)
{
  mContextIndex = ui->tableView->indexAt(point);;

  QMenu contextMenu(ui->tableView);

  QAction copyAction((mContextIndex.column() == 0) ? tr("Copy Path") : tr("Copy URL"), &contextMenu);
  connect(&copyAction, SIGNAL(triggered()), this, SLOT(copy()));
  contextMenu.addAction(&copyAction);

  QAction locationAction(tr("Open Location"), &contextMenu);

  if (mContextIndex.column() == 0)
  {
    connect(&locationAction, SIGNAL(triggered()), this, SLOT(location()));
    contextMenu.addAction(&locationAction);
  }
  else if (mContextIndex.data().toString() == QObject::tr("- not uploaded -")) {
    copyAction.setEnabled(false);
  }

  contextMenu.exec(QCursor::pos());
}

void UploadDialog::copy()
{
  qApp->clipboard()->setText(mContextIndex.data().toString());
}

void UploadDialog::location()
{
  QDesktopServices::openUrl("file:///" + QFileInfo(mContextIndex.data().toString()).absolutePath());
}

void UploadDialog::selectionChanged(QItemSelection selected, QItemSelection deselected)
{
  Q_UNUSED(deselected);

  QModelIndex index = selected.indexes().at(0);

  QString screenshot, url;

  if (index.column() == 0) {
    screenshot = index.data().toString();
    url = ui->tableView->model()->index(index.row(), 1).data().toString();
  }
  else {
    screenshot = ui->tableView->model()->index(index.row(), 0).data().toString();
    url = index.data().toString();
  }

  mSelectedScreenshot = screenshot;

  ui->uploadButton->setEnabled((url == QObject::tr("- not uploaded -") && QFile::exists(screenshot)));
}

void UploadDialog::open(QModelIndex index)
{
  if (index.column() == 0) {
    QDesktopServices::openUrl(QUrl("file:///" + index.data().toString()));
  }
  else {
    QDesktopServices::openUrl(index.data().toUrl());
  }
}

void UploadDialog::reloadHistory()
{
  if (ui->tableView->model()) {
    ui->tableView->model()->deleteLater();
  }

  QFile historyFile(ScreenshotManager::instance()->historyPath());

  QxtCsvModel *model = new QxtCsvModel(&historyFile, this, false, '|');
  model->setHeaderData(QStringList() << tr("Screenshot") << tr("URL"));

  mFilterModel = new QSortFilterProxyModel(model);
  mFilterModel->setSourceModel(model);
  mFilterModel->setDynamicSortFilter(true);

  ui->tableView->setModel(mFilterModel);

  ui->tableView->hideColumn(2); // No timestamp.

  ui->tableView->horizontalHeader()->setClickable(false);
  ui->tableView->horizontalHeader()->setMovable(false);

  ui->tableView->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
  ui->tableView->horizontalHeader()->setResizeMode(1, QHeaderView::ResizeToContents);

  ui->tableView->verticalHeader()->hide();
}

void UploadDialog::upload()
{
  Uploader::instance()->upload(mSelectedScreenshot);
  ui->uploadButton->setEnabled(false);
}

void UploadDialog::clear()
{
  if (QMessageBox::question(this,
                            tr("Clearing the screenshot history"),
                            tr("Are you sure you want to clear your entire screenshot history?\nThis cannot be undone."),
                            tr("Clear History"),
                            tr("Don't Clear")) == 1) {
    return;
  }

  QFile::remove(ScreenshotManager::instance()->historyPath());
  close();
}

bool UploadDialog::eventFilter(QObject *object, QEvent *event)
{
  if (object == ui->filterEdit) {
    if (event->type() == QEvent::FocusIn)
    {
      if (ui->filterEdit->text() == tr("Filter..")) {
        ui->filterEdit->setStyleSheet("");
        ui->filterEdit->setText("");
        mFilterModel->setFilterWildcard("");
        mFilterModel->sort(2, Qt::DescendingOrder);
      }
    }
    else if (event->type() == QEvent::FocusOut)
    {
      if (ui->filterEdit->text() == "") {
        ui->filterEdit->setStyleSheet("color: palette(mid);");
        ui->filterEdit->setText(tr("Filter.."));
        mFilterModel->sort(2, Qt::DescendingOrder);
      }
    }
    else if (event->type() == QEvent::KeyRelease)
    {
      if (ui->filterEdit->text() != tr("Filter..") && !ui->filterEdit->text().isEmpty()) {
        mFilterModel->setFilterWildcard(ui->filterEdit->text());
        mFilterModel->sort(2, Qt::DescendingOrder);
      }
      else {
        mFilterModel->setFilterWildcard("");
        mFilterModel->sort(2, Qt::DescendingOrder);
      }
    }
  }
  return QDialog::eventFilter(object, event);
}
