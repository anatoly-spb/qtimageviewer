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
 * @brief The ImageLoadingTask struct
 * Вспомогательная структура для фоновой загрузки
 */
struct ImageLoadingTask {
    int row;
    QString imageFileName;
    std::unique_ptr<QImage> image;
};
using ImageLoadingTaskSharedPtr = std::shared_ptr<ImageLoadingTask>;
using ImageLoadingTaskFutureWatcher = QFutureWatcher<ImageLoadingTaskSharedPtr>;

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
    QPair<int, int> modelRowRangeForViewportRect(const QRect& rect);
    /**
     * @brief startScrollDelayTimer запускает таймер отсрочки скрола
     */
    void startScrollDelayTimer();
    void stopScrollDelayTimer();
    void startAsyncImageLoading();
    void stopAsyncImageLoading();

    // QAbstractItemView interface
public:
    virtual QRect visualRect(const QModelIndex& index) const override;
    virtual QModelIndex indexAt(const QPoint& point) const override;
    virtual void scrollTo(const QModelIndex& index, ScrollHint hint) override;
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
     * @brief m_loadingDelayTimer таймер отсроченной реакции на скроллирование
     */
    QTimer* m_loadingDelayTimer = nullptr;
    /**
     * @brief m_updateDelayTimer таймер отсроченной реакции на обновление вида
     */
    QTimer* m_updatingDelayTimer = nullptr;
    /**
     * @brief m_imageLoadingFutureWatcher наблюдатель за фоновой загрузкой
     */
    ImageLoadingTaskFutureWatcher m_imageLoadingFutureWatcher;
    /**
     * @brief m_invalidatingModelRows список инвалидируемых строк модели
     */
    QList<int> m_invalidatingModelRows;
    /**
     * @brief m_imageCache кеш изображений фиксированного размера
     */
    QCache<QString, QImage> m_imageCache;
};

#endif // IMAGELISTVIEW_H
