#include "MdSpace.h"

#include "MdFlex.h"

#include <algorithm>

namespace md {

MdSpace::MdSpace(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdSpace::~MdSpace() = default;

void MdSpace::init()
{
    // Pure spacer: no focus, no chrome, never a mouse target.
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAccessibleName(QStringLiteral("space"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void MdSpace::setDirection(MdSpaceDirection direction)
{
    if (direction >= MdSpaceDirection::Count) {
        return;
    }
    if (m_direction == direction) {
        return;
    }
    m_direction = direction;
    updateGeometry();
    emit directionChanged(m_direction);
}

void MdSpace::setSize(MdFlexSpacing size)
{
    if (size >= MdFlexSpacing::Count) {
        return;
    }
    if (m_size == size) {
        return;
    }
    m_size = size;
    m_extent = spacingPx(m_size);
    updateGeometry();
    emit sizeChanged(m_size);
    emit extentChanged(m_extent);
}

void MdSpace::setExtent(qreal extent)
{
    const qreal clamped = std::max<qreal>(0.0, extent);
    if (qFuzzyCompare(m_extent, clamped)) {
        return;
    }
    m_extent = clamped;
    updateGeometry();
    emit extentChanged(m_extent);
}

qreal MdSpace::spacingPx(MdFlexSpacing spacing)
{
    // One ladder with MdFlex — the MD3 4 dp spacing scale.
    return MdFlex::spacingPx(spacing);
}

QSize MdSpace::sizeHint() const
{
    const int extent = qRound(m_extent);
    if (m_direction == MdSpaceDirection::Horizontal) {
        return QSize(extent, 0);
    }
    return QSize(0, extent);
}

QSize MdSpace::minimumSizeHint() const
{
    return sizeHint();
}

} // namespace md
