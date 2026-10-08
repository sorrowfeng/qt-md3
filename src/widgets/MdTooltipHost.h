#ifndef MD_TOOLTIP_HOST_H
#define MD_TOOLTIP_HOST_H

// MdTooltipHost — the anchor machinery around MdTooltip.
//
// Compose splits the family in two: PlainTooltip / RichTooltip are "only the
// visuals", and BasicTooltipBox / TooltipBox own everything else. This host is
// that other half, ported onto Qt's widget tree:
//
//   * the host wraps the anchor widget (setAnchorWidget reparents it in) and
//     drives a top-level tooltip popup above (or below) it;
//   * triggers, from BasicTooltip.kt: mouse hover shows immediately and —
//     being Compose's UserInput priority — never self-dismisses; the pointer
//     leaving dismisses it (unless persistent); keyboard focus shows it with
//     the BasicTooltipDefaults.TooltipDuration (1500 ms) auto-hide on a
//     non-persistent tooltip; touch long-press (QStyleHints'
//     mousePressAndHoldInterval stands in for viewConfiguration) shows it
//     with the same 1500 ms auto-hide; Escape dismisses; a Tab from a visible
//     action-bearing tooltip's anchor moves focus into the tooltip;
//   * BasicTooltipDefaults.GlobalMutatorMutex: exactly one tooltip visible
//     application-wide — a new show cancels the previous one instantly;
//   * the enter/exit transition is the same fade + scale the snackbar host
//     runs: opacity on the effects-fast spring, scale 0.8→1 on the
//     spatial-fast spring (TooltipBox animates exactly these two).
//
// The persistent flag (TooltipState.isPersistent) default false — an
// actionable rich tooltip is the documented case for setting it true.

#include "core/MdTooltipTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdTooltip;

class QT_MD3_EXPORT MdTooltipHost : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool persistent READ isPersistent WRITE setPersistent NOTIFY persistentChanged)
    /// Where the popup sits relative to the anchor. Left / right / start /
    /// end positioning is not ported (see docs/porting-todo.md).
    Q_PROPERTY(md::MdTooltipHost::AnchorPosition anchorPosition READ anchorPosition WRITE
                   setAnchorPosition NOTIFY anchorPositionChanged)
    /// Forwarded to the tooltip popup's caret side (Bottom above the anchor,
    /// Top below it).
    Q_PROPERTY(bool caretEnabled READ isCaretEnabled WRITE setCaretEnabled NOTIFY
                   caretEnabledChanged)

public:
    enum class AnchorPosition
    {
        Above,
        Below,
        Count,
    };
    Q_ENUM(AnchorPosition)

    explicit MdTooltipHost(QWidget *parent = nullptr);
    ~MdTooltipHost() override;

    // --- anchor ---------------------------------------------------------------
    /// The anchor widget, reparented into the host. The host's size hint is
    /// the anchor's.
    void setAnchorWidget(QWidget *anchor);
    QWidget *anchorWidget() const { return m_anchor; }

    // --- popup configuration ----------------------------------------------------
    /// The tooltip popup (owned, created lazily). Configure content through
    /// it: variant, text, title, actionLabel.
    MdTooltip *tooltip() const { return m_tooltip; }

    bool isPersistent() const { return m_persistent; }
    void setPersistent(bool persistent);

    AnchorPosition anchorPosition() const { return m_anchorPosition; }
    void setAnchorPosition(AnchorPosition position);

    bool isCaretEnabled() const { return m_caretEnabled; }
    void setCaretEnabled(bool enabled);

    // --- requests ---------------------------------------------------------------
    /// Show the popup now. The trigger decides the auto-hide: a programmatic
    /// (or focus / long-press) show on a non-persistent tooltip dismisses
    /// after TooltipDuration (1500 ms); pass `fromHover` for the hover path,
    /// which never self-dismisses.
    void showTooltip(bool fromHover = false);

    /// Dismiss the popup (with the exit transition).
    void dismiss();

    // --- observable state (for tests and callers) --------------------------------
    bool isShowing() const { return m_showing; }
    /// The auto-hide timer is scheduled (non-persistent, non-hover show).
    bool isAutoHideScheduled() const { return m_autoHideTimer->isActive(); }
    /// The current enter/exit transition values — 1/1 fully shown.
    qreal currentOpacity() const { return m_opacity; }
    qreal currentScale() const { return m_scale; }
    bool isTransitioning() const { return m_phase != Phase::Idle; }

signals:
    void persistentChanged(bool persistent);
    void anchorPositionChanged(md::MdTooltipHost::AnchorPosition position);
    void caretEnabledChanged(bool enabled);
    /// The popup became fully visible / fully hidden (transition settled).
    void shown();
    void hidden();
    /// The rich action was activated while the popup showed.
    void actionClicked();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onAutoHideTimeout();

private:
    enum class Phase
    {
        Idle,
        Entering,
        Leaving,
    };

    void ensureTooltip();
    void placePopup();
    void startTransition(Phase phase);
    void onTransitionTick();
    void finishLeaving();
    QRect anchorGlobalRect() const;
    QSize surfaceSize() const;

    QWidget *m_anchor = nullptr;
    MdTooltip *m_tooltip = nullptr;
    bool m_persistent = false;
    AnchorPosition m_anchorPosition = AnchorPosition::Above;
    bool m_caretEnabled = false;
    bool m_showing = false;

    QTimer *m_autoHideTimer = nullptr;
    QTimer *m_pressHoldTimer = nullptr;
    bool m_shownByLongPress = false;
    QTimer *m_transitionTimer = nullptr;
    QElapsedTimer m_transitionClock;
    Phase m_phase = Phase::Idle;
    qreal m_opacity = 1.0;
    qreal m_scale = 1.0;
    QRect m_restingGeometry;
};

} // namespace md

#endif // MD_TOOLTIP_HOST_H
