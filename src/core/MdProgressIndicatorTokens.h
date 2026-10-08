#ifndef MD_PROGRESS_INDICATOR_TOKENS_H
#define MD_PROGRESS_INDICATOR_TOKENS_H

// The merged `md.comp.progress-indicator.*` token set (base + linear +
// circular), resolved for one shape.
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-progress-indicator.scss           (base)
//     tokens/versions/latest/sass/_md-comp-progress-indicator-linear.scss    (linear)
//     tokens/versions/latest/sass/_md-comp-progress-indicator-circular.scss  (circular)
//     tokens/versions/latest/sass/_md-comp-{linear,circular}-progress-indicator.scss
//         (the deprecated per-shape sets — four-color rows only)
//
// Token export version 34.0.21.
//
// Facts of this family worth keeping visible:
//
//   * The old `_md-comp-linear-progress-indicator` / `_md-comp-circular-
//     progress-indicator` sets are **deprecated in favour of this merged
//     one**; they are the only source of the four-color rows, so those four
//     roles are carried here with that provenance.
//   * The `thick.*` rows are **deprecated as a variant** — "no longer
//     tokenized as a variant, but rather a sample configuration in code".
//     They are transcribed for completeness, not exposed as an API variant.
//   * The **wave rows are published, non-deprecated Expressive tokens**
//     (amplitude / wavelength, and the with-wave container sizes). The
//     material-web component predates Expressive and renders none of them;
//     this port carries the rows and registers wave *rendering* as the
//     family's open gap in docs/porting-todo.md (the Compose source is
//     `WavyLinearProgressIndicator` / `WavyCircularProgressIndicator`).
//   * The export publishes **no state rows at all** — a progress indicator is
//     not interactive: no hover, no press, no focus indicator, no disabled
//     row. Like Badge, the whole interaction contract is "nothing happens".

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// Everything needed to paint and lay out one progress indicator.
struct QT_MD3_EXPORT MdProgressIndicatorTokens
{
    // --- colours (md.comp.progress-indicator.*) -----------------------------
    /// `active-indicator.color`.
    ColorRole activeIndicatorColor = ColorRole::Primary;
    /// `stop-indicator.color` — the dot that trails the active indicator with
    /// the published gap; the linear shape only.
    ColorRole stopIndicatorColor = ColorRole::Primary;
    /// `track.color`.
    ColorRole trackColor = ColorRole::SecondaryContainer;

    // --- shapes (md.comp.progress-indicator.*) ------------------------------
    /// All three shape rows publish `corner-full` — rounded bar ends and
    /// round stroke caps.
    ShapeCorner activeIndicatorShape = ShapeCorner::Full;
    ShapeCorner stopIndicatorShape = ShapeCorner::Full;
    ShapeCorner trackShape = ShapeCorner::Full;

    // --- four-color (deprecated per-shape sets; kept for the fourColor API) --
    /// `four-color-active-indicator.{one,two,three,four}.color`, transcribed
    /// from the deprecated `_md-comp-{linear,circular}-progress-indicator`
    /// sets (identical in both). Both shapes publish the same four roles:
    /// primary, primary-container, tertiary, tertiary-container.
    ColorRole fourColorOne = ColorRole::Primary;
    ColorRole fourColorTwo = ColorRole::PrimaryContainer;
    ColorRole fourColorThree = ColorRole::Tertiary;
    ColorRole fourColorFour = ColorRole::TertiaryContainer;

    // --- deprecated base metrics (md.comp.progress-indicator.*) --------------
    /// The base set's four metric rows, each individually `@deprecated` in
    /// favour of the shape-qualified rows that supersede them. Transcribed
    /// for completeness; the shape rows are what the painter reads.
    qreal activeIndicatorTrackSpace = 4.0;
    qreal baseActiveIndicatorThickness = 4.0;
    qreal baseStopIndicatorSize = 4.0;
    qreal baseTrackThickness = 4.0;

    // --- linear (md.comp.progress-indicator.linear.*) ------------------------
    /// `linear.height` — the container height (track and active indicator
    /// agree at 4 px; the separate thickness rows are also carried).
    qreal linearHeight = 4.0;
    qreal linearActiveIndicatorThickness = 4.0;
    qreal linearTrackThickness = 4.0;
    /// `linear.track-active-indicator-space` — the gap between the active
    /// indicator's end and the stop indicator.
    qreal linearTrackActiveIndicatorSpace = 4.0;
    /// `linear.stop-indicator.size` and `.trailing-space` (0 px — the stop
    /// indicator is followed immediately by the track remainder).
    qreal linearStopIndicatorSize = 4.0;
    qreal linearStopIndicatorTrailingSpace = 0.0;
    // Wave rows: published Expressive tokens, carried but not rendered —
    // see the file comment and docs/porting-todo.md.
    qreal linearWaveAmplitude = 3.0;
    qreal linearWaveWavelength = 40.0;
    qreal linearIndeterminateWaveWavelength = 20.0;
    qreal linearWithWaveHeight = 10.0;
    // Deprecated `linear.thick.*` rows — "no longer tokenized as a variant,
    // but rather a sample configuration in code".
    qreal linearThickHeight = 8.0;
    qreal linearThickActiveIndicatorThickness = 8.0;
    qreal linearThickTrackThickness = 8.0;
    qreal linearThickTrackActiveIndicatorSpace = 4.0;
    qreal linearThickStopIndicatorSize = 4.0;
    qreal linearThickStopIndicatorTrailingSpace = 2.0;
    qreal linearThickWithWaveHeight = 14.0;

    // --- circular (md.comp.progress-indicator.circular.*) --------------------
    qreal circularSize = 40.0;
    qreal circularActiveIndicatorThickness = 4.0;
    qreal circularTrackThickness = 4.0;
    qreal circularTrackActiveIndicatorSpace = 4.0;
    qreal circularWaveAmplitude = 1.6;
    qreal circularWaveWavelength = 15.0;
    qreal circularWithWaveSize = 48.0;
    // Deprecated `circular.thick.*` rows — same provenance as the linear ones.
    qreal circularThickSize = 52.0;
    qreal circularThickActiveIndicatorThickness = 8.0;
    qreal circularThickTrackThickness = 8.0;
    qreal circularThickTrackActiveIndicatorSpace = 4.0;

    /// Build the published set for one shape (the base rows are shared).
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace, the
    /// shape-qualified key winning over the base one:
    ///
    ///   md.comp.progress-indicator.linear.height         (linear, wins)
    ///   md.comp.progress-indicator.height                (base fallback)
    ///
    /// Colours are not overridable — same rule as the button families,
    /// recorded in docs/porting-todo.md. Shape values accept both spellings.
    static MdProgressIndicatorTokens resolve(
        ProgressIndicatorShape shape, const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_PROGRESS_INDICATOR_TOKENS_H
