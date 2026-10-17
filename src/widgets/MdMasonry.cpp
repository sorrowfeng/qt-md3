#include "MdMasonry.h"

#include "MdGrid.h"
#include "styles/MdStyleBase.h"

#include <QtCore/QChildEvent>
#include <QtCore/QVector>
#include <QtGui/QResizeEvent>

namespace md {

MdMasonry::MdMasonry(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdMasonry::~MdMasonry() = default;

void MdMasonry::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("masonry"));
    MdStyleBase::connectThemeUpdate(this, &MdMasonry::onThemeChanged);
}

void MdMasonry::setColumns(int columns)
{
    const int clamped = columns <= 0 ? 0 : qBound(1, columns, 12);
    if (m_columns == clamped) {
        return;
    }
    m_columns = clamped;
    refreshEffectiveClass();
    relayout();
    updateGeometry();
    emit columnsChanged(m_columns);
}

void MdMasonry::setGap(qreal gap)
{
    const qreal clamped = qMax<qreal>(0.0, gap);
    if (qFuzzyCompare(m_gap, clamped)) {
        return;
    }
    m_gap = clamped;
    relayout();
    updateGeometry();
    emit gapChanged(m_gap);
}

int MdMasonry::columnsFor(MdWindowSizeClass sizeClass)
{
    switch (sizeClass) {
    case MdWindowSizeClass::Compact:
        return 1;
    case MdWindowSizeClass::Medium:
        return 2;
    case MdWindowSizeClass::Expanded:
    case MdWindowSizeClass::Large:
    case MdWindowSizeClass::ExtraLarge:
        return 3;
    case MdWindowSizeClass::Count:
        break;
    }
    return 1;
}

int MdMasonry::effectiveColumns() const
{
    return m_columns > 0 ? m_columns : columnsFor(m_effective);
}

void MdMasonry::refreshEffectiveClass()
{
    const MdWindowSizeClass next = MdGrid::sizeClassForWidth(width());
    if (m_effective == next) {
        return;
    }
    m_effective = next;
    emit effectiveSizeClassChanged(m_effective);
}

QSize MdMasonry::sizeHint() const
{
    return QSize(320, 240);
}

QSize MdMasonry::minimumSizeHint() const
{
    return QSize(120, 80);
}

int MdMasonry::heightForWidth(int width) const
{
    const int columns = effectiveColumns();
    if (columns <= 0) {
        return 80;
    }
    const qreal gap = m_gap;
    const qreal colW = (width - gap * (columns - 1)) / columns;
    if (colW <= 0) {
        return 80;
    }

    // Shortest-first packing, measure-in-layout (children supply sizeHint).
    QVector<qreal> heights(columns, 0.0);
    for (QObject *child : children()) {
        auto *widget = qobject_cast<QWidget *>(child);
        if (!widget || widget->isWindow() || !widget->isVisible()) {
            continue;
        }
        int best = 0;
        for (int c = 1; c < columns; ++c) {
            if (heights.at(c) < heights.at(best)) {
                best = c;
            }
        }
        const QSize hint = widget->sizeHint();
        const qreal h = hint.height() > 0 ? hint.height() : 64.0;
        heights[best] += h + gap;
    }
    qreal maxH = 0.0;
    for (qreal h : heights) {
        maxH = qMax(maxH, h);
    }
    return int(qMax<qreal>(80.0, maxH - gap));
}

void MdMasonry::relayout() const
{
    auto *self = const_cast<MdMasonry *>(this);
    const int columns = effectiveColumns();
    if (columns <= 0) {
        return;
    }
    const qreal gap = m_gap;
    const qreal width = self->width();
    const qreal colW = (width - gap * (columns - 1)) / columns;
    if (colW <= 0) {
        return;
    }

    QVector<qreal> heights(columns, 0.0);
    for (QObject *child : self->children()) {
        auto *widget = qobject_cast<QWidget *>(child);
        if (!widget || widget->isWindow()) {
            continue;
        }
        int best = 0;
        for (int c = 1; c < columns; ++c) {
            if (heights.at(c) < heights.at(best)) {
                best = c;
            }
        }
        const QSize hint = widget->sizeHint();
        const qreal h = hint.height() > 0 ? hint.height() : 64.0;
        const qreal x = best * (colW + gap);
        const qreal y = heights.at(best);
        widget->setGeometry(qRound(x), qRound(y), qRound(colW), qRound(h));
        heights[best] += h + gap;
    }
}

void MdMasonry::childEvent(QChildEvent *event)
{
    QWidget::childEvent(event);
    if (event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildRemoved) {
        relayout();
        updateGeometry();
    }
}

void MdMasonry::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    refreshEffectiveClass();
    relayout();
}

void MdMasonry::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LayoutDirectionChange) {
        relayout();
    }
    QWidget::changeEvent(event);
}

void MdMasonry::onThemeChanged()
{
    relayout();
}

} // namespace md
