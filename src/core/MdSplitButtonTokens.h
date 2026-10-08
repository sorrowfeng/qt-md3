#ifndef MD_SPLIT_BUTTON_TOKENS_H
#define MD_SPLIT_BUTTON_TOKENS_H

// The published `md.comp.split-button.*` token set, resolved for one
// (variant, size) combination.
//
// Sources, checked against each other line by line:
//
//   metric tokens — material-components/material-web
//     tokens/versions/latest/sass/_md-comp-split-button-<size>.scss
//     (five size sets: xsmall / small / medium / large / xlarge, each with the
//     between-space, the leading-button spaces, the trailing-button spaces and
//     icon size, and the inner-corner sizes for rest / hovered / pressed /
//     trailing-selected)
//
//   colour + type + focus rows — the spec page
//     https://m3.material.io/components/split-button/specs
//     "Split buttons use the same color schemes as standard buttons ... shown
//     in the following token module. Go to buttons for more details." The
//     export publishes *no* `md.comp.split-button` colour rows at all, so the
//     colour, state-layer, typography and focus-indicator rows are the
//     button family's, and the variants are the button family's
//     (Elevated / Filled / Tonal / Outlined / Text).
//
// Two recorded decisions (docs/porting-todo.md, Split buttons entry):
//
//   * the button token set used for the colour rows is the identity mapping
//     by size — the two Expressive scales agree on every height (32 / 40 /
//     56 / 96 / 136), so the borrowed typography stays aligned with the
//     physical size. Only colour, typography and focus rows come across; the
//     split button's own metric rows are always its own tokens.
//   * the export publishes no motion rows. The inner-corner morph runs on the
//     button family's press spring (`spring-fast-spatial`: 1400 / 0.9), which
//     is the spring the Compose Expressive split button uses for the same
//     transition; recorded rather than invented silently.

#include "MdButtonTokens.h"
#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QSizeF>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace md {

/// The metric rows of one `md.comp.split-button.<size>.*` set. Every number is
/// transcribed verbatim from the export; a reader can grep the upstream file
/// for any field name.
struct QT_MD3_EXPORT MdSplitButtonMetrics
{
    /// `between-space` — the gap between the two halves.
    qreal betweenSpace = 2.0;
    /// `container-height`.
    qreal containerHeight = 40.0;

    /// `leading-button.leading-space` / `leading-button.trailing-space`.
    qreal leadingLeadingSpace = 16.0;
    qreal leadingTrailingSpace = 12.0;

    /// `trailing-button.icon-size`.
    qreal trailingIconSize = 22.0;
    /// `trailing-button.leading-space` / `trailing-button.trailing-space`.
    qreal trailingLeadingSpace = 13.0;
    qreal trailingTrailingSpace = 13.0;

    /// The inner (facing) corners in logical px, resolved from the export's
    /// shape-scale references:
    ///
    ///   `inner-corner-corner-size`                     (rest)
    ///   `inner-corner-hovered-corner-size`             (hover)
    ///   `inner-corner-pressed-corner-size`             (press)
    ///
    /// The outer corners are `container-shape: corner-full` — half the height
    /// for every size — and the trailing half's facing corners become half the
    /// height too when it is selected (`trailing-button-inner-corner-selected-
    /// corner-size: 50%`), which is a radius, not a shape-scale step, so it is
    /// computed from the height rather than stored.
    qreal innerCornerRest = 4.0;
    qreal innerCornerHovered = 8.0;
    qreal innerCornerPressed = 8.0;

    /// The outer corner radius for this size: half the container height
    /// (`container-shape: corner-full`).
    qreal outerCornerRadius() const { return containerHeight / 2.0; }
    /// The trailing half's facing-corner radius while selected: 50% of the
    /// height (`trailing-button-inner-corner-selected-corner-size: 50%`).
    qreal selectedInnerCornerRadius() const { return containerHeight / 2.0; }
};

/// Everything needed to paint and lay out one split button.
struct QT_MD3_EXPORT MdSplitButtonTokens
{
    MdSplitButtonMetrics metrics;

    /// The colour / typography / focus rows, borrowed from the button family
    /// (see the file comment for why). The `buttonSize` used to resolve it is
    /// the height-matched one; the split button ignores the button's height,
    /// leading-space and shape rows.
    MdButtonTokens button;

    /// Build the published set for one combination.
    ///
    /// `overrides` is consulted through the `md.comp.split-button.*` key
    /// namespace for the *metric* rows only:
    ///
    ///   md.comp.split-button.<size>.container.height   (wins)
    ///   md.comp.split-button.container.height          (fallback)
    ///   published value                                (default)
    ///
    /// Colour rows are not overridable here — they ride on the button family
    /// and the colour scheme, and a second override path would be a second,
    /// competing theming mechanism (same decision the button family made).
    static MdSplitButtonTokens resolve(ButtonVariant variant,
                                       SplitButtonSize size,
                                       const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_SPLIT_BUTTON_TOKENS_H
