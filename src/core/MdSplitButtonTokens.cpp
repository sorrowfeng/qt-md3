#include "MdSplitButtonTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// The `md.comp.*` override parsing lives in MdCompTokenParse.h, shared with
// the other component resolvers.
using namespace comptoken;

// ---------------------------------------------------------------------------
// md.comp.split-button.<size> — metrics
//
// Transcribed from tokens/versions/latest/sass/_md-comp-split-button-<size>.scss.
// The inner-corner values below are the export's shape-scale references
// resolved through md.sys.shape.corner-value: extra-small 4, small 8,
// medium 12, large-increased 20.
// ---------------------------------------------------------------------------

struct SizeRow
{
    SplitButtonSize size;
    const char *token; ///< the `md.comp.split-button.<token>` size segment

    qreal containerHeight;

    qreal leadingLeadingSpace;
    qreal leadingTrailingSpace;

    qreal trailingIconSize;
    qreal trailingLeadingSpace;
    qreal trailingTrailingSpace;

    qreal innerCornerRest;
    qreal innerCornerHovered;
    qreal innerCornerPressed;
};

const SizeRow kSizeRows[] = {
    // md.comp.split-button.xsmall.* — 32 px; inner rest extra-small (4),
    // hovered/pressed small (8).
    {SplitButtonSize::XSmall, "xsmall", 32.0, 12.0, 10.0, 22.0, 13.0, 13.0, 4.0, 8.0, 8.0},

    // md.comp.split-button.small.* — 40 px; inner rest extra-small (4),
    // hovered/pressed medium (12).
    {SplitButtonSize::Small, "small", 40.0, 16.0, 12.0, 22.0, 13.0, 13.0, 4.0, 12.0, 12.0},

    // md.comp.split-button.medium.* — 56 px; icon 26, spaces 24/24 + 15/15;
    // inner rest extra-small (4), hovered/pressed medium (12).
    {SplitButtonSize::Medium, "medium", 56.0, 24.0, 24.0, 26.0, 15.0, 15.0, 4.0, 12.0, 12.0},

    // md.comp.split-button.large.* — 96 px; icon 38, spaces 48/48 + 29/29;
    // inner rest small (8), hovered/pressed large-increased (20).
    {SplitButtonSize::Large, "large", 96.0, 48.0, 48.0, 38.0, 29.0, 29.0, 8.0, 20.0, 20.0},

    // md.comp.split-button.xlarge.* — 136 px; icon 50, spaces 64/64 + 43/43;
    // inner rest medium (12), hovered/pressed large-increased (20).
    {SplitButtonSize::XLarge, "xlarge", 136.0, 64.0, 64.0, 50.0, 43.0, 43.0, 12.0, 20.0, 20.0},
};

// `between-space` is 2 px in all five published size sets — one row, no
// per-size override in the export.
constexpr qreal kBetweenSpace = 2.0;

const SizeRow &sizeRow(SplitButtonSize size)
{
    for (const SizeRow &row : kSizeRows) {
        if (row.size == size) {
            return row;
        }
    }
    return kSizeRows[1]; // Small, the 40 px row the base set matches.
}

// ---------------------------------------------------------------------------
// The button set the colour rows come from.
//
// The spec page: "Split buttons use the same color schemes as standard
// buttons ... shown in the following token module." The export publishes no
// split-button colour rows, so the rows are the button family's. The two
// Expressive size scales happen to agree on every height (32 / 40 / 56 /
// 96 / 136), so the identity mapping keeps the borrowed typography aligned
// with the physical size. Recorded in docs/porting-todo.md.
// ---------------------------------------------------------------------------

ButtonSize buttonSizeFor(SplitButtonSize size)
{
    switch (size) {
    case SplitButtonSize::XSmall: return ButtonSize::XSmall;
    case SplitButtonSize::Small: return ButtonSize::Small;
    case SplitButtonSize::Medium: return ButtonSize::Medium;
    case SplitButtonSize::Large: return ButtonSize::Large;
    case SplitButtonSize::XLarge: return ButtonSize::XLarge;
    case SplitButtonSize::Count: break;
    }
    return ButtonSize::Small;
}

} // namespace

MdSplitButtonTokens MdSplitButtonTokens::resolve(ButtonVariant variant,
                                                 SplitButtonSize size,
                                                 const MdComponentTokens *overrides)
{
    const SizeRow &row = sizeRow(size);
    MdSplitButtonTokens tokens;

    // --- metrics: md.comp.split-button.<size>.<token> ---------------------
    tokens.metrics.containerHeight = lengthOverride(
        overrides, sizeKeys("split-button", row.token, "container.height"), row.containerHeight);
    tokens.metrics.betweenSpace = lengthOverride(
        overrides, sizeKeys("split-button", row.token, "between.space"), kBetweenSpace);

    tokens.metrics.leadingLeadingSpace =
        lengthOverride(overrides, sizeKeys("split-button", row.token, "leading-button.leading-space"),
                       row.leadingLeadingSpace);
    tokens.metrics.leadingTrailingSpace =
        lengthOverride(overrides, sizeKeys("split-button", row.token, "leading-button.trailing-space"),
                       row.leadingTrailingSpace);

    tokens.metrics.trailingIconSize = lengthOverride(
        overrides, sizeKeys("split-button", row.token, "trailing-button.icon.size"),
        row.trailingIconSize);
    tokens.metrics.trailingLeadingSpace =
        lengthOverride(overrides, sizeKeys("split-button", row.token, "trailing-button.leading-space"),
                       row.trailingLeadingSpace);
    tokens.metrics.trailingTrailingSpace =
        lengthOverride(overrides, sizeKeys("split-button", row.token, "trailing-button.trailing-space"),
                       row.trailingTrailingSpace);

    tokens.metrics.innerCornerRest =
        lengthOverride(overrides, sizeKeys("split-button", row.token, "inner-corner.corner-size"),
                       row.innerCornerRest);
    tokens.metrics.innerCornerHovered =
        lengthOverride(overrides, sizeKeys("split-button", row.token, "inner-corner.hovered.corner-size"),
                       row.innerCornerHovered);
    tokens.metrics.innerCornerPressed =
        lengthOverride(overrides, sizeKeys("split-button", row.token, "inner-corner.pressed.corner-size"),
                       row.innerCornerPressed);

    // --- colour / type / focus rows: the button family's ------------------
    // Shape rows of the button set are ignored on purpose: the split button's
    // outer corner is its own corner-full token and the inner corners are the
    // metric rows above.
    tokens.button = MdButtonTokens::resolve(variant, buttonSizeFor(size), ButtonShape::Round,
                                            overrides);

    return tokens;
}

} // namespace md
