#ifndef MD_SIDE_SHEET_TOKENS_H
#define MD_SIDE_SHEET_TOKENS_H

// MdSideSheetTokens — the `md.comp.sheet.side.docked.*` export
// (34.0.21, file `_md-comp-sheet-side.scss`).
//
// The side sheet's published set covers the DOCKED presentation only, in two
// configuration rows that mirror the bottom sheet's split:
//
//   * standard: `surface` at elevation level 0 with `corner-none` — a
//     persistent sheet that sits flush against the parent's edge, co-existing
//     with the screen's UI (an optional `outline` divider separates it from
//     the content);
//   * modal: `surface-container-low` at elevation level 1 with
//     `corner-large-start` — the large (24 px) radius on the START corner
//     pair only, the pair facing the app content; the edge pair that meets
//     the parent's edge stays square.
//
// Behaviour source: NEITHER material-web (no sheet web component — token
// export only) NOR Compose M3 (no side sheet ships in material3) implements
// it. The behaviour source is the Android Views library (MDC-Android,
// `com.google.android.material.sidesheet`), which the official overview page
// lists as the one available implementation, plus the spec measurements from
// m3.material.io/components/side-sheets/specs. Two-state machine
// (STATE_HIDDEN / STATE_EXPANDED plus the transient dragging/settling),
// the Left/Right delegate geometry, and the dialog wrapper (scrim, cancel on
// hide) all transcribe from there.
//
// Recorded, not applied:
//
//   * `detached.container.shape` = corner-large — the detached sheet (16 px
//     margins all around, a floating card) is a separate presentation the
//     port does not ship;
//   * the `surface-tint-layer.color` row is deprecated (tonal surfaces
//     replaced tint layers) and NOT carried;
//   * the focus-indicator rows (secondary, sys thickness / outer offset) —
//     the sheet surface is not a focusable control;
//   * the action rows (label-text primary, state-layer primary at the hover
//     0.08 / focus 0.08 / pressed 0.12 opacities) belong to action buttons
//     INSIDE the sheet's content — the sheet surface paints none of them,
//     and the content is the caller's widgets (a text button placed inside
//     picks the rows up from the button family).
//
// The spec's headline is title-large in on-surface-variant; the divider is
// `outline`. Both paint on the sheet surface.

#include "MdTypes.h"
#include "MdTokens.h"
#include "QtMd3Export.h"

namespace md {

/// The two stable states of a side sheet — MDC-Android's `Sheet.StableSheetState`.
enum class MdSideSheetState
{
    Hidden,
    Expanded,
    Count,
};

/// Standard (persistent, docked) or modal. The kinds differ in paint (colors,
/// elevation, the start-corner radius) and defaults, not in their anchors —
/// both anchor Hidden and Expanded (MDC's behavior has the two states for
/// every configuration).
enum class MdSideSheetKind
{
    Standard,
    Modal,
};

/// Which edge the sheet docks against — MDC's `Sheet.SheetEdge` / its
/// Left/Right sheet delegates.
enum class MdSideSheetEdge
{
    Left,
    Right,
};

struct QT_MD3_EXPORT MdSideSheetTokens
{
    // --- container: md.comp.sheet.side.docked.(standard|modal).container.* --
    /// The standard row (surface). The modal row (surface-container-low)
    /// lives beside it; the style picks by kind.
    ColorRole standardContainerColor = ColorRole::Surface;
    ColorRole modalContainerColor = ColorRole::SurfaceContainerLow;
    /// The standard row (corner-none — flush against the edge). The modal row
    /// is `corner-large-start`, resolved by the style per edge.
    ShapeCorner standardContainerShape = ShapeCorner::None;
    ShapeCorner modalContainerShape = ShapeCorner::Large;
    ElevationLevel standardContainerElevation = ElevationLevel::Level0;
    ElevationLevel modalContainerElevation = ElevationLevel::Level1;
    ColorRole containerShadowColor = ColorRole::Shadow;
    /// `container.width` — 256 px (the docked sheet's width; the spec caps
    /// the content at 400 px, which a 256 px sheet never reaches).
    qreal containerWidth = 256.0;
    /// `container.height` = 100% — the sheet is as tall as its parent.
    /// The widget sizes itself to the parent's height.
    /// `detached.container.shape` = corner-large and the detached
    /// presentation (16 px margins) are recorded, not ported.
    ShapeCorner detachedShape = ShapeCorner::Large;

    // --- headline: md.comp.sheet.side.docked.headline.* ---------------------
    /// title-large, in on-surface-variant.
    ColorRole headlineColor = ColorRole::OnSurfaceVariant;
    /// The headline's type row (title-large: 28 px / 1.143 line height /
    /// 400 weight / 0 tracking). The sheet surface paints no text of its
    /// own — the headline is the caller's widget — but the row is resolved
    /// and exposed for the content layout.
    qreal headlineFontSize = 28.0;
    qreal headlineLineHeight = 32.0;
    int headlineWeight = 400;

    // --- divider: md.comp.sheet.side.docked.divider.* -----------------------
    ColorRole dividerColor = ColorRole::Outline;

    // --- action: md.comp.sheet.side.docked.action.* (recorded) --------------
    ColorRole actionLabelColor = ColorRole::Primary;

    // --- focus indicator: md.comp.sheet.side.docked.focus.indicator.* -------
    ColorRole focusIndicatorColor = ColorRole::Secondary;

    // --- resolution ---------------------------------------------------------
    /// The resolved `md.comp.sheet.side.*` set, after the application-wide
    /// and per-instance `md.comp.*` overrides (lengths and the shapes).
    static MdSideSheetTokens resolve(const MdComponentTokens *overrides = nullptr);

    // --- MDC-Android port constants (not export rows) -----------------------
    /// SideSheetBehavior.SIGNIFICANT_VEL_THRESHOLD (px/s). Recorded — the Qt
    /// port settles drags positionally (see docs/porting-todo.md).
    static constexpr qreal kSignificantVelocityThreshold = 500.0;
    /// SideSheetBehavior.HIDE_THRESHOLD — the drag's projected position must
    /// pass half the travel for a hide. The port settles on the midpoint
    /// rule positionally (the velocity-weighted projection is not ported).
    static constexpr qreal kHideThreshold = 0.5;
    /// SideSheetBehavior.HIDE_FRICTION. Recorded; no velocity fling.
    static constexpr qreal kHideFriction = 0.1;
    /// The detached presentation's margin (`m3_side_sheet_margin_detached`,
    /// 16 dp, also the spec's "margins (when detached)"). Recorded with the
    /// detached presentation it belongs to.
    static constexpr qreal kDetachedMargin = 16.0;
    /// md.sys.color.scrim — the modal dimming layer's opacity (black at
    /// 32%, via the scrim token set). Not a sheet row.
    static constexpr qreal kScrimOpacity = 0.32;

    /// The widget's shadow headroom on the shadowed edges — the elevation
    /// rings need a little headroom a Qt child widget cannot paint outside
    /// its rect. The DOCKED edge carries NO margin: a docked sheet is
    /// edge-to-edge with the parent's edge, and the hidden slide clips
    /// outside the parent anyway. Not a token row.
    static constexpr qreal kShadowMargin = 4.0;

    /// The spec's content padding ("start/end padding 24 dp"). Applied
    /// uniformly around the content rect.
    static constexpr qreal kContentPadding = 24.0;
};

} // namespace md

#endif // MD_SIDE_SHEET_TOKENS_H
