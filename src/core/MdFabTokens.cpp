#include "MdFabTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// The `md.comp.*` override parsing itself lives in MdCompTokenParse.h, shared
// with the other component resolvers so a length or a shape name can only be
// parsed one way across the whole library.
using namespace comptoken;

// ---------------------------------------------------------------------------
// md.comp.fab.<size>
// ---------------------------------------------------------------------------

struct SizeRow
{
    FabSize size;
    const char *token;   ///< the `md.comp.fab.<token>` size segment
    qreal height;
    qreal width;
    qreal iconSize;
    ShapeCorner shape;
};

const SizeRow kSizeRows[] = {
    // md.comp.fab.small.* — 40 × 40, corner-medium.
    {FabSize::Small, "small", 40.0, 40.0, 24.0, ShapeCorner::Medium},
    // md.comp.fab.medium.* — the base set: 56 × 56, corner-large.
    {FabSize::Medium, "medium", 56.0, 56.0, 24.0, ShapeCorner::Large},
    // md.comp.fab.large.* — 96 × 96, icon 36, corner-extra-large.
    {FabSize::Large, "large", 96.0, 96.0, 36.0, ShapeCorner::ExtraLarge},
};

const SizeRow &sizeRow(FabSize size)
{
    for (const SizeRow &row : kSizeRows) {
        if (row.size == size) {
            return row;
        }
    }
    return kSizeRows[1]; // Medium, the 56 px default the base token set matches.
}

// ---------------------------------------------------------------------------
// md.comp.fab.<variant> — colours
//
// The non-deprecated rows of each colour set restate the enabled roles for
// hovered/focused/pressed, so the interactive row is transcribed once per
// variant and the state-layer opacity stays md.sys.state's business.
// `md.comp.fab.surface.lowered.container.color` is the one variant-local
// container change: a lowered surface FAB sits on surface-container-low.
// ---------------------------------------------------------------------------

struct VariantRow
{
    FabVariant variant;

    ColorRole container;       ///< `container.color` (raised)
    ColorRole loweredContainer; ///< `lowered.container.color`; Count = same as raised
    ColorRole icon;            ///< `icon.color`
    ColorRole stateLayer;      ///< `hovered.state-layer.color` (== focused == pressed)
};

const VariantRow kVariantRows[] = {
    // md.comp.fab.surface.* — surface-container-high, primary icon.
    {FabVariant::Surface,
     ColorRole::SurfaceContainerHigh, ColorRole::SurfaceContainerLow,
     ColorRole::Primary, ColorRole::Primary},

    // md.comp.fab.primary.* — primary container, on-primary icon.
    {FabVariant::Primary,
     ColorRole::Primary, ColorRole::Count,
     ColorRole::OnPrimary, ColorRole::OnPrimary},

    // md.comp.fab.secondary.*
    {FabVariant::Secondary,
     ColorRole::Secondary, ColorRole::Count,
     ColorRole::OnSecondary, ColorRole::OnSecondary},

    // md.comp.fab.tertiary.*
    {FabVariant::Tertiary,
     ColorRole::Tertiary, ColorRole::Count,
     ColorRole::OnTertiary, ColorRole::OnTertiary},
};

const VariantRow &variantRow(FabVariant variant)
{
    for (const VariantRow &row : kVariantRows) {
        if (row.variant == variant) {
            return row;
        }
    }
    return kVariantRows[0]; // Surface, the base token set.
}

} // namespace

const MdFabStateColours &MdFabFamily::state(MdFabState which) const
{
    switch (which) {
    case MdFabState::Enabled: return enabled;
    case MdFabState::Hovered: return hovered;
    case MdFabState::Focused: return focused;
    case MdFabState::Pressed: return pressed;
    case MdFabState::Disabled: return disabled;
    case MdFabState::Count: break;
    }
    return enabled;
}

ElevationLevel MdFabFamily::elevation(MdFabState which) const
{
    switch (which) {
    case MdFabState::Enabled: return enabledElevation;
    case MdFabState::Hovered: return hoveredElevation;
    case MdFabState::Focused: return focusedElevation;
    case MdFabState::Pressed: return pressedElevation;
    case MdFabState::Disabled: return disabledElevation;
    case MdFabState::Count: break;
    }
    return enabledElevation;
}

MdFabTokens MdFabTokens::resolve(FabVariant variant,
                                 FabSize size,
                                 bool lowered,
                                 const MdComponentTokens *overrides)
{
    const SizeRow &row = sizeRow(size);
    const VariantRow &colours = variantRow(variant);
    MdFabTokens tokens;

    // --- metrics: md.comp.fab.<size>.<token> ------------------------------
    // The size segments are the more qualified key; the base
    // md.comp.fab.container.* rows are the fallback.
    tokens.containerHeight =
        lengthOverride(overrides, sizeKeys("fab", row.token, "container.height"), row.height);
    tokens.containerWidth =
        lengthOverride(overrides, sizeKeys("fab", row.token, "container.width"), row.width);
    tokens.iconSize =
        lengthOverride(overrides, sizeKeys("fab", row.token, "icon.size"), row.iconSize);
    tokens.containerShape = shapeOverride(
        overrides, sizeKeys("fab", row.token, "container.shape"), row.shape);

    // --- focus indicator (md-sys-state-focus-indicator) --------------------
    tokens.focusIndicator = ColorRole::Secondary;
    tokens.focusIndicatorThickness =
        lengthOverride(overrides, plainKeys("fab", "focus.indicator.thickness"), 3.0);
    tokens.focusIndicatorOffset =
        lengthOverride(overrides, plainKeys("fab", "focus.indicator.outline.offset"), 2.0);

    // --- colours ----------------------------------------------------------
    const ColorRole containerRole =
        lowered && colours.loweredContainer != ColorRole::Count ? colours.loweredContainer
                                                                : colours.container;
    const MdFabStateColours interactive{
        MdFabColourSlot{containerRole, 1.0},
        MdFabColourSlot{colours.icon, 1.0},
        colours.stateLayer,
    };
    tokens.family.enabled = interactive;
    tokens.family.hovered = interactive;
    tokens.family.focused = interactive;
    tokens.family.pressed = interactive;

    // The export publishes no disabled rows; the spec's disabled state table
    // (the same disabled row every push-button family shows) fills them.
    // See the file comment and docs/porting-todo.md.
    tokens.family.disabled = MdFabStateColours{
        MdFabColourSlot{ColorRole::OnSurface, 0.12},
        MdFabColourSlot{ColorRole::OnSurface, 0.38},
        ColorRole::Count,
    };

    // --- elevation --------------------------------------------------------
    // Raised: enabled/focused/pressed level3, hovered level4.
    // Lowered: enabled/focused/pressed level1, hovered level2.
    // Both from the non-deprecated rows; the deprecated lowered-pressed row
    // agrees with the lowered-enabled one anyway (level1).
    tokens.family.enabledElevation = lowered ? ElevationLevel::Level1 : ElevationLevel::Level3;
    tokens.family.hoveredElevation = lowered ? ElevationLevel::Level2 : ElevationLevel::Level4;
    tokens.family.focusedElevation = tokens.family.enabledElevation;
    tokens.family.pressedElevation = tokens.family.enabledElevation;
    tokens.family.disabledElevation = ElevationLevel::Level0;

    return tokens;
}

} // namespace md
