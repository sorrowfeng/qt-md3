#ifndef MD_BOTTOM_SHEET_TOKENS_H
#define MD_BOTTOM_SHEET_TOKENS_H

// MdBottomSheetTokens — the `md.comp.sheet.bottom.docked.*` export
// (34.0.21, file `_md-comp-sheet-bottom.scss`).
//
// One published set covering both the standard (persistent) and the modal
// bottom sheet — the export's container rows are shared and its elevation
// carries two rows (modal / standard) that resolve to the same level-1:
//
//   * container: surface-container-low at elevation level 1 with the
//     `corner-extra-large-top` shape — the extra-large (28 px) radius on the
//     TOP pair only, the bottom edge stays square (a sheet is edge-to-edge
//     with the parent's bottom);
//   * `minimized.container.shape` = corner-none — the published Hidden-state
//     shape. Compose publishes it as `BottomSheetDefaults.HiddenShape` but
//     `BottomSheetImpl` keeps the expanded shape constantly, so the paint
//     follows the implementation and the row is recorded only;
//   * drag handle: 32 x 4 px in on-surface-variant. The export's
//     `drag-handle.opacity = 0.4` row is deprecated (b/278783477) and
//     Compose's `BottomSheetDefaults.DragHandle` applies NO extra opacity —
//     recorded, not applied;
//   * `surface-tint-layer.color` (surface-tint) is deprecated and NOT
//     carried — tonal surfaces replaced tint layers;
//   * focus indicator: secondary, at the sys focus-indicator thickness and
//     outer offset. Compose publishes no focus ring for the sheet surface or
//     the drag handle (a clickable Box, not a focusable control), so the
//     rows are recorded, not painted.
//
// The Expressive `_md-comp-drag-handle.scss` export (a standalone 48 px
// handle that grows to 52 x 12 when pressed) is a SEPARATE component token
// set, not a bottom-sheet row — recorded in docs/porting-todo.md, not
// ported.
//
// Layout/motion constants the export does NOT publish (Compose
// SheetDefaults.kt private vals, transcribed and labelled): the 56 px peek
// height, the 640 px sheet max width, the 56 px positional and 125 px
// velocity drag thresholds, the 125 px boundary dampening zone, the drag
// handle's 22 px vertical padding, and the 0.32 scrim opacity
// (md.sys.color.scrim via the scrim token set, not a sheet row).

#include "MdTypes.h"
#include "MdTokens.h"
#include "QtMd3Export.h"

namespace md {

/// The three anchor states of a bottom sheet — Compose's `SheetValue`.
enum class MdSheetState
{
    Hidden,
    PartiallyExpanded,
    Expanded,
    Count,
};

/// Standard (persistent, BottomSheetScaffold) or modal (ModalBottomSheet).
/// The kinds differ in their anchor availability, not in their paint:
/// the standard sheet anchors PartiallyExpanded at the peek height and
/// skips Hidden by default; the modal sheet anchors Hidden always and
/// derives PartiallyExpanded from half the heights.
enum class MdSheetKind
{
    Standard,
    Modal,
};

struct QT_MD3_EXPORT MdBottomSheetTokens
{
    // --- container: md.comp.sheet.bottom.docked.container.* ----------------
    ColorRole containerColor = ColorRole::SurfaceContainerLow;
    /// `corner-extra-large-top`: the ExtraLarge radius on the top pair only.
    /// The style builds {r, r, 0, 0} from this row; the bottom edge is
    /// square because a sheet meets the parent's bottom edge.
    ShapeCorner containerShape = ShapeCorner::ExtraLarge;
    /// The published Hidden-state shape (corner-none). Recorded only —
    /// Compose's implementation keeps the expanded shape (see header).
    ShapeCorner minimizedShape = ShapeCorner::None;
    /// `modal.container.elevation` and `standard.container.elevation` both
    /// resolve to level 1.
    ElevationLevel containerElevation = ElevationLevel::Level1;
    ColorRole containerShadowColor = ColorRole::Shadow;

    // --- drag handle: md.comp.sheet.bottom.docked.drag-handle.* -------------
    qreal dragHandleWidth = 32.0;
    qreal dragHandleHeight = 4.0;
    ColorRole dragHandleColor = ColorRole::OnSurfaceVariant;
    /// Deprecated (b/278783477) and NOT applied — Compose applies no extra
    /// opacity to its drag handle. Kept as the export's record.
    qreal dragHandleOpacity = 0.4;

    // --- focus indicator: md.comp.sheet.bottom.focus.indicator.* ------------
    ColorRole focusIndicatorColor = ColorRole::Secondary;

    // --- resolution ---------------------------------------------------------
    /// The resolved `md.comp.sheet.bottom.*` set, after the application-wide
    /// and per-instance `md.comp.*` overrides (lengths and the shapes).
    static MdBottomSheetTokens resolve(const MdComponentTokens *overrides = nullptr);

    // --- Compose-port layout constants (not export rows) --------------------
    /// BottomSheetDefaults.SheetPeekHeight — the standard sheet's collapsed
    /// height (and its minimum height).
    static constexpr qreal kSheetPeekHeight = 56.0;
    /// BottomSheetDefaults.SheetMaxWidth.
    static constexpr qreal kSheetMaxWidth = 640.0;
    /// BottomSheetDefaults.PositionalThreshold — a drag settles to the next
    /// anchor once it has moved this far toward it.
    static constexpr qreal kPositionalThreshold = 56.0;
    /// BottomSheetDefaults.VelocityThreshold. Recorded — the Qt port settles
    /// drags positionally (see docs/porting-todo.md).
    static constexpr qreal kVelocityThreshold = 125.0;
    /// BottomSheetDefaults.BoundaryDampeningZone — the zone above the hidden
    /// anchor where fling velocity is exponentially dampened. Recorded; no
    /// velocity fling is ported.
    static constexpr qreal kBoundaryDampeningZone = 125.0;
    /// BottomSheetDefaults.DragHandle vertical padding (the handle's touch
    /// target extends 22 px above and below the 4 px bar).
    static constexpr qreal kDragHandleVerticalPadding = 22.0;
    /// md.sys.color.scrim — the modal dimming layer's opacity (black at
    /// 32%, via the scrim token set). Not a sheet row.
    static constexpr qreal kScrimOpacity = 0.32;

    /// The widget's shadow headroom on the top and side edges — the level-1
    /// rings need a little headroom a Qt child widget cannot paint outside
    /// its rect. The bottom edge carries NO margin: a sheet is edge-to-edge
    /// with the parent's bottom, and the hidden slide clips below the
    /// parent anyway. Not a token row.
    static constexpr qreal kShadowMargin = 4.0;
};

} // namespace md

#endif // MD_BOTTOM_SHEET_TOKENS_H
