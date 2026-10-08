#ifndef MD_BADGE_TOKENS_H
#define MD_BADGE_TOKENS_H

// The published `md.comp.badge.*` token set, resolved for one form.
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-badge.scss             (all rows)
//
// Token export version 34.0.21.
//
// Three facts of this family worth keeping visible:
//
//   * material-web **does not implement the Badge component** — there is no
//     `packages/badge` — so the token file above is the only material-web
//     fact. Every behaviour row (anchoring offsets, the 4 px label padding,
//     non-interactivity) comes from androidx Compose Material3 Badge.kt and
//     is annotated [compose] where it is used. Recorded in
//     docs/porting-todo.md.
//   * The export publishes **no state rows at all**: a badge is not
//     interactive — no hover, no press, no focus indicator, no disabled row.
//     It never takes focus and never paints a state layer or a ripple. This
//     is the first Actions/Communication family with an empty state table,
//     and it is a fact, not a gap.
//   * The two forms are one set: the 6 px dot (`size`) and the content form
//     (`large.*`, minimum 16 px). Which form a badge takes is decided by
//     whether it has text, not by a variant token.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// Everything needed to paint and lay out one badge.
struct QT_MD3_EXPORT MdBadgeTokens
{
    // --- metrics ----------------------------------------------------------
    /// `md.comp.badge.size` — the dot form, no content.
    qreal size = 6.0;
    /// `md.comp.badge.large.size` — the *minimum* of the content form; the
    /// label may widen and (with a CJK line height) heighten the pill.
    qreal largeSize = 16.0;

    // --- shape (md.comp.badge[.large].shape) -------------------------------
    /// Both forms are `corner-full`: a circle when square, a pill otherwise.
    ShapeCorner shape = ShapeCorner::Full;
    ShapeCorner largeShape = ShapeCorner::Full;

    // --- colours -----------------------------------------------------------
    /// `md.comp.badge.color` / `md.comp.badge.large.color`.
    ColorRole color = ColorRole::Error;
    ColorRole largeColor = ColorRole::Error;
    /// `md.comp.badge.large.label-text.color`.
    ColorRole largeLabelTextColor = ColorRole::OnError;

    // --- typography --------------------------------------------------------
    /// `md.comp.badge.large.label-text.*` — the label-small style
    /// (11 px / 16 px line height / 0.5 px tracking / weight 500).
    TypeStyle largeLabelTextType = TypeStyle::LabelSmall;

    /// Build the published set for one form. `large` is the content form.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace, the
    /// more qualified `large.*` key winning over the base one:
    ///
    ///   md.comp.badge.large.size   (large form, wins)
    ///   md.comp.badge.size         (dot form, and the large fallback)
    ///
    /// Colours are not overridable — same rule as the button families,
    /// recorded in docs/porting-todo.md. Shape values accept both spellings
    /// ("full" / "corner-full").
    static MdBadgeTokens resolve(bool large, const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_BADGE_TOKENS_H
