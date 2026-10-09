#include "MdAppBarScrollBehavior.h"

#include "core/MdMotion.h"

#include <QtCore/QTimer>
#include <QtCore/QtGlobal>
#include <QtWidgets/QScrollBar>

#include <cmath>

namespace md {

namespace {

/// How close two offsets have to be before a write counts as "no change".
/// Compose compares floats for equality directly; a pixel-scale epsilon is the
/// same test with the noise removed.
constexpr qreal kOffsetEpsilon = 0.01;

bool nearlyEqual(qreal a, qreal b)
{
    return std::fabs(a - b) <= kOffsetEpsilon;
}

} // namespace

MdAppBarScrollBehavior::MdAppBarScrollBehavior(QObject *parent)
    : QObject(parent)
{
    m_settleTimer = new QTimer(this);
    m_settleTimer->setInterval(8);
    connect(m_settleTimer, &QTimer::timeout, this, &MdAppBarScrollBehavior::onSettleTick);
}

MdAppBarScrollBehavior::MdAppBarScrollBehavior(MdAppBarScrollMode mode, QObject *parent)
    : MdAppBarScrollBehavior(parent)
{
    m_mode = mode;
}

MdAppBarScrollBehavior::~MdAppBarScrollBehavior() = default;

void MdAppBarScrollBehavior::setMode(MdAppBarScrollMode mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    emit modeChanged(m_mode);
    emit changed();
}

void MdAppBarScrollBehavior::setHeightOffset(qreal offset)
{
    // `TopAppBarState`'s setter coerces into [heightOffsetLimit, 0]; the limit
    // is negative, so `qBound` needs the arguments in that order.
    const qreal clamped = qBound(m_heightOffsetLimit, offset, 0.0);
    if (nearlyEqual(clamped, m_heightOffset)) {
        return;
    }
    m_heightOffset = clamped;
    emit changed();
}

void MdAppBarScrollBehavior::setHeightOffsetLimit(qreal limit)
{
    // The limit is a *negative* distance. A caller that hands over a positive
    // number means "this much may disappear", which is the same thing with the
    // sign the rest of the state uses.
    const qreal normalised = qMin(limit, 0.0);
    if (nearlyEqual(normalised, m_heightOffsetLimit)) {
        return;
    }
    m_heightOffsetLimit = normalised;
    // Re-coerce: a smaller limit can put the current offset out of range.
    m_heightOffset = qBound(m_heightOffsetLimit, m_heightOffset, 0.0);
    emit changed();
}

void MdAppBarScrollBehavior::setContentOffset(qreal offset)
{
    if (nearlyEqual(offset, m_contentOffset)) {
        return;
    }
    m_contentOffset = offset;
    emit changed();
}

void MdAppBarScrollBehavior::setCanScroll(bool canScroll)
{
    m_canScroll = canScroll;
}

void MdAppBarScrollBehavior::setScrollingContentAtStart(bool atStart)
{
    if (m_contentAtStart == atStart) {
        return;
    }
    m_contentAtStart = atStart;
    emit changed();
}

qreal MdAppBarScrollBehavior::collapsedFraction() const
{
    if (m_heightOffsetLimit == 0.0) {
        return 0.0;
    }
    return m_heightOffset / m_heightOffsetLimit;
}

qreal MdAppBarScrollBehavior::overlappedFraction() const
{
    // `TopAppBarState.overlappedFraction`, verbatim: the special case first,
    // then the clamped ratio.
    if (!m_contentAtStart && m_contentOffset == 0.0) {
        return 1.0;
    }
    if (m_heightOffsetLimit == 0.0) {
        return 0.0;
    }
    const qreal combined = qBound(m_heightOffsetLimit, m_heightOffsetLimit + std::fabs(m_contentOffset),
                                  0.0);
    return 1.0 - (combined / m_heightOffsetLimit);
}

bool MdAppBarScrollBehavior::isSettled() const
{
    const qreal fraction = collapsedFraction();
    return fraction < 0.01 || fraction >= 1.0;
}

qreal MdAppBarScrollBehavior::consumeScroll(qreal dy, bool contentAtStart)
{
    if (!m_canScroll || dy == 0.0) {
        return 0.0;
    }
    setScrollingContentAtStart(contentAtStart);

    const qreal before = m_heightOffset;
    qreal consumed = 0.0;

    /// Compose's two non-pinned behaviours both answer their pre-scroll hook
    /// with `available.copy(x = 0f)` — the **whole** delta — as soon as the
    /// height moves at all, even when only part of it fitted into the
    /// remaining travel. The surplus is not handed back to the content. That
    /// over-consumption is reproduced rather than "fixed", because it is what
    /// decides how a wheel notch feels: the bar is never half-scrolled and
    /// half-scrolled-content at once. Once the bar is at an end it stops
    /// moving, reports nothing, and the content takes the delta in full.
    const auto takeFromBar = [this, &consumed, before](qreal delta) {
        setHeightOffset(m_heightOffset + delta);
        if (nearlyEqual(m_heightOffset, before)) {
            return false;
        }
        consumed = delta;
        return true;
    };

    switch (m_mode) {
    case MdAppBarScrollMode::Pinned:
        // [compose] PinnedScrollBehavior.onPostScroll — the bar itself never
        // moves, so the whole delta belongs to the content.
        setContentOffset(m_contentOffset + dy);
        break;

    case MdAppBarScrollMode::EnterAlways:
        // [compose] onPreScroll hands any direction to the bar first.
        if (!takeFromBar(dy)) {
            setContentOffset(m_contentOffset + dy);
        }
        break;

    case MdAppBarScrollMode::ExitUntilCollapsed:
        if (dy < 0.0) {
            // [compose] onPreScroll intercepts the collapsing direction only
            // ("Don't intercept if scrolling down"), because a bar set up this
            // way must leave as soon as the content starts moving.
            if (!takeFromBar(dy)) {
                setContentOffset(m_contentOffset + dy);
            }
        } else {
            // The expanding direction is a *post*-scroll hook: the delta only
            // reaches the bar once the scrollable has refused it, which
            // happens exactly when the content is back at its start. Here the
            // hook returns the real travel rather than the whole delta — the
            // one place Compose does not over-consume.
            if (contentAtStart) {
                setHeightOffset(m_heightOffset + dy);
                consumed = m_heightOffset - before;
            } else {
                setContentOffset(m_contentOffset + dy);
            }
        }
        break;

    case MdAppBarScrollMode::Count:
        break;
    }

    return consumed;
}

void MdAppBarScrollBehavior::scrollBy(qreal dy, bool contentAtStart)
{
    consumeScroll(dy, contentAtStart);
}

void MdAppBarScrollBehavior::snapNow()
{
    m_settleTimer->stop();
    m_settling = false;
    if (isSettled()) {
        return;
    }
    // Compose's `settleAppBar` picks the end by `collapsedFraction < 0.5f`.
    setHeightOffset(collapsedFraction() < 0.5 ? 0.0 : m_heightOffsetLimit);
    emit changed();
}

void MdAppBarScrollBehavior::settle()
{
    if (isSettled()) {
        return;
    }
    m_settleFrom = m_heightOffset;
    m_settleTo = collapsedFraction() < 0.5 ? 0.0 : m_heightOffsetLimit;
    m_settleElapsedMs = 0;
    m_settling = true;
    m_settleTimer->start();
    emit changed();
}

bool MdAppBarScrollBehavior::isSettling() const
{
    return m_settling;
}

void MdAppBarScrollBehavior::onSettleTick()
{
    // `TopAppBarDefaults.snapAnimationSpec` is `DefaultEffects` — the same
    // spring the container colour rides, which is why the settle and the
    // colour cross-fade finish together.
    const MdSpring spring = MdMotion::spring(MotionSpring::EffectsDefault);
    m_settleElapsedMs += m_settleTimer->interval();
    const qreal seconds = qreal(m_settleElapsedMs) / 1000.0;
    const qreal progress = qBound(0.0, spring.valueAt(seconds), 1.0);

    const qreal value = m_settleFrom + (m_settleTo - m_settleFrom) * progress;
    setHeightOffset(value);

    if (seconds * 1000.0 >= spring.settlingDurationMs() || nearlyEqual(progress, 1.0)) {
        m_settleTimer->stop();
        m_settling = false;
        setHeightOffset(m_settleTo);
        emit changed();
    }
}

void MdAppBarScrollBehavior::followScrollBar(QScrollBar *bar)
{
    if (m_scrollBar == bar) {
        return;
    }
    if (m_scrollBar != nullptr) {
        disconnect(m_scrollBar, nullptr, this, nullptr);
    }
    m_scrollBar = bar;
    if (m_scrollBar == nullptr) {
        return;
    }
    connect(m_scrollBar, &QScrollBar::valueChanged, this,
            &MdAppBarScrollBehavior::onScrollBarValueChanged);
    onScrollBarValueChanged(m_scrollBar->value());
}

void MdAppBarScrollBehavior::onScrollBarValueChanged(int value)
{
    if (m_scrollBar == nullptr) {
        return;
    }
    // A scroll bar's value grows as the content moves up, which is the
    // negative direction in Compose's convention — hence the sign flip. The
    // offset is then exactly "how far the content has scrolled", which is the
    // definition `overlappedFraction` reads.
    setScrollingContentAtStart(value <= m_scrollBar->minimum());
    setContentOffset(-qreal(value - m_scrollBar->minimum()));
}

} // namespace md
