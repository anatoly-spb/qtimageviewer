#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class QFileSystemModel;
class ImageListModel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = Q_NULLPTR);
    ~MainWindow();

protected:
    void changeEvent(QEvent* e);

private slots:
    void on_treeView_clicked(const QModelIndex& index);

    void on_actionTwo_Columns_triggered();

    void on_actionThree_Columns_triggered();

private:
    Ui::MainWindow* ui;
    QFileSystemModel* fileSystemModel;
    ImageListModel* imageListModel;
};

#endif // MAINWINDOW_H
