#include "MdExtendedFabTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// The `md.comp.*` override parsing itself lives in MdCompTokenParse.h, shared
// with the other component resolvers so a length or a shape name can only be
// parsed one way across the whole library.
using namespace comptoken;

// ---------------------------------------------------------------------------
// md.comp.extended-fab.<size>
// ---------------------------------------------------------------------------

struct SizeRow
{
    ExtendedFabSize size;
    const char *token;   ///< the `md.comp.extended-fab.<token>` size segment
    qreal height;
    qreal iconSize;
    qreal iconLabelSpace;
    qreal leadingSpace;
    qreal trailingSpace;
    ShapeCorner shape;
    TypeStyle labelStyle;
};

const SizeRow kSizeRows[] = {
    // md.comp.extended-fab.small.* — 56 px tall, icon 24, title-medium,
    // corner-large.
    {ExtendedFabSize::Small, "small", 56.0, 24.0, 8.0, 16.0, 16.0, ShapeCorner::Large,
     TypeStyle::TitleMedium},
    // md.comp.extended-fab.medium.* — 80 px tall, icon 28, title-large,
    // corner-large-increased.
    {ExtendedFabSize::Medium, "medium", 80.0, 28.0, 12.0, 26.0, 26.0,
     ShapeCorner::LargeIncreased, TypeStyle::TitleLarge},
    // md.comp.extended-fab.large.* — 96 px tall, icon 36, headline-small,
    // corner-extra-large.
    {ExtendedFabSize::Large, "large", 96.0, 36.0, 16.0, 28.0, 28.0, ShapeCorner::ExtraLarge,
     TypeStyle::HeadlineSmall},
};

const SizeRow &sizeRow(ExtendedFabSize size)
{
    for (const SizeRow &row : kSizeRows) {
        if (row.size == size) {
            return row;
        }
    }
    return kSizeRows[0]; // Small — 56 px, the height the base token set also names.
}

// ---------------------------------------------------------------------------
// md.comp.extended-fab.<variant> — colours
//
// The non-deprecated rows of each colour set restate the enabled roles for
// hovered/focused/pressed, so the interactive row is transcribed once per
// set and the state-layer opacity stays md.sys.state's business. Unlike the
// FAB family there is no surface set and no lowered container colour — the
// lowered form changes elevation only.
// ---------------------------------------------------------------------------

struct VariantRow
{
    ExtendedFabVariant variant;

    ColorRole container;  ///< `container.color`
    ColorRole content;    ///< `icon.color` == `label-text.color` in every set
    ColorRole stateLayer; ///< `hovered.state-layer.color` (== focused == pressed)
};

const VariantRow kVariantRows[] = {
    // md.comp.extended-fab.primary.* — primary container, on-primary content.
    {ExtendedFabVariant::Primary, ColorRole::Primary, ColorRole::OnPrimary,
     ColorRole::OnPrimary},

    // md.comp.extended-fab.secondary.*
    {ExtendedFabVariant::Secondary, ColorRole::Secondary, ColorRole::OnSecondary,
     ColorRole::OnSecondary},

    // md.comp.extended-fab.tertiary.*
    {ExtendedFabVariant::Tertiary, ColorRole::Tertiary, ColorRole::OnTertiary,
     ColorRole::OnTertiary},

    // md.comp.extended-fab.primary-container.* — on-primary-container content.
    {ExtendedFabVariant::PrimaryContainer, ColorRole::PrimaryContainer,
     ColorRole::OnPrimaryContainer, ColorRole::OnPrimaryContainer},

    // md.comp.extended-fab.secondary-container.*
    {ExtendedFabVariant::SecondaryContainer, ColorRole::SecondaryContainer,
     ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer},

    // md.comp.extended-fab.tertiary-container.*
    {ExtendedFabVariant::TertiaryContainer, ColorRole::TertiaryContainer,
     ColorRole::OnTertiaryContainer, ColorRole::OnTertiaryContainer},
};

const VariantRow &variantRow(ExtendedFabVariant variant)
{
    for (const VariantRow &row : kVariantRows) {
        if (row.variant == variant) {
            return row;
        }
    }
    return kVariantRows[0]; // Primary, the first published set.
}

} // namespace

MdExtendedFabTokens MdExtendedFabTokens::resolve(ExtendedFabVariant variant,
                                                 ExtendedFabSize size,
                                                 bool lowered,
                                                 const MdComponentTokens *overrides)
{
    const SizeRow &row = sizeRow(size);
    const VariantRow &colours = variantRow(variant);
    MdExtendedFabTokens tokens;

    // --- metrics: md.comp.extended-fab.<size>.<token> ----------------------
    tokens.containerHeight =
        lengthOverride(overrides, sizeKeys("extended-fab", row.token, "container.height"),
                       row.height);
    tokens.iconSize =
        lengthOverride(overrides, sizeKeys("extended-fab", row.token, "icon.size"), row.iconSize);
    tokens.iconLabelSpace = lengthOverride(
        overrides, sizeKeys("extended-fab", row.token, "icon.label.space"), row.iconLabelSpace);
    tokens.leadingSpace = lengthOverride(
        overrides, sizeKeys("extended-fab", row.token, "leading.space"), row.leadingSpace);
    tokens.trailingSpace = lengthOverride(
        overrides, sizeKeys("extended-fab", row.token, "trailing.space"), row.trailingSpace);
    tokens.containerShape = shapeOverride(
        overrides, sizeKeys("extended-fab", row.token, "container.shape"), row.shape);

    // --- label text style --------------------------------------------------
    tokens.labelStyle = row.labelStyle;

    // --- focus indicator (md-sys-state-focus-indicator) --------------------
    tokens.focusIndicator = ColorRole::Secondary;
    tokens.focusIndicatorThickness =
        lengthOverride(overrides, plainKeys("extended-fab", "focus.indicator.thickness"), 3.0);
    tokens.focusIndicatorOffset = lengthOverride(
        overrides, plainKeys("extended-fab", "focus.indicator.outline.offset"), 2.0);

    // --- colours ----------------------------------------------------------
    const MdFabStateColours interactive{
        MdFabColourSlot{colours.container, 1.0},
        MdFabColourSlot{colours.content, 1.0},
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
    // Identical rows to the FAB family. Raised: enabled/focused/pressed
    // level3, hovered level4. Lowered: enabled/focused/pressed level1,
    // hovered level2.
    tokens.family.enabledElevation = lowered ? ElevationLevel::Level1 : ElevationLevel::Level3;
    tokens.family.hoveredElevation = lowered ? ElevationLevel::Level2 : ElevationLevel::Level4;
    tokens.family.focusedElevation = tokens.family.enabledElevation;
    tokens.family.pressedElevation = tokens.family.enabledElevation;
    tokens.family.disabledElevation = ElevationLevel::Level0;

    return tokens;
}

} // namespace md
