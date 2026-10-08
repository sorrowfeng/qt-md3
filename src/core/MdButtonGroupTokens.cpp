#include "MdButtonGroupTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

using namespace comptoken;

/// `md.comp.button-group.<form>.<size>`.
///
/// One row per size, carrying both forms' values, because the two files are
/// keyed by the same five size names and a lookup that had to consult two
/// tables would be harder to read than one row that shows the difference.
struct SizeRow
{
    ButtonSize size;
    const char *token;
    qreal containerHeight;
    /// `standard.between-space`.
    qreal standardBetweenSpace;
    /// `standard.pressed.item.width.multiplier`, as a fraction.
    qreal pressedWidthMultiplier;
    /// `connected.inner-corner.corner-size`, from the token export.
    ShapeCorner innerCorner;
    /// `connected.pressed.inner-corner.corner-size`.
    ShapeCorner pressedInnerCorner;
    /// The *square* form's `container.shape`, which the token export does not
    /// publish — taken from the specs page's "Square connected button group"
    /// measurement list. See the header note.
    ShapeCorner squareOuterCorner;
    /// The token export's xsmall `inner-corner` is corner-small; the specs page
    /// lists 4 dp. The export wins. Recorded, not reconciled.
    qreal minimumItemExtent;
};

const SizeRow kSizeRows[] = {
    // md.comp.button-group.{standard,connected}.xsmall.*
    {ButtonSize::XSmall, "xsmall", 32.0, 18.0, 0.15, ShapeCorner::Small, ShapeCorner::ExtraSmall,
     ShapeCorner::ExtraSmall, 48.0},
    // ...small.*
    {ButtonSize::Small, "small", 40.0, 12.0, 0.15, ShapeCorner::Small, ShapeCorner::ExtraSmall,
     ShapeCorner::Small, 48.0},
    // ...medium.*
    {ButtonSize::Medium, "medium", 56.0, 8.0, 0.15, ShapeCorner::Small, ShapeCorner::ExtraSmall,
     ShapeCorner::Small, 0.0},
    // ...large.*
    {ButtonSize::Large, "large", 96.0, 8.0, 0.15, ShapeCorner::Large, ShapeCorner::Medium,
     ShapeCorner::Large, 0.0},
    // ...xlarge.*
    {ButtonSize::XLarge, "xlarge", 136.0, 8.0, 0.15, ShapeCorner::LargeIncreased,
     ShapeCorner::Large, ShapeCorner::LargeIncreased, 0.0},
};

const SizeRow &sizeRow(ButtonSize size)
{
    for (const SizeRow &row : kSizeRows) {
        if (row.size == size) {
            return row;
        }
    }
    return kSizeRows[1]; // Small.
}

/// `connected.between-space` is 2 px at every size, from the shared base file.
constexpr qreal kConnectedBetweenSpace = 2.0;

/// `md.comp.button-group.standard.<size>.pressed.item.width.motion.spring.*`
constexpr qreal kSpringStiffness = 1400.0;
constexpr qreal kSpringDampingRatio = 0.9;

/// `connected.selected.inner-corner.corner-size: 50%`.
constexpr qreal kSelectedInnerCornerFraction = 0.5;

} // namespace

QString MdButtonGroupTokens::key(ButtonGroupVariant variant, ButtonSize size, const QString &token)
{
    return QStringLiteral("md.comp.button-group.%1.%2.%3")
        .arg(buttonGroupVariantName(variant), buttonSizeName(size), token);
}

MdButtonGroupTokens MdButtonGroupTokens::resolve(ButtonGroupVariant variant,
                                                ButtonSize size,
                                                ButtonGroupShape shape,
                                                const MdComponentTokens *overrides)
{
    const SizeRow &row = sizeRow(size);
    const char *form = variant == ButtonGroupVariant::Connected ? "connected" : "standard";

    MdButtonGroupTokens tokens;
    tokens.variant = variant;
    tokens.size = size;
    tokens.containerHeight =
        lengthOverride(overrides, variantSizeKeys("button-group", form, row.token,
                                                    "container.height"),
                       row.containerHeight);

    if (variant == ButtonGroupVariant::Connected) {
        tokens.betweenSpace =
            lengthOverride(overrides,
                           variantSizeKeys("button-group", form, row.token, "between-space"),
                           kConnectedBetweenSpace);

        // Connected items do not push their neighbours, so there is no width
        // multiplier and no spring to drive one.
        tokens.pressedWidthMultiplier = 0.0;
        tokens.springStiffness = 0.0;
        tokens.springDampingRatio = 0.0;

        // The export publishes the round form's outer shape only.
        const ShapeCorner publishedOuter = ShapeCorner::Full;
        tokens.outerCorner =
            shapeOverride(overrides,
                          variantSizeKeys("button-group", form, row.token, "container.shape"),
                          shape == ButtonGroupShape::Square ? row.squareOuterCorner : publishedOuter);

        tokens.innerCorner =
            shapeOverride(overrides,
                          variantSizeKeys("button-group", form, row.token,
                                            "inner-corner.corner-size"),
                          row.innerCorner);
        tokens.pressedInnerCorner =
            shapeOverride(overrides,
                          variantSizeKeys("button-group", form, row.token,
                                            "pressed.inner-corner.corner-size"),
                          row.pressedInnerCorner);

        tokens.selectedInnerCornerFraction =
            lengthOverride(overrides,
                           variantSizeKeys("button-group", form, row.token,
                                             "selected.inner-corner.corner-size"),
                           kSelectedInnerCornerFraction);

        tokens.minimumItemExtent = row.minimumItemExtent;
    } else {
        tokens.betweenSpace =
            lengthOverride(overrides,
                           variantSizeKeys("button-group", form, row.token, "between-space"),
                           row.standardBetweenSpace);
        tokens.pressedWidthMultiplier =
            lengthOverride(overrides,
                           variantSizeKeys("button-group", form, row.token,
                                             "pressed.item.width.multiplier"),
                           row.pressedWidthMultiplier);
        tokens.springStiffness =
            lengthOverride(overrides,
                           variantSizeKeys("button-group", form, row.token,
                                             "pressed.item.width.motion.spring.stiffness"),
                           kSpringStiffness);
        tokens.springDampingRatio =
            lengthOverride(overrides,
                           variantSizeKeys("button-group", form, row.token,
                                             "pressed.item.width.motion.spring.dampening"),
                           kSpringDampingRatio);
    }

    return tokens;
}

} // namespace md
