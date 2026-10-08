#ifndef MD_SIDE_SHEET_H
#define MD_SIDE_SHEET_H

// MdSideSheet — MD3 side sheets: the sheet surface itself.
//
// The `md.comp.sheet.side.*` family. Neither material-web (token export
// only) nor Compose M3 (no side sheet ships in material3) implements the
// behaviour, so the behaviour source is the Android Views library
// (MDC-Android, `com.google.android.material.sidesheet` — the one
// implementation the official overview page lists as available) plus the
// spec measurements from m3.material.io/components/side-sheets/specs:
//
//     Hidden      fully off the docked edge (the parent's width away)
//     Expanded    docked against the edge, the content-facing gap = the
//                 inner margin
//
// The two presentations share one surface here, selected by kind:
//
//   * `Standard` (docked, persistent): surface at level 0, square corners,
//     starts Expanded — a sheet co-existing with the screen's UI, the
//     optional `outline` divider separating it from the content;
//   * `Modal`: surface-container-low at level 1 with the `corner-large-start`
//     radius on the content-facing corner pair, starts Hidden. The modal
//     *wrapper* — scrim, Escape / outside-click dismissal — is
//     `MdSideSheetHost`, the same split MDC makes between
//     `SideSheetBehavior` and `SideSheetDialog`.
//
// Interaction, transcribed from the MDC sources:
//
//   * horizontal drags move the sheet; on release it settles by the
//     `isReleasedCloseToInnerEdge` midpoint rule — closer to the expanded
//     anchor than the hidden anchor expands, otherwise hides. The
//     velocity-weighted projection (the 500 px/s significant threshold, the
//     0.1 hide friction) is recorded, not ported (docs/porting-todo.md);
//   * expand animations run the spatial-default spring, hide animations the
//     fast-effects spring — matching the bottom sheet's transcription of
//     Compose's motion specs (MDC slides with window enter/exit animations);
//   * `confirmValueChange` can veto any pending state change;
//   * settling at Hidden hides the widget and emits `dismissed()` — the
//     dialog cancel flow.
//
// The widget rect carries the shadow margin on the top/bottom/inner edges
// and a flush DOCKED edge, so the *container* x — the anchor offsets — is
// the widget's x plus the margin on the right edge, the widget's x on the
// left. `currentOffset()` reports the container x.

#include "core/MdQtCompat.h"
#include "core/MdSideSheetTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"
#include "styles/MdSideSheetStyle.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRectF>
#include <QtCore/QTimer>
#include <QtWidgets/QWidget>

#include <functional>

namespace md {

class MdSideSheetStyle;

class QT_MD3_EXPORT MdSideSheet : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(MdSideSheetState state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(bool dividerVisible READ hasDivider WRITE setDividerVisible)

public:
    explicit MdSideSheet(MdSideSheetKind kind = MdSideSheetKind::Modal,
                         MdSideSheetEdge edge = MdSideSheetEdge::Right, QWidget *parent = nullptr);
    ~MdSideSheet() override;

    // --- configuration ------------------------------------------------------
    MdSideSheetKind kind() const { return m_kind; }
    void setSheetKind(MdSideSheetKind kind);

    MdSideSheetEdge edge() const { return m_edge; }
    void setSheetEdge(MdSideSheetEdge edge);

    /// The gap between the sheet's content edge and the parent's opposite
    /// edge when expanded (MDC's `innerMargin`, from the layout margins).
    /// 0 for the docked sheets; the detached presentation's 16 px margin is
    /// recorded, not ported.
    qreal innerMargin() const { return m_innerMargin; }
    void setInnerMargin(qreal margin);

    /// The optional divider strip along the content-facing edge (the
    /// standard sheet's anatomy lists it; the modal's too).
    bool hasDivider() const { return m_hasDivider; }
    void setDividerVisible(bool visible);

    /// The `confirmValueChange` veto: return false from the callback to
    /// refuse a pending state change (MDC's state setters).
    void setConfirmValueChange(std::function<bool(MdSideSheetState)> confirm);
    std::function<bool(MdSideSheetState)> confirmValueChange() const { return m_confirm; }

    /// Whether the sheet manages its own geometry inside the parent (the
    /// modal host takes over: anchoring, the hidden slide, the full-height
    /// sizing). Off by default so a statically-placed sheet — a gallery
    /// snapshot, a fixed scaffold slot — never jumps to its anchors when
    /// the parent resizes.
    bool geometryManaged() const { return m_geometryManaged; }
    void setGeometryManaged(bool managed);

    // --- tokens -------------------------------------------------------------
    /// The resolved `md.comp.sheet.side.*` set, after the application-wide
    /// and per-instance `md.comp.*` overrides.
    const MdSideSheetTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this sheet alone.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- state --------------------------------------------------------------
    MdSideSheetState state() const { return m_state; }
    /// The settled state's anchor target (the state an animation is running
    /// toward, or `state()` when settled).
    MdSideSheetState targetState() const { return m_target; }

    /// The container's x relative to the parent. Falls back to the tracked
    /// offset when there is no parent.
    qreal currentOffset() const;

    /// Snap the settled state without animating. No movement — callers
    /// position the widget (or let the anchor APIs do it).
    void setState(MdSideSheetState state);

    bool isAnimating() const { return m_phase != Phase::Idle; }

    // --- geometry -----------------------------------------------------------
    /// The painted container: the widget rect with the shadow margin on the
    /// top/bottom/inner edges and a flush docked edge.
    QRectF containerRect() const;

    /// The layout computed for the current size.
    MdSideSheetStyle::Layout currentLayout() const;

    /// The container's width (the widget rect minus the inner shadow
    /// margin).
    qreal sheetWidth() const;

public slots:
    /// Animate to Expanded — MDC's `setState(STATE_EXPANDED)`.
    void expandSheet();

    /// Animate to Hidden — MDC's `setState(STATE_HIDDEN)`. Runs the confirm
    /// veto; settling emits `dismissed()` and hides the widget.
    void hideSheet();

signals:
    void stateChanged(md::MdSideSheetState state);
    /// The sheet settled at Hidden (drag, hideSheet) — the dialog cancel
    /// flow.
    void dismissed();
    void offsetChanged(qreal offset);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class Phase
    {
        Idle,
        Animating,
    };

    void init();
    void onThemeChanged();

    // The anchor context: the parent's width and the container's width.
    qreal fullParentWidth() const;

    void animateTo(MdSideSheetState state);
    bool confirmed(MdSideSheetState state);
    void applyOffset(qreal offset);
    void snapToAnchor();
    void settleAt(MdSideSheetState state);
    QList<QPair<MdSideSheetState, qreal>> availableAnchors() const;
    void syncManagedHeight();

    MdSideSheetKind m_kind = MdSideSheetKind::Modal;
    MdSideSheetEdge m_edge = MdSideSheetEdge::Right;
    MdSideSheetState m_state = MdSideSheetState::Hidden;
    MdSideSheetState m_target = MdSideSheetState::Hidden;
    qreal m_offsetX = 0.0;
    qreal m_innerMargin = 0.0;
    bool m_hasDivider = false;

    std::function<bool(MdSideSheetState)> m_confirm;

    // Drag bookkeeping.
    bool m_dragging = false;
    bool m_dragMoved = false;
    qreal m_dragStartX = 0.0;
    qreal m_dragStartOffset = 0.0;

    Phase m_phase = Phase::Idle;
    bool m_geometryManaged = false;
    QTimer *m_animationTimer = nullptr;
    QElapsedTimer *m_animationClock = nullptr;
    qreal m_animationFrom = 0.0;
    qreal m_animationTo = 0.0;
    bool m_animationIsHide = false;

    mutable MdSideSheetTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    friend class MdSideSheetStyle;
    friend class MdSideSheetHost;
};

} // namespace md

#endif // MD_SIDE_SHEET_H
