#include "mainwindow.h"
#include "imagelistmodel.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFileInfo>
#include <QFileSystemModel>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    imageListModel = new ImageListModel{ this };
    fileSystemModel = new QFileSystemModel{ this };
    fileSystemModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    fileSystemModel->setRootPath("");
    ui->treeView->setModel(fileSystemModel);
    for (int hiddenColumn = fileSystemModel->columnCount(); hiddenColumn > 1; --hiddenColumn) {
        ui->treeView->hideColumn(hiddenColumn - 1);
    }
    ui->listView->setColumnCount(3);
    ui->actionThree_Columns->setChecked(true);
    ui->listView->setModel(imageListModel);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent* e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::on_treeView_clicked(const QModelIndex& index)
{
    QFileInfo fileInfo = fileSystemModel->fileInfo(index);
    qDebug() << "New folder " << fileInfo.absoluteFilePath() << "has been selected";
    if (fileInfo.isDir()) {
        imageListModel->loadDirectoryImageList(fileInfo.absoluteFilePath());
    }
}

void MainWindow::on_actionTwo_Columns_triggered()
{
    ui->actionTwo_Columns->setChecked(true);
    ui->actionThree_Columns->setChecked(false);
    ui->listView->setColumnCount(2);
}

void MainWindow::on_actionThree_Columns_triggered()
{
    ui->actionTwo_Columns->setChecked(false);
    ui->actionThree_Columns->setChecked(true);
    ui->listView->setColumnCount(3);
}
