#include "imagelistmodel.h"

#include <QDebug>
#include <QDir>

ImageListModel::ImageListModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    imageNameFilter << "*.png"
                    << "*.jpg"
                    << "*.gif";
}

bool ImageListModel::loadDirectoryImageList(const QString& fullPath)
{
    qInfo() << "Loading Image List From " << fullPath << "started";
    QDir directory{ fullPath };
    beginResetModel();
    imageFileInfoList = directory.entryInfoList(imageNameFilter, QDir::Files, QDir::Name);
    qInfo() << "Loading Image List From " << fullPath << "finished: " << imageFileInfoList.size() << "images";
    endResetModel();
    return true;
}

int ImageListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : imageFileInfoList.size();
}

int ImageListModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant ImageListModel::data(const QModelIndex& index, int role) const
{
    if (index.isValid()) {
        if (role == Qt::DisplayRole) {
            return imageFileInfoList[index.row()].absoluteFilePath();
        }
    }
    return QVariant();
}
