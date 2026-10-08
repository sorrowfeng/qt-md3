#include "MdCarousel.h"

#include "styles/MdCarouselStyle.h"
#include "core/MdMotion.h"
#include "core/MdQtCompat.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QRegion>
#include <QtCore/QTimer>
#include <QtWidgets/QWidget>

#include <cmath>

namespace md {

namespace {

constexpr int kAnimationTickMs = 16;

} // namespace

MdCarousel::MdCarousel(QWidget *parent)
    : QWidget(parent)
{
    init();
}

void MdCarousel::init()
{
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
    MdCarouselStyle::shared();
    MdStyleBase::connectThemeUpdate(this, &MdCarousel::rebuildStrategy);

    m_tokens = MdCarouselTokens::resolve();

    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(kAnimationTickMs);
    connect(m_animationTimer, &QTimer::timeout, this, [this] {
        if (m_phase != Phase::Animating || !m_animationClock) {
            return;
        }
        const qreal seconds = qreal(m_animationClock->elapsed()) / 1000.0;
        const qreal progress = m_animationSpring.valueAt(seconds);
        qreal offset = m_animationFrom + (m_animationTo - m_animationFrom) * progress;
        const bool settled = seconds * 1000.0 >= m_animationSpring.settlingDurationMs();
        if (settled) {
            offset = m_animationTo;
        }
        applyScrollOffset(offset);
        if (settled) {
            m_animationTimer->stop();
            m_phase = Phase::Idle;
        }
    });
}

int MdCarousel::addItem(QWidget *content)
{
    ItemRecord record;
    record.content = content;
    record.wrapper = new QWidget(this);
    // The carousel owns all mouse interaction (drag-scroll and hover); the
    // wrapper and its content stay transparent for mouse events — the
    // recorded divergence that item interaction is the caller's business.
    record.wrapper->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    record.wrapper->setAttribute(Qt::WA_StyledBackground, false);
    if (content->parentWidget() != record.wrapper) {
        content->setParent(record.wrapper);
    }
    content->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    content->show();
    m_items.append(record);
    rebuildStrategy();
    return m_items.size() - 1;
}

void MdCarousel::setPreferredItemSize(qreal width)
{
    m_preferredItemSize = width;
    rebuildStrategy();
}

void MdCarousel::setWithOutline(bool on)
{
    m_tokens.withOutline = on;
    update();
}

qreal MdCarousel::largeItemSize() const
{
    if (m_defaultKeylines.isEmpty()) {
        return 0.0;
    }
    qreal largest = 0.0;
    for (const auto &k : m_defaultKeylines) {
        if (!k.isAnchor) {
            largest = std::max(largest, k.size);
        }
    }
    return largest;
}

qreal MdCarousel::maxScrollOffset() const
{
    return MdCarouselStyle::maxScrollOffset(m_items.size(), largeItemSize(), m_tokens.itemSpacing,
                                            qreal(width()));
}

void MdCarousel::scrollTo(qreal offset)
{
    applyScrollOffset(std::clamp(offset, 0.0, maxScrollOffset()));
}

void MdCarousel::rebuildStrategy()
{
    m_tokens = MdCarouselTokens::resolve();

    const qreal mainAxis = qreal(width());
    const qreal preferred = m_preferredItemSize > 0.0 ? m_preferredItemSize : mainAxis;
    m_defaultKeylines = MdCarouselStyle::multiBrowseKeylineList(
        mainAxis, preferred, m_tokens.itemSpacing, m_items.size(), m_tokens.minSmallItemSize,
        m_tokens.maxSmallItemSize, m_tokens.anchorSize);
    m_startSteps = MdCarouselStyle::startKeylineSteps(m_defaultKeylines, mainAxis,
                                                      m_tokens.itemSpacing);
    m_endSteps = MdCarouselStyle::endKeylineSteps(m_defaultKeylines, mainAxis,
                                                  m_tokens.itemSpacing);
    relayoutItems();
}

void MdCarousel::applyScrollOffset(qreal offset)
{
    m_scrollOffset = offset;
    relayoutItems();
    emit scrollOffsetChanged(offset);
}

void MdCarousel::relayoutItems()
{
    if (m_defaultKeylines.isEmpty()) {
        m_currentKeylines.clear();
        update();
        return;
    }

    const qreal maxScroll = maxScrollOffset();
    m_currentKeylines = MdCarouselStyle::keylineListForScrollOffset(
        m_defaultKeylines, m_startSteps, m_endSteps, qreal(width()), m_tokens.itemSpacing,
        m_scrollOffset, maxScroll);

    const qreal largeSize = largeItemSize();
    const qreal crossPad = m_tokens.crossPadding;
    const qreal itemHeight = qreal(height()) - crossPad * 2.0;

    for (int i = 0; i < m_items.size(); ++i) {
        QWidget *wrapper = m_items[i].wrapper;
        if (!wrapper) {
            continue;
        }
        // The end-to-end scroll model: every item rides the Pager flow at
        // `i * (large + spacing)`, so its flow centre is
        // `i * (large + spacing) + large / 2` and the scroll offset shifts
        // it. The keyline interpolation then returns the container offset
        // the item's CENTRE should take — the wrapper's left edge is that
        // offset minus the half width (Compose's placeable sits at the flow
        // position and the keyline translation corrects it; the two fold
        // into one placement here).
        const qreal flowCenter =
            qreal(i) * (largeSize + m_tokens.itemSpacing) + largeSize / 2.0;
        const qreal unadjustedCenter = flowCenter - m_scrollOffset;
        const MdCarouselStyle::ItemMetrics metrics =
            MdCarouselStyle::itemMetrics(m_currentKeylines, unadjustedCenter);
        const qreal wrapperX = metrics.translation + unadjustedCenter - largeSize / 2.0;

        wrapper->setGeometry(int(std::round(wrapperX)), int(std::round(crossPad)),
                             int(std::round(largeSize)), int(std::round(itemHeight)));
        if (wrapper->contentsMargins().left() != 0) {
            wrapper->setContentsMargins(0, 0, 0, 0);
        }
        if (m_items[i].content) {
            m_items[i].content->setGeometry(wrapper->rect());
        }

        // The mask: the extra-large corners on the visible portion, centred
        // on the wrapper's centre — the carousel's resize-as-it-scrolls.
        const qreal visibleWidth = std::min(metrics.size, largeSize);
        if (visibleWidth < largeSize - 0.5) {
            const QRectF maskRect(largeSize / 2.0 - visibleWidth / 2.0, 0.0, visibleWidth,
                                  itemHeight);
            QRegion region(MdShape::roundedRect(maskRect, QList<qreal>{m_tokens.containerShapeRadius,
                                                        m_tokens.containerShapeRadius,
                                                        m_tokens.containerShapeRadius,
                                                        m_tokens.containerShapeRadius})
                               .toFillPolygon()
                               .toPolygon());
            wrapper->setMask(region);
        } else {
            const QRectF maskRect(0.0, 0.0, largeSize, itemHeight);
            QRegion region(MdShape::roundedRect(maskRect, QList<qreal>{m_tokens.containerShapeRadius,
                                                        m_tokens.containerShapeRadius,
                                                        m_tokens.containerShapeRadius,
                                                        m_tokens.containerShapeRadius})
                               .toFillPolygon()
                               .toPolygon());
            wrapper->setMask(region);
        }

        // The focal item paints on top (Compose's zIndex is 1 / (1 + distance)).
        wrapper->raise();
    }
    update();
}

void MdCarousel::paintItems(QPainter &painter)
{
    const qreal largeSize = largeItemSize();
    for (int i = 0; i < m_items.size(); ++i) {
        QWidget *wrapper = m_items[i].wrapper;
        if (!wrapper) {
            continue;
        }
        // The masked (visible) width for the state paint: the wrapper's own
        // mask does the clipping; this keeps the painted container and its
        // outline inside the same envelope.
        const qreal unadjustedCenter =
            qreal(i) * (largeSize + m_tokens.itemSpacing) + largeSize / 2.0 - m_scrollOffset;
        const qreal visibleWidth = m_currentKeylines.isEmpty()
            ? largeSize
            : std::min(MdCarouselStyle::itemMetrics(m_currentKeylines, unadjustedCenter).size,
                       largeSize);
        MdCarouselStyle::paintItem(painter, QRectF(wrapper->geometry()), visibleWidth,
                                   m_hoveredItem == i, false, m_pressedItem == i, isEnabled(),
                                   m_tokens);
    }
}

void MdCarousel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    rebuildStrategy();
}

void MdCarousel::animateTo(qreal targetOffset)
{
    const qreal clamped = std::clamp(targetOffset, 0.0, maxScrollOffset());
    if (m_phase == Phase::Animating && m_animationTo == clamped) {
        return;
    }
    m_animationFrom = m_scrollOffset;
    m_animationTo = clamped;
    // Compose's snap animation is a low-stiffness spring; the effects-default
    // slot is the library's stand-in for it.
    m_animationSpring = MdMotion::spring(MotionSpring::EffectsDefault);
    if (m_animationClock) {
        delete m_animationClock;
    }
    m_animationClock = new QElapsedTimer();
    m_animationClock->start();
    m_phase = Phase::Animating;
    m_animationTimer->start();
}

int MdCarousel::itemAt(const QPointF &pos) const
{
    for (int i = m_items.size() - 1; i >= 0; --i) {
        const QWidget *wrapper = m_items[i].wrapper;
        if (wrapper && wrapper->geometry().contains(pos.toPoint())) {
            return i;
        }
    }
    return -1;
}

void MdCarousel::updateHover(const QPointF &pos)
{
    const int item = m_dragging ? -1 : itemAt(pos);
    if (item != m_hoveredItem) {
        m_hoveredItem = item;
        update();
    }
}

void MdCarousel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    if (m_phase == Phase::Animating) {
        // Pressing a settling carousel grabs it mid-flight, Compose's
        // scrollable drag semantics.
        m_animationTimer->stop();
        m_phase = Phase::Idle;
    }
    m_dragging = true;
    m_dragStartOffset = m_scrollOffset;
    m_dragStartPos = mousePosition(event).x();
    m_pressedItem = itemAt(mousePosition(event));
    update();
}

void MdCarousel::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF pos = mousePosition(event);
    if (m_dragging) {
        const qreal dx = pos.x() - m_dragStartPos;
        applyScrollOffset(std::clamp(m_dragStartOffset - dx, 0.0, maxScrollOffset()));
    } else {
        updateHover(pos);
    }
}

void MdCarousel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    m_pressedItem = -1;
    if (m_dragging) {
        m_dragging = false;
        // Snap to the nearest item boundary — Compose Pager's page snap.
        const qreal largeSize = largeItemSize();
        const qreal step = largeSize + m_tokens.itemSpacing;
        const qreal target = m_scrollOffset > 0.0
            ? std::round(m_scrollOffset / step) * step
            : 0.0;
        animateTo(target);
    }
    update();
}

} // namespace md
