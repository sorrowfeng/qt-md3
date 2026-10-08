#ifndef MD_BOTTOM_SHEET_H
#define MD_BOTTOM_SHEET_H

// MdBottomSheet — MD3 bottom sheets: the sheet surface itself.
//
// The `md.comp.sheet.bottom.*` family. material-web publishes no sheet *web
// component* (token export only — the same situation as the card and the
// dialog), so the behaviour source is Compose M3's BottomSheet.kt /
// BottomSheetScaffold.kt / ModalBottomSheet.kt / SheetDefaults.kt: a Surface
// with a `draggableAnchors` block that settles between three states —
//
//     Hidden              the sheet's top sits at the parent's bottom edge
//     PartiallyExpanded   standard: the peek height (56 px) visible
//                         modal: min(parent/2, sheet/2) visible — Compose's
//                         deterministic rule, flag-on by default
//     Expanded            the full sheet height visible
//
// The two Compose presentations share one surface here, selected by kind:
//
//   * `Standard` (BottomSheetScaffold): the peek height drives the partial
//     anchor and the hidden state is skipped by default (`skipHiddenState`
//     defaults true) — a persistent sheet co-existing with the screen's UI;
//   * `Modal` (BottomSheetImpl): the hidden anchor always exists and the
//     partial anchor derives from the heights. The modal *wrapper* — scrim,
//     Escape / outside-click dismissal — is `MdBottomSheetHost`, the same
//     split Compose makes between `BottomSheet` and `ModalBottomSheet`.
//
// Interaction, transcribed from the Compose sources:
//
//   * vertical drags move the sheet; on release it settles to the nearest
//     anchor once the drag has moved at least the 56 px positional threshold
//     (the velocity fling is not ported — recorded in docs/porting-todo.md);
//   * the drag handle toggles: Expanded collapses (modal: dismiss flow;
//     standard: partial expand, or hide when the hidden state exists),
//     PartiallyExpanded expands, Hidden shows;
//   * show animations run the spatial-default spring, hide animations the
//     fast-effects spring — `state.showMotionSpec` / `state.hideMotionSpec`;
//   * `confirmValueChange` can veto any pending state change;
//   * settling at Hidden hides the widget and emits `dismissed()` — the
//     onDismissRequest flow.
//
// The widget rect carries the shadow margin on the top/left/right and a
// flush bottom edge (a sheet is edge-to-edge with the parent's bottom), so
// the *container* top-y — the anchor offsets — is the widget's y plus the
// margin. `currentOffset()` reports the container top-y, matching Compose's
// offset semantics.

#include "core/MdBottomSheetTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"
#include "styles/MdBottomSheetStyle.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRectF>
#include <QtWidgets/QWidget>

#include <functional>

namespace md {

class MdBottomSheetStyle;

class QT_MD3_EXPORT MdBottomSheet : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(MdSheetState state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(bool handleVisible READ hasHandle WRITE setHandleVisible)

public:
    explicit MdBottomSheet(MdSheetKind kind = MdSheetKind::Modal, QWidget *parent = nullptr);
    ~MdBottomSheet() override;

    // --- configuration ------------------------------------------------------
    MdSheetKind kind() const { return m_kind; }
    void setSheetKind(MdSheetKind kind);

    /// The standard sheet's collapsed height (Compose `SheetPeekHeight`,
    /// 56 px). Modal sheets ignore it — their partial anchor is derived.
    qreal peekHeight() const { return m_peekHeight; }
    void setPeekHeight(qreal height);

    /// Skip the Hidden anchor (the standard sheet's default: true). A
    /// skipped state has no anchor, so drags and calls never settle there.
    bool skipHiddenState() const { return m_skipHidden; }
    void setSkipHiddenState(bool skip);

    /// Skip the PartiallyExpanded anchor (default: false).
    bool skipPartiallyExpanded() const { return m_skipPartial; }
    void setSkipPartiallyExpanded(bool skip);

    /// The drag handle pill (Compose's `dragHandle` slot defaults to one).
    bool hasHandle() const { return m_hasHandle; }
    void setHandleVisible(bool visible);

    /// The `confirmValueChange` veto: return false from the callback to
    /// refuse a pending state change (Compose's `SheetState` parameter).
    void setConfirmValueChange(std::function<bool(MdSheetState)> confirm);
    std::function<bool(MdSheetState)> confirmValueChange() const { return m_confirm; }

    /// Whether the sheet manages its own geometry inside the parent (the
    /// modal host takes over: anchoring, centring, the hidden slide). Off by
    /// default so a statically-placed sheet — a gallery snapshot, a fixed
    /// scaffold slot — never jumps to its anchors when the parent resizes.
    bool geometryManaged() const { return m_geometryManaged; }
    void setGeometryManaged(bool managed);

    // --- tokens -------------------------------------------------------------
    /// The resolved `md.comp.sheet.bottom.*` set, after the application-wide
    /// and per-instance `md.comp.*` overrides.
    const MdBottomSheetTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this sheet alone.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- state --------------------------------------------------------------
    MdSheetState state() const { return m_state; }
    /// The settled state's anchor target (the state an animation is running
    /// toward, or `state()` when settled).
    MdSheetState targetState() const { return m_target; }

    /// The container's top-y relative to the parent — Compose's offset.
    /// Falls back to the widget's y when there is no parent.
    qreal currentOffset() const;

    /// Snap the settled state without animating. No movement — callers
    /// position the widget (or let the anchor APIs do it).
    void setState(MdSheetState state);

    bool isAnimating() const { return m_phase != Phase::Idle; }

    // --- geometry -----------------------------------------------------------
    /// The painted container: the widget rect with the shadow margin on the
    /// top/left/right and a flush bottom edge.
    QRectF containerRect() const;

    /// The layout computed for the current size: the geometry the drag
    /// handle paints into and the content area below it.
    MdBottomSheetStyle::Layout currentLayout() const;

    /// The height the sheet needs for `width` — the container's flush-bottom
    /// geometry means the widget height is the content height plus the top
    /// shadow margin. The sheet is height-driven by its caller (Compose's
    /// sheet height is its content's height), so this is a pass-through
    /// rather than a computed layout.
    QSize sizeHint() const override;

public slots:
    /// Animate from wherever the sheet is to PartiallyExpanded (or Expanded
    /// when partial is skipped) — Compose's `state.show()`.
    void showSheet();

    /// Animate to Hidden — Compose's `state.hide()`. Runs the confirm veto;
    /// settling emits `dismissed()` and hides the widget.
    void hideSheet();

    /// Animate to Expanded — Compose's `state.expand()`.
    void expand();

    /// Animate to PartiallyExpanded — Compose's `state.partialExpand()`.
    void partialExpand();

signals:
    void stateChanged(md::MdSheetState state);
    /// The sheet settled at Hidden (drag, handle collapse, hideSheet) — the
    /// onDismissRequest flow.
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

    // The anchor context: the parent's height ("fullHeight" in Compose) and
    // the container's height.
    qreal fullParentHeight() const;
    qreal sheetHeight() const;

    void animateTo(MdSheetState state);
    bool confirmed(MdSheetState state);
    void applyOffset(qreal offset);
    void snapToAnchor();
    void settleAt(MdSheetState state);
    QList<QPair<MdSheetState, qreal>> availableAnchors() const;
    void handleToggle();
    void syncHandleGeometry();

    MdSheetKind m_kind = MdSheetKind::Modal;
    MdSheetState m_state = MdSheetState::Hidden;
    MdSheetState m_target = MdSheetState::Hidden;
    qreal m_offsetY = 0.0;
    qreal m_peekHeight = MdBottomSheetTokens::kSheetPeekHeight;
    bool m_skipHidden = true;
    bool m_skipPartial = false;
    bool m_hasHandle = true;

    std::function<bool(MdSheetState)> m_confirm;

    // Drag bookkeeping.
    bool m_dragging = false;
    bool m_dragMoved = false;
    bool m_pressedHandle = false;
    qreal m_dragStartY = 0.0;
    qreal m_dragStartOffset = 0.0;

    Phase m_phase = Phase::Idle;
    bool m_geometryManaged = false;
    class QTimer *m_animationTimer = nullptr;
    QElapsedTimer *m_animationClock = nullptr;
    qreal m_animationFrom = 0.0;
    qreal m_animationTo = 0.0;
    bool m_animationIsHide = false;

    mutable MdBottomSheetTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    friend class MdBottomSheetStyle;
    friend class MdBottomSheetHost;
};

} // namespace md

#endif // MD_BOTTOM_SHEET_H
