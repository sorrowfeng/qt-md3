#include "MdNavigationBarTokens.h"

#include "MdCompTokenParse.h"

#include <QtCore/QStringList>

namespace md {

const MdNavigationColourSlot &MdNavigationItemColours::iconFor(MdNavigationItemState state) const
{
    return icon[qBound(0, int(state), navItemStateCount - 1)];
}

const MdNavigationColourSlot &MdNavigationItemColours::labelFor(MdNavigationItemState state) const
{
    return label[qBound(0, int(state), navItemStateCount - 1)];
}

const MdNavigationColourSlot &MdNavigationItemColours::stateLayerFor(MdNavigationItemState state) const
{
    return stateLayer[qBound(0, int(state), navItemStateCount - 1)];
}

const MdNavigationItemColours &MdNavigationBarVariantTokens::coloursFor(bool selected) const
{
    return selected ? this->selected : unselected;
}

qreal MdNavigationBarVariantTokens::indicatorHorizontalPadding() const
{
    return (activeIndicatorWidth - iconSize) / 2.0;
}

qreal MdNavigationBarVariantTokens::indicatorVerticalPadding() const
{
    return (activeIndicatorHeight - iconSize) / 2.0;
}

qreal MdNavigationBarVariantTokens::horizontalIndicatorVerticalPadding() const
{
    return (horizontalIndicatorHeight - iconSize) / 2.0;
}

const MdNavigationBarVariantTokens &
MdNavigationBarTokens::forVariant(MdNavigationBarVariant variant) const
{
    return this->variant[qBound(0, int(variant), navVariantCount - 1)];
}

namespace {

void fillBaselineColours(MdNavigationBarVariantTokens *tokens);
void fillFlexibleColours(MdNavigationBarVariantTokens *tokens);

/// The disabled pair, which **neither** family publishes as a row of its own.
///
/// Compose writes the rule in code: `disabledIconColor =
/// unselectedIconColor.copy(alpha = DisabledAlpha)` and the same for the label,
/// with `DisabledAlpha` the system 0.38. That is what the export's silence
/// means, so it is reproduced here rather than left as "no colour".
void fillDisabledPair(MdNavigationItemColours *colours,
                      ColorRole unselectedIcon,
                      ColorRole unselectedLabel,
                      qreal disabledAlpha)
{
    colours->icon[int(MdNavigationItemState::Disabled)] = {unselectedIcon, disabledAlpha};
    colours->label[int(MdNavigationItemState::Disabled)] = {unselectedLabel, disabledAlpha};
}

} // namespace

void fillNavigationItemColours(MdNavigationBarVariantTokens *tokens, MdNavigationBarVariant variant)
{
    if (variant == MdNavigationBarVariant::Flexible) {
        fillFlexibleColours(tokens);
    } else {
        fillBaselineColours(tokens);
    }
}

namespace {

/// The baseline family: `md.comp.navigation-bar.*`.
///
/// Every state that has a row in the export gets it; the states the export
/// leaves out keep their `Enabled` value, which is what the SCSS variables
/// resolve to as well.
void fillBaselineColours(MdNavigationBarVariantTokens *tokens)
{
    const ColorRole activeIcon = ColorRole::OnSecondaryContainer;
    const ColorRole activeLabel = ColorRole::OnSurface;
    const ColorRole inactiveIcon = ColorRole::OnSurfaceVariant;
    const ColorRole inactiveLabel = ColorRole::OnSurfaceVariant;
    const ColorRole stateLayer = ColorRole::OnSurface;

    // --- selected ------------------------------------------------------------
    MdNavigationItemColours &sel = tokens->selected;
    for (int i = 0; i < navItemStateCount; ++i) {
        sel.icon[i] = {activeIcon, 1.0};
        sel.label[i] = {activeLabel, 1.0};
    }
    sel.stateLayer[int(MdNavigationItemState::Hovered)] = {stateLayer, 1.0};
    sel.stateLayer[int(MdNavigationItemState::Focused)] = {stateLayer, 1.0};
    sel.stateLayer[int(MdNavigationItemState::Pressed)] = {stateLayer, 1.0};
    sel.indicator = {ColorRole::SecondaryContainer, 1.0};

    // --- unselected ----------------------------------------------------------
    MdNavigationItemColours &uns = tokens->unselected;
    for (int i = 0; i < navItemStateCount; ++i) {
        uns.icon[i] = {inactiveIcon, 1.0};
        uns.label[i] = {inactiveLabel, 1.0};
    }
    // At rest there is no state layer; in a state the icon and the label both
    // lift to `on-surface` and the layer is `on-surface` too.
    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused),
                  int(MdNavigationItemState::Pressed)}) {
        uns.icon[i] = {ColorRole::OnSurface, 1.0};
        uns.label[i] = {ColorRole::OnSurface, 1.0};
        uns.stateLayer[i] = {stateLayer, 1.0};
    }

    fillDisabledPair(&sel, inactiveIcon, inactiveLabel, 0.38);
    fillDisabledPair(&uns, inactiveIcon, inactiveLabel, 0.38);
}

/// The flexible family: `md.comp.nav-bar.*`.
///
/// This family publishes **no per-state icon or label rows at all** — only
/// `item.active.icon.color`, `item.active.label-text.color` and the three state
/// layers — so every state carries the enabled value and the only colour that
/// moves is the state layer's. Recorded rather than invented.
void fillFlexibleColours(MdNavigationBarVariantTokens *tokens)
{
    const ColorRole activeIcon = ColorRole::OnSecondaryContainer;
    const ColorRole activeLabel = ColorRole::Secondary;
    const ColorRole inactiveIcon = ColorRole::OnSurfaceVariant;
    const ColorRole inactiveLabel = ColorRole::OnSurfaceVariant;
    // The flexible family's state layers are `on-secondary-container` on *both*
    // sides of the selection, because the layer rides the pill and the pill is
    // `secondary-container` whether or not the item is selected.
    const ColorRole stateLayer = ColorRole::OnSecondaryContainer;

    MdNavigationItemColours &sel = tokens->selected;
    for (int i = 0; i < navItemStateCount; ++i) {
        sel.icon[i] = {activeIcon, 1.0};
        sel.label[i] = {activeLabel, 1.0};
    }
    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused),
                  int(MdNavigationItemState::Pressed)}) {
        sel.stateLayer[i] = {stateLayer, 1.0};
    }
    sel.indicator = {ColorRole::SecondaryContainer, 1.0};

    MdNavigationItemColours &uns = tokens->unselected;
    for (int i = 0; i < navItemStateCount; ++i) {
        uns.icon[i] = {inactiveIcon, 1.0};
        uns.label[i] = {inactiveLabel, 1.0};
    }
    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused),
                  int(MdNavigationItemState::Pressed)}) {
        uns.stateLayer[i] = {stateLayer, 1.0};
    }

    fillDisabledPair(&sel, inactiveIcon, inactiveLabel, 0.38);
    fillDisabledPair(&uns, inactiveIcon, inactiveLabel, 0.38);

    // The `Start` position puts the label inside the pill, so it is painted in
    // the pill's own content colour rather than in the `Top` table's
    // `secondary`. `nav-bar-item-horizontal.active-label-text.color`.
    tokens->labelColorStart = activeIcon;
}

} // namespace

MdNavigationBarTokens MdNavigationBarTokens::resolve(const MdComponentTokens *overrides)
{
    MdNavigationBarTokens tokens;

    using namespace comptoken;

    // --- baseline: md.comp.navigation-bar.* ---------------------------------
    {
        MdNavigationBarVariantTokens &v = tokens.variant[int(MdNavigationBarVariant::Baseline)];
        v.containerHeight = lengthOverride(overrides, plainKeys("navigation-bar", "container.height"),
                                           v.containerHeight);
        v.activeIndicatorHeight =
            lengthOverride(overrides, plainKeys("navigation-bar", "active-indicator.height"),
                           v.activeIndicatorHeight);
        v.activeIndicatorWidth =
            lengthOverride(overrides, plainKeys("navigation-bar", "active-indicator.width"),
                           v.activeIndicatorWidth);
        v.iconSize =
            lengthOverride(overrides, plainKeys("navigation-bar", "icon.size"), v.iconSize);
        v.containerShape = shapeOverride(
            overrides, plainKeys("navigation-bar", "container.shape"), v.containerShape);
        v.indicatorShape = shapeOverride(
            overrides, plainKeys("navigation-bar", "active-indicator.shape"), v.indicatorShape);
        v.hoverStateLayerOpacity =
            lengthOverride(overrides, plainKeys("navigation-bar", "hover-state-layer-opacity"),
                           v.hoverStateLayerOpacity);
        v.focusStateLayerOpacity =
            lengthOverride(overrides, plainKeys("navigation-bar", "focus-state-layer-opacity"),
                           v.focusStateLayerOpacity);
        v.pressedStateLayerOpacity =
            lengthOverride(overrides, plainKeys("navigation-bar", "pressed-state-layer-opacity"),
                           v.pressedStateLayerOpacity);
        v.focusIndicatorOffset =
            lengthOverride(overrides, plainKeys("navigation-bar", "focus-indicator-outline-offset"),
                           v.focusIndicatorOffset);
        v.focusIndicatorThickness = lengthOverride(
            overrides, plainKeys("navigation-bar", "focus-indicator-thickness"),
            v.focusIndicatorThickness);

        // The baseline publishes no item rows of its own — no icon-label space,
        // no item padding, no horizontal item. Zero padding and no item-level
        // gap is what `NavigationBarItem` does with `weight(1f)` items; the
        // 8 px between items is the *container's* arrangement, not the item's,
        // and lives in `MdNavigationBar` as `itemBetweenSpace`.
        v.containerBetweenSpace = 0.0;

        fillBaselineColours(&v);
    }

    // --- flexible: md.comp.nav-bar.* + md.comp.nav-bar-item-* ---------------
    {
        MdNavigationBarVariantTokens &v = tokens.variant[int(MdNavigationBarVariant::Flexible)];

        // The struct's defaults are the *baseline's* numbers, so the flexible
        // family's own published values are spelled out here rather than
        // inherited — inheriting them would silently resolve the short family
        // to 80 px, a 64 px pill and no item padding whenever no override is
        // set, which is exactly the confusion the two families are about.
        v.containerHeight = lengthOverride(overrides, plainKeys("nav-bar", "container.height"),
                                           64.0);
        v.iconSize = lengthOverride(overrides, plainKeys("nav-bar", "item.icon.size"), 24.0);
        v.containerShape = shapeOverride(overrides, plainKeys("nav-bar", "container.shape"),
                                         ShapeCorner::None);
        v.indicatorShape = shapeOverride(
            overrides, plainKeys("nav-bar", "item.active-indicator.shape"), ShapeCorner::Full);
        v.hoverStateLayerOpacity =
            lengthOverride(overrides, plainKeys("nav-bar", "item.active.hovered.state-layer.opacity"),
                           0.08);
        v.focusStateLayerOpacity =
            lengthOverride(overrides, plainKeys("nav-bar", "item.active.focused.state-layer.opacity"),
                           0.12);
        v.pressedStateLayerOpacity = lengthOverride(
            overrides, plainKeys("nav-bar", "item.active.pressed.state-layer.opacity"), 0.12);

        // Top position — md.comp.nav-bar-item-vertical.*
        v.activeIndicatorHeight = lengthOverride(
            overrides, plainKeys("nav-bar-item-vertical", "active-indicator.height"), 32.0);
        v.activeIndicatorWidth = lengthOverride(
            overrides, plainKeys("nav-bar-item-vertical", "active-indicator.width"), 56.0);
        v.containerBetweenSpace = lengthOverride(
            overrides, plainKeys("nav-bar-item-vertical", "container.between-space"), 6.0);

        // Indicator-to-label. Both positions read 4 and both files publish it,
        // so either row may carry the override; the vertical one is asked first
        // because it is the position the baseline also has.
        v.indicatorIconLabelSpace = lengthOverride(
            overrides,
            {plainKeys("nav-bar-item-vertical", "active-indicator.icon-label-space").first(),
             plainKeys("nav-bar-item-horizontal", "active-indicator.icon-label-space").first(),
             plainKeys("nav-bar", "item.active-indicator.icon-label-space").first()},
            4.0);

        // Start position — md.comp.nav-bar-item-horizontal.*
        v.horizontalIndicatorHeight = lengthOverride(
            overrides, plainKeys("nav-bar-item-horizontal", "active-indicator.height"), 40.0);
        v.horizontalIndicatorLeadingSpace = lengthOverride(
            overrides, plainKeys("nav-bar-item-horizontal", "active-indicator.leading-space"),
            16.0);
        v.horizontalIndicatorTrailingSpace = lengthOverride(
            overrides, plainKeys("nav-bar-item-horizontal", "active-indicator.trailing-space"),
            16.0);

        fillFlexibleColours(&v);

        // The flexible family publishes no focus-indicator rows, so its ring
        // values are the baseline's — the ring is a system token, and the
        // variant struct carrying them twice is cheaper than a second lookup
        // from the paint path. See the header.
        v.focusIndicatorColor = tokens.variant[int(MdNavigationBarVariant::Baseline)]
                                    .focusIndicatorColor;
    }

    return tokens;
}

} // namespace md
