#ifndef MD_APP_BAR_TOKENS_H
#define MD_APP_BAR_TOKENS_H

// The published `md.comp.app-bar.*` and `md.comp.bottom-app-bar.*` token sets.
//
// Every number and every role below is transcribed from the authoritative
// sources, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-app-bar.scss              (common)
//     tokens/versions/latest/sass/_md-comp-app-bar-small.scss
//     tokens/versions/latest/sass/_md-comp-app-bar-medium.scss
//     tokens/versions/latest/sass/_md-comp-app-bar-large.scss
//     tokens/versions/latest/sass/_md-comp-app-bar-medium-flexible.scss
//     tokens/versions/latest/sass/_md-comp-app-bar-large-flexible.scss
//     tokens/versions/latest/sass/_md-comp-bottom-app-bar.scss
//   androidx/androidx
//     compose/material3/.../AppBar.kt                     (layout + behaviour)
//     compose/material3/.../tokens/AppBar*Tokens.kt        (cross-check)
//
// Token export version 34.0.21. Compose's token files agree with the export
// row for row, including the two rows the export marks deprecated; where this
// port follows one source over the other it says so below.
//
// Six facts of this family worth keeping visible:
//
//   * **The export is split, the component is not.** The common set carries
//     the colours, the elevation pair and the spacing; each size set carries
//     only its container height plus the two type styles. `MdAppBarTokens::
//     resolve(variant)` folds the two so a caller never has to know which
//     half a row lives in.
//   * **Baseline medium and large are deprecated as *designs*, not as
//     tokens.** m3.material.io's variant table says so outright: "The baseline
//     M3 medium and large app bars are no longer recommended in M3 Expressive,
//     and should be replaced with medium flexible and large flexible app bars,
//     which are similar visually, but have multi-line support, a shorter
//     height". Both are still published and both are still reachable here —
//     a port that dropped them would not be able to render a 2024 app.
//   * **The 16 px edge distance is not a row.** Compose hardcodes
//     `TopAppBarHorizontalPadding = 4.dp` (which *is* the published
//     `leading-space` / `trailing-space`) and derives
//     `TopAppBarTitleInset = 16.dp - TopAppBarHorizontalPadding`. So the
//     space a caller sees beside a leading button is the icon button's own
//     12 px of internal padding plus this 4 px. Both constants are carried
//     here, the 16 px as `edgeSpace` so the intent stays legible.
//   * **The two "bottom padding" values are baseline distances.** Compose
//     places the expanded title's *baseline* 24 dp (medium) / 28 dp (large)
//     above the container's bottom edge, and shrinks that padding when the
//     title would overflow. They are not token rows.
//   * **`TopTitleAlphaEasing` is published in code only.** The easing that
//     cross-fades a two-row bar's small title against its expanded title
//     (`cubic-bezier(.8, 0, .8, .15)`) exists in Compose and nowhere else.
//   * **`with-fab` and `surface-tint-layer` are deprecated rows.** The
//     comment on `with-fab.container.height` says the bottom app bar "design
//     updated to use a single height for all configurations, with vertically
//     centered content"; the surface-tint layer row is deprecated "as part of
//     the update from opacity based surfaces to tonal surfaces". Both are
//     carried and neither is used, so a 2022 theme that sets them still
//     resolves.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// The five published app bar layouts.
///
/// m3.material.io lists seven entries in its variant table, but two of them
/// are not layouts of their own:
///
///   * "Center-aligned" is *merged into small* — "Use centered-text
///     configuration" — so it is `MdAppBarAlignment::Center` on `Small`, not
///     a sixth variant. Compose agrees: `CenterAlignedTopAppBar` is
///     `SingleRowTopAppBar` with `Alignment.CenterHorizontally`.
///   * the search app bar is a configuration in which the centre of the bar
///     is a search field (`md.comp.app-bar.search.*`); those rows live here
///     and the centre slot is where a caller puts the field. Rendering the
///     field itself belongs to the Search family.
enum class MdAppBarVariant
{
    Small,
    Medium,
    Large,
    MediumFlexible,
    LargeFlexible,
    Count,
};

/// Where a bar's text sits horizontally. m3.material.io's "Text alignment"
/// configuration: leading edge (the default) or centred.
enum class MdAppBarAlignment
{
    Leading,
    Center,
    Count,
};

/// Everything needed to lay out and paint one top app bar.
///
/// The struct holds the *common* set and the *resolved* size set together, so
/// `variant` is the only field a caller has to look at to know which rows are
/// live.
struct QT_MD3_EXPORT MdAppBarTokens
{
    // --- the common set (`md.comp.app-bar.*`) ------------------------------
    /// `md.comp.app-bar.avatar.size` — a trailing avatar's diameter.
    qreal avatarSize = 32.0;
    /// `md.comp.app-bar.icon-button-space` — 0, i.e. adjacent icon buttons
    /// touch (each brings its own 12 px of internal padding).
    qreal iconButtonSpace = 0.0;
    /// `md.comp.app-bar.icon.size` (the common row; the per-size rows that
    /// repeat it are deprecated in favour of this one).
    qreal iconSize = 24.0;
    /// `md.comp.app-bar.leading-space` — the container's start padding.
    qreal leadingSpace = 4.0;
    /// `md.comp.app-bar.trailing-space` — the container's end padding.
    qreal trailingSpace = 4.0;
    /// `md.comp.app-bar.search.leading-space`.
    qreal searchLeadingSpace = 8.0;
    /// `md.comp.app-bar.search.trailing-space`.
    qreal searchTrailingSpace = 8.0;

    ColorRole containerColor = ColorRole::Surface;
    ColorRole onScrollContainerColor = ColorRole::SurfaceContainer;
    ColorRole leadingIconColor = ColorRole::OnSurface;
    ColorRole titleColor = ColorRole::OnSurface;
    ColorRole trailingIconColor = ColorRole::OnSurfaceVariant;
    ColorRole subtitleColor = ColorRole::OnSurfaceVariant;
    /// `md.comp.app-bar.search.container.color`.
    ColorRole searchContainerColor = ColorRole::SurfaceContainer;
    /// `md.comp.app-bar.search.label.color`.
    ColorRole searchLabelColor = ColorRole::OnSurfaceVariant;
    /// `md.comp.app-bar.search.on-scroll.container.color` — the one on-scroll
    /// row that steps *two* stops up rather than one.
    ColorRole searchOnScrollContainerColor = ColorRole::SurfaceContainerHighest;

    ElevationLevel containerElevation = ElevationLevel::Level0;
    ElevationLevel onScrollContainerElevation = ElevationLevel::Level2;
    ShapeCorner containerShape = ShapeCorner::None;

    // --- the resolved size set (`md.comp.app-bar.<size>.*`) ----------------
    MdAppBarVariant variant = MdAppBarVariant::Small;
    /// `md.comp.app-bar.<size>.container.height`.
    qreal containerHeight = 64.0;
    /// `md.comp.app-bar.<size>-flexible.with-subtitle.container.height` — 0
    /// for a variant whose text cannot wrap to a second line.
    qreal withSubtitleContainerHeight = 0.0;
    /// The height a two-row bar's *first* row keeps when fully collapsed.
    ///
    /// Compose reads `AppBarSmallTokens.ContainerHeight` for every two-row
    /// variant — medium, large and both flexible ones — so it is the *small*
    /// size group's row that decides where the collapse stops. Retuning only
    /// `md.comp.app-bar.small.container.height` therefore moves it, which is
    /// what Compose would do too.
    qreal collapsedRowHeight = 64.0;
    /// `md.comp.app-bar.<size>.title.font`.
    TypeStyle titleTypeStyle = TypeStyle::TitleLarge;
    /// `md.comp.app-bar.<size>.subtitle.font`.
    TypeStyle subtitleTypeStyle = TypeStyle::LabelMedium;

    // --- the collapsed-row text styles (Compose, not a token row) ----------
    /// The style a two-row bar's *first* row uses, widened from
    /// `AppBarSmallTokens.TitleFont` in the same way `Collapsed` bars work:
    /// Compose composes the expanded title with the small app bar's own
    /// styles and cross-fades between the two.
    TypeStyle collapsedTitleTypeStyle = TypeStyle::TitleLarge;
    /// `AppBarSmallTokens.SubtitleFont` — what the first row uses when the
    /// bar has a subtitle.
    TypeStyle collapsedSubtitleTypeStyle = TypeStyle::LabelMedium;

    // --- the search configuration (`md.comp.app-bar.<size>.search.*`) ------
    /// `md.comp.app-bar.small.search.container.height` — published by the
    /// small size set only.
    qreal searchContainerHeight = 56.0;
    /// `md.comp.app-bar.small.search.container.shape`.
    ShapeCorner searchContainerShape = ShapeCorner::Full;
    /// `md.comp.app-bar.small.search.label-text.font`.
    TypeStyle searchLabelTypeStyle = TypeStyle::BodyLarge;

    // --- derived layout constants (Compose) --------------------------------
    /// The 16 px the spec means by "16dp from the edge": the icon button's own
    /// 12 px plus `leadingSpace`. Compose spells the whole thing as
    /// `TopAppBarTitleInset = 16.dp - TopAppBarHorizontalPadding`.
    qreal edgeSpace = 16.0;

    /// `TopAppBarTitleInset` — the smallest start inset the title may take,
    /// i.e. `edgeSpace - leadingSpace`. Used as a floor when there is no
    /// leading element, and as the spacer's width in that case.
    qreal titleInset() const { return edgeSpace - leadingSpace; }

    /// `MediumTitleBottomPadding` — where the expanded title's baseline sits
    /// above the bottom edge of a medium bar.
    qreal mediumTitleBottomPadding = 24.0;
    /// `LargeTitleBottomPadding`, same, for a large bar.
    qreal largeTitleBottomPadding = 28.0;

    /// `TopAppBarExpandedHeight` = `AppBarSmallTokens.ContainerHeight`.
    ///
    /// Every two-row bar collapses to `collapsedRowHeight`, not to zero: a
    /// medium or large bar keeps its icon/action row and only the text row
    /// disappears. That ratio is what makes the collapsed fraction meaningful.
    ///
    /// `TopAppBarDefaults`' snap spec is `MotionSchemeKeyTokens.DefaultEffects`,
    /// the same spec the container colour animates on.
    ///
    /// The threshold below is Compose's: `SingleRowTopAppBar` asks for the
    /// scrolled container colour as soon as `overlappedFraction > 0.01`.
    static constexpr qreal scrolledColourThreshold = 0.01;

    /// True when the variant lays its title out in a second row under the
    /// navigation and action row (medium, large, and their flexible forms).
    bool isTwoRows() const { return variant != MdAppBarVariant::Small; }

    /// True when the variant's own text may wrap to a second line and the bar
    /// therefore grows when a subtitle is present.
    bool supportsSubtitle() const
    {
        return variant == MdAppBarVariant::Small || variant == MdAppBarVariant::MediumFlexible
            || variant == MdAppBarVariant::LargeFlexible;
    }

    /// The baseline distance the expanded title keeps from the bottom edge —
    /// 0 for a single-row bar, which centres its title instead.
    qreal titleBottomPadding() const;

    /// The expanded height, i.e. `containerHeight` raised to
    /// `withSubtitleContainerHeight` when the caller has a subtitle and the
    /// variant grows for one.
    qreal expandedHeight(bool hasSubtitle) const;

    /// The height the bar currently draws at, given how far it has collapsed.
    /// `collapsedFraction` is 0 (fully expanded) to 1 (fully collapsed).
    qreal heightFor(qreal collapsedFraction, bool hasSubtitle) const;

    /// Build the published set for `variant`.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace, in the
    /// size-then-common order `comptoken::sizeKeys` defines:
    ///
    ///   md.comp.app-bar.<size>.container.height
    ///   md.comp.app-bar.<size>.with-subtitle.container.height
    ///   md.comp.app-bar.<size>.title.font        (name only; see below)
    ///   md.comp.app-bar.container.color          (and the other common rows)
    ///   md.comp.app-bar.leading-space
    ///   …
    ///
    /// Lengths and shapes are overridable. Colours are not — the same rule the
    /// rest of the library follows, recorded in docs/porting-todo.md. Type
    /// styles are not overridable either: a `title.font` row names a *style*,
    /// and the library's type-scale override is the type scale's own API
    /// (`MdTypeScale::setFamily`), not a component key.
    static MdAppBarTokens resolve(MdAppBarVariant variant,
                                  const MdComponentTokens *overrides = nullptr);
};

/// Everything needed to lay out and paint one bottom app bar.
///
/// The bottom app bar is the other half of the official "App bars" component:
/// m3.material.io's bottom app bar "displays navigation and key actions at the
/// bottom of small screens". It shares no layout with the top app bar — it is
/// a single centred row — which is why it has its own token namespace rather
/// than a variant of the one above.
///
/// `FlexibleBottomAppBar` is deliberately *not* here: its content padding and
/// height come from `DockedToolbarTokens`, i.e. it is the docked toolbar that
/// the ★ Toolbars family will port, not a bottom app bar.
struct QT_MD3_EXPORT MdBottomAppBarTokens
{
    /// `md.comp.bottom-app-bar.container.height` — one height for every
    /// configuration since the design update.
    qreal containerHeight = 80.0;
    /// `md.comp.bottom-app-bar.with-fab.container.height` — deprecated by the
    /// comment quoted in the header above. Carried, never used.
    qreal withFabContainerHeight = 72.0;

    ColorRole containerColor = ColorRole::SurfaceContainer;
    ElevationLevel containerElevation = ElevationLevel::Level2;
    ShapeCorner containerShape = ShapeCorner::None;

    /// `md.comp.bottom-app-bar.container.surface-tint-layer.color` —
    /// deprecated by the move from opacity-based to tonal surfaces. Carried,
    /// never used.
    ColorRole containerSurfaceTintLayerColor = ColorRole::SurfaceTint;

    /// `BottomAppBarDefaults.ContentPadding`'s start / top / end. Compose
    /// writes them as `16.dp - 12.dp`: the 12 px is the icon button's own
    /// internal padding, so a leading icon button lands 16 px from the edge.
    /// The bottom edge carries no padding — the row is vertically centred in
    /// the 80 px container instead.
    qreal contentLeadingSpace = 4.0;
    qreal contentTopSpace = 4.0;
    qreal contentTrailingSpace = 4.0;

    /// The extra padding a hosted floating action button takes beyond the
    /// row's own: `FABHorizontalPadding = 16 - 4` and
    /// `FABVerticalPadding = 12 - 4`, so the FAB sits 16 px from the trailing
    /// edge and 12 px from the top.
    qreal fabLeadingSpace = 12.0;
    qreal fabTopSpace = 8.0;

    /// See `MdAppBarTokens::edgeSpace`.
    qreal edgeSpace = 16.0;

    /// Build the published set. Only lengths and the shape are overridable
    /// (`md.comp.bottom-app-bar.container.height`, `…container.shape`, the
    /// spacing rows above, which are not token rows and therefore take the
    /// `md.comp.bottom-app-bar.content.*` / `.fab.*` key prefixes invented
    /// here — the same treatment the divider's non-token 16 px inset gets).
    static MdBottomAppBarTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_APP_BAR_TOKENS_H
