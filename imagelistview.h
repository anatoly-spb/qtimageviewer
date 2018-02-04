#ifndef IMAGELISTVIEW_H
#define IMAGELISTVIEW_H

#include <QAbstractItemView>
#include <QCache>
#include <QFuture>
#include <QFutureWatcher>
#include <QImage>
#include <QMetaObject>

#include <memory>

class QTimer;

/**
 * @brief The LoadedImage struct
 * Вспомогательная структура для фоновой загрузки
 */
struct ImageLoadingTask {
    int row;
    QString imageFileName;
    std::unique_ptr<QImage> image;
};

/**
 * @brief The ImageListView class
 * ImageListView - класс вида списка изображений
 */
class ImageListView : public QAbstractItemView {
    Q_OBJECT
public:
    ImageListView(QWidget* parent = Q_NULLPTR);

    // ImageListView interface
public:
    /**
     * @brief columnCount возвращает число колонок вида
     * @return число колонок вида
     */
    int columnCount() const;
    /**
     * @brief setColumnCount устанавливает число колонок вида
     * @param columnCount новое число колонок
     */
    void setColumnCount(int columnCount);

protected:
    /**
     * @brief modelIndexRangeForRect возвращает полуоткрытый диапазон модельных строк,
     * попадающих в для заданную прямоугольную область rect
     * @param rect
     * @return полуотркрытый диапазон модельных строк (model index row)
     */
    QPair<int, int> modelIndexRangeForRect(const QRect& rect);
    /**
     * @brief startScrollDelayTimer запускает таймер отсрочки скрола
     */
    void startScrollDelayTimer();
    /**
     * @brief stopScroollDelayTimer останавливает таймер отсрочки скрола
     */
    void stopScrollDelayTimer();
    /**
     * @brief startBackgroundLoading запускает фоновую загрузку
     */
    void startBackgroundLoading();
    /**
     * @brief stopBackgroundLoading останавливает фоновую загрузку
     */
    void stopBackgroundLoading();

    // QAbstractItemView interface
public:
    virtual QRect visualRect(const QModelIndex& index) const override;
    virtual void scrollTo(const QModelIndex& index, ScrollHint hint) override;
    virtual QModelIndex indexAt(const QPoint& point) const override;
    virtual void setModel(QAbstractItemModel* model) override;

public slots:
    virtual void reset() override;

protected:
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    virtual int horizontalOffset() const override;
    virtual int verticalOffset() const override;
    virtual bool isIndexHidden(const QModelIndex& index) const override;
    virtual void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command) override;
    virtual QRegion visualRegionForSelection(const QItemSelection& selection) const override;

protected slots:
    virtual void updateGeometries() override;
    virtual void verticalScrollbarValueChanged(int value) override;

    // QWidget interface
protected:
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;

    // State
private:
    /**
     * @brief m_columnCount число колонок изображений
     */
    int m_columnCount = 5;
    /**
     * @brief m_scrollDelayTimer таймер отсроченной реакции на скроллирование
     */
    QTimer* m_scrollDelayTimer = nullptr;
    QTimer* m_updateDelayTimer = nullptr;
    /**
     * @brief m_loadFuture результат фоновой загрузки рисунков
     */
    QFuture<std::shared_ptr<ImageLoadingTask>> m_loadFuture;
    /**
     * @brief m_loadFutureWatcher наблюдатель за фоновой загрузкой
     */
    QFutureWatcher<std::shared_ptr<ImageLoadingTask>> m_loadFutureWatcher;
    /**
     * @brief m_updatedModelRows список модифицированный строк модели
     */
    QList<int> m_updatedModelRows;
    /**
     * @brief m_imageCache кеш изображений фиксированного размера
     */
    QCache<QString, QImage> m_imageCache;
};

#endif // IMAGELISTVIEW_H
