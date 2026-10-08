#include "MdBottomSheet.h"

#include "styles/MdBottomSheetStyle.h"

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

MdBottomSheet::MdBottomSheet(MdSheetKind kind, QWidget *parent)
    : QWidget(parent)
    , m_kind(kind)
{
    // Compose's kinds: the standard sheet skips the hidden state by default,
    // the modal sheet always has it.
    m_skipHidden = kind == MdSheetKind::Standard;

    init();
}

MdBottomSheet::~MdBottomSheet() = default;

void MdBottomSheet::init()
{
    setFocusPolicy(Qt::NoFocus);
    MdBottomSheetStyle::shared();
    MdStyleBase::connectThemeUpdate(this, &MdBottomSheet::onThemeChanged);
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
}

void MdBottomSheet::onThemeChanged()
{
    m_tokensDirty = true;
    update();
}

void MdBottomSheet::setSheetKind(MdSheetKind kind)
{
    if (m_kind == kind) {
        return;
    }
    m_kind = kind;
    // The kinds carry different hidden-state defaults.
    m_skipHidden = kind == MdSheetKind::Standard;
}

void MdBottomSheet::setPeekHeight(qreal height)
{
    if (qFuzzyCompare(m_peekHeight, height)) {
        return;
    }
    m_peekHeight = qMax(0.0, height);
    if (!isAnimating()) {
        snapToAnchor();
    }
    update();
}

void MdBottomSheet::setSkipHiddenState(bool skip)
{
    if (m_skipHidden == skip) {
        return;
    }
    m_skipHidden = skip;
    if (!isAnimating()) {
        snapToAnchor();
    }
}

void MdBottomSheet::setSkipPartiallyExpanded(bool skip)
{
    if (m_skipPartial == skip) {
        return;
    }
    m_skipPartial = skip;
    if (!isAnimating()) {
        snapToAnchor();
    }
}

void MdBottomSheet::setGeometryManaged(bool managed)
{
    if (m_geometryManaged == managed) {
        return;
    }
    m_geometryManaged = managed;
    if (managed && !isAnimating()) {
        snapToAnchor();
        if (m_state != MdSheetState::Hidden) {
            show();
        }
    }
}

void MdBottomSheet::setHandleVisible(bool visible)
{
    if (m_hasHandle == visible) {
        return;
    }
    m_hasHandle = visible;
    update();
}

void MdBottomSheet::setConfirmValueChange(std::function<bool(MdSheetState)> confirm)
{
    m_confirm = std::move(confirm);
}

const MdBottomSheetTokens &MdBottomSheet::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdBottomSheetTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

qreal MdBottomSheet::currentOffset() const
{
    if (parentWidget()) {
        return qreal(y()) + MdBottomSheetTokens::kShadowMargin;
    }
    return m_offsetY;
}

void MdBottomSheet::setState(MdSheetState state)
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

QRectF MdBottomSheet::containerRect() const
{
    return QRectF(rect()).adjusted(MdBottomSheetTokens::kShadowMargin,
                                   MdBottomSheetTokens::kShadowMargin,
                                   -MdBottomSheetTokens::kShadowMargin, 0.0);
}

MdBottomSheetStyle::Layout MdBottomSheet::currentLayout() const
{
    return MdBottomSheetStyle::layoutFor(qreal(width()), qreal(height()), m_hasHandle, tokens());
}

QSize MdBottomSheet::sizeHint() const
{
    // Compose's sheet height is its content's height; this surface is
    // height-driven by its caller, so the hint is the width cap and a
    // workable default height.
    qreal width = MdBottomSheetTokens::kSheetMaxWidth;
    if (parentWidget()) {
        width = qMin(width, qreal(parentWidget()->width()));
    }
    return QSize(int(width), 256);
}

void MdBottomSheet::showSheet()
{
    // Compose's show(): PartiallyExpanded when it has the anchor, otherwise
    // Expanded (skipPartiallyExpanded sheets go straight up).
    if (MdBottomSheetStyle::hasAnchor(m_kind, MdSheetState::PartiallyExpanded, fullParentHeight(),
                                      sheetHeight(), m_peekHeight, m_skipHidden, m_skipPartial)) {
        animateTo(MdSheetState::PartiallyExpanded);
    } else {
        animateTo(MdSheetState::Expanded);
    }
}

void MdBottomSheet::hideSheet()
{
    animateTo(MdSheetState::Hidden);
}

void MdBottomSheet::expand()
{
    animateTo(MdSheetState::Expanded);
}

void MdBottomSheet::partialExpand()
{
    animateTo(MdSheetState::PartiallyExpanded);
}

bool MdBottomSheet::confirmed(MdSheetState state)
{
    if (!m_confirm) {
        return true;
    }
    return m_confirm(state);
}

qreal MdBottomSheet::fullParentHeight() const
{
    // Compose's fullHeight: the layout height the sheet lives in — here the
    // parent's height, or the sheet's own when it has no parent (rare).
    return parentWidget() ? qreal(parentWidget()->height()) : qreal(height());
}

qreal MdBottomSheet::sheetHeight() const
{
    // The container's height: the widget rect carries the top shadow margin
    // only (the bottom edge is flush).
    return qMax(0.0, qreal(height()) - MdBottomSheetTokens::kShadowMargin);
}

QList<QPair<MdSheetState, qreal>> MdBottomSheet::availableAnchors() const
{
    QList<QPair<MdSheetState, qreal>> anchors;
    const qreal full = fullParentHeight();
    const qreal content = sheetHeight();
    for (int i = 0; i < int(MdSheetState::Count); ++i) {
        const auto state = MdSheetState(i);
        const qreal offset = MdBottomSheetStyle::anchorOffset(m_kind, state, full, content,
                                                              m_peekHeight, m_skipHidden,
                                                              m_skipPartial);
        if (!std::isnan(offset)) {
            anchors.append({state, offset});
        }
    }
    return anchors;
}

void MdBottomSheet::applyOffset(qreal offset)
{
    m_offsetY = offset;
    if (parentWidget() && m_geometryManaged) {
        // The container top-y is the anchor; the widget sits one shadow
        // margin above it and is centred horizontally (Compose's scaffold
        // centres the sheet; the widthIn(max) cap is the caller's business).
        const int x = int(std::round((qreal(parentWidget()->width()) - qreal(width())) / 2.0));
        const int y = int(std::round(offset - MdBottomSheetTokens::kShadowMargin));
        if (pos() != QPoint(x, y)) {
            move(x, y);
        }
    }
    emit offsetChanged(offset);
}

void MdBottomSheet::snapToAnchor()
{
    const qreal offset = MdBottomSheetStyle::anchorOffset(m_kind, m_state, fullParentHeight(),
                                                          sheetHeight(), m_peekHeight,
                                                          m_skipHidden, m_skipPartial);
    if (!std::isnan(offset)) {
        applyOffset(offset);
    }
    if (m_state == MdSheetState::Hidden) {
        hide();
    }
}

void MdBottomSheet::animateTo(MdSheetState state)
{
    const bool returningToState = state == m_state && m_phase == Phase::Idle;
    if (!returningToState && !confirmed(state)) {
        // Vetoed: fall back to the settled state's anchor.
        if (m_offsetY != MdBottomSheetStyle::anchorOffset(m_kind, m_state, fullParentHeight(),
                                                          sheetHeight(), m_peekHeight,
                                                          m_skipHidden, m_skipPartial)) {
            state = m_state;
        } else {
            return;
        }
    }

    const qreal target =
        MdBottomSheetStyle::anchorOffset(m_kind, state, fullParentHeight(), sheetHeight(),
                                         m_peekHeight, m_skipHidden, m_skipPartial);
    if (std::isnan(target)) {
        return;
    }

    m_target = state;
    m_animationFrom = m_offsetY;
    m_animationTo = target;
    m_animationIsHide = state == MdSheetState::Hidden;
    m_phase = Phase::Animating;
    m_animationClock->restart();
    m_animationTimer->start();

    if (state != MdSheetState::Hidden) {
        raise();
        show();
    }
}

void MdBottomSheet::settleAt(MdSheetState state)
{
    const bool wasHidden = m_state == MdSheetState::Hidden;
    m_state = state;
    if (!wasHidden || state != MdSheetState::Hidden) {
        emit stateChanged(state);
    }
    if (state == MdSheetState::Hidden) {
        hide();
        // The onDismissRequest flow — only when the sheet actually went away
        // from a visible state (Compose invokes it when `!state.isVisible`).
        if (!wasHidden) {
            emit dismissed();
        }
    }
}

void MdBottomSheet::handleToggle()
{
    // Compose's DragHandleWithTooltip click behaviour. Modal: Expanded
    // collapses through the dismiss flow; Standard: Expanded collapses to
    // partial when the hidden state is skipped, otherwise hides.
    switch (m_state) {
    case MdSheetState::Expanded:
        if (m_kind == MdSheetKind::Modal || !m_skipHidden) {
            hideSheet();
        } else {
            partialExpand();
        }
        break;
    case MdSheetState::PartiallyExpanded:
        expand();
        break;
    case MdSheetState::Hidden:
    default:
        showSheet();
        break;
    }
}

void MdBottomSheet::mousePressEvent(QMouseEvent *event)
{
    // A press interrupts a running animation at the current offset (Compose's
    // anchoredDraggable does the same to its motion).
    if (m_phase == Phase::Animating) {
        m_animationTimer->stop();
        m_phase = Phase::Idle;
    }
    m_dragging = true;
    m_dragMoved = false;
    m_pressedHandle = currentLayout().handleRect.contains(mousePosition(event));
    m_dragStartY = mousePosition(event).y();
    m_dragStartOffset = m_offsetY;
    event->accept();
}

void MdBottomSheet::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        QWidget::mouseMoveEvent(event);
        return;
    }
    const qreal dy = mousePosition(event).y() - m_dragStartY;
    if (!m_dragMoved && qAbs(dy) > kDragSlop) {
        m_dragMoved = true;
    }
    if (!m_dragMoved) {
        return;
    }

    // Clamp between the topmost available anchor and the hidden anchor —
    // Compose lets a bouncy spring overshoot and scales; the Qt port clamps
    // (no velocity fling is ported, so the overshoot regime is unreachable).
    qreal lowest = fullParentHeight();
    qreal highest = 0.0;
    const auto anchors = availableAnchors();
    for (const auto &anchor : anchors) {
        lowest = qMin(lowest, anchor.second);
        highest = qMax(highest, anchor.second);
    }
    const qreal offset = qBound(lowest, m_dragStartOffset + dy, highest);
    applyOffset(offset);
    event->accept();
}

void MdBottomSheet::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    m_dragging = false;
    if (m_dragMoved) {
        m_dragMoved = false;
        // Positional settle: once the drag has moved at least the 56 px
        // positional threshold from the settled state's anchor, the sheet
        // goes to the nearest anchor; otherwise it returns. Compose's
        // anchoredDraggable additionally weighs velocity — not ported.
        const qreal original =
            MdBottomSheetStyle::anchorOffset(m_kind, m_state, fullParentHeight(), sheetHeight(),
                                             m_peekHeight, m_skipHidden, m_skipPartial);
        qreal originalOffset = std::isnan(original) ? m_offsetY : original;
        MdSheetState target = m_state;
        if (qAbs(m_offsetY - originalOffset) >= MdBottomSheetTokens::kPositionalThreshold) {
            qreal best = std::numeric_limits<qreal>::max();
            const auto anchors = availableAnchors();
            for (const auto &anchor : anchors) {
                const qreal distance = qAbs(anchor.second - m_offsetY);
                if (distance < best) {
                    best = distance;
                    target = anchor.first;
                }
            }
        }
        animateTo(target);
        return;
    }
    if (m_pressedHandle && currentLayout().handleRect.contains(mousePosition(event))) {
        m_pressedHandle = false;
        handleToggle();
    }
    QWidget::mouseReleaseEvent(event);
}

void MdBottomSheet::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

bool MdBottomSheet::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize && m_geometryManaged) {
        if (m_phase == Phase::Animating) {
            // Keep chasing the target anchor at the parent's new size.
            const qreal target =
                MdBottomSheetStyle::anchorOffset(m_kind, m_target, fullParentHeight(),
                                                 sheetHeight(), m_peekHeight, m_skipHidden,
                                                 m_skipPartial);
            if (!std::isnan(target)) {
                m_animationTo = target;
            }
        } else if (m_state == MdSheetState::Hidden) {
            hide();
        } else {
            snapToAnchor();
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace md
