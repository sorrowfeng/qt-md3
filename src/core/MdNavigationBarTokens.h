#ifndef MD_NAVIGATION_BAR_TOKENS_H
#define MD_NAVIGATION_BAR_TOKENS_H

// MdNavigationBarTokens — the MD3 navigation bar, both of its published
// variants, in one structure.
//
// ## Two families, one component
//
// The export ships **two unrelated token families for the navigation bar in the
// same release** (both report version 34.0.21), and they are not two
// generations of one thing:
//
//   | | family | height | indicator | active label |
//   | --- | --- | --- | --- | --- |
//   | Baseline  | `md.comp.navigation-bar.*`      | 80 | 64 x 32 | `on-surface` |
//   | Flexible  | `md.comp.nav-bar.*` (+ items)   | 64 | 56 x 32 | `secondary` |
//
// The spec calls the second one the *flexible* navigation bar and says it
// replaces the baseline one: m3.material.io/components/navigation-bar, "M3
// Expressive update (May 2025)" — "A new flexible navigation bar was introduced
// to replace the baseline navigation bar. It's shorter and supports horizontal
// navigation items in medium windows", and under Color, "Active label changed
// from on-surface-variant to secondary". Availability on that page reads `Web:
// Unavailable` and `Web: Expressive: Unavailable`, so material-web implements
// neither in its shipped product; its `labs/navigationbar` is a baseline
// implementation whose values come from `tokens/versions/v0_192` and match the
// `navigation-bar` family row for row, which is what makes that family the
// baseline rather than the older of two snapshots.
//
// ## Why the numbers are picked the way they are
//
// Three sources disagree in four places, and each is resolved here rather than
// silently:
//
//   * **height 80 / 64** — the export and the spec agree; Compose's
//     `NavigationBar.kt` reads `NavigationBarTokens.TallContainerHeight` (80)
//     while its own token file declares `ContainerHeight = 64` and marks the
//     tall row `// TODO: Update this file to include the following missing
//     tokens`. So Compose's *baseline* is 80 and its short bar
//     (`ShortNavigationBar.kt`) is 64 — one behaviour source per variant, which
//     is what this port follows.
//   * **indicator 64 or 56 wide in the baseline** — the export's
//     `navigation-bar.active-indicator.width` and material-web's
//     `v0_192/navigation-bar` both say **64**; Compose's baseline reuses
//     `NavigationBarVerticalItemTokens.ActiveIndicatorWidth` (**56**), whose
//     natural home is the flexible family. Two independent sources beat one, so
//     the baseline is 64 and the divergence is recorded in
//     docs/porting-todo.md.
//   * **`item.between-space` 0 or 8** — the export publishes `0px`; Compose's
//     baseline hard-codes `Arrangement.spacedBy(8.dp)` and its `Centered`
//     arrangement offsets the run by a percentage instead. The gap is a
//     behaviour, so Compose wins and `itemBetweenSpace` is 8.
//   * **elevation level2 or none** — the export says `level2`; Compose
//     hard-codes `NavigationBarDefaults.Elevation = ElevationTokens.Level0` and
//     the spec's "Differences from M2" says "Elevation: No shadow". The row is
//     carried at its published value and the *paint* decides; see
//     `MdNavigationBarStyle`.
//
// ## The item's shape
//
// One item, two layouts. Compose's expressive `NavigationItem` takes a
// `NavigationItemIconPosition` of `Top` (icon above label, the baseline
// arrangement) or `Start` (icon beside label, inside the pill), and the
// flexible bar is the only variant that offers `Start` — the baseline publishes
// no horizontal item rows at all. Both are carried: `Top` reads
// `nav-bar-item-vertical`, `Start` reads `nav-bar-item-horizontal`.

#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which published family the bar is drawn from. There is no default variant
/// name in the export; the spec's words are "baseline" and "flexible".
enum class MdNavigationBarVariant {
    Baseline,
    Flexible,
    Count,
};

constexpr int navVariantCount = int(MdNavigationBarVariant::Count);

/// Compose's `NavigationItemIconPosition`. `Top` is icon above label, `Start`
/// is icon beside label.
enum class MdNavigationItemIconPosition {
    Top,
    Start,
    Count,
};

/// Compose's `ShortNavigationBarArrangement`. `EqualWeight` divides the bar's
/// width between the items; `Centered` groups them in the middle and shrinks
/// the side padding as the item count grows.
enum class MdNavigationBarArrangement {
    EqualWeight,
    Centered,
    Count,
};

/// The six states an item can be painted in. `Disabled` comes first because it
/// wins over everything else, matching Compose's `!enabled -> ... -> selected`
/// when-clauses.
enum class MdNavigationItemState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Disabled,
    Count,
};

constexpr int navItemStateCount = int(MdNavigationItemState::Count);

/// A colour plus the alpha the export asks for it at. `role == Count` means the
/// export publishes no such row, which is not the same as "transparent".
struct QT_MD3_EXPORT MdNavigationColourSlot
{
    ColorRole role = ColorRole::Count;
    qreal opacity = 1.0;

    bool isPresent() const { return role != ColorRole::Count; }
};

/// One item's colours, indexed by state — the shape the export's rows actually
/// have (`active.hovered.icon.color`, `inactive.focused.label-text.color`, ...).
struct QT_MD3_EXPORT MdNavigationItemColours
{
    MdNavigationColourSlot icon[navItemStateCount];
    MdNavigationColourSlot label[navItemStateCount];
    MdNavigationColourSlot stateLayer[navItemStateCount];
    /// The pill behind a selected item. Absent when the item is not selected.
    MdNavigationColourSlot indicator;

    const MdNavigationColourSlot &iconFor(MdNavigationItemState state) const;
    const MdNavigationColourSlot &labelFor(MdNavigationItemState state) const;
    const MdNavigationColourSlot &stateLayerFor(MdNavigationItemState state) const;
};

/// One variant's rows. The two families differ in their component name in the
/// override namespace as well as in their numbers, so `resolve()` reads
/// `navigation-bar` for one and `nav-bar` for the other.
struct QT_MD3_EXPORT MdNavigationBarVariantTokens
{
    /// 80 baseline, 64 flexible.
    qreal containerHeight = 80.0;
    /// The pill's width for the `Top` position. 64 baseline, 56 flexible.
    qreal activeIndicatorWidth = 64.0;
    /// 32 in both families.
    qreal activeIndicatorHeight = 32.0;
    qreal iconSize = 24.0;
    /// Indicator-to-label for `Top`, icon-to-label for `Start`. 4 in both.
    qreal indicatorIconLabelSpace = 4.0;
    /// The item's vertical padding for `Top`: 0 baseline, 6 flexible.
    qreal containerBetweenSpace = 0.0;
    /// The `Start` position's rows. The baseline publishes none, so both
    /// variants carry the flexible ones — see the header note.
    qreal horizontalIndicatorHeight = 40.0;
    qreal horizontalIndicatorLeadingSpace = 16.0;
    qreal horizontalIndicatorTrailingSpace = 16.0;
    /// The `Start` position's icon-label space, when a family publishes a
    /// different row from the `Top` position's. The rail's horizontal item
    /// reads 8 where its vertical item reads 4; the bar's two positions agree
    /// at 4, so its resolve leaves this 0 and the item falls back to
    /// `indicatorIconLabelSpace`.
    qreal horizontalIconLabelSpace = 0.0;
    /// The `Top` position's pill height when the item has **no label**. The
    /// baseline rail publishes `no-label-active-indicator-height: 56px` — a
    /// 56 x 56 square pill around the bare icon; 0 means the family has no
    /// such row and the pill is `activeIndicatorHeight` tall either way.
    qreal noLabelIndicatorHeight = 0.0;

    /// `label-text-*`: `label-medium` in both families. The selected label is
    /// painted with `label-medium-weight-prominent`, which is what
    /// `TypeEmphasis::Emphasized` resolves to.
    TypeStyle labelTextType = TypeStyle::LabelMedium;
    /// The `Start` position's label type, when the family styles it
    /// differently from the `Top` position's. The rail's expanded item is
    /// `label-large` where its collapsed item is `label-medium`; the bar's two
    /// positions agree, so its resolve leaves this at the shared default.
    TypeStyle horizontalLabelTextType = TypeStyle::LabelMedium;

    ColorRole containerColor = ColorRole::SurfaceContainer;
    ColorRole containerShadowColor = ColorRole::Shadow;
    /// Baseline only. Carried and read by nothing: this library has no tonal
    /// elevation, and the row is the export's, not the paint's, to own.
    ColorRole containerSurfaceTintLayerColor = ColorRole::SurfaceTint;
    /// The `Start` position's label colour, which the flexible family reads
    /// from its *icon* colour (`on-secondary-container`) because the label
    /// sits inside the pill. `Count` means "use the `Top` table".
    ColorRole labelColorStart = ColorRole::Count;

    ShapeCorner containerShape = ShapeCorner::None;
    ShapeCorner indicatorShape = ShapeCorner::Full;
    ElevationLevel containerElevation = ElevationLevel::Level2;

    /// The focus-ring rows. The baseline family publishes them
    /// (`focus-indicator-color` / `-outline-offset` / `-thickness`); the
    /// flexible family publishes none and inherits the shared values, because
    /// the ring is a cross-cutting system token rather than a family one.
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    MdNavigationItemColours unselected;
    MdNavigationItemColours selected;

    const MdNavigationItemColours &coloursFor(bool selected) const;

    /// One side's padding inside the pill for the `Top` position:
    /// `(activeIndicatorWidth - iconSize) / 2` — 20 at the baseline's 64,
    /// 16 at the flexible family's 56.
    qreal indicatorHorizontalPadding() const;
    /// `(activeIndicatorHeight - iconSize) / 2` — 4 in both families.
    qreal indicatorVerticalPadding() const;
    /// `(horizontalIndicatorHeight - iconSize) / 2` — 8.
    qreal horizontalIndicatorVerticalPadding() const;
};

struct QT_MD3_EXPORT MdNavigationBarTokens
{
    MdNavigationBarVariantTokens variant[navVariantCount];

    /// The gap between two items. **Not a token row** — the export's
    /// `item.between-space` is `0px` while Compose's baseline hard-codes
    /// `Arrangement.spacedBy(8.dp)`, and a gap between items is behaviour, so
    /// Compose's 8 wins. Recorded in docs/porting-todo.md.
    qreal itemBetweenSpace = 8.0;

    const MdNavigationBarVariantTokens &forVariant(MdNavigationBarVariant variant) const;

    /// Resolves both families in one pass. Colours are not overridable
    /// library-wide, so only the lengths and the two shapes are read back.
    static MdNavigationBarTokens resolve(const MdComponentTokens *overrides = nullptr);
};

/// Fills one family's item colour tables — the icon / label / state-layer rows
/// and the indicator. Extracted because the *rail's* colour tables
/// (`nav-rail.scss`, `navigation-rail.scss`) agree with the bar's row for row,
/// and the rail's item is this same structure pushed into the shared item
/// widget.
QT_MD3_EXPORT void fillNavigationItemColours(MdNavigationBarVariantTokens *tokens,
                                             MdNavigationBarVariant variant);

} // namespace md

#endif // MD_NAVIGATION_BAR_TOKENS_H
