#include "MdSideSheet.h"

#include "styles/MdSideSheetStyle.h"

#include "core/MdMotion.h"
#include "core/MdTheme.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QWidget>

#include <cmath>
#include <limits>

namespace md {

namespace {

/// The animation tick granularity (~60 fps, the same cadence the other
/// animation cycles use).
constexpr int kAnimationTickMs = 16;

/// The drag slop before a press counts as a drag (px).
constexpr qreal kDragSlop = 4.0;

} // namespace

MdSideSheet::MdSideSheet(MdSideSheetKind kind, MdSideSheetEdge edge, QWidget *parent)
    : QWidget(parent)
    , m_kind(kind)
    , m_edge(edge)
{
    // MDC's defaults: a behavior-created sheet starts hidden (the dialog
    // starts expanded with its enter animation); a docked standard sheet
    // sits expanded in the layout from the start.
    m_state = m_target = kind == MdSideSheetKind::Standard ? MdSideSheetState::Expanded
                                                           : MdSideSheetState::Hidden;

    init();
}

MdSideSheet::~MdSideSheet() = default;

void MdSideSheet::init()
{
    setFocusPolicy(Qt::NoFocus);
    MdSideSheetStyle::shared();
    MdStyleBase::connectThemeUpdate(this, &MdSideSheet::onThemeChanged);
    if (parentWidget()) {
        parentWidget()->installEventFilter(this);
    }

    m_animationTimer = new QTimer(this);
    m_animationTimer->setInterval(kAnimationTickMs);
    connect(m_animationTimer, &QTimer::timeout, this, [this] {
        if (m_phase != Phase::Animating || m_animationClock == nullptr) {
            return;
        }
        const MdSpring spring =
            MdMotion::spring(m_animationIsHide ? MotionSpring::EffectsFast
                                               : MotionSpring::SpatialDefault);
        const qreal seconds = qreal(m_animationClock->elapsed()) / 1000.0;
        const qreal offset =
            m_animationFrom + (m_animationTo - m_animationFrom) * spring.valueAt(seconds);
        applyOffset(offset);

        if (seconds * 1000.0 >= spring.settlingDurationMs()) {
            m_animationTimer->stop();
            m_phase = Phase::Idle;
            applyOffset(m_animationTo);
            settleAt(m_target);
        }
    });
    m_animationClock = new QElapsedTimer();

    if (m_kind == MdSideSheetKind::Standard) {
        // A standard sheet starts visible at its expanded anchor.
        snapToAnchor();
    }
}

void MdSideSheet::onThemeChanged()
{
    m_tokensDirty = true;
    update();
}

void MdSideSheet::setSheetKind(MdSideSheetKind kind)
{
    if (m_kind == kind) {
        return;
    }
    m_kind = kind;
    update();
}

void MdSideSheet::setSheetEdge(MdSideSheetEdge edge)
{
    if (m_edge == edge) {
        return;
    }
    m_edge = edge;
    if (!isAnimating()) {
        snapToAnchor();
    }
    update();
}

void MdSideSheet::setInnerMargin(qreal margin)
{
    if (qFuzzyCompare(m_innerMargin, margin)) {
        return;
    }
    m_innerMargin = qMax(0.0, margin);
    if (!isAnimating()) {
        snapToAnchor();
    }
    update();
}

void MdSideSheet::setDividerVisible(bool visible)
{
    if (m_hasDivider == visible) {
        return;
    }
    m_hasDivider = visible;
    update();
}

void MdSideSheet::setGeometryManaged(bool managed)
{
    if (m_geometryManaged == managed) {
        return;
    }
    m_geometryManaged = managed;
    if (managed && !isAnimating()) {
        syncManagedHeight();
        snapToAnchor();
        if (m_state != MdSideSheetState::Hidden) {
            show();
        }
    }
}

void MdSideSheet::setConfirmValueChange(std::function<bool(MdSideSheetState)> confirm)
{
    m_confirm = std::move(confirm);
}

const MdSideSheetTokens &MdSideSheet::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdSideSheetTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

qreal MdSideSheet::currentOffset() const
{
    if (parentWidget()) {
        return m_edge == MdSideSheetEdge::Right
            ? qreal(x()) + MdSideSheetTokens::kShadowMargin
            : qreal(x());
    }
    return m_offsetX;
}

void MdSideSheet::setState(MdSideSheetState state)
{
    if (m_state == state && m_target == state) {
        return;
    }
    m_state = state;
    m_target = state;
    m_phase = Phase::Idle;
    m_animationTimer->stop();
    emit stateChanged(state);
}

QRectF MdSideSheet::containerRect() const
{
    const qreal m = MdSideSheetTokens::kShadowMargin;
    return m_edge == MdSideSheetEdge::Right ? QRectF(rect()).adjusted(m, m, 0.0, -m)
                                            : QRectF(rect()).adjusted(0.0, m, -m, -m);
}

MdSideSheetStyle::Layout MdSideSheet::currentLayout() const
{
    return MdSideSheetStyle::layoutFor(m_edge, m_kind, qreal(width()), qreal(height()),
                                       m_hasDivider, tokens());
}

qreal MdSideSheet::sheetWidth() const
{
    // The container's width: the widget rect carries the inner shadow margin
    // only (the docked edge is flush).
    return qMax(0.0, qreal(width()) - MdSideSheetTokens::kShadowMargin);
}

void MdSideSheet::expandSheet()
{
    animateTo(MdSideSheetState::Expanded);
}

void MdSideSheet::hideSheet()
{
    animateTo(MdSideSheetState::Hidden);
}

bool MdSideSheet::confirmed(MdSideSheetState state)
{
    if (!m_confirm) {
        return true;
    }
    return m_confirm(state);
}

qreal MdSideSheet::fullParentWidth() const
{
    // MDC's `getParentWidth` — the layout width the sheet lives in.
    return parentWidget() ? qreal(parentWidget()->width()) : qreal(width());
}

QList<QPair<MdSideSheetState, qreal>> MdSideSheet::availableAnchors() const
{
    QList<QPair<MdSideSheetState, qreal>> anchors;
    const qreal full = fullParentWidth();
    const qreal content = sheetWidth();
    for (int i = 0; i < int(MdSideSheetState::Count); ++i) {
        const auto state = MdSideSheetState(i);
        const qreal offset =
            MdSideSheetStyle::anchorOffsetX(m_edge, state, full, content, m_innerMargin);
        if (!std::isnan(offset)) {
            anchors.append({state, offset});
        }
    }
    return anchors;
}

void MdSideSheet::syncManagedHeight()
{
    if (!parentWidget()) {
        return;
    }
    // The docked sheet is as tall as its parent (`container.height` = 100%).
    if (height() != parentWidget()->height()) {
        resize(width(), parentWidget()->height());
    }
}

void MdSideSheet::applyOffset(qreal offset)
{
    m_offsetX = offset;
    if (parentWidget() && m_geometryManaged) {
        // The container x is the anchor; the widget sits one shadow margin
        // to the LEFT of it on the right edge (the margin is on the inner
        // side), flush on the left edge.
        const int x = int(std::round(m_edge == MdSideSheetEdge::Right
                                         ? offset - MdSideSheetTokens::kShadowMargin
                                         : offset));
        const int y = 0;
        if (pos() != QPoint(x, y)) {
            move(x, y);
        }
    }
    emit offsetChanged(offset);
}

void MdSideSheet::snapToAnchor()
{
    const qreal offset =
        MdSideSheetStyle::anchorOffsetX(m_edge, m_state, fullParentWidth(), sheetWidth(),
                                        m_innerMargin);
    if (!std::isnan(offset)) {
        applyOffset(offset);
    }
    if (m_state == MdSideSheetState::Hidden) {
        hide();
    }
}

void MdSideSheet::animateTo(MdSideSheetState state)
{
    const bool returningToState = state == m_state && m_phase == Phase::Idle;
    if (!returningToState && !confirmed(state)) {
        // Vetoed: fall back to the settled state's anchor.
        const qreal settled =
            MdSideSheetStyle::anchorOffsetX(m_edge, m_state, fullParentWidth(), sheetWidth(),
                                            m_innerMargin);
        if (!std::isnan(settled) && m_offsetX != settled) {
            state = m_state;
        } else {
            return;
        }
    }

    const qreal target = MdSideSheetStyle::anchorOffsetX(m_edge, state, fullParentWidth(),
                                                         sheetWidth(), m_innerMargin);
    if (std::isnan(target)) {
        return;
    }

    m_target = state;
    m_animationFrom = m_offsetX;
    m_animationTo = target;
    m_animationIsHide = state == MdSideSheetState::Hidden;
    m_phase = Phase::Animating;
    m_animationClock->restart();
    m_animationTimer->start();

    if (state != MdSideSheetState::Hidden) {
        raise();
        show();
    }
}

void MdSideSheet::settleAt(MdSideSheetState state)
{
    const bool wasHidden = m_state == MdSideSheetState::Hidden;
    m_state = state;
    if (!wasHidden || state != MdSideSheetState::Hidden) {
        emit stateChanged(state);
    }
    if (state == MdSideSheetState::Hidden) {
        hide();
        // The dialog cancel flow — only when the sheet actually went away
        // from a visible state (MDC's SideSheetDialog cancels when the
        // behavior reports HIDDEN).
        if (!wasHidden) {
            emit dismissed();
        }
    }
}

void MdSideSheet::mousePressEvent(QMouseEvent *event)
{
    // A press interrupts a running animation at the current offset (MDC's
    // drag helper does the same to its settle motion).
    if (m_phase == Phase::Animating) {
        m_animationTimer->stop();
        m_phase = Phase::Idle;
    }
    m_dragging = true;
    m_dragMoved = false;
    m_dragStartX = mousePosition(event).x();
    m_dragStartOffset = m_offsetX;
    event->accept();
}

void MdSideSheet::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    const qreal dx = mousePosition(event).x() - m_dragStartX;
    if (!m_dragMoved && qAbs(dx) > kDragSlop) {
        m_dragMoved = true;
    }
    if (!m_dragMoved) {
        return;
    }

    // Clamp between the hidden anchor and the expanded anchor — MDC clamps
    // the drag into [minViewPositionHorizontal, maxViewPositionHorizontal];
    // the Qt port clamps identically (no velocity fling is ported).
    qreal lowest = std::numeric_limits<qreal>::max();
    qreal highest = std::numeric_limits<qreal>::lowest();
    const auto anchors = availableAnchors();
    for (const auto &anchor : anchors) {
        lowest = qMin(lowest, anchor.second);
        highest = qMax(highest, anchor.second);
    }
    const qreal offset = qBound(lowest, m_dragStartOffset + dx, highest);
    applyOffset(offset);
    event->accept();
}

void MdSideSheet::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    m_dragging = false;
    if (m_dragMoved) {
        m_dragMoved = false;
        // The `isReleasedCloseToInnerEdge` midpoint rule, positional: a drag
        // released closer to the expanded anchor than the hidden anchor
        // settles expanded, otherwise hidden. MDC additionally weighs the
        // release velocity (the 500 px/s threshold, the 0.1 friction against
        // the 0.5 hide threshold) — not ported.
        qreal best = std::numeric_limits<qreal>::max();
        MdSideSheetState target = m_state;
        const auto anchors = availableAnchors();
        for (const auto &anchor : anchors) {
            const qreal distance = qAbs(anchor.second - m_offsetX);
            if (distance < best) {
                best = distance;
                target = anchor.first;
            }
        }
        animateTo(target);
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void MdSideSheet::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

bool MdSideSheet::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize && m_geometryManaged) {
        syncManagedHeight();
        if (m_phase == Phase::Animating) {
            // Keep chasing the target anchor at the parent's new size.
            const qreal target = MdSideSheetStyle::anchorOffsetX(
                m_edge, m_target, fullParentWidth(), sheetWidth(), m_innerMargin);
            if (!std::isnan(target)) {
                m_animationTo = target;
            }
        } else if (m_state == MdSideSheetState::Hidden) {
            hide();
        } else {
            snapToAnchor();
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace md
