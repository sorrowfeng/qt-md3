#ifndef MD_CARD_TOKENS_H
#define MD_CARD_TOKENS_H

// The published card token sets, resolved for one variant:
//
//   variant  md.comp.filled-card.* | elevated-card.* | outlined-card.*
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-filled-card.scss
//     tokens/versions/latest/sass/_md-comp-elevated-card.scss
//     tokens/versions/latest/sass/_md-comp-outlined-card.scss
//     tokens/versions/latest/sass/_md-sys-state*.scss (opacities, focus
//     indicator)
//
// Token export version 34.0.21.
//
// material-web publishes no card *web component* (token export only — the
// same situation as the loading indicator), so the behaviour source is
// Compose M3's Card.kt. Facts of the export worth keeping visible:
//
//   * All three variants share one container shape (corner-medium) and the
//     state-layer colour on-surface; the variants differ in the container
//     colour and the elevation ladder.
//   * Elevation is where the states differ. Pressed does NOT raise in this
//     export (the "Pressed (ripple)" rows repeat the enabled elevation) —
//     the press response is the ripple. Dragged climbs highest (level3 /
//     level4 for elevated).
//   * Unlike the FAB export, the card export DOES publish disabled rows:
//     the container (filled: surface-variant, elevated: surface, outlined:
//     surface unchanged) and — outlined only — the outline. Compose applies
//     `DisabledContainerOpacity` (0.38) by compositing the disabled colour
//     *over the enabled container colour*; the constants below carry that
//     arithmetic, and the outlined export publishes a separate outline
//     opacity (0.12).
//   * Content colour is not a token row: Compose derives it through
//     `contentColorFor(containerColor)`, which for every surface role here
//     resolves to on-surface. Disabled content drops to
//     `DisabledAlpha` (0.38).
//   * Outlined quirk recorded in docs/porting-todo.md: Compose composites
//     the *disabled* outline over ElevatedCardTokens.ContainerColor rather
//     than the outlined card's own container — almost certainly a bug (an
//     over-eager copy of the elevated-card branch); this port composites
//     over the card's own container colour instead.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// The three published card variants, `md.comp.<variant>-card.*`.
enum class MdCardVariant {
    Filled,
    Elevated,
    Outlined,
    Count,
};

/// The states a card's token table is published for.
enum class MdCardState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Dragged,
    Disabled,
    Count,
};

/// Compose's `DisabledContainerOpacity` / `DisabledAlpha` /
/// `DisabledOutlineOpacity`, applied by the paint code (see the file comment).
inline constexpr qreal kDisabledContainerOpacity = 0.38;
inline constexpr qreal kDisabledContentAlpha = 0.38;
inline constexpr qreal kDisabledOutlineOpacity = 0.12;

/// One interaction state of one variant: what the container, its outline and
/// the state layer paint with, and the elevation the container sits at.
struct QT_MD3_EXPORT MdCardStateRow
{
    /// `container.color`.
    ColorRole container = ColorRole::Count;
    /// The content colour (Compose's `contentColorFor`); on-surface for all
    /// three published containers.
    ColorRole content = ColorRole::Count;
    /// `state-layer.color`. Hover / keyboard-focus / dragged overlay source;
    /// the press overlay is the ripple, not this layer.
    ColorRole stateLayer = ColorRole::Count;
    /// `container.elevation` for this state.
    ElevationLevel elevation = ElevationLevel::Level0;
    /// Outline colour — outlined variant only, `ColorRole::Count` otherwise.
    ColorRole outline = ColorRole::Count;
};

/// The six states across one variant.
struct QT_MD3_EXPORT MdCardFamily
{
    MdCardStateRow enabled;
    MdCardStateRow hovered;
    MdCardStateRow focused;
    MdCardStateRow pressed;
    MdCardStateRow dragged;
    MdCardStateRow disabled;

    const MdCardStateRow &state(MdCardState which) const;
};

/// Everything needed to paint and lay out one card.
struct QT_MD3_EXPORT MdCardTokens
{
    // --- shape (md.comp.<variant>-card.container.shape) --------------------
    ShapeCorner containerShape = ShapeCorner::Medium;

    // --- outline (md.comp.outlined-card.outline.width) ----------------------
    /// Published by the outlined export only; the other variants ignore it.
    qreal outlineWidth = 1.0;

    // --- icon (md.comp.<variant>-card.icon.size) ----------------------------
    /// The leading-icon slot size; a card paints no icon itself, but the
    /// token exists and content layouts may want it.
    qreal iconSize = 24.0;

    // --- focus indicator (md-sys-state-focus-indicator) ---------------------
    ColorRole focusIndicator = ColorRole::Secondary;
    qreal focusIndicatorThickness = 3.0;
    qreal focusIndicatorOffset = 2.0;

    // --- colours + elevation ------------------------------------------------
    MdCardFamily family;

    /// Build the published set for one variant.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace, with
    /// the more qualified key winning (the same convention the FAB uses).
    /// Colours are not overridable — same rule as the other families,
    /// recorded in docs/porting-todo.md. Shape values accept both spellings
    /// ("full" / "corner-full").
    static MdCardTokens resolve(MdCardVariant variant,
                                const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_CARD_TOKENS_H
