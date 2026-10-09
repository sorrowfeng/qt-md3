#ifndef MD_CHIP_TOKENS_H
#define MD_CHIP_TOKENS_H

// MdChipTokens — the four published chip families at export version 34.0.21:
// `md.comp.assist-chip.*`, `md.comp.filter-chip.*`, `md.comp.input-chip.*` and
// `md.comp.suggestion-chip.*`.
//
// ## Two axes the rows cross-cut
//
// **Variant** (which family) and **kind** (flat vs elevated — assist, filter
// and suggestion publish both; input is flat only, which Compose agrees with:
// there is no `ElevatedInputChip`). On top of those, filter and input chips
// are *selectable*: their colour tables split into a selected and an
// unselected side. Assist and suggestion are click-only; their two sides are
// filled identically.
//
// ## The pressed state-layer swap
//
// Filter chips swap the pressed state layer across selection: an unselected
// press ripples `on-secondary-container` (the colour the chip is about to
// earn) while a selected press ripples `on-surface-variant`. Input chips do
// not swap — both sides press in the colour they have. Assist and suggestion
// press `on-surface` / `on-surface-variant`.
//
// ## Divergences pinned here rather than smoothed
//
// * **No colour animation.** Compose's `SelectableChipColors` resolve by state
//   and pass straight to the `Surface` — a selection's container/label colour
//   change is instant. (The *icons* appearing and disappearing do animate —
//   that is `AnimatingChipContent`, a presence animation, not a colour one.)
// * **The Expressive corner morph is opt-in upstream and not ported.** Compose's
//   second `SelectableChip` overload morphs between `ChipsTokens` shapes
//   (unselected `corner-medium`, selected `corner-full`, pressed
//   `corner-small`) on the fast spatial spring — but only through the explicit
//   `shapes` parameter; the default overload and the export both keep
//   `container-shape: corner-small` in every state. The classic family wins;
//   the trio is recorded in porting-todo.md.
// * **The avatar is a photo slot rendered as an icon.** The export sizes the
//   avatar (24 px, corner-full) and fades it when disabled (0.38) but
//   publishes no colour — upstream it carries a caller-supplied image. Qt has
//   no image slot in this library, so the avatar paints an icon glyph inside
//   the 24 px circle; the divergence is recorded.
// * **Drag is a programmatic state.** The dragged rows (level 4 elevation, the
//   dragged colour tables) are carried and resolved, but nothing here runs a
//   drag controller; `MdChip::setDragged()` reports the state the way Compose's
//   `InteractionSource` would emit `Drag`.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which published family the chip is drawn from.
enum class MdChipVariant {
    Assist,
    Filter,
    Input,
    Suggestion,
    Count,
};

constexpr int chipVariantCount = int(MdChipVariant::Count);

/// Flat (outlined) or elevated. Input publishes no elevated rows.
enum class MdChipKind {
    Flat,
    Elevated,
    Count,
};

constexpr int chipKindCount = int(MdChipKind::Count);

/// The seven states a chip paints in. `Dragged` sits with the interaction
/// states because the export publishes a full dragged colour table
/// (`dragged.state-layer.*`, `dragged.label-text.*`, the level-4 elevation).
enum class MdChipInteraction {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Dragged,
    Disabled,
    Count,
};

constexpr int chipInteractionCount = int(MdChipInteraction::Count);

/// One variant's rows for one kind. The colour tables are indexed
/// `[selection][interaction]`; the selection half is filled identically for
/// the two click-only families.
struct QT_MD3_EXPORT MdChipSurfaceTokens
{
    /// The container fill. Absent where the export publishes no row (a flat
    /// unselected chip is transparent over its parent surface).
    MdNavigationColourSlot container[2][chipInteractionCount];
    MdNavigationColourSlot label[2][chipInteractionCount];
    /// The 18 px leading slot (an icon, or the avatar's fallback glyph).
    MdNavigationColourSlot leadingIcon[2][chipInteractionCount];
    /// The trailing slot (filter's optional checkmark, input's action icon).
    MdNavigationColourSlot trailingIcon[2][chipInteractionCount];
    /// The state layer behind the content, clipped to the chip's shape.
    MdNavigationColourSlot stateLayer[2][chipInteractionCount];
    /// The flat outline. Present rows are one colour; the *width* lives in
    /// `outlineWidth` (a flat chip can have no outline at all when selected).
    MdNavigationColourSlot outline[2][chipInteractionCount];
    qreal outlineWidth[2] = {1.0, 1.0};
    /// The elevated container's shadow, per interaction. Flat is level 0
    /// everywhere; the elevated table is where the row lives.
    ElevationLevel containerElevation[chipInteractionCount] = {
        ElevationLevel::Level0, ElevationLevel::Level0, ElevationLevel::Level0,
        ElevationLevel::Level0, ElevationLevel::Level0, ElevationLevel::Level0,
    };
    ColorRole containerShadowColor = ColorRole::Shadow;
};

/// One family. `surface[kind]` holds the flat and elevated tables.
struct QT_MD3_EXPORT MdChipVariantTokens
{
    MdChipSurfaceTokens surface[chipKindCount];

    // --- metrics ---------------------------------------------------------------
    qreal containerHeight = 32.0;
    /// `container.shape: corner-small` — resolved through `MdShape::radius`.
    ShapeCorner containerShape = ShapeCorner::Small;
    qreal iconSize = 18.0;
    qreal avatarSize = 24.0;
    /// The avatar's own published shape: corner-full regardless of the
    /// container's.
    ShapeCorner avatarShape = ShapeCorner::Full;
    qreal disabledLabelTextOpacity = 0.38;
    qreal disabledIconOpacity = 0.38;
    /// The disabled container / outline opacity (the elevated container and
    /// the flat outline fade to 0.12, not the content's 0.38).
    qreal disabledContainerOpacity = 0.12;

    // --- content type ------------------------------------------------------------
    TypeStyle labelTextType = TypeStyle::LabelLarge;

    // --- state-layer opacities -----------------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;
    /// `md.sys.state.dragged-state-layer-opacity` — 0.16, the same system row
    /// every dragged family reads.
    qreal draggedStateLayerOpacity = 0.16;

    // --- focus indicator ----------------------------------------------------------
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOuterOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    // --- Compose behaviour constants (not export rows) ------------------------------
    /// The horizontal padding around the whole content row
    /// (`AssistChipDefaults.ContentPadding`).
    qreal contentPadding = 8.0;
    /// `HorizontalElementsPadding` — the shared default arrangement spacing.
    qreal elementSpacing = 8.0;
    /// `CompactHorizontalSpacing` — filter and input chips tighten the gap
    /// beside a leading icon to 4.
    qreal compactSpacing = 4.0;
    /// Compose's `maxChipWidth` cap.
    qreal maxChipWidth = 1000.0;

    // --- accessors -------------------------------------------------------------------
    bool isSelectable() const;
    const MdNavigationColourSlot &containerFor(MdChipKind kind, int selection,
                                               MdChipInteraction state) const;

    static MdChipVariantTokens resolve(MdChipVariant variant,
                                       const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_CHIP_TOKENS_H
