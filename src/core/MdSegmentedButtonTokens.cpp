#include "MdSegmentedButtonTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

using namespace comptoken;

} // namespace

MdSegmentedButtonTokens MdSegmentedButtonTokens::resolve(const MdComponentTokens *overrides)
{
    MdSegmentedButtonTokens tokens;

    // --- metrics: md.comp.outlined-segmented-button.<token> ---------------
    tokens.containerHeight = lengthOverride(
        overrides, plainKeys("outlined-segmented-button", "container.height"), 40.0);
    tokens.outlineWidth = lengthOverride(
        overrides, plainKeys("outlined-segmented-button", "outline.width"), 1.0);
    tokens.iconSize = lengthOverride(
        overrides, plainKeys("outlined-segmented-button", "with-icon.icon-size"), 18.0);

    // [compose] rows the export does not publish.
    tokens.iconSpacing = lengthOverride(
        overrides, plainKeys("outlined-segmented-button", "icon.spacing"), 8.0);
    tokens.contentPadding = lengthOverride(
        overrides, plainKeys("outlined-segmented-button", "leading.space"), 12.0);

    // --- disabled opacities -------------------------------------------------
    tokens.disabledLabelOpacity = lengthOverride(
        overrides, plainKeys("outlined-segmented-button", "disabled.label-text.opacity"), 0.38);
    tokens.disabledIconOpacity = lengthOverride(
        overrides, plainKeys("outlined-segmented-button", "disabled.icon.opacity"), 0.38);
    tokens.disabledOutlineOpacity = lengthOverride(
        overrides, plainKeys("outlined-segmented-button", "disabled.outline.opacity"), 0.12);

    // --- focus indicator ------------------------------------------------------
    tokens.focusIndicatorThickness = lengthOverride(
        overrides,
        plainKeys("outlined-segmented-button", "focus-indicator.thickness"), 3.0);
    tokens.focusIndicatorOffset = lengthOverride(
        overrides,
        plainKeys("outlined-segmented-button", "focus-indicator.outline.offset"), 2.0);

    // --- colours --------------------------------------------------------------
    // Unselected [export]: on-surface label/icon/state-layer, outline stroke,
    // no container row at all.
    tokens.unselected = MdSegmentedButtonRow{
        ColorRole::OnSurface, ColorRole::OnSurface, ColorRole::OnSurface,
        ColorRole::Count,     ColorRole::Outline,
    };
    // Selected [export]: secondary-container container, on-secondary-container
    // content — the same row for every interactive state.
    tokens.selected = MdSegmentedButtonRow{
        ColorRole::OnSecondaryContainer, ColorRole::OnSecondaryContainer,
        ColorRole::OnSecondaryContainer, ColorRole::SecondaryContainer,
        ColorRole::Outline,
    };

    // Disabled colours [export]: on-surface everything (the opacities above
    // fold into alpha at paint time).
    tokens.disabledContent = ColorRole::OnSurface;
    tokens.disabledOutline = ColorRole::OnSurface;

    return tokens;
}

} // namespace md
