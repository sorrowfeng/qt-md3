#include "MdTooltipHost.h"

#include "MdTooltip.h"

#include "styles/MdTooltipStyle.h"

#include "core/MdMotion.h"

#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtGui/QStyleHints>
#include <QtCore/QTimer>
#include <QtGui/QCursor>
#include <QtGui/QGuiApplication>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QApplication>

#include <cmath>

namespace md {

namespace {

/// The transition tick granularity (~60 fps, the same cadence the other
/// animation cycles use).
constexpr int kTransitionTickMs = 16;

/// The scale value Compose's FadeInFadeOutWithScale starts from.
constexpr qreal kEnterScaleStart = 0.8;

/// BasicTooltipDefaults.GlobalMutatorMutex: the host whose tooltip is showing
/// application-wide, so a new show cancels the previous one instantly.
QPointer<MdTooltipHost> &globalShowingHost()
{
    static QPointer<MdTooltipHost> current;
    return current;
}

} // namespace

MdTooltipHost::MdTooltipHost(QWidget *parent)
    : QWidget(parent)
{
    m_autoHideTimer = new QTimer(this);
    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(int(MdTooltipTokens::kDurationMs));
    connect(m_autoHideTimer, &QTimer::timeout, this, &MdTooltipHost::onAutoHideTimeout);

    m_pressHoldTimer = new QTimer(this);
    m_pressHoldTimer->setSingleShot(true);
    m_pressHoldTimer->setInterval(QGuiApplication::styleHints()->mousePressAndHoldInterval());
    connect(m_pressHoldTimer, &QTimer::timeout, this, [this] {
        m_shownByLongPress = true;
        showTooltip(false);
    });

    // The popup exists from construction — callers configure its content
    // through tooltip() before showing.
    ensureTooltip();

    m_transitionTimer = new QTimer(this);
    m_transitionTimer->setInterval(kTransitionTickMs);
    connect(m_transitionTimer, &QTimer::timeout, this, &MdTooltipHost::onTransitionTick);
}

MdTooltipHost::~MdTooltipHost()
{
    if (globalShowingHost() == this) {
        globalShowingHost() = nullptr;
    }
}

void MdTooltipHost::setAnchorWidget(QWidget *anchor)
{
    if (m_anchor == anchor) {
        return;
    }
    if (m_anchor) {
        m_anchor->removeEventFilter(this);
    }
    m_anchor = anchor;
    if (m_anchor) {
        m_anchor->setParent(this);
        m_anchor->installEventFilter(this);
        m_anchor->setGeometry(rect());
    }
}

void MdTooltipHost::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // The anchor always fills the host, so a layout that moves the host moves
    // the anchor with it.
    if (m_anchor) {
        m_anchor->setGeometry(rect());
    }
}

void MdTooltipHost::setPersistent(bool persistent)
{
    if (m_persistent == persistent) {
        return;
    }
    m_persistent = persistent;
    emit persistentChanged(m_persistent);
}

void MdTooltipHost::setAnchorPosition(AnchorPosition position)
{
    if (m_anchorPosition == position) {
        return;
    }
    m_anchorPosition = position;
    if (m_tooltip) {
        m_tooltip->setCaretSide(
            m_caretEnabled
                ? (position == AnchorPosition::Above ? MdTooltipCaretSide::Bottom
                                                     : MdTooltipCaretSide::Top)
                : MdTooltipCaretSide::None);
        if (m_showing) {
            placePopup();
        }
    }
    emit anchorPositionChanged(m_anchorPosition);
}

void MdTooltipHost::setCaretEnabled(bool enabled)
{
    if (m_caretEnabled == enabled) {
        return;
    }
    m_caretEnabled = enabled;
    ensureTooltip();
    m_tooltip->setCaretSide(
        enabled ? (m_anchorPosition == AnchorPosition::Above ? MdTooltipCaretSide::Bottom
                                                             : MdTooltipCaretSide::Top)
                : MdTooltipCaretSide::None);
    if (m_showing) {
        placePopup();
    }
    emit caretEnabledChanged(m_caretEnabled);
}

void MdTooltipHost::ensureTooltip()
{
    if (m_tooltip) {
        return;
    }
    m_tooltip = new MdTooltip(QString(), nullptr);
    // The popup is a transient tool-tip window: frameless, translucent so the
    // shadow and caret margins show through.
    m_tooltip->setWindowFlag(Qt::ToolTip, true);
    m_tooltip->setAttribute(Qt::WA_TranslucentBackground);
    m_tooltip->setAttribute(Qt::WA_NoSystemBackground);
    m_tooltip->setCaretSide(
        m_caretEnabled
            ? (m_anchorPosition == AnchorPosition::Above ? MdTooltipCaretSide::Bottom
                                                         : MdTooltipCaretSide::Top)
            : MdTooltipCaretSide::None);
    connect(m_tooltip, &MdTooltip::actionClicked, this, &MdTooltipHost::actionClicked);
    // Escape from the popup itself dismisses too (a Tab may have moved focus
    // into an action-bearing tooltip).
    m_tooltip->installEventFilter(this);
}

QRect MdTooltipHost::anchorGlobalRect() const
{
    if (!m_anchor) {
        return {};
    }
    QRect anchorRect = m_anchor->rect();
    anchorRect.moveTopLeft(m_anchor->mapTo(m_anchor->window(), QPoint(0, 0)));
    return anchorRect;
}

QSize MdTooltipHost::surfaceSize() const
{
    if (!m_tooltip) {
        return {};
    }
    // The provider positions the SURFACE (container); the widget rect is the
    // container plus the shadow / caret margins.
    const MdTooltip *tooltip = m_tooltip;
    const bool rich = tooltip->variant() == MdTooltipVariant::Rich;
    const int margins = int(rich ? MdTooltipTokens::kShadowMargin * 2.0 : 0.0);
    const int widgetWidth = tooltip->sizeHint().width();
    const int containerWidth = widgetWidth - margins;
    const int containerHeight =
        tooltip->heightForWidth(widgetWidth)
        - int(rich ? MdTooltipTokens::kShadowMargin * 2.0 : 0.0);
    return QSize(containerWidth, containerHeight);
}

void MdTooltipHost::placePopup()
{
    if (!m_tooltip || !m_anchor) {
        return;
    }
    const QRect anchorRect = anchorGlobalRect();
    const QWidget *window = m_anchor->window();
    const QSize windowSize = window ? window->size() : QSize();
    const QSize size = surfaceSize();

    const bool above = m_anchorPosition == AnchorPosition::Above;
    const QPoint surface = above
        ? MdTooltipStyle::abovePopupPosition(anchorRect, size, windowSize,
                                             MdTooltipTokens::kAnchorSpacing)
        : MdTooltipStyle::richPopupPosition(anchorRect, size, windowSize,
                                            MdTooltipTokens::kAnchorSpacing);

    // The caret keeps its tip exactly kAnchorSpacing from the anchor
    // (recorded divergence: Compose draws the caret INTO the gap, overlapping
    // the anchor by caret - spacing).
    const bool rich = m_tooltip->variant() == MdTooltipVariant::Rich;
    const qreal horizontal = rich ? MdTooltipTokens::kShadowMargin : 0.0;
    const qreal shadowVertical = rich ? MdTooltipTokens::kShadowMargin : 0.0;
    const qreal caretMargin =
        m_caretEnabled ? MdTooltipTokens::kCaretMargin : 0.0;
    const int surfaceY = above ? surface.y() - int(caretMargin) : surface.y() + int(caretMargin);
    const QPoint widgetTopLeft(surface.x() - int(horizontal),
                               surfaceY - int(shadowVertical));

    const QSize widgetSize = m_tooltip->sizeHint();
    m_restingGeometry = QRect(widgetTopLeft, widgetSize);
    m_tooltip->setGeometry(m_restingGeometry);
}

void MdTooltipHost::showTooltip(bool fromHover)
{
    ensureTooltip();
    // GlobalMutatorMutex: the previous tooltip cancels instantly, without its
    // exit animation. dismiss() clears the global pointer, so hold the host
    // reference across both calls.
    if (MdTooltipHost *showing = globalShowingHost(); showing && showing != this) {
        showing->dismiss();
        showing->finishLeaving();
    }
    globalShowingHost() = this;

    placePopup();
    m_tooltip->show();
    m_showing = true;

    // Hover show is Compose's UserInput priority — no timeout; the pointer
    // leaving dismisses instead. Every other trigger self-dismisses a
    // non-persistent tooltip after TooltipDuration.
    if (!fromHover && !m_persistent) {
        m_autoHideTimer->start();
    } else {
        m_autoHideTimer->stop();
    }

    startTransition(Phase::Entering);
}

void MdTooltipHost::dismiss()
{
    if (!m_showing) {
        return;
    }
    m_autoHideTimer->stop();
    if (globalShowingHost() == this) {
        globalShowingHost() = nullptr;
    }
    startTransition(Phase::Leaving);
}

void MdTooltipHost::onAutoHideTimeout()
{
    dismiss();
}

bool MdTooltipHost::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_tooltip && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            dismiss();
            return true;
        }
        return QWidget::eventFilter(watched, event);
    }
    if (watched != m_anchor) {
        return QWidget::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::Enter: {
        // Mouse hover: shows immediately and never self-dismisses. Some
        // platforms synthesise an Enter when a window shows regardless of
        // the pointer's position; real hover means the pointer is inside the
        // anchor's bounds, so gate on the cursor before showing.
        const QPoint cursor = m_anchor->mapFromGlobal(QCursor::pos());
        if (m_anchor->rect().contains(cursor)) {
            showTooltip(true);
        }
        break;
    }
    case QEvent::Leave:
        if (!m_persistent) {
            dismiss();
        }
        break;
    case QEvent::MouseButtonPress:
        // Touch long-press: QStyleHints' press-and-hold interval stands in
        // for viewConfiguration.longPressTimeoutMillis. Releasing before the
        // interval cancels; releasing after the tooltip showed dismisses it
        // (Compose's isLongPressed flow).
        if (!m_pressHoldTimer->isActive()) {
            m_pressHoldTimer->start();
        }
        break;
    case QEvent::MouseButtonRelease:
        m_pressHoldTimer->stop();
        if (m_shownByLongPress) {
            m_shownByLongPress = false;
            dismiss();
        }
        break;
    case QEvent::FocusIn: {
        auto *focusEvent = static_cast<QFocusEvent *>(event);
        const Qt::FocusReason reason = focusEvent->reason();
        if (reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason
            || reason == Qt::ShortcutFocusReason) {
            m_shownByLongPress = false;
            showTooltip(false);
        }
        break;
    }
    case QEvent::FocusOut:
        // Dismiss only when focus did not move INTO the tooltip popup (a Tab
        // from the anchor lands there).
        if (!m_tooltip || !m_tooltip->hasFocus()) {
            dismiss();
        }
        break;
    case QEvent::KeyPress: {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape && m_showing) {
            dismiss();
            return true;
        }
        // A Tab from the anchor moves focus into a visible action-bearing
        // tooltip (BasicTooltip's isTab branch).
        if ((keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) && m_showing
            && m_tooltip && m_tooltip->hasAction()) {
            m_tooltip->setFocus(keyEvent->key() == Qt::Key_Tab ? Qt::TabFocusReason
                                                               : Qt::BacktabFocusReason);
            return true;
        }
        break;
    }
    case QEvent::Resize:
        if (m_showing) {
            placePopup();
        }
        break;
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

void MdTooltipHost::startTransition(Phase phase)
{
    m_phase = phase;
    m_transitionClock.restart();
    m_transitionTimer->start();
    onTransitionTick();
}

void MdTooltipHost::onTransitionTick()
{
    if (m_phase == Phase::Idle) {
        m_transitionTimer->stop();
        return;
    }
    const MdSpring opacitySpring = MdMotion::spring(MotionSpring::EffectsFast);
    const MdSpring scaleSpring = MdMotion::spring(MotionSpring::SpatialFast);
    const qreal seconds = qreal(m_transitionClock.elapsed()) / 1000.0;
    const qreal settled = qMax(opacitySpring.settlingDurationMs(),
                               scaleSpring.settlingDurationMs());
    qreal opacity;
    qreal scale;
    if (m_phase == Phase::Entering) {
        opacity = opacitySpring.valueAt(seconds);
        scale = kEnterScaleStart + (1.0 - kEnterScaleStart) * scaleSpring.valueAt(seconds);
    } else {
        opacity = 1.0 - opacitySpring.valueAt(seconds);
        scale = 1.0 - (1.0 - kEnterScaleStart) * scaleSpring.valueAt(seconds);
    }
    m_opacity = qBound(0.0, opacity, 1.0);
    m_scale = qBound<qreal>(kEnterScaleStart, scale, 1.0);

    if (seconds * 1000.0 >= settled) {
        m_transitionTimer->stop();
        const bool wasLeaving = m_phase == Phase::Leaving;
        m_opacity = m_phase == Phase::Entering ? 1.0 : 0.0;
        m_scale = m_phase == Phase::Entering ? 1.0 : kEnterScaleStart;
        m_phase = Phase::Idle;
        if (wasLeaving) {
            finishLeaving();
        } else {
            emit shown();
        }
    }

    if (m_tooltip) {
        // The popup has no window opacity we can trust across platforms; the
        // style multiplies the painter opacity instead, and the scale
        // animates the geometry around the resting rect.
        m_tooltip->setPaintOpacity(m_opacity);
        const QPointF center = m_restingGeometry.center();
        QRect scaled = m_restingGeometry;
        scaled.setSize(QSize(int(std::round(qreal(m_restingGeometry.width()) * m_scale)),
                             int(std::round(qreal(m_restingGeometry.height()) * m_scale))));
        scaled.moveCenter(center.toPoint());
        m_tooltip->setGeometry(scaled);
    }
}

void MdTooltipHost::finishLeaving()
{
    m_showing = false;
    if (m_tooltip) {
        m_tooltip->hide();
    }
    emit hidden();
}

} // namespace md
