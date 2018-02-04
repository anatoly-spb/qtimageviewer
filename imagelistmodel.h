#ifndef IMAGELISTMODEL_H
#define IMAGELISTMODEL_H

#include <QAbstractTableModel>
#include <QFileInfoList>
#include <QList>

/**
 * @brief The ImageListModel class
 * ImageListModel - класс модели, содержащей список имен файлов изображений
 */
class ImageListModel : public QAbstractTableModel {
    Q_OBJECT
public:
    ImageListModel(QObject* parent = Q_NULLPTR);

    // ImageListModel interface
public:
    /**
     * @brief loadDirectoryImageList
     * @param fullPath
     */
    bool loadDirectoryImageList(const QString& fullPath);

    // QAbstractItemModel interface
public:
    /**
     * @brief rowCount возвращает число файлов модели
     * @param parent
     * @return число файлов модели
     */
    virtual int rowCount(const QModelIndex& parent) const override;
    /**
     * @brief columnCount возвращает число столбцов модели
     * @param parent
     * @return число столбцов модели
     */
    virtual int columnCount(const QModelIndex& parent) const override;
    /**
     * @brief data возращает данные по индексу модели index и роли role
     * @param index
     * @param role
     * @return данные по индексу модели index и роли role
     */
    virtual QVariant data(const QModelIndex& index, int role) const override;

private:
    /**
     * @brief imageNameFilter
     * Список масок файлов изображений
     */
    QStringList imageNameFilter;
    /**
     * @brief imageFileInfoList
     * Список файлов
     */
    QFileInfoList imageFileInfoList;
};

#endif // IMAGELISTMODEL_H
