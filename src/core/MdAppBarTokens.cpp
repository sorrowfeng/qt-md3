#include "MdAppBarTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

using namespace comptoken;

/// The published name of a variant's row group. These are the exact
/// `<size>` segments the export uses in `md.comp.app-bar.<size>.*`.
const char *sizeName(MdAppBarVariant variant)
{
    switch (variant) {
    case MdAppBarVariant::Small:
        return "small";
    case MdAppBarVariant::Medium:
        return "medium";
    case MdAppBarVariant::Large:
        return "large";
    case MdAppBarVariant::MediumFlexible:
        return "medium-flexible";
    case MdAppBarVariant::LargeFlexible:
        return "large-flexible";
    case MdAppBarVariant::Count:
        break;
    }
    return "small";
}

/// `md.comp.app-bar.<token>` for the common set.
QStringList commonKeys(const char *token)
{
    return plainKeys("app-bar", token);
}

/// `md.comp.app-bar.<size>.<token>`, falling back to the common set.
QStringList appBarKeys(MdAppBarVariant variant, const char *token)
{
    return sizeKeys("app-bar", sizeName(variant), token);
}

/// `md.comp.app-bar.search.<token>`.
QStringList searchKeys(const char *token)
{
    return {QStringLiteral("md.comp.app-bar.search.%1").arg(QLatin1String(token))};
}

} // namespace

// ---------------------------------------------------------------------------
// MdAppBarTokens
// ---------------------------------------------------------------------------

qreal MdAppBarTokens::titleBottomPadding() const
{
    switch (variant) {
    case MdAppBarVariant::Small:
        return 0.0;
    case MdAppBarVariant::Medium:
    case MdAppBarVariant::MediumFlexible:
        return mediumTitleBottomPadding;
    case MdAppBarVariant::Large:
    case MdAppBarVariant::LargeFlexible:
        return largeTitleBottomPadding;
    case MdAppBarVariant::Count:
        break;
    }
    return 0.0;
}

qreal MdAppBarTokens::expandedHeight(bool hasSubtitle) const
{
    if (hasSubtitle && withSubtitleContainerHeight > 0.0) {
        return withSubtitleContainerHeight;
    }
    return containerHeight;
}

qreal MdAppBarTokens::heightFor(qreal collapsedFraction, bool hasSubtitle) const
{
    const qreal expanded = expandedHeight(hasSubtitle);

    // A single-row bar has nothing to collapse: Compose applies the height
    // offset limit to a one-row layout whose whole height is the container
    // height, so a "fully collapsed" small bar is 0 px tall. A two-row bar
    // only ever loses its *second* row — `adjustHeightOffsetLimit` runs on the
    // text row alone — which is what keeps the icon row visible.
    if (!isTwoRows()) {
        return expanded * (1.0 - collapsedFraction);
    }
    const qreal secondRow = expanded - collapsedRowHeight;
    return expanded - secondRow * collapsedFraction;
}

MdAppBarTokens MdAppBarTokens::resolve(MdAppBarVariant variant, const MdComponentTokens *overrides)
{
    MdAppBarTokens tokens;
    tokens.variant = variant;

    // --- 1. the size set ----------------------------------------------------
    switch (variant) {
    case MdAppBarVariant::Small:
        tokens.containerHeight = 64.0;
        tokens.withSubtitleContainerHeight = 0.0;
        tokens.titleTypeStyle = TypeStyle::TitleLarge;
        tokens.subtitleTypeStyle = TypeStyle::LabelMedium;
        break;
    case MdAppBarVariant::Medium:
        tokens.containerHeight = 112.0;
        tokens.withSubtitleContainerHeight = 0.0;
        tokens.titleTypeStyle = TypeStyle::HeadlineSmall;
        // Deprecated upstream ("No subtitle support on the legacy app bar"),
        // but published, so it is carried at its published value.
        tokens.subtitleTypeStyle = TypeStyle::LabelLarge;
        break;
    case MdAppBarVariant::Large:
        tokens.containerHeight = 152.0;
        tokens.withSubtitleContainerHeight = 0.0;
        tokens.titleTypeStyle = TypeStyle::HeadlineMedium;
        tokens.subtitleTypeStyle = TypeStyle::TitleMedium;
        break;
    case MdAppBarVariant::MediumFlexible:
        tokens.containerHeight = 112.0;
        tokens.withSubtitleContainerHeight = 136.0;
        tokens.titleTypeStyle = TypeStyle::HeadlineMedium;
        tokens.subtitleTypeStyle = TypeStyle::LabelLarge;
        break;
    case MdAppBarVariant::LargeFlexible:
        tokens.containerHeight = 120.0;
        tokens.withSubtitleContainerHeight = 152.0;
        tokens.titleTypeStyle = TypeStyle::DisplaySmall;
        tokens.subtitleTypeStyle = TypeStyle::TitleMedium;
        break;
    case MdAppBarVariant::Count:
        break;
    }

    tokens.containerHeight =
        lengthOverride(overrides, appBarKeys(variant, "container.height"), tokens.containerHeight);
    if (tokens.withSubtitleContainerHeight > 0.0) {
        tokens.withSubtitleContainerHeight =
            lengthOverride(overrides, appBarKeys(variant, "with-subtitle.container.height"),
                           tokens.withSubtitleContainerHeight);
    }

    // The collapsed row is the *small* size group's container height for every
    // variant — Compose reads `AppBarSmallTokens.ContainerHeight` in
    // `MediumAppBarCollapsedHeight` and `LargeAppBarCollapsedHeight` alike.
    tokens.collapsedRowHeight = lengthOverride(
        overrides, sizeKeys("app-bar", "small", "container.height"), tokens.collapsedRowHeight);

    // --- 2. the search configuration (small publishes it; the rows are
    //        meaningful for every size because the common set owns the
    //        spacings and colours) ------------------------------------------
    tokens.searchContainerHeight =
        lengthOverride(overrides, appBarKeys(variant, "search.container.height"),
                       tokens.searchContainerHeight);
    tokens.searchContainerShape = shapeOverride(overrides,
                                                appBarKeys(variant, "search.container.shape"),
                                                tokens.searchContainerShape);

    // --- 3. the common set --------------------------------------------------
    tokens.avatarSize = lengthOverride(overrides, commonKeys("avatar.size"), tokens.avatarSize);
    tokens.iconButtonSpace =
        lengthOverride(overrides, commonKeys("icon-button-space"), tokens.iconButtonSpace);
    tokens.iconSize = lengthOverride(overrides, commonKeys("icon.size"), tokens.iconSize);
    tokens.leadingSpace =
        lengthOverride(overrides, commonKeys("leading-space"), tokens.leadingSpace);
    tokens.trailingSpace =
        lengthOverride(overrides, commonKeys("trailing-space"), tokens.trailingSpace);
    tokens.searchLeadingSpace =
        lengthOverride(overrides, searchKeys("leading-space"), tokens.searchLeadingSpace);
    tokens.searchTrailingSpace =
        lengthOverride(overrides, searchKeys("trailing-space"), tokens.searchTrailingSpace);
    tokens.containerShape = shapeOverride(overrides, commonKeys("container.shape"),
                                          tokens.containerShape);

    // The 16 px edge distance is Compose's intent written as a subtraction;
    // keeping it a field means a theme that retunes `leading-space` keeps
    // `titleInset()` consistent instead of silently changing the meaning of
    // the title's floor.
    tokens.edgeSpace = lengthOverride(overrides, commonKeys("edge-space"), tokens.edgeSpace);

    return tokens;
}

// ---------------------------------------------------------------------------
// MdBottomAppBarTokens
// ---------------------------------------------------------------------------

MdBottomAppBarTokens MdBottomAppBarTokens::resolve(const MdComponentTokens *overrides)
{
    MdBottomAppBarTokens tokens;

    using namespace comptoken;

    tokens.containerHeight = lengthOverride(
        overrides, plainKeys("bottom-app-bar", "container.height"), tokens.containerHeight);
    tokens.withFabContainerHeight =
        lengthOverride(overrides, plainKeys("bottom-app-bar", "with-fab.container.height"),
                       tokens.withFabContainerHeight);
    tokens.containerShape = shapeOverride(overrides,
                                          plainKeys("bottom-app-bar", "container.shape"),
                                          tokens.containerShape);

    // The three content paddings and the two FAB paddings are *not* token
    // rows; Compose derives them from the 16 px edge distance. They get the
    // `content.*` / `fab.*` prefixes so a theme can still retune them, the
    // same treatment MdDivider's non-token 16 px inset receives.
    tokens.contentLeadingSpace =
        lengthOverride(overrides, plainKeys("bottom-app-bar", "content.leading-space"),
                       tokens.contentLeadingSpace);
    tokens.contentTopSpace = lengthOverride(overrides,
                                            plainKeys("bottom-app-bar", "content.top-space"),
                                            tokens.contentTopSpace);
    tokens.contentTrailingSpace =
        lengthOverride(overrides, plainKeys("bottom-app-bar", "content.trailing-space"),
                       tokens.contentTrailingSpace);
    tokens.fabLeadingSpace = lengthOverride(overrides,
                                            plainKeys("bottom-app-bar", "fab.leading-space"),
                                            tokens.fabLeadingSpace);
    tokens.fabTopSpace =
        lengthOverride(overrides, plainKeys("bottom-app-bar", "fab.top-space"), tokens.fabTopSpace);
    tokens.edgeSpace =
        lengthOverride(overrides, plainKeys("bottom-app-bar", "edge-space"), tokens.edgeSpace);

    return tokens;
}

} // namespace md
