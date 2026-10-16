#include "MdDescriptions.h"

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtMath>

namespace md {

namespace {

/// List one-line horizontal inset (`md.comp.list.list-item.leading-icon-space` 16).
constexpr qreal kInset = 16.0;
/// Vertical padding floor inside a row (`md.comp.list.list-item.*` 12/16 family).
constexpr qreal kRowPadY = 12.0;
/// Gap between label and value stacks (`md.sys.spacing` 4).
constexpr qreal kLabelValueGap = 4.0;
/// Column gutter (`md.sys.spacing` 16).
constexpr qreal kColumnGutter = 16.0;

} // namespace

MdDescriptions::MdDescriptions(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdDescriptions::~MdDescriptions() = default;

void MdDescriptions::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAccessibleName(QStringLiteral("descriptions"));
    MdStyleBase::connectThemeUpdate(this, &MdDescriptions::onThemeChanged);
}

void MdDescriptions::setItems(const QList<MdDescriptionItem> &items)
{
    if (m_items == items) {
        return;
    }
    m_items = items;
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdDescriptions::addItem(const QString &label, const QString &value)
{
    m_items.append(MdDescriptionItem(label, value));
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdDescriptions::clearItems()
{
    if (m_items.isEmpty()) {
        return;
    }
    m_items.clear();
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdDescriptions::setLayout(MdDescriptionsLayout layout)
{
    if (layout >= MdDescriptionsLayout::Count) {
        return;
    }
    if (m_layout == layout) {
        return;
    }
    m_layout = layout;
    updateGeometry();
    update();
    emit layoutChanged(m_layout);
}

void MdDescriptions::setColumns(int columns)
{
    const int clamped = qMax(1, columns);
    if (m_columns == clamped) {
        return;
    }
    m_columns = clamped;
    updateGeometry();
    update();
    emit columnsChanged(m_columns);
}

void MdDescriptions::setBordered(bool bordered)
{
    if (m_bordered == bordered) {
        return;
    }
    m_bordered = bordered;
    update();
    emit borderedChanged(m_bordered);
}

void MdDescriptions::setRowSpacing(qreal spacing)
{
    const qreal clamped = qMax<qreal>(0.0, spacing);
    if (qFuzzyCompare(m_rowSpacing, clamped)) {
        return;
    }
    m_rowSpacing = clamped;
    updateGeometry();
    update();
    emit rowSpacingChanged(m_rowSpacing);
}

QColor MdDescriptions::resolvedLabelColor() const
{
    // md.sys.color.on-surface-variant
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

QColor MdDescriptions::resolvedValueColor() const
{
    // md.sys.color.on-surface
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QColor MdDescriptions::resolvedBorderColor() const
{
    // md.sys.color.outline-variant
    return MdTheme::instance().color(ColorRole::OutlineVariant);
}

int MdDescriptions::rowCount() const
{
    if (m_items.isEmpty()) {
        return 0;
    }
    return (m_items.size() + m_columns - 1) / m_columns;
}

MdDescriptions::CellGeom MdDescriptions::cellGeom(const QRectF &cell,
                                                  const MdDescriptionItem &item) const
{
    CellGeom geom;
    const QFont labelFont = MdTypeScale::font(TypeStyle::LabelMedium);
    const QFont valueFont = MdTypeScale::font(TypeStyle::BodyMedium);
    const qreal labelLine = MdTypeScale::lineHeight(TypeStyle::LabelMedium);
    const qreal valueLine = MdTypeScale::lineHeight(TypeStyle::BodyMedium);
    const QFontMetricsF labelFm(labelFont);
    const QFontMetricsF valueFm(valueFont);

    if (m_layout == MdDescriptionsLayout::Horizontal) {
        // Label column is a fixed share so long labels do not shove values.
        const qreal labelW = qMin(cell.width() * 0.38,
                                  labelFm.horizontalAdvance(QStringLiteral("WWWWWWWWWW")) + 8.0);
        const qreal valueW = qMax<qreal>(1.0, cell.width() - labelW);
        const qreal labelH = qMax(labelLine,
                                  qreal(labelFm.boundingRect(QRectF(0, 0, labelW, 10000).toRect(),
                                                             Qt::TextWordWrap, item.label)
                                            .height()));
        const qreal valueH = qMax(valueLine,
                                  qreal(valueFm.boundingRect(QRectF(0, 0, valueW, 10000).toRect(),
                                                             Qt::TextWordWrap, item.value)
                                            .height()));
        const qreal h = qMax(labelH, valueH);
        geom.label = QRectF(cell.left(), cell.top(), labelW, h);
        geom.value = QRectF(cell.left() + labelW, cell.top(), valueW, h);
        geom.height = h;
        return geom;
    }

    qreal h = 0.0;
    const qreal labelH = qMax(labelLine,
                              qreal(labelFm.boundingRect(QRectF(0, 0, cell.width(), 10000).toRect(),
                                                         Qt::TextWordWrap, item.label)
                                        .height()));
    geom.label = QRectF(cell.left(), cell.top(), cell.width(), labelH);
    h += labelH + kLabelValueGap;

    const qreal valueH = qMax(valueLine,
                              qreal(valueFm.boundingRect(QRectF(0, 0, cell.width(), 10000).toRect(),
                                                         Qt::TextWordWrap, item.value)
                                        .height()));
    geom.value = QRectF(cell.left(), cell.top() + h, cell.width(), valueH);
    h += valueH;
    geom.height = h;
    return geom;
}

QSizeF MdDescriptions::measure(int width) const
{
    if (m_items.isEmpty()) {
        return QSizeF(width, 0.0);
    }
    const int cols = qMax(1, m_columns);
    const qreal cellW = (qMax(0, width - int(kInset * 2)) - kColumnGutter * (cols - 1)) / cols;
    qreal totalH = 0.0;
    const int rows = rowCount();
    for (int r = 0; r < rows; ++r) {
        qreal rowH = 0.0;
        for (int c = 0; c < cols; ++c) {
            const int idx = r * cols + c;
            if (idx >= m_items.size()) {
                break;
            }
            const CellGeom g = cellGeom(QRectF(0, 0, cellW, 0), m_items.at(idx));
            rowH = qMax(rowH, g.height);
        }
        totalH += rowH + kRowPadY * 2.0;
        if (r + 1 < rows) {
            totalH += m_rowSpacing;
        }
    }
    return QSizeF(width, totalH);
}

QSize MdDescriptions::sizeHint() const
{
    // A comfortable default width so the two-column case wraps sensibly.
    const QSizeF s = measure(360);
    return QSize(qCeil(s.width()), qCeil(s.height()));
}

QSize MdDescriptions::minimumSizeHint() const
{
    return QSize(120, 0);
}

int MdDescriptions::heightForWidth(int width) const
{
    return qCeil(measure(width).height());
}

void MdDescriptions::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF bounds = QRectF(rect());
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    const int cols = qMax(1, m_columns);
    const qreal contentW = qMax<qreal>(0.0, bounds.width() - kInset * 2.0);
    const qreal cellW = (contentW - kColumnGutter * (cols - 1)) / cols;
    const QFont labelFont = MdTypeScale::font(TypeStyle::LabelMedium);
    const QFont valueFont = MdTypeScale::font(TypeStyle::BodyMedium);
    const QColor labelColor = resolvedLabelColor();
    const QColor valueColor = resolvedValueColor();
    const QColor borderColor = resolvedBorderColor();

    qreal y = bounds.top();
    const int rows = rowCount();
    for (int r = 0; r < rows; ++r) {
        qreal rowH = 0.0;
        for (int c = 0; c < cols; ++c) {
            const int idx = r * cols + c;
            if (idx >= m_items.size()) {
                break;
            }
            rowH = qMax(rowH, cellGeom(QRectF(0, 0, cellW, 0), m_items.at(idx)).height);
        }
        const qreal rowTop = y + kRowPadY;

        for (int c = 0; c < cols; ++c) {
            const int idx = r * cols + c;
            if (idx >= m_items.size()) {
                break;
            }
            // RTL mirrors the column walk.
            const int col = rtl ? (cols - 1 - c) : c;
            const qreal x = bounds.left() + kInset + col * (cellW + kColumnGutter);
            const QRectF cell(x, rowTop, cellW, rowH);
            const CellGeom g = cellGeom(cell, m_items.at(idx));

            painter.setFont(labelFont);
            painter.setPen(labelColor);
            painter.drawText(g.label, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                             m_items.at(idx).label);

            painter.setFont(valueFont);
            painter.setPen(valueColor);
            painter.drawText(g.value, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                             m_items.at(idx).value);
        }

        y += rowH + kRowPadY * 2.0;
        if (m_bordered && r + 1 < rows) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(borderColor);
            painter.drawRect(QRectF(bounds.left() + kInset, y + m_rowSpacing * 0.5,
                                    contentW, 1.0));
        }
        if (r + 1 < rows) {
            y += m_rowSpacing;
        }
    }
}

void MdDescriptions::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
    }
}

void MdDescriptions::onThemeChanged()
{
    update();
}

} // namespace md
