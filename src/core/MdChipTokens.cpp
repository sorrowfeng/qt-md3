#include "MdChipTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

constexpr int kinds = chipKindCount;
constexpr int states = chipInteractionCount;
constexpr int selections = 2;

const int kEnabled = int(MdChipInteraction::Enabled);
const int kHovered = int(MdChipInteraction::Hovered);
const int kFocused = int(MdChipInteraction::Focused);
const int kPressed = int(MdChipInteraction::Pressed);
const int kDragged = int(MdChipInteraction::Dragged);
const int kDisabled = int(MdChipInteraction::Disabled);

/// A drag state's colour rows read the interaction table of the *label* and
/// the state layer; only the elevation and the container carry their own
/// dragged rows. This fills the four content tables' shared spine for one
/// side of one kind.
///
/// `labelColour`, `stateLayerColour` and `leadingIconColour` are the enabled
/// interaction colours; `focusedLabelColour` / `focusedStateLayerColour` /
/// `focusedLeadingIconColour` are the focused overrides when the family lifts
/// them. Trailing rows are the caller's.
struct Spine
{
    ColorRole label;
    ColorRole stateLayer;
    ColorRole leadingIcon;
    ColorRole focusedLabel;      // label's focused lift
    ColorRole focusedStateLayer;
    ColorRole hoveredLeadingIcon; // the interaction lifts some families' icons
    ColorRole focusedLeadingIcon;
    ColorRole pressedLeadingIcon;
    ColorRole draggedLabel;
    ColorRole draggedStateLayer;
    ColorRole draggedLeadingIcon;
};

void fillSpine(MdNavigationColourSlot *label, MdNavigationColourSlot *stateLayer,
               MdNavigationColourSlot *leadingIcon, const Spine &spine, qreal disabledOpacity)
{
    for (int i = 0; i < states; ++i) {
        label[i] = {spine.label, 1.0};
        stateLayer[i] = {ColorRole::Count, 1.0};
        leadingIcon[i] = {spine.leadingIcon, 1.0};
    }
    if (spine.focusedLabel != spine.label) {
        label[kFocused] = {spine.focusedLabel, 1.0};
    }
    if (spine.draggedLabel != spine.label) {
        label[kDragged] = {spine.draggedLabel, 1.0};
    }
    stateLayer[kHovered] = {spine.stateLayer, 1.0};
    stateLayer[kFocused] = {spine.focusedStateLayer, 1.0};
    stateLayer[kDragged] = {spine.draggedStateLayer, 1.0};
    if (spine.hoveredLeadingIcon != spine.leadingIcon) {
        leadingIcon[kHovered] = {spine.hoveredLeadingIcon, 1.0};
    }
    if (spine.focusedLeadingIcon != spine.leadingIcon) {
        leadingIcon[kFocused] = {spine.focusedLeadingIcon, 1.0};
    }
    if (spine.draggedLeadingIcon != spine.leadingIcon) {
        leadingIcon[kDragged] = {spine.draggedLeadingIcon, 1.0};
    }
    // The pressed overrides land after — the fill functions' special cases
    // call this first and then overwrite the pressed rows.
    label[kPressed] = {spine.label, 1.0};
    stateLayer[kPressed] = {spine.stateLayer, 1.0};
    leadingIcon[kPressed] = {spine.pressedLeadingIcon, 1.0};

    label[kDisabled] = {ColorRole::OnSurface, disabledOpacity};
    leadingIcon[kDisabled] = {ColorRole::OnSurface, disabledOpacity};
}

/// The flat outline table. `focusedColour` is the focus lift (assist lifts to
/// `on-surface`, the others to `on-surface-variant` or nothing).
void fillFlatOutline(MdNavigationColourSlot *outline, ColorRole resting, ColorRole focusedColour,
                     qreal disabledOpacity)
{
    for (int i = 0; i < states; ++i) {
        outline[i] = {resting, 1.0};
    }
    outline[kFocused] = {focusedColour, 1.0};
    outline[kDisabled] = {ColorRole::OnSurface, disabledOpacity};
}

/// The elevated container: the resting colour with the disabled fade to the
/// 0.12 container opacity.
void fillElevatedContainer(MdNavigationColourSlot *container, ColorRole resting,
                           qreal disabledOpacity, ColorRole selectedResting = ColorRole::Count)
{
    for (int i = 0; i < kDisabled; ++i) {
        container[i] = {resting, 1.0};
    }
    container[kDisabled] = {ColorRole::OnSurface, disabledOpacity};
    if (selectedResting != ColorRole::Count) {
        // Unused here — the selected side's own call overwrites.
    }
}

void zeroElevation(ElevationLevel *elevation)
{
    for (int i = 0; i < states; ++i) {
        elevation[i] = ElevationLevel::Level0;
    }
}

/// The elevated elevation ladder every elevated family shares: resting 1,
/// hover 2, focus 1, press 1, drag 4, disabled 0.
void fillElevatedLadder(ElevationLevel *elevation)
{
    elevation[kEnabled] = ElevationLevel::Level1;
    elevation[kHovered] = ElevationLevel::Level2;
    elevation[kFocused] = ElevationLevel::Level1;
    elevation[kPressed] = ElevationLevel::Level1;
    elevation[kDragged] = ElevationLevel::Level4;
    elevation[kDisabled] = ElevationLevel::Level0;
}

/// One side of the filter/input trailing rows: `resting` with the pressed
/// override where the family publishes one.
void fillTrailing(MdNavigationColourSlot *trailing, ColorRole resting, ColorRole pressedOverride,
                  qreal disabledOpacity)
{
    for (int i = 0; i < states; ++i) {
        trailing[i] = {resting, 1.0};
    }
    if (pressedOverride != resting) {
        trailing[kPressed] = {pressedOverride, 1.0};
    }
    trailing[kDisabled] = {ColorRole::OnSurface, disabledOpacity};
}

/// The click-only families (assist, suggestion): both selection halves filled
/// the same, no trailing rows.
void fillClickOnly(MdChipVariantTokens *tokens, ColorRole labelColour,
                   ColorRole stateLayerColour, ColorRole focusedFlatOutline,
                   ColorRole leadingIconColour)
{
    const qreal labelOpacity = tokens->disabledLabelTextOpacity;
    const qreal iconOpacity = tokens->disabledIconOpacity;
    const qreal containerOpacity = tokens->disabledContainerOpacity;

    for (int kind = 0; kind < kinds; ++kind) {
        MdChipSurfaceTokens &s = tokens->surface[kind];
        for (int side = 0; side < selections; ++side) {
            fillSpine(s.label[side], s.stateLayer[side], s.leadingIcon[side],
                      {labelColour, stateLayerColour, leadingIconColour,
                       labelColour, stateLayerColour, leadingIconColour,
                       leadingIconColour, leadingIconColour,
                       labelColour, stateLayerColour, leadingIconColour},
                      labelOpacity);
            // The disabled icon rows fade at the icon opacity, not the label's.
            s.leadingIcon[side][kDisabled] = {ColorRole::OnSurface, iconOpacity};
        }
    }

    // The flat outline: 1 px, resting `outline-variant`, the focus lift, the
    // disabled fade to 0.12.
    MdChipSurfaceTokens &flat = tokens->surface[int(MdChipKind::Flat)];
    for (int side = 0; side < selections; ++side) {
        fillFlatOutline(flat.outline[side], ColorRole::OutlineVariant, focusedFlatOutline,
                        containerOpacity);
        flat.outlineWidth[side] = 1.0;
        for (int i = 0; i < states; ++i) {
            flat.container[side][i] = {ColorRole::Count, 1.0};
        }
        for (int i = 0; i < states; ++i) {
            flat.trailingIcon[side][i] = {ColorRole::Count, 1.0};
        }
    }
    zeroElevation(flat.containerElevation);

    // The elevated container: `surface-container-low`, disabled `on-surface`
    // at the 0.12 container opacity, and the shared elevation ladder. No
    // outline rows — the width is 0.
    MdChipSurfaceTokens &elevated = tokens->surface[int(MdChipKind::Elevated)];
    for (int side = 0; side < selections; ++side) {
        fillElevatedContainer(elevated.container[side], ColorRole::SurfaceContainerLow,
                              containerOpacity);
        for (int i = 0; i < states; ++i) {
            elevated.outline[side][i] = {ColorRole::Count, 1.0};
            elevated.trailingIcon[side][i] = {ColorRole::Count, 1.0};
        }
        elevated.outlineWidth[side] = 0.0;
    }
    fillElevatedLadder(elevated.containerElevation);
}

/// The selectable families (filter, input): the two sides differ.
void fillSelectedSpines(MdChipVariantTokens *tokens, const Spine &unselected,
                        const Spine &selected, const ColorRole pressedUnselectedLayer,
                        const ColorRole pressedSelectedLayer, qreal iconOpacity)
{
    for (int kind = 0; kind < kinds; ++kind) {
        MdChipSurfaceTokens &s = tokens->surface[kind];
        fillSpine(s.label[0], s.stateLayer[0], s.leadingIcon[0], unselected,
                  tokens->disabledLabelTextOpacity);
        fillSpine(s.label[1], s.stateLayer[1], s.leadingIcon[1], selected,
                  tokens->disabledLabelTextOpacity);
        // The pressed state-layer rows are each side's own special case.
        s.stateLayer[0][kPressed] = {pressedUnselectedLayer, 1.0};
        s.stateLayer[1][kPressed] = {pressedSelectedLayer, 1.0};
        for (int side = 0; side < selections; ++side) {
            s.leadingIcon[side][kDisabled] = {ColorRole::OnSurface, iconOpacity};
        }
    }
}

} // namespace

bool MdChipVariantTokens::isSelectable() const
{
    return surface[int(MdChipKind::Flat)].container[0][kPressed].role
           != surface[int(MdChipKind::Flat)].container[1][kPressed].role;
}

const MdNavigationColourSlot &
MdChipVariantTokens::containerFor(MdChipKind kind, int selection, MdChipInteraction state) const
{
    const int k = qBound(0, int(kind), kinds - 1);
    const int s = qBound(0, selection, selections - 1);
    const int i = qBound(0, int(state), states - 1);
    return surface[k].container[s][i];
}

MdChipVariantTokens MdChipVariantTokens::resolve(MdChipVariant variant,
                                                 const MdComponentTokens *overrides)
{
    MdChipVariantTokens tokens;

    using namespace comptoken;
    const char *family = [&] {
        switch (variant) {
        case MdChipVariant::Assist:
            return "assist-chip";
        case MdChipVariant::Filter:
            return "filter-chip";
        case MdChipVariant::Input:
            return "input-chip";
        case MdChipVariant::Suggestion:
        case MdChipVariant::Count:
        default:
            return "suggestion-chip";
        }
    }();

    tokens.containerHeight = lengthOverride(
        overrides, plainKeys(family, "container.height"), 32.0);
    tokens.containerShape = shapeOverride(
        overrides, plainKeys(family, "container.shape"), ShapeCorner::Small);
    tokens.iconSize = lengthOverride(
        overrides, plainKeys(family, "with-icon.icon.size"), 18.0);
    if (variant == MdChipVariant::Input) {
        tokens.iconSize = lengthOverride(
            overrides, plainKeys("input-chip", "with-leading-icon.leading-icon.size"), 18.0);
        tokens.avatarSize = lengthOverride(
            overrides, plainKeys("input-chip", "with-avatar.avatar-size"), 24.0);
        tokens.avatarShape = shapeOverride(
            overrides, plainKeys("input-chip", "with-avatar.avatar-shape"), ShapeCorner::Full);
    }
    tokens.disabledLabelTextOpacity = lengthOverride(
        overrides, plainKeys(family, "disabled.label-text.opacity"), 0.38);
    tokens.disabledContainerOpacity = lengthOverride(
        overrides, plainKeys(family, "elevated.disabled.container.opacity"), 0.12);
    tokens.hoverStateLayerOpacity = lengthOverride(
        overrides, plainKeys(family, "hover.state-layer.opacity"), 0.08);
    tokens.focusStateLayerOpacity = lengthOverride(
        overrides, plainKeys(family, "focus.state-layer.opacity"), 0.12);
    tokens.pressedStateLayerOpacity = lengthOverride(
        overrides, plainKeys(family, "pressed.state-layer.opacity"), 0.12);
    tokens.focusIndicatorOuterOffset = lengthOverride(
        overrides, plainKeys(family, "focus.indicator.outline.offset"), 2.0);
    tokens.focusIndicatorThickness = lengthOverride(
        overrides, plainKeys(family, "focus.indicator.thickness"), 3.0);

    const qreal iconOpacity = tokens.disabledIconOpacity;
    const qreal containerOpacity = tokens.disabledContainerOpacity;

    switch (variant) {
    case MdChipVariant::Assist:
        // Label `on-surface`, state layer `on-surface`, leading icon
        // `primary`, the flat focus outline lifting to `on-surface`.
        fillClickOnly(&tokens, ColorRole::OnSurface, ColorRole::OnSurface, ColorRole::OnSurface,
                      ColorRole::Primary);
        break;

    case MdChipVariant::Suggestion:
        // The same shape as assist with the muted label: `on-surface-variant`
        // everywhere, the leading icon still `primary`.
        fillClickOnly(&tokens, ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant,
                      ColorRole::OnSurfaceVariant, ColorRole::Primary);
        break;

    case MdChipVariant::Filter: {
        // Unselected: `on-surface-variant` content, the leading icon `primary`
        // (the `with-leading-icon.*` rows; the `with-icon.*` duplicates are
        // recorded), the pressed state layer the colour the chip is about to
        // earn (`on-secondary-container`).
        const Spine unselected{ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant,
                               ColorRole::Primary,
                               ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant,
                               ColorRole::Primary, ColorRole::Primary, ColorRole::Primary,
                               ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant,
                               ColorRole::Primary};
        // Selected: `secondary-container` content, the pressed layer the other
        // side of the swap (`on-surface-variant`).
        const Spine selected{ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer,
                             ColorRole::OnSecondaryContainer,
                             ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer,
                             ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer,
                             ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer,
                             ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer};
        fillSelectedSpines(&tokens, unselected, selected, ColorRole::OnSecondaryContainer,
                           ColorRole::OnSurfaceVariant, iconOpacity);

        for (int kind = 0; kind < kinds; ++kind) {
            MdChipSurfaceTokens &s = tokens.surface[kind];
            // Trailing rows (the optional checkmark's slot): unselected
            // `on-surface-variant`, selected `on-secondary-container`.
            fillTrailing(s.trailingIcon[0], ColorRole::OnSurfaceVariant,
                         ColorRole::OnSurfaceVariant, iconOpacity);
            fillTrailing(s.trailingIcon[1], ColorRole::OnSecondaryContainer,
                         ColorRole::OnSecondaryContainer, iconOpacity);
            // Disabled containers: the selected flat side fades to `on-surface`
            // at 0.12; the unselected flat side is transparent with the outline
            // fading (filled below).
            s.container[0][kDisabled] = {ColorRole::Count, 1.0};
            s.container[1][kDisabled] = {ColorRole::OnSurface, containerOpacity};
        }

        // Flat: unselected 1 px `outline-variant` (focus `on-surface-variant`).
        MdChipSurfaceTokens &flat = tokens.surface[int(MdChipKind::Flat)];
        fillFlatOutline(flat.outline[0], ColorRole::OutlineVariant,
                        ColorRole::OnSurfaceVariant, containerOpacity);
        flat.outlineWidth[0] = 1.0;
        for (int i = 0; i < states; ++i) {
            flat.container[0][i] = {ColorRole::Count, 1.0};
        }
        // Selected: no outline at all.
        for (int i = 0; i < states; ++i) {
            flat.outline[1][i] = {ColorRole::Count, 1.0};
            flat.container[1][i] = {ColorRole::SecondaryContainer, 1.0};
        }
        // The disabled selected container fades to `on-surface` at 0.12.
        flat.container[1][kDisabled] = {ColorRole::OnSurface, containerOpacity};
        flat.outlineWidth[1] = 0.0;
        zeroElevation(flat.containerElevation);
        // The one flat elevation row: a *selected* chip rises to level 1 on
        // hover.
        flat.containerElevation[kHovered] = ElevationLevel::Level1;

        // Elevated: unselected `surface-container-low`, selected
        // `secondary-container`, the shared ladder, no outline.
        MdChipSurfaceTokens &elevated = tokens.surface[int(MdChipKind::Elevated)];
        for (int i = 0; i < kDisabled; ++i) {
            elevated.container[0][i] = {ColorRole::SurfaceContainerLow, 1.0};
            elevated.container[1][i] = {ColorRole::SecondaryContainer, 1.0};
        }
        elevated.container[0][kDisabled] = {ColorRole::OnSurface, containerOpacity};
        for (int i = 0; i < states; ++i) {
            elevated.outline[0][i] = {ColorRole::Count, 1.0};
            elevated.outline[1][i] = {ColorRole::Count, 1.0};
        }
        elevated.outlineWidth[0] = elevated.outlineWidth[1] = 0.0;
        fillElevatedLadder(elevated.containerElevation);
        break;
    }

    case MdChipVariant::Input:
    default: {
        // Flat only. Unselected: `on-surface-variant` content with the
        // interaction lift — the leading and trailing icons read `primary`
        // under hover/focus/press/drag; the pressed state layer stays
        // `on-surface-variant` (no swap).
        const Spine unselected{ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant,
                               ColorRole::OnSurfaceVariant,
                               ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant,
                               ColorRole::Primary, ColorRole::Primary, ColorRole::Primary,
                               ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant,
                               ColorRole::Primary};
        const Spine selected{ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer,
                             ColorRole::OnSecondaryContainer,
                             ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer,
                             ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer,
                             ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer,
                             ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer};
        fillSelectedSpines(&tokens, unselected, selected, ColorRole::OnSurfaceVariant,
                           ColorRole::OnSecondaryContainer, iconOpacity);

        for (int kind = 0; kind < kinds; ++kind) {
            MdChipSurfaceTokens &s = tokens.surface[kind];
            // Trailing rows: unselected `on-surface-variant` with the pressed
            // override reading `primary`; selected `on-secondary-container`.
            fillTrailing(s.trailingIcon[0], ColorRole::OnSurfaceVariant, ColorRole::Primary,
                         iconOpacity);
            fillTrailing(s.trailingIcon[1], ColorRole::OnSecondaryContainer,
                         ColorRole::OnSecondaryContainer, iconOpacity);
        }

        // Flat: 1 px unselected outline; selected none, `secondary-container`.
        MdChipSurfaceTokens &flat = tokens.surface[int(MdChipKind::Flat)];
        fillFlatOutline(flat.outline[0], ColorRole::OutlineVariant,
                        ColorRole::OnSurfaceVariant, containerOpacity);
        flat.outlineWidth[0] = 1.0;
        for (int i = 0; i < states; ++i) {
            flat.container[0][i] = {ColorRole::Count, 1.0};
            flat.outline[1][i] = {ColorRole::Count, 1.0};
            flat.container[1][i] = {ColorRole::SecondaryContainer, 1.0};
        }
        // The disabled selected container fades to `on-surface` at 0.12.
        flat.container[1][kDisabled] = {ColorRole::OnSurface, containerOpacity};
        flat.outlineWidth[1] = 0.0;
        zeroElevation(flat.containerElevation);
        // Dragged rises to level 4 in every family (the shared
        // `dragged-container-elevation` row).
        flat.containerElevation[kDragged] = ElevationLevel::Level4;

        // The input family publishes no elevated rows — the elevated kind
        // mirrors the flat one (an `ElevatedInputChip` does not exist
        // upstream either).
        MdChipSurfaceTokens &elevated = tokens.surface[int(MdChipKind::Elevated)];
        for (int side = 0; side < selections; ++side) {
            for (int i = 0; i < states; ++i) {
                elevated.container[side][i] = flat.container[side][i];
                elevated.outline[side][i] = flat.outline[side][i];
                elevated.outlineWidth[side] = flat.outlineWidth[side];
            }
        }
        for (int i = 0; i < states; ++i) {
            elevated.containerElevation[i] = flat.containerElevation[i];
        }
        break;
    }
    }

    return tokens;
}

} // namespace md
