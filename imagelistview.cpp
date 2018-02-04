#include "imagelistview.h"

#include <QImage>
#include <QPaintEvent>
#include <QScrollBar>
#include <QStylePainter>
#include <QThread>
#include <QTimer>
#include <QtConcurrent>
#include <QtDebug>

ImageListView::ImageListView(QWidget* parent)
    : QAbstractItemView(parent)
    , m_columnCount{ 5 }
    , m_scrollDelayTimer{ new QTimer{ this } }
    , m_updateDelayTimer{ new QTimer{ this } }
    , m_imageCache(100)
{
    horizontalScrollBar()->setRange(0, 0);
    verticalScrollBar()->setRange(0, 0);
    setSelectionMode(ExtendedSelection);
    setSelectionBehavior(SelectItems);

    //  подписываемся на таймер отложенной загрузки
    m_scrollDelayTimer->setSingleShot(true);
    connect(m_scrollDelayTimer, &QTimer::timeout, [this] {
        qDebug() << "Scroll Delay Timer Fired";
        startBackgroundLoading();
    });
    //  подписываемся на результат загрузки
    connect(&m_loadFutureWatcher, &QFutureWatcherBase::resultsReadyAt, [this](int begin, int end) {
        qDebug() << "Background Loading for images [" << begin << ":" << end << ") finished";
        for (int index = begin; index < end; ++index) {
            auto item = m_loadFuture.resultAt(index);
            m_updatedModelRows.append(item->row);
            m_imageCache.insert(item->imageFileName, item->image.release());
            qDebug() << "Loading" << item->imageFileName << "finished";
        }
        m_updateDelayTimer->start(250);
    });

    //
    m_updateDelayTimer->setSingleShot(true);
    connect(m_updateDelayTimer, &QTimer::timeout, [this] {
        qDebug() << "Update Delay Timer Fired";
        QRect invalidatingRect;
        for (auto&& row : m_updatedModelRows) {
            auto rect = visualRect(model()->index(row, 0, rootIndex()));
            invalidatingRect = invalidatingRect.united(rect);
        }
        if (viewport()->rect().intersects(invalidatingRect)) {
            qDebug() << "Update the " << invalidatingRect << "region starting..";
            viewport()->update(invalidatingRect);
        }
    });
}

void ImageListView::startScrollDelayTimer()
{
    qDebug() << "Scroll Delay Timer Restarted";
    stopScrollDelayTimer();
    m_scrollDelayTimer->start(250);
}

void ImageListView::stopScrollDelayTimer()
{
    m_updateDelayTimer->stop();
    stopBackgroundLoading();
    m_scrollDelayTimer->stop();
}

int ImageListView::columnCount() const
{
    return m_columnCount;
}

void ImageListView::setColumnCount(int columnCount)
{
    qDebug() << "Image List View setColumnCount" << columnCount << "called";
    m_columnCount = columnCount;
    reset();
}

QPair<int, int> ImageListView::modelIndexRangeForRect(const QRect& rect)
{
    QRect r = rect.normalized();
    int rowCount = model()->rowCount(rootIndex());
    int begin = 0;
    {
        QModelIndex startIndex = indexAt(r.topLeft());
        if (startIndex.isValid()) {
            begin = startIndex.row();
        }
    }
    int end = begin;
    {
        QModelIndex finishIndex = indexAt(r.bottomRight());
        if (finishIndex.isValid()) {
            end = finishIndex.row() + 1;
        } else {
            end = rowCount;
        }
    }
    return QPair<int, int>(begin, end);
}

void ImageListView::startBackgroundLoading()
{
    class ImageLoader {
    public:
        typedef std::shared_ptr<ImageLoadingTask> result_type;

    public:
        std::shared_ptr<ImageLoadingTask> operator()(std::shared_ptr<ImageLoadingTask> item)
        {
            if (!item->image) {
                item->image = std::make_unique<QImage>();
            }
            if (item->image->isNull()) {
                qDebug() << "ThreadId:" << QThread::currentThreadId() << "Loading" << item->imageFileName << "..";
                if (!item->image->load(item->imageFileName)) {
                    qWarning() << "Loading" << item->imageFileName << "failed";
                }
            }
            return item;
        }
    };
    stopBackgroundLoading();
    QList<std::shared_ptr<ImageLoadingTask>> viewportItems;
    QPair<int, int> modelRowRange = modelIndexRangeForRect(viewport()->rect());
    for (int row = modelRowRange.first; row < modelRowRange.second; ++row) {
        QString imageFileName = model()->data(model()->index(row, 0, rootIndex())).toString();
        QImage* ptr = m_imageCache.take(imageFileName);
        ImageLoadingTask item{ row, imageFileName };
        if (ptr) {
            item.image.reset(ptr);
        }
        viewportItems << std::make_shared<ImageLoadingTask>(std::move(item));
    }
    qDebug() << "Background Loading the " << viewportItems.size() << "images started";
    m_loadFuture = QtConcurrent::mapped(viewportItems, ImageLoader{});
    m_loadFutureWatcher.setFuture(m_loadFuture);
}

void ImageListView::stopBackgroundLoading()
{
    if (m_loadFuture.isRunning()) {
        qDebug() << "Scroll Background Loading Is Running. Canceling...";
        m_loadFuture.cancel();
        qDebug() << "Scroll Background Loading Canceled";
    }
}

QRect ImageListView::visualRect(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QRect();
    }
    // по строке модельного индекса вычисляем
    // строку плитки
    int r = index.row() / m_columnCount;
    // колонку плитки
    int c = index.row() % m_columnCount;
    // вычисляем ширину плитки
    int width = viewport()->width() / m_columnCount;
    // вычисляем высоту плитки
    int height = qMin(width, viewport()->height());
    // получаем координаты плитки в системе координат пространства модели
    int x = c * width;
    int y = r * height;
    // переводим в систему координат видового окна
    QRect result{
        x - horizontalOffset(),
        y - verticalOffset(),
        width,
        height
    };
    return result;
}

void ImageListView::scrollTo(const QModelIndex& index, ScrollHint hint)
{
    Q_UNUSED(hint)

    QRect view = viewport()->rect();
    QRect rect = visualRect(index);

    if (rect.top() < view.top()) {
        verticalScrollBar()->setValue(verticalScrollBar()->value() + rect.top() - view.top());
    } else if (rect.bottom() > view.bottom()) {
        verticalScrollBar()->setValue(
            verticalScrollBar()->value() + qMin(rect.bottom() - view.bottom(), rect.top() - view.top()));
    }

    update();
}

QModelIndex ImageListView::indexAt(const QPoint& point) const
{
    if (model()) {
        // point передан в системе координат viewport-a, поэтому
        // переводим координаты точки в систему координат window
        QPoint p{ point.x() + horizontalOffset(), point.y() + verticalOffset() };
        // расчитываем ширину плитки
        int width = viewport()->width() / m_columnCount;
        // расчитываем высоту плитки
        int height = qMin(width, viewport()->height());
        // расчитываем колонку плитки
        int c = p.x() / width;
        // расчитываем строку плитки
        int r = p.y() / height;
        // переводим в линейный индекс
        int i = r * m_columnCount + c;

        if (i >= 0 && i < model()->rowCount(rootIndex())) {
            return model()->index(i, 0, rootIndex());
        }
    }
    return QModelIndex();
}

QModelIndex ImageListView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(modifiers)

    QModelIndex index = currentIndex();
    if (!index.isValid()) {
        return index;
    }
    int rowCount = model()->rowCount(rootIndex());
    QRect viewRect = viewport()->rect();
    int tileWidth = viewRect.width() / m_columnCount;
    int tileHeight = qMin(tileWidth, viewRect.height());
    int viewColumnCount = viewRect.width() / tileWidth;
    int viewRowCount = viewRect.height() / tileHeight;
    int pageOffset = viewColumnCount * viewRowCount;

    int offset = 0;
    switch (cursorAction) {
    case MoveHome:
        offset = -index.row();
        break;
    case MoveEnd:
        offset = qMax(rowCount - index.row() - 1, 0);
        break;
    case MovePageDown:
        offset += pageOffset;
        break;
    case MovePageUp:
        offset -= pageOffset;
        break;
    case MovePrevious:
    case MoveLeft:
        offset = -1;
        break;
    case MoveNext:
    case MoveRight:
        offset = 1;
        break;
    case MoveUp:
        if ((index.row() + 1) > m_columnCount) {
            offset = -m_columnCount;
        }
        break;
    case MoveDown:
        if ((index.row() + m_columnCount) < rowCount) {
            offset = +m_columnCount;
        }
    }
    return model()->index(qMax(0, qMin(index.row() + offset, rowCount - 1)), index.column(), rootIndex());
}

int ImageListView::horizontalOffset() const
{
    // у нас не будет скроллирования в горизонтальной плоскости
    return 0;
}

int ImageListView::verticalOffset() const
{
    return verticalScrollBar()->value();
}

bool ImageListView::isIndexHidden(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return false;
}

void ImageListView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command)
{
    QPair<int, int> rowRange = modelIndexRangeForRect(rect);
    QItemSelection selection;
    int begin = -1;
    int end = -1;
    for (int row = rowRange.first; row < rowRange.second; row++) {
        QModelIndex index = model()->index(row, 0, rootIndex());
        QRect indexRect = visualRect(index);
        if (indexRect.intersects(rect)) {
            if (begin == -1) {
                begin = end = row;
            } else {
                if ((end + 1) == row) {
                    end = row;
                } else {
                    QModelIndex startIndex = model()->index(begin, 0, rootIndex());
                    QModelIndex finishIndex = model()->index(end, 0, rootIndex());
                    QItemSelection continuousSelection(startIndex, finishIndex);
                    selection.merge(continuousSelection, command);
                    begin = end = row;
                }
            }
        }
    }
    if (begin != -1) {
        QModelIndex startIndex = model()->index(begin, 0, rootIndex());
        QModelIndex finishIndex = model()->index(end, 0, rootIndex());
        QItemSelection continuousSelection(startIndex, finishIndex);
        selection.merge(continuousSelection, command);
    }
    selectionModel()->select(selection, command);
}

QRegion ImageListView::visualRegionForSelection(const QItemSelection& selection) const
{
    QModelIndexList list = selection.indexes();
    QRegion region;
    foreach (const QModelIndex& index, list) {
        QRect rect = visualRect(index);
        if (rect.isValid()) {
            region += rect;
        }
    }
    return region;
}

namespace {
void paintOutline(QPainter& painter, const QRect& rect)
{
    QRect r = rect.adjusted(1, 1, -1, -1);
    painter.save();
    painter.drawRect(r);
    painter.restore();
}
}

void ImageListView::paintEvent(QPaintEvent* event)
{
    m_updatedModelRows.clear();
    Q_UNUSED(event);
    QList<int> imageIndexList;
    QPair<int, int> rowRange = modelIndexRangeForRect(event->rect());
    for (int row = rowRange.first; row < rowRange.second; ++row) {
        imageIndexList.append(row);
    }
    QStylePainter painter(viewport());
    painter.setRenderHints(QPainter::Antialiasing);

    foreach (int row, imageIndexList) {
        QModelIndex index = model()->index(row, 0, rootIndex());
        if (!index.isValid()) {
            continue;
        }
        QRect rect = visualRect(index);
        if (!rect.isValid() || rect.bottom() < 0 || rect.y() > viewport()->height())
            continue;
        QString imageFileName = model()->data(index).toString();
        QImage* ptr = m_imageCache.object(imageFileName);
        if (ptr) {
            QImage* image = ptr;
            QRectF imageRect = image->rect();
            QRectF drawRect = rect.adjusted(2, 2, -2, -2);
            if (imageRect.width() < imageRect.height()) {
                auto delta = (drawRect.width() - drawRect.width() * imageRect.width() / imageRect.height()) / 2.0;
                drawRect.adjust(delta, 0, -delta, 0);
            } else {
                auto delta = (drawRect.height() - drawRect.height() * imageRect.height() / imageRect.width()) / 2.0;
                drawRect.adjust(0, delta, 0, -delta);
            }
            painter.drawImage(drawRect, *image, imageRect, nullptr);
        } else {
            painter.setPen(QPen(QColor("gray"), 1));
            painter.drawText(rect, Qt::AlignCenter, "Loading...");
        }
        if (selectionModel()->isSelected(index)) {
            painter.setPen(QPen(QColor("red"), 1));
            paintOutline(painter, rect);
        } else {
            if (currentIndex() == index) {
                painter.setPen(QPen(QColor("yellow"), 1));
                paintOutline(painter, rect);
            }
        }
    }
}

void ImageListView::updateGeometries()
{
    qDebug() << "Image List View updateGeometries called";

    // получаем прямоугольник, описывающий окно просмотра
    QRect viewportRect = viewport()->rect();
    // получаем ширину окна просмотра
    int viewportWidth = viewportRect.width();
    // получаем ширину вертикальной полосы прокрутки
    int verticalScrollBarWidth = verticalScrollBar()->width();
    // если вертикальная полоса прокрутки видна
    if (verticalScrollBar()->isVisible()) {
        // корректируем ширину окна просмотра
        viewportWidth += verticalScrollBarWidth;
    }
    // получаем количество строк модели
    int modelRowCount = model()->rowCount(rootIndex());
    // расчитываем число строк в видовом окне с учетом числа столбцов
    int viewportRowCount = modelRowCount / m_columnCount + ((modelRowCount % m_columnCount) ? 1 : 0);
    // расчитываем ширину плитки в видовом окне
    int tileWidth = viewportWidth / m_columnCount;
    // расчитываем высоту плитки в видовом окне
    int tileHeight = qMin(tileWidth, viewportRect.height());

    qDebug() << "Image List View set image cache size to " << viewportRowCount * m_columnCount * 5;
    // устанавливаем размер кеша равный пятикратной емкости видового окна
    m_imageCache.setMaxCost(viewportRowCount * m_columnCount * 5);

    // если высоты вида недостаточна для показа модели целиком
    if (viewportRowCount * tileHeight > viewportRect.height()) {
        // корректируем ширину окна просмотра, поскольку станет видима полоса прокрутки
        viewportWidth -= verticalScrollBarWidth;
        // расчитываем новую ширину плитки в видовом окне с учетом корректировки
        int tileWidth = viewportWidth / m_columnCount;
        // расчитываем новую высоту плитки в видовом окне с учетом корректировки
        int tileHeight = qMin(tileWidth, viewportRect.height());
        // расчитываем максимальное смещение вертикальной полосы прокрутки с учетом корректировки
        int verticalScrollBarMaximum = viewportRowCount * tileHeight;
        // если после корректировки высоты видового окна достаточно, чтобы вместить модель целиком
        if (verticalScrollBarMaximum < viewportRect.height()) {
            // оставляем один пиксель, чтобы полоса прокрутки осталась видима
            verticalScrollBarMaximum = 1;
        } else {
            // убираем одну страницу
            verticalScrollBarMaximum -= viewportRect.height();
        }
        // расчитываем шаг для листания page up/down
        int pageStep = viewportRect.height() / tileHeight * tileHeight;
        // расчитываем шаг для прокрутки через up/down
        int singleStep = tileHeight / 2;

        // настраиваем параметры вертикальной полосы прокрутки
        verticalScrollBar()->setRange(0, verticalScrollBarMaximum);
        verticalScrollBar()->setPageStep(pageStep);
        verticalScrollBar()->setSingleStep(singleStep);

    } else {
        // окна просмотра достаточно, чтобы вместить модель целиком
        // поэтому скрываем вертикальную полосу прокрутки
        verticalScrollBar()->setRange(0, 0);
    }
}

void ImageListView::verticalScrollbarValueChanged(int value)
{
    qDebug() << "Image List View verticalScrollbarValueChanged" << value << "called";
    QAbstractItemView::verticalScrollbarValueChanged(value);
    startScrollDelayTimer();
}

void ImageListView::resizeEvent(QResizeEvent* event)
{
    qDebug() << "Image List View resizeEvent called";
    QAbstractItemView::resizeEvent(event);
    startScrollDelayTimer();
}

void ImageListView::setModel(QAbstractItemModel* model)
{
    qDebug() << "Image List View setModel called";
    QAbstractItemView::setModel(model);
}

void ImageListView::reset()
{
    qDebug() << "Image List View reset called";
    m_imageCache.clear();
    m_updatedModelRows.clear();
    QAbstractItemView::reset();
    startScrollDelayTimer();
}
