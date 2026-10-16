#include "MdTable.h"

#include "core/MdListTokens.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// The list's one-line height is the default row height — appendix D
/// anchors the body on list row semantics.
qreal defaultRowHeight()
{
    return MdListTokens::resolve(MdListVariant::Standard).oneLineHeight;
}

/// Horizontal padding inside a cell, matching the list item's leading /
/// trailing spaces (16 px, `list-item.leading-space`).
constexpr qreal kCellPad = 16.0;

} // namespace

MdTable::MdTable(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdTable::~MdTable() = default;

void MdTable::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    MdStyleBase::connectThemeUpdate(this, &MdTable::onThemeChanged);
}

void MdTable::setRowCount(int rows)
{
    const int clamped = qMax(0, rows);
    if (m_rows == clamped) {
        return;
    }
    m_rows = clamped;
    while (m_cells.size() < m_rows) {
        m_cells.append(QStringList());
        m_cellWidgets.append(QList<QWidget *>());
    }
    while (m_cells.size() > m_rows) {
        QList<QWidget *> &rowWidgets = m_cellWidgets.last();
        for (QWidget *widget : rowWidgets) {
            if (widget != nullptr) {
                widget->hide();
                widget->deleteLater();
            }
        }
        m_cells.removeLast();
        m_cellWidgets.removeLast();
    }
    for (QStringList &row : m_cells) {
        while (row.size() < m_columns) {
            row.append(QString());
        }
        while (row.size() > m_columns) {
            row.removeLast();
        }
    }
    for (QList<QWidget *> &row : m_cellWidgets) {
        while (row.size() < m_columns) {
            row.append(nullptr);
        }
        while (row.size() > m_columns) {
            QWidget *widget = row.last();
            if (widget != nullptr) {
                widget->hide();
                widget->deleteLater();
            }
            row.removeLast();
        }
    }
    if (m_currentRow >= m_rows) {
        setCurrentRow(m_rows - 1);
    }
    updateGeometry();
    layoutCellWidgets();
    update();
    emit rowCountChanged(m_rows);
}

void MdTable::setColumnCount(int columns)
{
    const int clamped = qMax(0, columns);
    if (m_columns == clamped) {
        return;
    }
    m_columns = clamped;
    for (QStringList &row : m_cells) {
        while (row.size() < m_columns) {
            row.append(QString());
        }
        while (row.size() > m_columns) {
            row.removeLast();
        }
    }
    for (QList<QWidget *> &row : m_cellWidgets) {
        while (row.size() < m_columns) {
            row.append(nullptr);
        }
        while (row.size() > m_columns) {
            QWidget *widget = row.last();
            if (widget != nullptr) {
                widget->hide();
                widget->deleteLater();
            }
            row.removeLast();
        }
    }
    recomputeColumnWidths();
    updateGeometry();
    layoutCellWidgets();
    update();
    emit columnCountChanged(m_columns);
}

void MdTable::setHorizontalHeaderLabels(const QStringList &labels)
{
    if (m_headerLabels == labels) {
        return;
    }
    m_headerLabels = labels;
    update();
}

void MdTable::setCellText(int row, int column, const QString &text)
{
    if (row < 0 || row >= m_rows || column < 0 || column >= m_columns) {
        return;
    }
    while (m_cells.size() <= row) {
        m_cells.append(QStringList());
        m_cellWidgets.append(QList<QWidget *>());
    }
    QStringList &cells = m_cells[row];
    while (cells.size() < m_columns) {
        cells.append(QString());
    }
    if (cells.at(column) == text) {
        return;
    }
    cells[column] = text;
    update();
}

QString MdTable::cellText(int row, int column) const
{
    if (row < 0 || row >= m_cells.size() || column < 0 || column >= m_columns) {
        return QString();
    }
    const QStringList &cells = m_cells.at(row);
    if (column >= cells.size()) {
        return QString();
    }
    return cells.at(column);
}

void MdTable::setCellWidget(int row, int column, QWidget *widget)
{
    if (row < 0 || row >= m_rows || column < 0 || column >= m_columns) {
        return;
    }
    while (m_cellWidgets.size() <= row) {
        m_cells.append(QStringList());
        m_cellWidgets.append(QList<QWidget *>());
    }
    QList<QWidget *> &rowWidgets = m_cellWidgets[row];
    while (rowWidgets.size() < m_columns) {
        rowWidgets.append(nullptr);
    }
    if (rowWidgets.at(column) == widget) {
        return;
    }
    if (QWidget *previous = rowWidgets.at(column)) {
        previous->hide();
        previous->deleteLater();
    }
    rowWidgets[column] = widget;
    if (widget != nullptr) {
        widget->setParent(this);
        widget->show();
    }
    layoutCellWidgets();
}

QWidget *MdTable::cellWidget(int row, int column) const
{
    if (row < 0 || row >= m_cellWidgets.size() || column < 0 || column >= m_columns) {
        return nullptr;
    }
    const QList<QWidget *> &rowWidgets = m_cellWidgets.at(row);
    if (column >= rowWidgets.size()) {
        return nullptr;
    }
    return rowWidgets.at(column);
}

void MdTable::setCurrentRow(int row)
{
    const int clamped = row < 0 ? -1 : qMin(row, m_rows - 1);
    if (m_currentRow == clamped) {
        return;
    }
    m_currentRow = clamped;
    update();
    emit currentRowChanged(m_currentRow);
}

void MdTable::setShowHeader(bool show)
{
    if (m_showHeader == show) {
        return;
    }
    m_showHeader = show;
    updateGeometry();
    layoutCellWidgets();
    update();
    emit showHeaderChanged(m_showHeader);
}

void MdTable::setShowDividers(bool show)
{
    if (m_showDividers == show) {
        return;
    }
    m_showDividers = show;
    update();
    emit showDividersChanged(m_showDividers);
}

void MdTable::setAlternatingRowColors(bool alternating)
{
    if (m_alternating == alternating) {
        return;
    }
    m_alternating = alternating;
    update();
    emit alternatingRowColorsChanged(m_alternating);
}

void MdTable::setRowHeight(qreal height)
{
    const qreal clamped = qMax(0.0, height);
    if (qFuzzyCompare(m_rowHeight, clamped)) {
        return;
    }
    m_rowHeight = clamped;
    updateGeometry();
    layoutCellWidgets();
    update();
    emit rowHeightChanged(m_rowHeight);
}

void MdTable::setHeaderHeight(qreal height)
{
    const qreal clamped = qMax(0.0, height);
    if (qFuzzyCompare(m_headerHeight, clamped)) {
        return;
    }
    m_headerHeight = clamped;
    updateGeometry();
    layoutCellWidgets();
    update();
    emit headerHeightChanged(m_headerHeight);
}

qreal MdTable::rowHeight() const
{
    return m_rowHeight > 0.0 ? m_rowHeight : defaultRowHeight();
}

qreal MdTable::headerHeight() const
{
    return m_headerHeight > 0.0 ? m_headerHeight : defaultRowHeight();
}

QRectF MdTable::headerRect() const
{
    if (!m_showHeader) {
        return QRectF();
    }
    return QRectF(0, 0, width(), headerHeight());
}

QRectF MdTable::bodyRect() const
{
    const qreal top = m_showHeader ? headerHeight() : 0.0;
    return QRectF(0, top, width(), qMax(0.0, height() - top));
}

QRectF MdTable::rowRect(int row) const
{
    const QRectF body = bodyRect();
    return QRectF(body.left(), body.top() + row * rowHeight(), body.width(), rowHeight());
}

QRectF MdTable::headerCellRect(int column) const
{
    recomputeColumnWidths();
    if (column < 0 || column >= m_columnWidths.size()) {
        return QRectF();
    }
    qreal x = 0.0;
    for (int i = 0; i < column; ++i) {
        x += m_columnWidths.at(i);
    }
    return QRectF(x, headerRect().top(), m_columnWidths.at(column), headerHeight());
}

QRectF MdTable::cellRect(int row, int column) const
{
    recomputeColumnWidths();
    if (row < 0 || row >= m_rows || column < 0 || column >= m_columnWidths.size()) {
        return QRectF();
    }
    qreal x = 0.0;
    for (int i = 0; i < column; ++i) {
        x += m_columnWidths.at(i);
    }
    const QRectF rowBox = rowRect(row);
    return QRectF(x, rowBox.top(), m_columnWidths.at(column), rowBox.height());
}

void MdTable::recomputeColumnWidths() const
{
    // Equal columns — a data table with no sizing model. Callers that need
    // weighted columns can drive setCellWidget geometry themselves.
    m_columnWidths.clear();
    if (m_columns <= 0) {
        return;
    }
    const qreal each = width() / qreal(m_columns);
    for (int i = 0; i < m_columns; ++i) {
        m_columnWidths.append(each);
    }
}

void MdTable::layoutCellWidgets()
{
    for (int row = 0; row < m_cellWidgets.size(); ++row) {
        const QList<QWidget *> &rowWidgets = m_cellWidgets.at(row);
        for (int column = 0; column < rowWidgets.size(); ++column) {
            QWidget *widget = rowWidgets.at(column);
            if (widget == nullptr) {
                continue;
            }
            widget->setGeometry(cellRect(row, column).toRect());
        }
    }
}

int MdTable::rowAtPosition(const QPointF &position) const
{
    const QRectF body = bodyRect();
    if (!body.contains(position)) {
        return -1;
    }
    const int row = int((position.y() - body.top()) / rowHeight());
    return (row >= 0 && row < m_rows) ? row : -1;
}

int MdTable::columnAtPosition(const QPointF &position) const
{
    recomputeColumnWidths();
    qreal x = 0.0;
    for (int i = 0; i < m_columnWidths.size(); ++i) {
        const qreal w = m_columnWidths.at(i);
        if (position.x() >= x && position.x() < x + w) {
            return i;
        }
        x += w;
    }
    return -1;
}

QSize MdTable::sizeHint() const
{
    return QSize(int(width() > 0 ? width() : 480), heightForWidth(width() > 0 ? width() : 480));
}

int MdTable::heightForWidth(int width) const
{
    Q_UNUSED(width);
    const qreal header = m_showHeader ? headerHeight() : 0.0;
    return int(header + m_rows * rowHeight());
}

void MdTable::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QColor surface = MdTheme::instance().color(ColorRole::Surface);
    const QColor onSurface = MdTheme::instance().color(ColorRole::OnSurface);
    const QColor onSurfaceVariant = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
    const QColor outlineVariant = MdTheme::instance().color(ColorRole::OutlineVariant);

    // Surface behind the whole control.
    painter.fillRect(rect(), surface);

    // Header: surface-container, label-large.
    if (m_showHeader) {
        const QRectF header = headerRect();
        painter.fillRect(header, MdTheme::instance().color(ColorRole::SurfaceContainer));
        QFont headerFont = MdTypeScale::font(TypeStyle::LabelLarge);
        painter.setFont(headerFont);
        painter.setPen(onSurface);
        for (int column = 0; column < m_columns; ++column) {
            const QRectF cell = headerCellRect(column);
            const QString label =
                column < m_headerLabels.size() ? m_headerLabels.at(column) : QString();
            painter.drawText(cell.adjusted(kCellPad, 0, -kCellPad, 0),
                             Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, label);
        }
        if (m_showDividers) {
            painter.fillRect(QRectF(header.left(), header.bottom() - 1.0, header.width(), 1.0),
                             outlineVariant);
        }
    }

    QFont bodyFont = MdTypeScale::font(TypeStyle::BodyMedium);
    painter.setFont(bodyFont);

    for (int row = 0; row < m_rows; ++row) {
        const QRectF rowBox = rowRect(row);
        // Optional tonal alternation from the surface-container ladder.
        if (m_alternating) {
            const QColor base = (row % 2 == 0)
                                    ? MdTheme::instance().color(ColorRole::SurfaceContainerLowest)
                                    : MdTheme::instance().color(ColorRole::SurfaceContainerLow);
            painter.fillRect(rowBox, base);
        }

        // List selected family: secondary-container + on-secondary-container.
        const bool selected = (row == m_currentRow);
        const bool hovered = (row == m_hoveredRow);
        if (selected) {
            painter.fillRect(rowBox, MdTheme::instance().color(ColorRole::SecondaryContainer));
        } else if (hovered) {
            // hover state layer, on-surface @ 0.08 — the list's unselected
            // hover row. Painted as a composite so it works on any base.
            QColor base = m_alternating
                              ? ((row % 2 == 0)
                                     ? MdTheme::instance().color(ColorRole::SurfaceContainerLowest)
                                     : MdTheme::instance().color(ColorRole::SurfaceContainerLow))
                              : surface;
            painter.fillRect(rowBox,
                             MdStateLayer::over(base, onSurface, true, false, false, false));
        }

        const QColor text = selected ? MdTheme::instance().color(ColorRole::OnSecondaryContainer)
                                     : onSurface;
        for (int column = 0; column < m_columns; ++column) {
            const QRectF cell = cellRect(row, column);
            if (m_cellWidgets.size() > row && m_cellWidgets.at(row).size() > column
                && m_cellWidgets.at(row).at(column) != nullptr) {
                continue; // the child paints itself
            }
            painter.setPen(text);
            painter.drawText(cell.adjusted(kCellPad, 0, -kCellPad, 0),
                             Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                             cellText(row, column));
        }

        if (m_showDividers && row < m_rows - 1) {
            painter.fillRect(QRectF(rowBox.left(), rowBox.bottom() - 1.0, rowBox.width(), 1.0),
                             outlineVariant);
        }
    }

    Q_UNUSED(onSurfaceVariant);
}

void MdTable::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const int row = rowAtPosition(md::mousePosition(event));
    const int column = columnAtPosition(md::mousePosition(event));
    if (row >= 0) {
        setCurrentRow(row);
        if (column >= 0) {
            emit cellActivated(row, column);
        }
    }
    setFocus(Qt::MouseFocusReason);
    event->accept();
}

void MdTable::mouseMoveEvent(QMouseEvent *event)
{
    const int row = rowAtPosition(md::mousePosition(event));
    if (m_hoveredRow != row) {
        m_hoveredRow = row;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void MdTable::leaveEvent(QEvent *event)
{
    if (m_hoveredRow != -1) {
        m_hoveredRow = -1;
        update();
    }
    QWidget::leaveEvent(event);
}

void MdTable::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Down:
        setCurrentRow(m_currentRow < 0 ? 0 : qMin(m_currentRow + 1, m_rows - 1));
        event->accept();
        return;
    case Qt::Key_Up:
        setCurrentRow(m_currentRow < 0 ? 0 : qMax(m_currentRow - 1, 0));
        event->accept();
        return;
    case Qt::Key_Home:
        setCurrentRow(0);
        event->accept();
        return;
    case Qt::Key_End:
        setCurrentRow(m_rows - 1);
        event->accept();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        if (m_currentRow >= 0) {
            emit cellActivated(m_currentRow, 0);
            event->accept();
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdTable::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    recomputeColumnWidths();
    layoutCellWidgets();
}

void MdTable::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::EnabledChange) {
        update();
    }
}

void MdTable::onThemeChanged()
{
    update();
}

} // namespace md
