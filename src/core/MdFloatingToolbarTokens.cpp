#include "MdFloatingToolbarTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

using namespace comptoken;

/// `md.comp.toolbar.<token>`.
QStringList toolbarKeys(const char *token)
{
    return plainKeys("toolbar", token);
}

/// A slot with a role and the default (absent) opacity.
MdToolbarColourSlot slot(ColorRole role)
{
    MdToolbarColourSlot out;
    out.role = role;
    return out;
}

/// A slot with a role and an explicit disabled opacity.
MdToolbarColourSlot faded(ColorRole role, qreal opacity)
{
    MdToolbarColourSlot out;
    out.role = role;
    out.opacity = opacity;
    return out;
}

/// The published disabled opacities, which both schemes share.
constexpr qreal kDisabledIconOpacity = 0.38;
constexpr qreal kDisabledLabelOpacity = 0.38;

/// Fill one scheme's tables for `icon`, `button` and `selected*` roles.
///
/// The four published groups reduce to this: the unselected rows use
/// `iconRole` throughout and `on-surface` when disabled; the selected rows use
/// `selectedRole` throughout, publish a container colour, and repeat the
/// unselected disabled rows (the export has no selected disabled row).
void fillScheme(MdToolbarScheme *scheme,
                ColorRole containerColor,
                ColorRole buttonContainerColor,
                ColorRole iconRole,
                ColorRole selectedRole,
                ColorRole selectedContainerColor)
{
    scheme->containerColor = containerColor;
    scheme->buttonContainerColor = buttonContainerColor;
    scheme->containerShape = ShapeCorner::Full;

    // --- unselected --------------------------------------------------------
    scheme->unselected[int(MdToolbarItemState::Enabled)] = {};
    scheme->unselected[int(MdToolbarItemState::Enabled)].icon = slot(iconRole);
    scheme->unselected[int(MdToolbarItemState::Enabled)].labelText = slot(iconRole);

    scheme->unselected[int(MdToolbarItemState::Hovered)] = {};
    scheme->unselected[int(MdToolbarItemState::Hovered)].icon = slot(iconRole);
    scheme->unselected[int(MdToolbarItemState::Hovered)].labelText = slot(iconRole);
    scheme->unselected[int(MdToolbarItemState::Hovered)].stateLayer = iconRole;

    scheme->unselected[int(MdToolbarItemState::Focused)] =
        scheme->unselected[int(MdToolbarItemState::Hovered)];
    scheme->unselected[int(MdToolbarItemState::Pressed)] =
        scheme->unselected[int(MdToolbarItemState::Hovered)];

    scheme->unselected[int(MdToolbarItemState::Disabled)] = {};
    scheme->unselected[int(MdToolbarItemState::Disabled)].icon =
        faded(ColorRole::OnSurface, kDisabledIconOpacity);
    scheme->unselected[int(MdToolbarItemState::Disabled)].labelText =
        faded(ColorRole::OnSurface, kDisabledLabelOpacity);

    // --- selected ----------------------------------------------------------
    scheme->selected[int(MdToolbarItemState::Enabled)] = {};
    scheme->selected[int(MdToolbarItemState::Enabled)].container = slot(selectedContainerColor);
    scheme->selected[int(MdToolbarItemState::Enabled)].icon = slot(selectedRole);
    scheme->selected[int(MdToolbarItemState::Enabled)].labelText = slot(selectedRole);

    scheme->selected[int(MdToolbarItemState::Hovered)] =
        scheme->selected[int(MdToolbarItemState::Enabled)];
    scheme->selected[int(MdToolbarItemState::Hovered)].stateLayer = selectedRole;

    scheme->selected[int(MdToolbarItemState::Focused)] =
        scheme->selected[int(MdToolbarItemState::Hovered)];
    scheme->selected[int(MdToolbarItemState::Pressed)] =
        scheme->selected[int(MdToolbarItemState::Hovered)];

    // No selected-disabled rows are published; Compose's `!enabled ->
    // disabled` precedence means the unselected disabled colours are what a
    // disabled selected item paints anyway.
    scheme->selected[int(MdToolbarItemState::Disabled)] =
        scheme->unselected[int(MdToolbarItemState::Disabled)];
}

} // namespace

// ---------------------------------------------------------------------------
// MdToolbarItemColours
// ---------------------------------------------------------------------------

qreal MdToolbarItemColours::stateLayerOpacityFor(MdToolbarItemState state) const
{
    switch (state) {
    case MdToolbarItemState::Hovered:
        return hoverStateLayerOpacity;
    case MdToolbarItemState::Focused:
        return focusStateLayerOpacity;
    case MdToolbarItemState::Pressed:
        return pressedStateLayerOpacity;
    case MdToolbarItemState::Enabled:
    case MdToolbarItemState::Disabled:
    case MdToolbarItemState::Count:
        break;
    }
    return 0.0;
}

// ---------------------------------------------------------------------------
// MdToolbarScheme
// ---------------------------------------------------------------------------

const MdToolbarItemColours &MdToolbarScheme::item(bool selected, MdToolbarItemState state) const
{
    const int index =
        (state >= MdToolbarItemState::Count) ? int(MdToolbarItemState::Enabled) : int(state);
    return selected ? this->selected[index] : unselected[index];
}

// ---------------------------------------------------------------------------
// MdToolbarFabTokens
// ---------------------------------------------------------------------------

qreal MdToolbarFabTokens::sizeFor(qreal expansionProgress) const
{
    // Compose lerps `FabSizeRange` by `1 - expandedProgress`, i.e. the FAB is
    // at its *smallest* when the toolbar is fully expanded.
    const qreal clamped = qBound<qreal>(0.0, expansionProgress, 1.0);
    return collapsedSize + (expandedSize - collapsedSize) * clamped;
}

ShapeCorner MdToolbarFabTokens::shapeFor(qreal expansionProgress) const
{
    return expansionProgress >= 0.5 ? expandedShape : collapsedShape;
}

// ---------------------------------------------------------------------------
// MdFloatingToolbarTokens
// ---------------------------------------------------------------------------

const MdToolbarScheme &MdFloatingToolbarTokens::schemeFor(MdToolbarColorScheme scheme) const
{
    return scheme == MdToolbarColorScheme::Vibrant ? vibrant : standard;
}

qreal MdFloatingToolbarTokens::containerCrossExtent(MdToolbarOrientation orientation) const
{
    return orientation == MdToolbarOrientation::Vertical ? verticalContainerWidth
                                                        : horizontalContainerHeight;
}

qreal MdFloatingToolbarTokens::externalSpaceFor(MdToolbarOrientation orientation) const
{
    return orientation == MdToolbarOrientation::Vertical ? verticalContainerExternalSpace
                                                        : horizontalContainerExternalSpace;
}

MdFloatingToolbarTokens MdFloatingToolbarTokens::resolve(const MdComponentTokens *overrides)
{
    MdFloatingToolbarTokens tokens;

    using namespace comptoken;

    // --- container geometry -------------------------------------------------
    tokens.containerHeight =
        lengthOverride(overrides, toolbarKeys("floating.container.height"), tokens.containerHeight);
    tokens.horizontalContainerHeight =
        lengthOverride(overrides, toolbarKeys("floating.horizontal.container.height"),
                       tokens.horizontalContainerHeight);
    tokens.verticalContainerWidth = lengthOverride(
        overrides, toolbarKeys("floating.vertical.container.width"), tokens.verticalContainerWidth);

    tokens.containerLeadingSpace =
        lengthOverride(overrides, toolbarKeys("floating.container.leading-space"),
                       tokens.containerLeadingSpace);
    tokens.containerTrailingSpace =
        lengthOverride(overrides, toolbarKeys("floating.container.trailing-space"),
                       tokens.containerTrailingSpace);
    tokens.containerBetweenSpace =
        lengthOverride(overrides, toolbarKeys("floating.container.between-space"),
                       tokens.containerBetweenSpace);

    tokens.containerExternalPadding =
        lengthOverride(overrides, toolbarKeys("floating.container.external-padding"),
                       tokens.containerExternalPadding);
    tokens.horizontalContainerExternalSpace =
        lengthOverride(overrides, toolbarKeys("floating.horizontal.container.external-space"),
                       tokens.horizontalContainerExternalSpace);
    tokens.verticalContainerExternalSpace =
        lengthOverride(overrides, toolbarKeys("floating.vertical.container.external-space"),
                       tokens.verticalContainerExternalSpace);

    tokens.containerShape = shapeOverride(
        overrides, toolbarKeys("floating.container.shape"), tokens.containerShape);

    tokens.scrollDistanceThreshold =
        lengthOverride(overrides, toolbarKeys("scroll-distance-threshold"),
                       tokens.scrollDistanceThreshold);

    // --- the adjacent FAB ---------------------------------------------------
    tokens.fab.betweenSpace = lengthOverride(
        overrides, toolbarKeys("floating.fab.between-space"), tokens.fab.betweenSpace);
    tokens.fab.expandedSize = lengthOverride(
        overrides, toolbarKeys("floating.fab.container.width"), tokens.fab.expandedSize);
    tokens.fab.expandedIconSize = lengthOverride(
        overrides, toolbarKeys("floating.fab.icon.size"), tokens.fab.expandedIconSize);
    tokens.fab.expandedShape = shapeOverride(
        overrides, toolbarKeys("floating.fab.container.shape"), tokens.fab.expandedShape);
    tokens.fab.collapsedSize = lengthOverride(
        overrides, toolbarKeys("floating.fab.medium.container.width"), tokens.fab.collapsedSize);
    tokens.fab.collapsedIconSize = lengthOverride(
        overrides, toolbarKeys("floating.fab.medium.icon.size"), tokens.fab.collapsedIconSize);
    tokens.fab.collapsedShape = shapeOverride(
        overrides, toolbarKeys("floating.fab.medium.container.shape"), tokens.fab.collapsedShape);

    // --- the two colour schemes --------------------------------------------
    //
    // Every colour below is a published role, and colours are not overridable
    // — the same rule the rest of the library follows.
    fillScheme(&tokens.standard, ColorRole::SurfaceContainer, ColorRole::SurfaceContainer,
               ColorRole::OnSurfaceVariant, ColorRole::OnSecondaryContainer,
               ColorRole::SecondaryContainer);
    fillScheme(&tokens.vibrant, ColorRole::PrimaryContainer, ColorRole::PrimaryContainer,
               ColorRole::OnPrimaryContainer, ColorRole::OnSurface, ColorRole::SurfaceContainer);

    // Only the shape rows are overridable per scheme.
    tokens.standard.containerShape =
        shapeOverride(overrides, toolbarKeys("standard.container.shape"),
                      tokens.standard.containerShape);
    tokens.vibrant.containerShape =
        shapeOverride(overrides, toolbarKeys("vibrant.container.shape"),
                      tokens.vibrant.containerShape);

    return tokens;
}

} // namespace md
