#include "MdGrid.h"

#include "styles/MdStyleBase.h"

#include <QtCore/QChildEvent>
#include <QtCore/QHash>
#include <QtGui/QResizeEvent>
#include <algorithm>

namespace md {

namespace {

/// Published window-size-class breakpoints, in logical px (M3 layout
/// foundations: window size classes). 600 / 840 / 1200 / 1600.
constexpr qreal kBreakpointMedium = 600.0;
constexpr qreal kBreakpointExpanded = 840.0;
constexpr qreal kBreakpointLarge = 1200.0;
constexpr qreal kBreakpointExtraLarge = 1600.0;

} // namespace

MdGrid::MdGrid(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdGrid::~MdGrid() = default;

void MdGrid::init()
{
    // Pure container: no focus, no chrome (appendix D anchor).
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("grid"));

    MdStyleBase::connectThemeUpdate(this, &MdGrid::onThemeChanged);
    refreshEffectiveClass();
}

MdWindowSizeClass MdGrid::sizeClassForWidth(qreal width)
{
    // M3 window size classes (layout foundations).
    if (width >= kBreakpointExtraLarge) {
        return MdWindowSizeClass::ExtraLarge;
    }
    if (width >= kBreakpointLarge) {
        return MdWindowSizeClass::Large;
    }
    if (width >= kBreakpointExpanded) {
        return MdWindowSizeClass::Expanded;
    }
    if (width >= kBreakpointMedium) {
        return MdWindowSizeClass::Medium;
    }
    return MdWindowSizeClass::Compact;
}

MdGridClassSpec MdGrid::specFor(MdWindowSizeClass sizeClass)
{
    // Responsive layout grid defaults. Compact 4 columns with a 16/16
    // margin/gutter; Medium 8 columns at 24/24; the desktop classes share the
    // 12-column track at 24/24. See the header comment and
    // docs/porting-todo.md (the material-web export publishes no
    // md.sys.layout.* family).
    MdGridClassSpec spec;
    switch (sizeClass) {
    case MdWindowSizeClass::Compact:
        spec.columns = 4;
        spec.gutter = 16.0;
        spec.margin = 16.0;
        break;
    case MdWindowSizeClass::Medium:
        spec.columns = 8;
        spec.gutter = 24.0;
        spec.margin = 24.0;
        break;
    case MdWindowSizeClass::Expanded:
    case MdWindowSizeClass::Large:
    case MdWindowSizeClass::ExtraLarge:
        spec.columns = 12;
        spec.gutter = 24.0;
        spec.margin = 24.0;
        break;
    case MdWindowSizeClass::Count:
        break;
    }
    return spec;
}

void MdGrid::setSizeClass(MdWindowSizeClass sizeClass)
{
    // Count is the auto sentinel: pin a concrete class, or return to auto
    // by passing Count.
    if (sizeClass != MdWindowSizeClass::Count && sizeClass >= MdWindowSizeClass::Count) {
        return;
    }
    if (m_sizeClass == sizeClass) {
        return;
    }
    m_sizeClass = sizeClass;
    refreshEffectiveClass();
    updateGeometry();
    relayout();
    emit sizeClassChanged(m_sizeClass);
}

MdWindowSizeClass MdGrid::effectiveSizeClass() const
{
    return m_effective;
}

void MdGrid::refreshEffectiveClass()
{
    const MdWindowSizeClass next = (m_sizeClass != MdWindowSizeClass::Count)
        ? m_sizeClass
        : sizeClassForWidth(width());
    if (m_effective == next) {
        return;
    }
    m_effective = next;
    emit effectiveSizeClassChanged(m_effective);
}

int MdGrid::columns() const
{
    if (m_columnsOverride > 0) {
        return m_columnsOverride;
    }
    return specFor(m_effective).columns;
}

void MdGrid::setColumns(int columns)
{
    const int clamped = columns < 1 ? 0 : columns;
    if (m_columnsOverride == clamped) {
        return;
    }
    m_columnsOverride = clamped;
    updateGeometry();
    relayout();
    emit columnsChanged(this->columns());
}

qreal MdGrid::gutter() const
{
    return m_gutterOverride >= 0.0 ? m_gutterOverride : specFor(m_effective).gutter;
}

void MdGrid::setGutter(qreal gutter)
{
    const qreal clamped = gutter < 0.0 ? -1.0 : gutter;
    if (qFuzzyCompare(m_gutterOverride + 1.0, clamped + 1.0)) {
        return;
    }
    m_gutterOverride = clamped;
    updateGeometry();
    relayout();
    emit gutterChanged(this->gutter());
}

qreal MdGrid::margin() const
{
    return m_marginOverride >= 0.0 ? m_marginOverride : specFor(m_effective).margin;
}

void MdGrid::setMargin(qreal margin)
{
    const qreal clamped = margin < 0.0 ? -1.0 : margin;
    if (qFuzzyCompare(m_marginOverride + 1.0, clamped + 1.0)) {
        return;
    }
    m_marginOverride = clamped;
    updateGeometry();
    relayout();
    emit marginChanged(this->margin());
}

qreal MdGrid::columnWidth() const
{
    const int cols = columns();
    if (cols <= 0) {
        return 0.0;
    }
    const qreal inner = width() - 2.0 * margin();
    const qreal totalGutter = gutter() * qMax(0, cols - 1);
    return qMax(0.0, (inner - totalGutter) / qreal(cols));
}

void MdGrid::setItemSpan(QWidget *child, int span)
{
    if (child == nullptr) {
        return;
    }
    const int clamped = qBound(1, span, qMax(1, columns()));
    if (m_spans.value(child, 1) == clamped) {
        return;
    }
    m_spans.insert(child, clamped);
    relayout();
}

int MdGrid::itemSpan(const QWidget *child) const
{
    return m_spans.value(child, 1);
}

QSize MdGrid::sizeHint() const
{
    const int cols = columns();
    const MdGridClassSpec spec = specFor(m_effective);
    // A comfortable default: the full track at the class's own width floor.
    qreal width = spec.margin * 2.0 + spec.gutter * qMax(0, cols - 1);
    // Approximate a track as 80 px so the hint is useful before children land.
    width += 80.0 * cols;
    return QSize(qRound(width), 200);
}

QSize MdGrid::minimumSizeHint() const
{
    // The grid can always shrink; wrapping is the caller's escape hatch.
    return QSize(0, 0);
}

void MdGrid::childEvent(QChildEvent *event)
{
    QWidget::childEvent(event);
    if (event->type() == QEvent::ChildRemoved) {
        m_spans.remove(qobject_cast<QWidget *>(event->child()));
    }
    if (event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildRemoved) {
        updateGeometry();
        relayout();
    }
}

void MdGrid::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    refreshEffectiveClass();
    relayout();
}

void MdGrid::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange) {
        relayout();
    }
}

void MdGrid::onThemeChanged()
{
    relayout();
}

void MdGrid::relayout() const
{
    auto *self = const_cast<MdGrid *>(this);

    QVector<QWidget *> items;
    for (QObject *child : self->children()) {
        auto *widget = qobject_cast<QWidget *>(child);
        if (widget != nullptr && !widget->isWindow() && !widget->isHidden()) {
            items.append(widget);
        }
    }
    if (items.isEmpty()) {
        return;
    }

    const int cols = columns();
    const qreal gutter = this->gutter();
    const qreal margin = this->margin();
    const qreal innerWidth = qMax(0.0, self->width() - 2.0 * margin);
    const qreal trackWidth = cols > 0
        ? qMax(0.0, (innerWidth - gutter * qMax(0, cols - 1)) / qreal(cols))
        : innerWidth;

    // Resolve a child's hint the way a layout would.
    auto hintOf = [](QWidget *item) {
        QSize hint = item->sizeHint();
        hint = hint.expandedTo(item->minimumSizeHint());
        hint = hint.expandedTo(item->minimumSize());
        if (hint.height() <= 0) {
            hint.setHeight(item->height());
        }
        return hint;
    };

    struct Placement {
        int item = 0;
        int startTrack = 0;
        int span = 1;
        int row = 0;
    };

    // Row-major placement. A child takes `span` tracks and wraps when it
    // does not fit on the current row.
    QVector<Placement> placements;
    placements.reserve(items.size());
    int track = 0;
    int row = 0;
    for (int i = 0; i < items.size(); ++i) {
        const int span = qBound(1, self->itemSpan(items.at(i)), cols);
        if (track + span > cols && track > 0) {
            ++row;
            track = 0;
        }
        Placement p;
        p.item = i;
        p.startTrack = track;
        p.span = span;
        p.row = row;
        placements.append(p);
        track += span;
        if (track >= cols) {
            ++row;
            track = 0;
        }
    }

    // Row heights from the tallest child on each row.
    QHash<int, qreal> rowHeights;
    for (const Placement &p : placements) {
        const qreal h = hintOf(items.at(p.item)).height();
        if (h > rowHeights.value(p.row, 0.0)) {
            rowHeights.insert(p.row, h);
        }
    }

    // Place. RTL mirrors the column walk so the first track sits at the
    // reading start.
    const bool rtl = self->layoutDirection() == Qt::RightToLeft;
    qreal y = margin;
    int currentRow = -1;
    for (const Placement &p : placements) {
        if (p.row != currentRow) {
            if (currentRow >= 0) {
                y += rowHeights.value(currentRow, 0.0) + gutter;
            }
            currentRow = p.row;
        }
        QWidget *item = items.at(p.item);
        const QSize hint = hintOf(item);
        const qreal itemWidth = p.span * trackWidth + (p.span - 1) * gutter;
        qreal x = margin + p.startTrack * (trackWidth + gutter);
        if (rtl) {
            x = self->width() - margin - itemWidth - p.startTrack * (trackWidth + gutter);
        }
        item->setGeometry(QRectF(x, y, itemWidth, hint.height()).toRect());
    }
}

} // namespace md
