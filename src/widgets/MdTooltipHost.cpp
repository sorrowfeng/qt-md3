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
#include <QtGui/QScreen>
#include <QtGui/QWindow>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
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
        m_tooltip->setCaretSide(caretSideFor(position, m_caretEnabled));
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
    m_tooltip->setCaretSide(caretSideFor(m_anchorPosition, enabled));
    if (m_showing) {
        placePopup();
    }
    emit caretEnabledChanged(m_caretEnabled);
}

MdTooltipCaretSide MdTooltipHost::caretSideFor(AnchorPosition position, bool enabled)
{
    if (!enabled) {
        return MdTooltipCaretSide::None;
    }
    // The caret points back at the anchor: Bottom when the tip sits above,
    // Top when below, Right when the tip sits to the anchor's left, Left
    // when to the right. Start/End resolve through Left/Right at paint time
    // — the caret follows the *rendered* side, so the host maps them once
    // the popup has been placed; here the LTR reading is the default.
    switch (position) {
    case AnchorPosition::Above:
        return MdTooltipCaretSide::Bottom;
    case AnchorPosition::Below:
        return MdTooltipCaretSide::Top;
    case AnchorPosition::Left:
    case AnchorPosition::Start:
        return MdTooltipCaretSide::Right;
    case AnchorPosition::Right:
    case AnchorPosition::End:
        return MdTooltipCaretSide::Left;
    default:
        break;
    }
    return MdTooltipCaretSide::None;
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
    m_tooltip->setCaretSide(caretSideFor(m_anchorPosition, m_caretEnabled));
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
    // The popup is a top-level tool-tip window, so its geometry is global:
    // the anchor has to be reported in the same space or the tooltip lands
    // at the window-relative point as if it were a screen position.
    QRect anchorRect = m_anchor->rect();
    anchorRect.moveTopLeft(m_anchor->mapToGlobal(QPoint(0, 0)));
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
    // Clamp against the anchor window's global bounds (or the screen when
    // there is no window), so the popup stays in the same coordinate space
    // as `anchorRect` — both global.
    const QWidget *window = m_anchor->window();
    QRect available;
    if (window != nullptr && window->windowHandle() != nullptr) {
        available = window->windowHandle()->geometry();
    } else if (const QScreen *screen = QGuiApplication::primaryScreen()) {
        available = screen->availableGeometry();
    }
    const QSize size = surfaceSize();
    const bool rtl = m_anchor->layoutDirection() == Qt::RightToLeft;

    // Compose TooltipPositionProviderImpl: the placement flips to the
    // opposite side when it would clip `available`, then both axes are
    // coerced into the window. Start/End resolve through the layout
    // direction.
    QPoint surface;
    AnchorPosition resolved = m_anchorPosition;
    const bool rich = m_tooltip->variant() == MdTooltipVariant::Rich;
    switch (m_anchorPosition) {
    case AnchorPosition::Below:
        // The rich Below path keeps Compose's still-shipping rich provider
        // (start-aligned); plain Below uses the modern belowPositioning.
        if (rich) {
            surface = MdTooltipStyle::richPopupPosition(anchorRect, size, available,
                                                        MdTooltipTokens::kAnchorSpacing);
        } else {
            surface = MdTooltipStyle::belowPopupPosition(anchorRect, size, available,
                                                         MdTooltipTokens::kAnchorSpacing);
        }
        break;
    case AnchorPosition::Left:
        surface = MdTooltipStyle::leftPopupPosition(anchorRect, size, available,
                                                    MdTooltipTokens::kAnchorSpacing);
        break;
    case AnchorPosition::Right:
        surface = MdTooltipStyle::rightPopupPosition(anchorRect, size, available,
                                                     MdTooltipTokens::kAnchorSpacing);
        break;
    case AnchorPosition::Start:
        resolved = rtl ? AnchorPosition::Right : AnchorPosition::Left;
        surface = rtl ? MdTooltipStyle::rightPopupPosition(anchorRect, size, available,
                                                           MdTooltipTokens::kAnchorSpacing)
                      : MdTooltipStyle::leftPopupPosition(anchorRect, size, available,
                                                          MdTooltipTokens::kAnchorSpacing);
        break;
    case AnchorPosition::End:
        resolved = rtl ? AnchorPosition::Left : AnchorPosition::Right;
        surface = rtl ? MdTooltipStyle::leftPopupPosition(anchorRect, size, available,
                                                          MdTooltipTokens::kAnchorSpacing)
                      : MdTooltipStyle::rightPopupPosition(anchorRect, size, available,
                                                           MdTooltipTokens::kAnchorSpacing);
        break;
    case AnchorPosition::Above:
    default:
        // The Above provider (the modern Compose default): centre → start →
        // end horizontally, above → below vertically.
        surface = MdTooltipStyle::abovePopupPosition(anchorRect, size, available,
                                                     MdTooltipTokens::kAnchorSpacing);
        break;
    }

    // The caret follows the *resolved* side (Start/End flip under RTL).
    if (m_caretEnabled) {
        m_tooltip->setCaretSide(caretSideFor(resolved, true));
    }

    // The caret keeps its tip exactly kAnchorSpacing from the anchor
    // (recorded divergence: Compose draws the caret INTO the gap, overlapping
    // the anchor by caret - spacing).
    const bool horizontalPlacement =
        resolved == AnchorPosition::Left || resolved == AnchorPosition::Right;
    const qreal shadowHorizontal = rich ? MdTooltipTokens::kShadowMargin : 0.0;
    const qreal shadowVertical = rich ? MdTooltipTokens::kShadowMargin : 0.0;
    const qreal caretMargin = m_caretEnabled ? MdTooltipTokens::kCaretMargin : 0.0;
    // The caret margin sits on the anchor-facing edge.
    int surfaceX = surface.x();
    int surfaceY = surface.y();
    if (horizontalPlacement) {
        if (resolved == AnchorPosition::Left) {
            surfaceX += int(caretMargin);
        } else {
            surfaceX -= int(caretMargin);
        }
    } else if (resolved == AnchorPosition::Above) {
        surfaceY -= int(caretMargin);
    } else {
        surfaceY += int(caretMargin);
    }
    const QPoint widgetTopLeft(surfaceX - int(shadowHorizontal),
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
    // Closed values before the first paint: showing at the default
    // paintOpacity of 1.0 and only then fading is the flash this host
    // must not have.
    m_opacity = 0.0;
    m_scale = kEnterScaleStart;
    m_tooltip->setPaintOpacity(0.0);
    {
        const QPointF centre = m_restingGeometry.center();
        QRect scaled = m_restingGeometry;
        scaled.setSize(QSize(int(std::round(qreal(m_restingGeometry.width()) * m_scale)),
                             int(std::round(qreal(m_restingGeometry.height()) * m_scale))));
        scaled.moveCenter(centre.toPoint());
        m_tooltip->setGeometry(scaled);
    }
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

    // Outside-click dismissal of a persistent tooltip (the Compose
    // contract). A transient tooltip already dies on pointer leave / timeout.
    if (m_persistent && qApp != nullptr) {
        qApp->installEventFilter(this);
    }

    startTransition(Phase::Entering);
}

void MdTooltipHost::dismiss()
{
    if (!m_showing) {
        return;
    }
    m_autoHideTimer->stop();
    if (qApp != nullptr) {
        qApp->removeEventFilter(this);
    }
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
    // Outside-click dismissal for a persistent tooltip: any press that lands
    // outside the popup and outside the anchor closes it (Compose's
    // documented contract for actionable rich tooltips). The filter is
    // installed on qApp, so `watched` is the receiving object — not qApp.
    if (m_persistent && m_showing
        && (event->type() == QEvent::MouseButtonPress
            || event->type() == QEvent::TouchBegin)
        && watched != m_tooltip && watched != m_anchor) {
        // A press on anything that is not the anchor, the popup, or one of
        // their children is an outside click. Prefer widget topology over
        // global coordinates — synthetic test events do not always carry a
        // usable global position.
        auto *watchedWidget = qobject_cast<QWidget *>(watched);
        const bool insideTooltip =
            m_tooltip != nullptr
            && (watchedWidget == m_tooltip
                || (watchedWidget != nullptr && m_tooltip->isAncestorOf(watchedWidget)));
        const bool insideAnchor =
            m_anchor != nullptr
            && (watchedWidget == m_anchor
                || (watchedWidget != nullptr && m_anchor->isAncestorOf(watchedWidget)));
        if (!insideTooltip && !insideAnchor) {
            dismiss();
        }
        return false;
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
