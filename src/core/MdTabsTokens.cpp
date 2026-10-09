#include "MdTabsTokens.h"

#include "MdCompTokenParse.h"

#include <QtCore/QStringList>

namespace md {

namespace {

constexpr int stateCount = int(MdNavigationItemState::Count);

/// The disabled pair, which neither family publishes as a row of its own and
/// which Compose writes in code as the system `DisabledAlpha` over the
/// unselected colour. Reproduced here rather than left as "no colour".
void fillDisabledPair(MdNavigationColourSlot *active,
                      MdNavigationColourSlot *inactive,
                      ColorRole unselectedColor)
{
    active[int(MdNavigationItemState::Disabled)] = {unselectedColor, 0.38};
    inactive[int(MdNavigationItemState::Disabled)] = {unselectedColor, 0.38};
}

/// The primary family: `md.comp.primary-navigation-tab.*`.
///
/// Active rows are all `primary`; inactive rows are `on-surface-variant` at
/// rest and lift to `on-surface` in every interaction state — with the one
/// special case that the *inactive pressed* state layer is `primary`, the
/// colour the tab is about to earn, not the one it has.
void fillPrimaryColours(MdTabsVariantTokens *tokens)
{
    for (int i = 0; i < stateCount; ++i) {
        tokens->activeLabel[i] = {ColorRole::Primary, 1.0};
        tokens->activeIcon[i] = {ColorRole::Primary, 1.0};
        tokens->inactiveLabel[i] = {ColorRole::OnSurfaceVariant, 1.0};
        tokens->inactiveIcon[i] = {ColorRole::OnSurfaceVariant, 1.0};
        tokens->activeStateLayer[i] = {ColorRole::Primary, 1.0};
        tokens->inactiveStateLayer[i] = {ColorRole::Count, 1.0};
    }

    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused),
                  int(MdNavigationItemState::Pressed)}) {
        tokens->inactiveLabel[i] = {ColorRole::OnSurface, 1.0};
        tokens->inactiveIcon[i] = {ColorRole::OnSurface, 1.0};
        tokens->inactiveStateLayer[i] = {ColorRole::OnSurface, 1.0};
    }
    // The inactive pressed layer's colour row: `primary`, not `on-surface`.
    tokens->inactiveStateLayer[int(MdNavigationItemState::Pressed)] = {ColorRole::Primary, 1.0};

    fillDisabledPair(tokens->activeLabel, tokens->inactiveLabel, ColorRole::OnSurfaceVariant);
    fillDisabledPair(tokens->activeIcon, tokens->inactiveIcon, ColorRole::OnSurfaceVariant);
}

/// The secondary family: `md.comp.secondary-navigation-tab.*`.
///
/// One shared table: active content `on-surface`, inactive
/// `on-surface-variant`, every state layer `on-surface` on both sides. There
/// are no per-side state-layer rows to split.
void fillSecondaryColours(MdTabsVariantTokens *tokens)
{
    for (int i = 0; i < stateCount; ++i) {
        tokens->activeLabel[i] = {ColorRole::OnSurface, 1.0};
        tokens->activeIcon[i] = {ColorRole::OnSurface, 1.0};
        tokens->inactiveLabel[i] = {ColorRole::OnSurfaceVariant, 1.0};
        tokens->inactiveIcon[i] = {ColorRole::OnSurfaceVariant, 1.0};
        tokens->activeStateLayer[i] = {ColorRole::OnSurface, 1.0};
        tokens->inactiveStateLayer[i] = {ColorRole::OnSurface, 1.0};
    }

    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused),
                  int(MdNavigationItemState::Pressed)}) {
        tokens->activeLabel[i] = {ColorRole::OnSurface, 1.0};
        tokens->activeIcon[i] = {ColorRole::OnSurface, 1.0};
    }

    fillDisabledPair(tokens->activeLabel, tokens->inactiveLabel, ColorRole::OnSurfaceVariant);
    fillDisabledPair(tokens->activeIcon, tokens->inactiveIcon, ColorRole::OnSurfaceVariant);
}

} // namespace

const MdNavigationColourSlot &MdTabsVariantTokens::activeLabelFor(MdNavigationItemState state) const
{
    return activeLabel[qBound(0, int(state), stateCount - 1)];
}

const MdNavigationColourSlot &
MdTabsVariantTokens::inactiveLabelFor(MdNavigationItemState state) const
{
    return inactiveLabel[qBound(0, int(state), stateCount - 1)];
}

const MdNavigationColourSlot &MdTabsVariantTokens::activeIconFor(MdNavigationItemState state) const
{
    return activeIcon[qBound(0, int(state), stateCount - 1)];
}

const MdNavigationColourSlot &MdTabsVariantTokens::inactiveIconFor(MdNavigationItemState state) const
{
    return inactiveIcon[qBound(0, int(state), stateCount - 1)];
}

const MdNavigationColourSlot &
MdTabsVariantTokens::activeStateLayerFor(MdNavigationItemState state) const
{
    return activeStateLayer[qBound(0, int(state), stateCount - 1)];
}

const MdNavigationColourSlot &
MdTabsVariantTokens::inactiveStateLayerFor(MdNavigationItemState state) const
{
    return inactiveStateLayer[qBound(0, int(state), stateCount - 1)];
}

const MdTabsVariantTokens &MdTabsTokens::forVariant(MdTabsVariant variant) const
{
    return this->variant[qBound(0, int(variant), tabsVariantCount - 1)];
}

MdTabsTokens MdTabsTokens::resolve(const MdComponentTokens *overrides)
{
    MdTabsTokens tokens;

    using namespace comptoken;

    // --- primary: md.comp.primary-navigation-tab.* ---------------------------
    {
        MdTabsVariantTokens &v = tokens.variant[int(MdTabsVariant::Primary)];

        v.containerHeight = lengthOverride(
            overrides, plainKeys("primary-navigation-tab", "container.height"), 48.0);
        v.iconLabelTextContainerHeight = lengthOverride(
            overrides,
            plainKeys("primary-navigation-tab", "with-icon-and-label-text.container.height"), 64.0);
        v.iconSize = lengthOverride(
            overrides, plainKeys("primary-navigation-tab", "with-icon.icon.size"), 24.0);
        v.activeIndicatorHeight = lengthOverride(
            overrides, plainKeys("primary-navigation-tab", "active-indicator.height"), 3.0);
        v.dividerHeight = lengthOverride(
            overrides, plainKeys("primary-navigation-tab", "divider.height"), 1.0);
        v.containerShape = shapeOverride(
            overrides, plainKeys("primary-navigation-tab", "container.shape"), ShapeCorner::None);
        v.hoverStateLayerOpacity = lengthOverride(
            overrides, plainKeys("primary-navigation-tab", "inactive.hover.state-layer.opacity"),
            0.08);
        v.focusStateLayerOpacity = lengthOverride(
            overrides, plainKeys("primary-navigation-tab", "inactive.focus.state-layer.opacity"),
            0.12);
        v.pressedStateLayerOpacity = lengthOverride(
            overrides, plainKeys("primary-navigation-tab", "inactive.pressed.state-layer.opacity"),
            0.12);
        v.focusIndicatorOffset = lengthOverride(
            overrides, plainKeys("primary-navigation-tab", "focus.indicator.outline.offset"), 2.0);
        v.focusIndicatorThickness = lengthOverride(
            overrides, plainKeys("primary-navigation-tab", "focus.indicator.thickness"), 3.0);
        v.activeIndicatorTopRounded = true;

        fillPrimaryColours(&v);
    }

    // --- secondary: md.comp.secondary-navigation-tab.* ------------------------
    {
        MdTabsVariantTokens &v = tokens.variant[int(MdTabsVariant::Secondary)];

        // The struct's defaults are the *primary's* numbers, so the secondary
        // family's own published values are spelled out — the two families'
        // indicator heights and shapes differ, and inheriting them would
        // silently publish a 3 px rounded indicator under the secondary name.
        v.containerHeight = lengthOverride(
            overrides, plainKeys("secondary-navigation-tab", "container.height"), 48.0);
        // The secondary export publishes no icon+label height row; the tab
        // inherits the primary's 64 (Compose's two families share one
        // `TabBaselineLayout`). Carried at the primary value.
        v.iconLabelTextContainerHeight = 64.0;
        v.iconSize = lengthOverride(
            overrides, plainKeys("secondary-navigation-tab", "with-icon.icon.size"), 24.0);
        // The export's own row (2) beats Compose's fallback default parameter
        // (3, borrowed from the primary) — see the header note.
        v.activeIndicatorHeight = lengthOverride(
            overrides, plainKeys("secondary-navigation-tab", "active-indicator.height"), 2.0);
        v.dividerHeight = lengthOverride(
            overrides, plainKeys("secondary-navigation-tab", "divider.height"), 1.0);
        v.containerShape = shapeOverride(
            overrides, plainKeys("secondary-navigation-tab", "container.shape"), ShapeCorner::None);
        v.hoverStateLayerOpacity = lengthOverride(
            overrides, plainKeys("secondary-navigation-tab", "hover.state-layer.opacity"), 0.08);
        v.focusStateLayerOpacity = lengthOverride(
            overrides, plainKeys("secondary-navigation-tab", "focus.state-layer.opacity"), 0.12);
        v.pressedStateLayerOpacity = lengthOverride(
            overrides, plainKeys("secondary-navigation-tab", "pressed.state-layer.opacity"), 0.12);
        v.focusIndicatorOffset = lengthOverride(
            overrides, plainKeys("secondary-navigation-tab", "focus.indicator.outline.offset"),
            2.0);
        v.focusIndicatorThickness = lengthOverride(
            overrides, plainKeys("secondary-navigation-tab", "focus.indicator.thickness"), 3.0);
        // No shape row: a plain rectangle.
        v.activeIndicatorTopRounded = false;

        fillSecondaryColours(&v);
    }

    return tokens;
}

} // namespace md
