#ifndef MD_BUTTON_GROUP_TOKENS_H
#define MD_BUTTON_GROUP_TOKENS_H

// The published `md.comp.button-group.*` token set, resolved for one
// (form, size) combination.
//
// Source of truth, as for MdButton:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-button-group-standard-<size>.scss
//     tokens/versions/latest/sass/_md-comp-button-group-connected-<size>.scss
//
// Token export version 34.0.21. The keys are carried through verbatim.
//
// A button group publishes *no colours at all* and the design spec says so in
// as many words -- "Button groups have no color properties. They can use the
// default button or toggle button color styles, like filled, tonal, and
// outlined." Everything a group does is spacing and shape, and everything it
// looks like is the buttons inside it. That is why this table is small and why
// there is no `MdButtonGroupColourSlot`.
//
// The two forms publish different things, because they are different
// mechanisms:
//
//   standard   between-space, container.height, and a pressed item's width
//              multiplier with its spring. Nothing about shape: each item keeps
//              the shape its own button tokens give it.
//   connected  between-space (always 2), container.height, the group's outer
//              shape, and the inner corner an item shows towards its
//              neighbours -- plus the corner it shows while pressed and the
//              one it shows while selected.
//
// Two divergences between the token export and the design spec are recorded in
// docs/porting-todo.md and deliberately not reconciled here:
//
//   * `md.comp.button-group.connected.xsmall.inner-corner.corner-size` is
//     corner-small (8 px) in the export, while the specs page lists the
//     extra-small inner corner as 4 px. The export wins, because the project
//     rule is that numeric values come from the tokens.
//   * the export publishes only the *round* connected form
//     (`container.shape: corner-full`). The square form's outer corner sizes
//     appear only on the specs page, so `MdButtonGroupShape::Square` is built
//     from that list and is marked as such rather than pretended to be a token.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QStringList>
#include <QtCore/QtGlobal>

namespace md {

/// The `md.comp.button-group.connected` outer shape. The export publishes the
/// round form only; see the header note.
enum class ButtonGroupShape {
    Round,
    Square,
    Count,
};

/// Everything a button group needs to place and shape its items.
struct QT_MD3_EXPORT MdButtonGroupTokens
{
    ButtonGroupVariant variant = ButtonGroupVariant::Standard;
    ButtonSize size = ButtonSize::Small;

    /// `container.height`. The group does not paint a container, but it does
    /// use this to know how tall its items are meant to be.
    qreal containerHeight = 40.0;

    /// `between-space` — the gap between two neighbouring *containers*.
    ///
    /// Standard publishes 18 / 12 / 8 / 8 / 8 so that the resulting target
    /// area clears the 48 dp minimum. Connected publishes 2 at every size
    /// "to provide visual consistency at scale".
    qreal betweenSpace = 12.0;

    /// `pressed.item.width.multiplier`, 0.15 = +15%. Standard only; connected
    /// publishes no such token because a connected item does not push its
    /// neighbours.
    qreal pressedWidthMultiplier = 0.15;

    /// `pressed.item.width.motion.spring.*` — spring-fast-spatial.
    qreal springStiffness = 1400.0;
    qreal springDampingRatio = 0.9;

    /// Connected only: the corner the group's outer edge shows.
    ShapeCorner outerCorner = ShapeCorner::Full;
    /// Connected only: the corner an item shows towards its neighbours.
    ShapeCorner innerCorner = ShapeCorner::Small;
    /// Connected only: the inner corner while the item is pressed.
    ShapeCorner pressedInnerCorner = ShapeCorner::ExtraSmall;
    /// Connected only: `selected.inner-corner.corner-size`, published as the
    /// literal `50%`. Stored as a fraction of the cross-axis extent because
    /// that is what the token means — on a 40 px group it is 20 px, on a
    /// 136 px group it is 68 px.
    qreal selectedInnerCornerFraction = 0.5;

    /// Connected only: the minimum width of an `xsmall` or `small` item.
    ///
    /// "Extra small and small connected button groups have 48 dp target areas
    /// and a minimum width of 48 dp." 0 for every other combination.
    qreal minimumItemExtent = 0.0;

    /// Build the published set for one combination.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace with the
    /// size-qualified key winning over the generic one, exactly as MdButton's
    /// resolver does:
    ///
    ///   md.comp.button-group.<variant>.<size>.<token>   (wins)
    ///   md.comp.button-group.<variant>.<token>          (fallback)
    ///   published value                                 (default)
    static MdButtonGroupTokens resolve(ButtonGroupVariant variant,
                                       ButtonSize size,
                                       ButtonGroupShape shape,
                                       const MdComponentTokens *overrides = nullptr);

    /// The `md.comp.button-group.<variant>.<size>.<token>` key for a token.
    static QString key(ButtonGroupVariant variant, ButtonSize size, const QString &token);
};

} // namespace md

#endif // MD_BUTTON_GROUP_TOKENS_H
