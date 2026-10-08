#ifndef MD_SEGMENTED_BUTTON_TOKENS_H
#define MD_SEGMENTED_BUTTON_TOKENS_H

// The published `md.comp.outlined-segmented-button.*` token set.
//
// The export publishes exactly one set for this family — there is no size
// scale and no colour variants (the family is outlined-only). Sources:
//
//   tokens/versions/latest/sass/_md-comp-outlined-segmented-button.scss
//     every number below marked [export]
//
//   androidx Compose material3 SegmentedButton.kt (androidx-main), which the
//     Expressive behaviour comes from, marked [compose]:
//     - the container row overlaps segments by exactly `outline-width`
//       (`Arrangement.spacedBy(-BorderWidth)` — "handles overlapping items so
//       that strokes of the item are correctly on top of each other"), which
//       is how the divider between segments is produced: it *is* the shared
//       1 px stroke, not a separate element
//     - `itemShape(index, count)`: the first segment rounds its inline-start
//       corners, the last rounds its inline-end corners, and every middle
//       segment is a rectangle (base shape corner-full)
//     - the icon slot is *always* reserved (IconSize + 8 px IconSpacing) so
//       the label never moves when the check scales in on selection
//     - the horizontal content padding is 12 px
//
// Two recorded quirks (docs/porting-todo.md, Segmented buttons entry):
//
//   * the export's pressed state-layer *opacity* is the focus one
//     (`pressed-state-layer-opacity: focus-state-layer-opacity`) — honoured
//     by the pressed overlay kind, noted because it is not a typo;
//   * the export publishes no motion rows; the check's scale-in runs on
//     `md.sys.motion` spring-fast-spatial, the same spring Compose uses for
//     it (MotionSchemeKeyTokens.FastSpatial).

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QSizeF>
#include <QtCore/QtGlobal>

namespace md {

/// The colour rows for one selection family. The export's rows are constant
/// across the interactive states (only the state-layer *opacity* changes,
/// which is md.sys.state's business), so one row per family + the disabled
/// opacities is the whole published table.
struct QT_MD3_EXPORT MdSegmentedButtonRow
{
    /// `label-text.color` / `icon.color` / `state-layer.color`.
    ColorRole labelText = ColorRole::OnSurface;
    ColorRole icon = ColorRole::OnSurface;
    ColorRole stateLayer = ColorRole::OnSurface;
    /// `container.color`. Absent for the unselected family — outlined
    /// segments are transparent until selected.
    ColorRole container = ColorRole::Count;
    /// `outline.color`.
    ColorRole outline = ColorRole::Outline;
};

/// Everything needed to paint and lay out one segmented button.
struct QT_MD3_EXPORT MdSegmentedButtonTokens
{
    // --- metrics ([export] / [compose]) -----------------------------------
    /// `container-height` [export].
    qreal containerHeight = 40.0;
    /// `outline-width` [export]; also the segment overlap in the row.
    qreal outlineWidth = 1.0;
    /// `with-icon.icon-size` [export] — the check and the custom icons.
    qreal iconSize = 18.0;
    /// The gap between the icon slot and the label [compose: IconSpacing].
    qreal iconSpacing = 8.0;
    /// The reserved icon slot is always this wide [export], selected or not.
    qreal iconSlotWidth() const { return iconSize; }
    /// Horizontal content padding [compose: ContentPadding start/end].
    qreal contentPadding = 12.0;

    // --- type --------------------------------------------------------------
    /// `label-text.*` [export]: label-large.
    TypeStyle labelStyle = TypeStyle::LabelLarge;

    // --- shape ([compose] itemShape + [export] container-shape) -----------
    /// The base corner: `container-shape: corner-full` [export], resolved
    /// against a box as half the height. First / last / middle positions
    /// round subsets of it; see MdSegmentedButtonStyle.
    qreal baseCornerRadius() const { return containerHeight / 2.0; }

    // --- disabled ([export] opacities + colours) ---------------------------
    qreal disabledLabelOpacity = 0.38;
    qreal disabledIconOpacity = 0.38;
    qreal disabledOutlineOpacity = 0.12;
    ColorRole disabledContent = ColorRole::OnSurface;
    ColorRole disabledOutline = ColorRole::OnSurface;

    // --- focus indicator ([export] + sys) ---------------------------------
    /// `focus-indicator-color` [export]; thickness/offset are
    /// md-sys-state-focus-indicator's.
    ColorRole focusIndicator = ColorRole::Secondary;
    qreal focusIndicatorThickness = 3.0;
    qreal focusIndicatorOffset = 2.0;

    // --- colours ------------------------------------------------------------
    /// The unselected family: on-surface content, outline stroke, no
    /// container. Constant across enabled / hovered / focused / pressed
    /// [export].
    MdSegmentedButtonRow unselected;
    /// The selected family: secondary-container container,
    /// on-secondary-container content [export].
    MdSegmentedButtonRow selected;

    /// Build the published set. `overrides` is consulted through the
    /// `md.comp.outlined-segmented-button.*` key namespace for the numeric
    /// tokens; colour rows ride on the scheme.
    static MdSegmentedButtonTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_SEGMENTED_BUTTON_TOKENS_H
