#include "MdNavigationRailTokens.h"

#include "MdCompTokenParse.h"

#include <QtCore/QStringList>

namespace md {

const MdNavigationRailVariantTokens &
MdNavigationRailTokens::forVariant(MdNavigationRailVariant variant) const
{
    return this->variant[qBound(0, int(variant), navRailVariantCount - 1)];
}

namespace {

/// The baseline rail: `md.comp.navigation-rail.*`.
///
/// The container rows come from the export; the spacing rows do not exist in
/// it, and Compose's `NavigationRail.kt` hard-codes all three (4 between
/// items, 4 of item vertical padding, 8 after the header) — those literals are
/// carried here as the behaviour's numbers. The no-label indicator's 56 px
/// height is published (`no-label-active-indicator-height`) and is what makes
/// a label-less rail item's pill a 56 x 56 square.
void fillBaselineRail(MdNavigationRailVariantTokens *rail, const MdComponentTokens *overrides)
{
    using namespace comptoken;

    MdNavigationRailVariantTokens &v = *rail;
    MdNavigationBarVariantTokens &item = v.item;

    v.containerWidth = lengthOverride(overrides, plainKeys("navigation-rail", "container.width"),
                                      80.0);
    // `NavigationRailCollapsedTokens.NarrowContainerWidth` — the collapsed
    // family's row, read by the baseline behaviour across files. The baseline
    // family publishes no narrow row of its own.
    v.narrowContainerWidth = 80.0;
    v.containerColor = ColorRole::Surface;
    v.containerElevation = ElevationLevel::Level0;
    v.containerShape = ShapeCorner::None;

    // Compose's `NavigationRail.kt` hard-codes these three; the baseline
    // family publishes no spacing rows.
    v.containerVerticalPadding = 4.0;
    v.itemVerticalSpace = 4.0;
    v.headerSpace = 8.0;

    // The item rows. `no-label-active-indicator-height` is the one row the bar
    // has no counterpart for: a label-less pill is 56 x 56, not 56 x 32.
    item.activeIndicatorWidth = lengthOverride(
        overrides, plainKeys("navigation-rail", "active-indicator.width"), 56.0);
    item.activeIndicatorHeight = lengthOverride(
        overrides, plainKeys("navigation-rail", "active-indicator.height"), 32.0);
    item.iconSize =
        lengthOverride(overrides, plainKeys("navigation-rail", "icon.size"), 24.0);
    item.indicatorShape = shapeOverride(
        overrides, plainKeys("navigation-rail", "active-indicator.shape"), ShapeCorner::Full);
    // The no-label height is carried on the shared item's tokens; 0 would mean
    // "fall back to `activeIndicatorHeight`".
    item.noLabelIndicatorHeight = lengthOverride(
        overrides, plainKeys("navigation-rail", "no-label-active-indicator-height"), 56.0);
    // Compose's `NavigationRailItemVerticalPadding` (4, hard-coded): the item's
    // own vertical padding, the slot the bar's flexible family names
    // `container.between-space`.
    item.containerBetweenSpace = 4.0;
    item.labelTextType = TypeStyle::LabelMedium;
    item.hoverStateLayerOpacity = lengthOverride(
        overrides, plainKeys("navigation-rail", "hover-state-layer-opacity"), 0.08);
    item.focusStateLayerOpacity = lengthOverride(
        overrides, plainKeys("navigation-rail", "focus-state-layer-opacity"), 0.12);
    item.pressedStateLayerOpacity = lengthOverride(
        overrides, plainKeys("navigation-rail", "pressed-state-layer-opacity"), 0.12);

    fillNavigationItemColours(&item, MdNavigationBarVariant::Baseline);
}

/// The flexible rail: `nav-rail-collapsed` / `nav-rail-expanded` / `nav-rail` /
/// `nav-rail-item*`.
void fillFlexibleRail(MdNavigationRailVariantTokens *rail, const MdComponentTokens *overrides)
{
    using namespace comptoken;

    MdNavigationRailVariantTokens &v = *rail;
    MdNavigationBarVariantTokens &item = v.item;

    // --- container ----------------------------------------------------------
    v.containerWidth = lengthOverride(
        overrides, plainKeys("nav-rail-collapsed", "container.width"), 96.0);
    // Published (`narrow-container-width: 80px`) and read by nothing in
    // Compose — carried, as with every row of its kind.
    v.narrowContainerWidth = lengthOverride(
        overrides, plainKeys("nav-rail-collapsed", "narrow-container-width"), 80.0);
    v.expandedWidthMinimum = lengthOverride(
        overrides, plainKeys("nav-rail-expanded", "container-width-minimum"), 220.0);
    v.expandedWidthMaximum = lengthOverride(
        overrides, plainKeys("nav-rail-expanded", "container-width-maximum"), 360.0);
    // Compose's `WNRTopPadding` — both the collapsed and the expanded file
    // publish 44.
    v.containerTopSpace = lengthOverride(
        overrides, plainKeys("nav-rail-collapsed", "top-space"), 44.0);
    // Collapsed 4, expanded 0 — the layout picks by state.
    v.itemVerticalSpace = lengthOverride(
        overrides, plainKeys("nav-rail-collapsed", "item-vertical-space"), 4.0);
    v.expandedBetweenItemSpace = lengthOverride(
        overrides, plainKeys("nav-rail-expanded", "between-item-space"), 0.0);
    // The wide rail's header gap is the *baseline item* family's
    // `header-space-minimum` (40) — Compose's `WNRHeaderPadding` reads across
    // files. Recorded rather than silently picked.
    v.headerSpace = lengthOverride(
        overrides, plainKeys("nav-rail-item", "header-space-minimum"), 40.0);
    // Compose's `WNRItemHorizontalPadding` (20, hard-coded); numerically the
    // expanded family's `vertical-trailing-space`, from a different source.
    v.expandedItemPadding = lengthOverride(
        overrides, plainKeys("nav-rail-expanded", "vertical-trailing-space"), 20.0);

    v.containerColor = ColorRole::Surface;
    v.containerElevation = ElevationLevel::Level0;
    v.containerShape = ShapeCorner::None;
    // The modal rows — the drawer's replacement.
    v.modalContainerColor = ColorRole::SurfaceContainer;
    v.modalContainerElevation = ElevationLevel::Level2;
    v.modalContainerShape = ShapeCorner::Large;

    // --- item ---------------------------------------------------------------
    // Shared rows (`nav-rail-item.*`).
    item.iconSize = lengthOverride(overrides, plainKeys("nav-rail-item", "icon-size"), 24.0);
    // Compose's `TopIconItemMinHeight` is the *baseline* item family's
    // `ContainerHeight` (64) — the item's own height in the collapsed state is
    // the bar's item arithmetic plus this family's 6 px item padding.
    item.containerBetweenSpace = lengthOverride(
        overrides, plainKeys("nav-rail-item", "container-vertical-space"), 6.0);
    item.indicatorShape = shapeOverride(
        overrides, plainKeys("nav-rail-item", "active-indicator-shape"), ShapeCorner::Full);
    item.containerShape = ShapeCorner::None;

    // `Top` position — `nav-rail-item-vertical.*`.
    item.activeIndicatorHeight = lengthOverride(
        overrides, plainKeys("nav-rail-item-vertical", "active-indicator-height"), 32.0);
    item.activeIndicatorWidth = lengthOverride(
        overrides, plainKeys("nav-rail-item-vertical", "active-indicator-width"), 56.0);
    item.indicatorIconLabelSpace = lengthOverride(
        overrides, plainKeys("nav-rail-item-vertical", "icon-label-space"), 4.0);
    item.horizontalIndicatorLeadingSpace = lengthOverride(
        overrides, plainKeys("nav-rail-item-vertical", "leading-space"), 16.0);
    item.horizontalIndicatorTrailingSpace = lengthOverride(
        overrides, plainKeys("nav-rail-item-vertical", "trailing-space"), 16.0);
    item.labelTextType = TypeStyle::LabelMedium;

    // `Start` position — `nav-rail-item-horizontal.*`. The expanded item's
    // pill is 56 tall, its icon-label gap 8 where the vertical item's is 4,
    // and its label is `label-large`.
    item.horizontalIndicatorHeight = lengthOverride(
        overrides, plainKeys("nav-rail-item-horizontal", "active-indicator-height"), 56.0);
    item.horizontalIndicatorLeadingSpace = lengthOverride(
        overrides, plainKeys("nav-rail-item-horizontal", "full-width-leading-space"), 16.0);
    item.horizontalIndicatorTrailingSpace = lengthOverride(
        overrides, plainKeys("nav-rail-item-horizontal", "full-width-trailing-space"), 16.0);
    item.horizontalIconLabelSpace = lengthOverride(
        overrides, plainKeys("nav-rail-item-horizontal", "icon-label-space"), 8.0);
    item.horizontalLabelTextType = TypeStyle::LabelLarge;

    item.hoverStateLayerOpacity = lengthOverride(
        overrides, plainKeys("nav-rail", "item-active-hovered-state-layer-opacity"), 0.08);
    item.focusStateLayerOpacity = lengthOverride(
        overrides, plainKeys("nav-rail", "item-active-focused-state-layer-opacity"), 0.12);
    item.pressedStateLayerOpacity = lengthOverride(
        overrides, plainKeys("nav-rail", "item-active-pressed-state-layer-opacity"), 0.12);

    fillNavigationItemColours(&item, MdNavigationBarVariant::Flexible);
}

} // namespace

MdNavigationRailTokens MdNavigationRailTokens::resolve(const MdComponentTokens *overrides)
{
    MdNavigationRailTokens tokens;

    fillBaselineRail(&tokens.variant[int(MdNavigationRailVariant::Baseline)], overrides);
    fillFlexibleRail(&tokens.variant[int(MdNavigationRailVariant::Flexible)], overrides);

    return tokens;
}

} // namespace md
