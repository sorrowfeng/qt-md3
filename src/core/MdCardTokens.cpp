#include "MdCardTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// The `md.comp.*` override parsing itself lives in MdCompTokenParse.h, shared
// with the other component resolvers so a length or a shape name can only be
// parsed one way across the whole library.
using namespace comptoken;

// ---------------------------------------------------------------------------
// md.comp.<variant>-card
// ---------------------------------------------------------------------------

struct VariantRow
{
    MdCardVariant variant;
    const char *token; ///< the `md.comp.<token>-card.*` segment

    // Enabled row.
    ColorRole container;
    ColorRole content;      ///< contentColorFor(container) — on-surface here
    ElevationLevel enabledElevation;

    // The state rows that climb.
    ElevationLevel hoveredElevation;
    ElevationLevel focusedElevation;
    ElevationLevel pressedElevation;
    ElevationLevel draggedElevation;

    // The disabled row: a container role and the *outline* colour, both
    // resolved against the shared disabled opacities. `disabledContainer ==
    // Count` means the container colour does not change when disabled (the
    // outlined card).
    ColorRole disabledContainer;
    ColorRole disabledOutline; ///< Count = the variant publishes no outline

    // The enabled outline row (outlined variant only).
    ColorRole enabledOutline;
    ColorRole hoveredOutline;
    ColorRole focusedOutline;
    ColorRole pressedOutline;
    ColorRole draggedOutline;
};

// Transcribed from _md-comp-filled-card.scss / _md-comp-elevated-card.scss /
// _md-comp-outlined-card.scss (34.0.21). The state-layer colour is
// on-surface for every variant and state (including dragged); the *opacity*
// stays md.sys.state's business, resolved by MdStateLayer per state kind.
const VariantRow kVariantRows[] = {
    // md.comp.filled-card.* — surface-container-highest at level0; hover
    // level1, dragged level3; disabled surface-variant.
    {MdCardVariant::Filled, "filled-card",
     ColorRole::SurfaceContainerHighest, ColorRole::OnSurface,
     ElevationLevel::Level0,
     ElevationLevel::Level1, ElevationLevel::Level0, ElevationLevel::Level0,
     ElevationLevel::Level3,
     ColorRole::SurfaceVariant, ColorRole::Count,
     ColorRole::Count, ColorRole::Count, ColorRole::Count, ColorRole::Count,
     ColorRole::Count},

    // md.comp.elevated-card.* — surface-container-low at level1; hover
    // level2, dragged level4; disabled surface.
    {MdCardVariant::Elevated, "elevated-card",
     ColorRole::SurfaceContainerLow, ColorRole::OnSurface,
     ElevationLevel::Level1,
     ElevationLevel::Level2, ElevationLevel::Level1, ElevationLevel::Level1,
     ElevationLevel::Level4,
     ColorRole::Surface, ColorRole::Count,
     ColorRole::Count, ColorRole::Count, ColorRole::Count, ColorRole::Count,
     ColorRole::Count},

    // md.comp.outlined-card.* — surface at level0 with a 1 px outline-variant
    // stroke; hover level1, dragged level3; keyboard focus turns the outline
    // on-surface; disabled keeps the container and fades the outline.
    {MdCardVariant::Outlined, "outlined-card",
     ColorRole::Surface, ColorRole::OnSurface,
     ElevationLevel::Level0,
     ElevationLevel::Level1, ElevationLevel::Level0, ElevationLevel::Level0,
     ElevationLevel::Level3,
     ColorRole::Count, ColorRole::OutlineVariant,
     ColorRole::OutlineVariant, ColorRole::OutlineVariant, ColorRole::OnSurface,
     ColorRole::OutlineVariant, ColorRole::OutlineVariant},
};

const VariantRow &variantRow(MdCardVariant variant)
{
    for (const VariantRow &row : kVariantRows) {
        if (row.variant == variant) {
            return row;
        }
    }
    return kVariantRows[0]; // Filled, the default card.
}

} // namespace

const MdCardStateRow &MdCardFamily::state(MdCardState which) const
{
    switch (which) {
    case MdCardState::Enabled: return enabled;
    case MdCardState::Hovered: return hovered;
    case MdCardState::Focused: return focused;
    case MdCardState::Pressed: return pressed;
    case MdCardState::Dragged: return dragged;
    case MdCardState::Disabled: return disabled;
    case MdCardState::Count: break;
    }
    return enabled;
}

MdCardTokens MdCardTokens::resolve(MdCardVariant variant,
                                   const MdComponentTokens *overrides)
{
    const VariantRow &row = variantRow(variant);
    const char *component = row.token;
    MdCardTokens tokens;

    // --- shape / metrics ---------------------------------------------------
    tokens.containerShape =
        shapeOverride(overrides, plainKeys(component, "container.shape"),
                      ShapeCorner::Medium);
    tokens.outlineWidth =
        lengthOverride(overrides, plainKeys(component, "outline.width"), 1.0);
    tokens.iconSize =
        lengthOverride(overrides, plainKeys(component, "icon.size"), 24.0);

    // --- focus indicator (md-sys-state-focus-indicator) --------------------
    tokens.focusIndicator = ColorRole::Secondary;
    tokens.focusIndicatorThickness =
        lengthOverride(overrides, plainKeys(component, "focus.indicator.thickness"), 3.0);
    tokens.focusIndicatorOffset =
        lengthOverride(overrides, plainKeys(component, "focus.indicator.outline.offset"),
                       2.0);

    // --- state rows ---------------------------------------------------------
    // The enabled row restated for hovered/focused/pressed: the colour
    // columns of those exports repeat the enabled roles; only elevation and
    // the outlined variant's outline colour move.
    MdCardStateRow enabledRow;
    enabledRow.container = row.container;
    enabledRow.content = row.content;
    enabledRow.elevation = row.enabledElevation;
    enabledRow.outline = row.enabledOutline;

    MdCardStateRow interactive = enabledRow;
    interactive.stateLayer = ColorRole::OnSurface;

    tokens.family.enabled = enabledRow;

    tokens.family.hovered = interactive;
    tokens.family.hovered.elevation = row.hoveredElevation;
    tokens.family.hovered.outline = row.hoveredOutline;

    tokens.family.focused = interactive;
    tokens.family.focused.elevation = row.focusedElevation;
    tokens.family.focused.outline = row.focusedOutline;

    tokens.family.pressed = interactive;
    tokens.family.pressed.elevation = row.pressedElevation;
    tokens.family.pressed.outline = row.pressedOutline;

    tokens.family.dragged = interactive;
    tokens.family.dragged.elevation = row.draggedElevation;
    tokens.family.dragged.outline = row.draggedOutline;

    // Disabled: the export publishes a disabled container (outlined: the
    // container is unchanged) and — outlined only — a disabled outline. The
    // opacities (0.38 container, 0.12 outline) and the disabled content
    // alpha (0.38) are applied by the paint code, which composites the
    // faded colour over the enabled one exactly as Compose does.
    MdCardStateRow disabledRow = enabledRow;
    if (row.disabledContainer != ColorRole::Count) {
        disabledRow.container = row.disabledContainer;
    }
    disabledRow.stateLayer = ColorRole::Count;
    disabledRow.elevation = row.enabledElevation;
    disabledRow.outline = row.disabledOutline;
    tokens.family.disabled = disabledRow;

    return tokens;
}

} // namespace md
