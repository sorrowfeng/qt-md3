#include "MdNavigationDrawerTokens.h"

#include "MdCompTokenParse.h"

#include <QtCore/QStringList>

namespace md {

const MdNavigationItemColours &MdNavigationDrawerItemTokens::coloursFor(bool selected) const
{
    return selected ? this->selected : this->unselected;
}

const MdNavigationDrawerVariantTokens &
MdNavigationDrawerTokens::forVariant(MdNavigationDrawerVariant variant) const
{
    return this->variant[qBound(0, int(variant), navDrawerVariantCount - 1)];
}

namespace {

/// The disabled pair, which the drawer — like the bar and the rail — does not
/// publish as a row of its own: the unselected enabled colours at the system's
/// 0.38, the library-wide rule.
void fillDisabledPair(MdNavigationItemColours *colours,
                      ColorRole unselectedIcon,
                      ColorRole unselectedLabel)
{
    colours->icon[int(MdNavigationItemState::Disabled)] = {unselectedIcon, 0.38};
    colours->label[int(MdNavigationItemState::Disabled)] = {unselectedLabel, 0.38};
}

/// The drawer's own colour table — `navigation-drawer.scss`'s rows, which
/// disagree with the bar's in three places the header records.
void fillItemColours(MdNavigationDrawerItemTokens *item)
{
    // --- selected: every row reads `on-secondary-container` -------------------
    // `active-*` / `active-hover-*` / `active-focus-*` / `active-pressed-*`:
    // icon, label and state layer all agree, in all four interactive states.
    MdNavigationItemColours &sel = item->selected;
    for (int i = 0; i < navItemStateCount; ++i) {
        sel.icon[i] = {ColorRole::OnSecondaryContainer, 1.0};
        sel.label[i] = {ColorRole::OnSecondaryContainer, 1.0};
        sel.stateLayer[i] = {ColorRole::OnSecondaryContainer, 1.0};
    }
    sel.indicator = {ColorRole::SecondaryContainer, 1.0};
    fillDisabledPair(&sel, ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer);

    // --- unselected ------------------------------------------------------------
    // `inactive-*`: enabled content is `on-surface-variant`; hover and focus
    // lift icon, label and state layer to `on-surface`. The pressed state
    // layer is the table's special case — `on-secondary-container`, the
    // *active* content colour — while the pressed icon and label read
    // `on-surface` like the other interactive states. Enabled publishes no
    // state-layer row: nothing is overlaid at rest.
    MdNavigationItemColours &uns = item->unselected;
    const int enabled = int(MdNavigationItemState::Enabled);
    const int hovered = int(MdNavigationItemState::Hovered);
    const int focused = int(MdNavigationItemState::Focused);
    const int pressed = int(MdNavigationItemState::Pressed);

    uns.icon[enabled] = {ColorRole::OnSurfaceVariant, 1.0};
    uns.label[enabled] = {ColorRole::OnSurfaceVariant, 1.0};

    for (const int state : {hovered, focused, pressed}) {
        uns.icon[state] = {ColorRole::OnSurface, 1.0};
        uns.label[state] = {ColorRole::OnSurface, 1.0};
    }
    uns.stateLayer[hovered] = {ColorRole::OnSurface, 1.0};
    uns.stateLayer[focused] = {ColorRole::OnSurface, 1.0};
    // `inactive-pressed-state-layer-color: on-secondary-container`.
    uns.stateLayer[pressed] = {ColorRole::OnSecondaryContainer, 1.0};

    fillDisabledPair(&uns, ColorRole::OnSurfaceVariant, ColorRole::OnSurfaceVariant);
}

/// The one family, both container row groups: everything except the container
/// colour / elevation is shared.
void fillVariant(MdNavigationDrawerVariantTokens *v, const MdComponentTokens *overrides)
{
    using namespace comptoken;

    // --- container ------------------------------------------------------------
    v->containerWidth = lengthOverride(
        overrides, plainKeys("navigation-drawer", "container.width"), 360.0);
    v->containerShape = shapeOverride(
        overrides, plainKeys("navigation-drawer", "container.shape"), ShapeCorner::Large);

    // --- the scrim, carried ------------------------------------------------------
    v->scrimOpacity = lengthOverride(
        overrides, plainKeys("navigation-drawer", "scrim.opacity"), 0.4);

    // --- headline and divider ------------------------------------------------------
    v->headlineType = TypeStyle::TitleSmall;

    // --- item -----------------------------------------------------------------
    MdNavigationDrawerItemTokens &item = v->item;
    item.activeIndicatorHeight = lengthOverride(
        overrides, plainKeys("navigation-drawer", "active-indicator.height"), 56.0);
    item.activeIndicatorWidth = lengthOverride(
        overrides, plainKeys("navigation-drawer", "active-indicator.width"), 336.0);
    item.iconSize =
        lengthOverride(overrides, plainKeys("navigation-drawer", "icon.size"), 24.0);
    item.indicatorShape = shapeOverride(
        overrides, plainKeys("navigation-drawer", "active-indicator.shape"), ShapeCorner::Full);

    item.hoverStateLayerOpacity = lengthOverride(
        overrides, plainKeys("navigation-drawer", "hover.state-layer.opacity"), 0.08);
    item.focusStateLayerOpacity = lengthOverride(
        overrides, plainKeys("navigation-drawer", "focus.state-layer.opacity"), 0.12);
    item.pressedStateLayerOpacity = lengthOverride(
        overrides, plainKeys("navigation-drawer", "pressed.state-layer.opacity"), 0.12);

    item.focusIndicatorColor = ColorRole::Secondary;
    item.focusIndicatorOffset = lengthOverride(
        overrides, plainKeys("navigation-drawer", "focus-indicator.outline-offset"), 2.0);
    item.focusIndicatorThickness = lengthOverride(
        overrides, plainKeys("navigation-drawer", "focus-indicator.thickness"), 3.0);

    item.labelTextType = TypeStyle::LabelLarge;

    fillItemColours(&item);
}

} // namespace

MdNavigationDrawerTokens MdNavigationDrawerTokens::resolve(const MdComponentTokens *overrides)
{
    MdNavigationDrawerTokens tokens;

    fillVariant(&tokens.variant[int(MdNavigationDrawerVariant::Modal)], overrides);
    fillVariant(&tokens.variant[int(MdNavigationDrawerVariant::Standard)], overrides);

    // The only rows the two variants disagree on — one file's two container
    // row groups.
    tokens.variant[int(MdNavigationDrawerVariant::Modal)].containerColor =
        ColorRole::SurfaceContainerLow;
    tokens.variant[int(MdNavigationDrawerVariant::Modal)].containerElevation =
        ElevationLevel::Level1;
    tokens.variant[int(MdNavigationDrawerVariant::Standard)].containerColor = ColorRole::Surface;
    tokens.variant[int(MdNavigationDrawerVariant::Standard)].containerElevation =
        ElevationLevel::Level0;

    return tokens;
}

} // namespace md
