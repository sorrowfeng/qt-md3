#include "MdIconButtonTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// The `md.comp.*` override parsing itself lives in MdCompTokenParse.h, shared
// with the other component resolvers so a length or a shape name can only be
// parsed one way across the whole library.
using namespace comptoken;

// ---------------------------------------------------------------------------
// md.comp.icon-button.<size>
// ---------------------------------------------------------------------------

struct SizeRow
{
    ButtonSize size;
    const char *token;          ///< the `md.comp.icon-button.<token>` segment
    qreal height;
    qreal iconSize;
    qreal defaultSpace;
    qreal narrowSpace;
    qreal wideSpace;
    qreal outlineWidth;
    ShapeCorner roundResting;
    ShapeCorner squareResting;
    ShapeCorner pressed;
    ShapeCorner selectedRound;
    ShapeCorner selectedSquare;
};

const SizeRow kSizeRows[] = {
    // md.comp.icon-button.xsmall.*
    {ButtonSize::XSmall, "xsmall", 32.0, 20.0, 6.0, 4.0, 10.0, 1.0,
     ShapeCorner::Full, ShapeCorner::Medium, ShapeCorner::Small,
     ShapeCorner::Medium, ShapeCorner::Full},
    // md.comp.icon-button.small.*
    {ButtonSize::Small, "small", 40.0, 24.0, 8.0, 4.0, 14.0, 1.0,
     ShapeCorner::Full, ShapeCorner::Medium, ShapeCorner::Small,
     ShapeCorner::Medium, ShapeCorner::Full},
    // md.comp.icon-button.medium.*
    {ButtonSize::Medium, "medium", 56.0, 24.0, 16.0, 12.0, 24.0, 1.0,
     ShapeCorner::Full, ShapeCorner::Large, ShapeCorner::Medium,
     ShapeCorner::Large, ShapeCorner::Full},
    // md.comp.icon-button.large.*
    {ButtonSize::Large, "large", 96.0, 32.0, 32.0, 16.0, 48.0, 2.0,
     ShapeCorner::Full, ShapeCorner::ExtraLarge, ShapeCorner::Large,
     ShapeCorner::ExtraLarge, ShapeCorner::Full},
    // md.comp.icon-button.xlarge.*
    {ButtonSize::XLarge, "xlarge", 136.0, 40.0, 48.0, 32.0, 72.0, 3.0,
     ShapeCorner::Full, ShapeCorner::ExtraLarge, ShapeCorner::Large,
     ShapeCorner::ExtraLarge, ShapeCorner::Full},
};

const SizeRow &sizeRow(ButtonSize size)
{
    for (const SizeRow &row : kSizeRows) {
        if (row.size == size) {
            return row;
        }
    }
    return kSizeRows[1]; // Small, the 40 px default the base token set matches.
}

// ---------------------------------------------------------------------------
// md.comp.icon-button.<style> — colours
//
// The published colour matrix is three families (plain / selected /
// unselected), each over five states, and the families are published
// incompletely on purpose: the base file carries the unselected toggle
// colours, a style file only republishes what it changes, and a missing
// selected slot means "same as the plain one". The resolver below therefore
// transcribes each family per style with the fallback folded in, and
// TestMd3IconButton pins the folded result against the files slot by slot.
//
// One structural fact worth keeping visible: the *base* file
// (`_md-comp-icon-button.scss`) and the filled style file publish the same
// values — the base set is the filled set. It is transcribed once.
// ---------------------------------------------------------------------------

struct StyleRow
{
    IconButtonVariant variant;

    // --- plain family: enabled/hovered/focused/pressed, then disabled -----
    ColorRole container;
    ColorRole icon;
    ColorRole stateLayer;
    ColorRole outline;

    // --- selected family ---------------------------------------------------
    ColorRole selectedContainer;   ///< Count = "same as the plain container"
    ColorRole selectedIcon;
    ColorRole selectedStateLayer;
    /// Only the outlined style publishes selected-disabled container tokens;
    /// Count here means "selected disabled falls back to the plain disabled".
    ColorRole selectedDisabledContainer;

    // --- unselected family -------------------------------------------------
    /// Count = "same as the plain container/icon" — true for every style
    /// except filled, whose toggle shows a surface-container chip while
    /// unchecked, and tonal, which re-states what it inherits.
    ColorRole unselectedContainer;
    ColorRole unselectedIcon;
    ColorRole unselectedStateLayer;
};

const StyleRow kStyleRows[] = {
    // md.comp.icon-button.standard.* — no container, ever. Selected turns the
    // icon primary; the state layer follows it.
    {IconButtonVariant::Standard,
     ColorRole::Count, ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant, ColorRole::Count,
     ColorRole::Count, ColorRole::Primary, ColorRole::Primary, ColorRole::Count,
     ColorRole::Count, ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant},

    // md.comp.icon-button.filled.* — the base set. Primary chip, on-primary
    // icon; the unchecked toggle shows the surface-container chip the base
    // file publishes as `unselected-*`.
    {IconButtonVariant::Filled,
     ColorRole::Primary, ColorRole::OnPrimary, ColorRole::OnPrimary, ColorRole::Count,
     ColorRole::Primary, ColorRole::OnPrimary, ColorRole::OnPrimary, ColorRole::Count,
     ColorRole::SurfaceContainer, ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant},

    // md.comp.icon-button.tonal.*
    {IconButtonVariant::Tonal,
     ColorRole::SecondaryContainer, ColorRole::OnSecondaryContainer,
     ColorRole::OnSecondaryContainer, ColorRole::Count,
     ColorRole::Secondary, ColorRole::OnSecondary, ColorRole::OnSecondary, ColorRole::Count,
     ColorRole::SecondaryContainer, ColorRole::OnSecondaryContainer,
     ColorRole::OnSecondaryContainer},

    // md.comp.icon-button.outlined.* — outline-variant stroke, no fill; the
    // checked toggle fills with inverse-surface and drops the stroke (the
    // export publishes no `selected.outline.color`). It is also the one style
    // with selected-disabled container tokens.
    {IconButtonVariant::Outlined,
     ColorRole::Count, ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant,
     ColorRole::OutlineVariant,
     ColorRole::InverseSurface, ColorRole::InverseOnSurface, ColorRole::InverseOnSurface,
     ColorRole::OnSurface,
     ColorRole::Count, ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant},
};

const StyleRow &styleRow(IconButtonVariant variant)
{
    for (const StyleRow &row : kStyleRows) {
        if (row.variant == variant) {
            return row;
        }
    }
    return kStyleRows[1]; // Filled, the base set.
}

/// The disabled container keeps the family's presence: a style that paints no
/// container enabled paints none disabled either, and a selected outlined
/// toggle keeps its chip while disabled, faded by its own opacity token.
MdIconButtonColourSlot disabledContainerFor(const StyleRow &style,
                                            bool selectedFamily,
                                            qreal fallbackOpacity)
{
    if (selectedFamily && style.selectedDisabledContainer != ColorRole::Count) {
        // md.comp.icon-button.outlined.selected-disabled.container.color and
        // .opacity — the only published selected-disabled pair.
        return MdIconButtonColourSlot{style.selectedDisabledContainer, 0.10};
    }
    const ColorRole role = selectedFamily && style.selectedContainer != ColorRole::Count
                               ? style.selectedContainer
                               : style.container;
    if (role == ColorRole::Count) {
        return MdIconButtonColourSlot();
    }
    return MdIconButtonColourSlot{ColorRole::OnSurface, fallbackOpacity};
}

} // namespace

const MdIconButtonStateColours &MdIconButtonFamily::state(MdIconButtonState which) const
{
    switch (which) {
    case MdIconButtonState::Enabled: return enabled;
    case MdIconButtonState::Hovered: return hovered;
    case MdIconButtonState::Focused: return focused;
    case MdIconButtonState::Pressed: return pressed;
    case MdIconButtonState::Disabled: return disabled;
    case MdIconButtonState::Count: break;
    }
    return enabled;
}

const MdIconButtonFamily &MdIconButtonTokens::familyFor(bool toggleable, bool selectedNow) const
{
    if (!toggleable) {
        return plain;
    }
    return selectedNow ? selected : unselected;
}

MdIconButtonTokens MdIconButtonTokens::resolve(IconButtonVariant variant,
                                               ButtonSize size,
                                               ButtonShape shape,
                                               IconButtonSpaceTrack track,
                                               const MdComponentTokens *overrides)
{
    const SizeRow &row = sizeRow(size);
    const StyleRow &style = styleRow(variant);
    MdIconButtonTokens tokens;

    // --- metrics: md.comp.icon-button.<size>.<token> ---------------------
    tokens.containerHeight =
        lengthOverride(overrides, sizeKeys("icon-button", row.token, "container.height"),
                       row.height);
    tokens.iconSize =
        lengthOverride(overrides, sizeKeys("icon-button", row.token, "icon.size"), row.iconSize);
    // The three padding tracks are flat, hyphenated tokens in the export —
    // `md.comp.icon-button.<size>.default-leading-space` and friends — not
    // variant segments, so they read through sizeKeys with the track in the
    // token name.
    const auto space = [&](const char *track, const char *side, qreal fallback) {
        const QByteArray key = QByteArray(track) + '-' + side;
        return lengthOverride(overrides, sizeKeys("icon-button", row.token, key.constData()),
                              fallback);
    };
    tokens.defaultLeadingSpace = space("default", "leading-space", row.defaultSpace);
    tokens.defaultTrailingSpace = space("default", "trailing-space", row.defaultSpace);
    tokens.narrowLeadingSpace = space("narrow", "leading-space", row.narrowSpace);
    tokens.narrowTrailingSpace = space("narrow", "trailing-space", row.narrowSpace);
    tokens.wideLeadingSpace = space("wide", "leading-space", row.wideSpace);
    tokens.wideTrailingSpace = space("wide", "trailing-space", row.wideSpace);
    tokens.outlineWidth =
        lengthOverride(overrides, sizeKeys("icon-button", row.token, "outlined.outline.width"),
                       row.outlineWidth);

    // --- shapes -----------------------------------------------------------
    tokens.roundShape = shapeOverride(
        overrides, sizeKeys("icon-button", row.token, "container.shape.round"), row.roundResting);
    tokens.squareShape = shapeOverride(
        overrides, sizeKeys("icon-button", row.token, "container.shape.square"),
        row.squareResting);
    tokens.pressedShape = shapeOverride(
        overrides, sizeKeys("icon-button", row.token, "pressed.container.shape"),
        row.pressed);
    tokens.selectedRoundShape = shapeOverride(
        overrides, sizeKeys("icon-button", row.token, "selected.container.shape.round"),
        row.selectedRound);
    tokens.selectedSquareShape = shapeOverride(
        overrides, sizeKeys("icon-button", row.token, "selected.container.shape.square"),
        row.selectedSquare);

    // --- press morph spring: the whole size scale shares spring-fast-spatial
    tokens.springStiffness = lengthOverride(
        overrides,
        plainKeys("icon-button", "pressed.container.corner-size.motion.spring.stiffness"), 1400.0);
    tokens.springDampingRatio = lengthOverride(
        overrides,
        plainKeys("icon-button", "pressed.container.corner-size.motion.spring.damping"), 0.9);

    // --- focus indicator --------------------------------------------------
    tokens.focusIndicator = ColorRole::Secondary;
    tokens.focusIndicatorThickness =
        lengthOverride(overrides, plainKeys("icon-button", "focus.indicator.thickness"), 3.0);
    tokens.focusIndicatorOffset =
        lengthOverride(overrides, plainKeys("icon-button", "focus.indicator.outline.offset"), 2.0);

    // --- colours ----------------------------------------------------------
    const qreal disabledIconOpacity =
        lengthOverride(overrides, plainKeys("icon-button", "disabled.icon.opacity"), 0.38);

    // Plain family. The four interactive states share one published row per
    // style — the files publish `hovered.*`, `focused.*` and `pressed.*` as
    // the same role as enabled for every style, so the row is transcribed once
    // and the state layer opacity is md.sys.state's business, not this table's.
    const MdIconButtonStateColours interactive{
        MdIconButtonColourSlot{style.container, 1.0},
        MdIconButtonColourSlot{style.icon, 1.0},
        style.stateLayer,
        style.outline,
    };
    tokens.plain.enabled = interactive;
    tokens.plain.hovered = interactive;
    tokens.plain.focused = interactive;
    tokens.plain.pressed = interactive;
    tokens.plain.disabled = MdIconButtonStateColours{
        disabledContainerFor(style, /*selectedFamily=*/false, 0.10),
        MdIconButtonColourSlot{ColorRole::OnSurface, disabledIconOpacity},
        ColorRole::Count,
        style.outline,
    };

    // Selected family. A Count slot in the row means "the plain colour
    // carries over", which is how the files are shaped: filled's
    // `selected.container.color` re-states primary that `container.color`
    // already says, while standard's selected set only turns the icon
    // primary and has no container tokens at all.
    const ColorRole selectedContainer =
        style.selectedContainer != ColorRole::Count ? style.selectedContainer : style.container;
    const MdIconButtonStateColours selectedInteractive{
        MdIconButtonColourSlot{selectedContainer, 1.0},
        MdIconButtonColourSlot{style.selectedIcon, 1.0},
        style.selectedStateLayer,
        // No selected outline exists in the export — for any style. The
        // checked outlined toggle is a filled chip, and filled and tonal have
        // no outline to lose in the first place.
        ColorRole::Count,
    };
    tokens.selected.enabled = selectedInteractive;
    tokens.selected.hovered = selectedInteractive;
    tokens.selected.focused = selectedInteractive;
    tokens.selected.pressed = selectedInteractive;
    tokens.selected.disabled = MdIconButtonStateColours{
        disabledContainerFor(style, /*selectedFamily=*/true, 0.10),
        MdIconButtonColourSlot{ColorRole::OnSurface, disabledIconOpacity},
        ColorRole::Count,
        // The export publishes no selected outline: the checked outlined
        // toggle is a filled chip, not a stroked one.
        ColorRole::Count,
    };

    // Unselected family. For every style except filled (and the re-stating
    // tonal) this is the plain set, which is what the base file's
    // `unselected-*` block records; filled's unchecked toggle is the
    // surface-container chip.
    const ColorRole unselectedContainer =
        style.unselectedContainer != ColorRole::Count ? style.unselectedContainer
                                                      : style.container;
    const ColorRole unselectedStateLayer =
        style.unselectedStateLayer != ColorRole::Count ? style.unselectedStateLayer
                                                       : style.stateLayer;
    const MdIconButtonStateColours unselectedInteractive{
        MdIconButtonColourSlot{unselectedContainer, 1.0},
        MdIconButtonColourSlot{style.unselectedIcon, 1.0},
        unselectedStateLayer,
        style.outline,
    };
    tokens.unselected.enabled = unselectedInteractive;
    tokens.unselected.hovered = unselectedInteractive;
    tokens.unselected.focused = unselectedInteractive;
    tokens.unselected.pressed = unselectedInteractive;
    tokens.unselected.disabled = MdIconButtonStateColours{
        disabledContainerFor(style, /*selectedFamily=*/false, 0.10),
        MdIconButtonColourSlot{ColorRole::OnSurface, disabledIconOpacity},
        ColorRole::Count,
        style.outline,
    };

    return tokens;
}

} // namespace md
