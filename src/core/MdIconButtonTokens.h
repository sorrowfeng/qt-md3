#ifndef MD_ICON_BUTTON_TOKENS_H
#define MD_ICON_BUTTON_TOKENS_H

// The published `md.comp.icon-button.*` token set, resolved for one
// (variant, size, shape, space-track, selected) combination.
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-icon-button.scss           (base,
//                                                                      = filled)
//     tokens/versions/latest/sass/_md-comp-icon-button-<style>.scss   (4 styles)
//     tokens/versions/latest/sass/_md-comp-icon-button-<size>.scss    (5 sizes)
//
// Token export version 34.0.21 — the same `latest` export the button and
// button-group components read, for the same reason: the five-step size scale
// is Expressive-only and does not exist in the v0_192 export `MdTokens` pins.
//
// The colour set is *three families*, not one:
//
//   plain       the enabled/hovered/focused/pressed/disabled tokens — what a
//               non-toggle icon button paints.
//   selected    the `selected-<state>` tokens — what a toggle shows while
//               checked.
//   unselected  the `unselected-<state>` tokens — what a toggle shows while
//               unchecked.
//
// The families are published incompletely on purpose (the base file carries
// `unselected-*` for every state but the style files only override what
// changes), so resolve() falls back family -> plain, state by state, and the
// fallback order is part of this table's contract: a missing plain slot means
// "this variant does not paint this part", while a missing selected slot means
// "same as the plain one".

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace md {

/// One colour slot of an icon button. Same shape as the button's slot: a role
/// plus an opacity multiplier, where "absent" is a real value.
struct QT_MD3_EXPORT MdIconButtonColourSlot
{
    /// `ColorRole::Count` means "this family/state does not paint this part".
    ColorRole role = ColorRole::Count;
    qreal opacity = 1.0;

    bool isPresent() const { return role != ColorRole::Count; }
};

/// The colours one interaction state paints with.
struct QT_MD3_EXPORT MdIconButtonStateColours
{
    /// `container.color`. Absent for standard and for the unselected outlined
    /// toggle.
    MdIconButtonColourSlot container;
    /// `icon.color`. Present whenever the icon is visible at all.
    MdIconButtonColourSlot icon;
    /// `state-layer.color`.
    ColorRole stateLayer = ColorRole::Count;
    /// `outline.color`. Only the outlined style publishes one.
    ColorRole outline = ColorRole::Count;

    bool paintsContainer() const { return container.isPresent(); }
    bool paintsOutline() const { return outline != ColorRole::Count; }
};

/// The five states an icon button's token table is published for. The same
/// enumeration the common button uses, because the states are the same states.
enum class MdIconButtonState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Disabled,
    Count,
};

/// One colour family across all five states.
struct QT_MD3_EXPORT MdIconButtonFamily
{
    MdIconButtonStateColours enabled;
    MdIconButtonStateColours hovered;
    MdIconButtonStateColours focused;
    MdIconButtonStateColours pressed;
    MdIconButtonStateColours disabled;

    const MdIconButtonStateColours &state(MdIconButtonState which) const;
};

/// Everything needed to paint and lay out one icon button.
struct QT_MD3_EXPORT MdIconButtonTokens
{
    // --- metrics (md.comp.icon-button.<size>.*) --------------------------
    qreal containerHeight = 40.0;
    qreal iconSize = 24.0;
    /// `<track>-leading-space` / `-trailing-space` for the three published
    /// tracks. The default track makes the container exactly square at every
    /// size (leading + icon + trailing == height); narrow and wide are real
    /// published sets with their own numbers.
    qreal defaultLeadingSpace = 8.0;
    qreal defaultTrailingSpace = 8.0;
    qreal narrowLeadingSpace = 4.0;
    qreal narrowTrailingSpace = 4.0;
    qreal wideLeadingSpace = 14.0;
    qreal wideTrailingSpace = 14.0;
    /// `outlined-outline-width`, published by every size set (not only the
    /// outlined style) because it is a size fact, not a colour fact.
    qreal outlineWidth = 1.0;

    // --- shapes -----------------------------------------------------------
    /// `container.shape.round` / `.square` — the resting corners of the two
    /// shape knobs.
    ShapeCorner roundShape = ShapeCorner::Full;
    ShapeCorner squareShape = ShapeCorner::Medium;
    /// `pressed.container.shape` — where the spring morphs the corners to.
    ShapeCorner pressedShape = ShapeCorner::Small;
    /// `selected-container.shape.round` / `.square` — the *other* knob, which
    /// is how the spec's "selected shape changes between square and round"
    /// behaviour is published: selected-round is the square corner and
    /// selected-square is full.
    ShapeCorner selectedRoundShape = ShapeCorner::Medium;
    ShapeCorner selectedSquareShape = ShapeCorner::Full;

    // --- press shape morph
    //     (pressed.container.corner-size.motion.spring.*) -----------------
    qreal springStiffness = 1400.0;
    qreal springDampingRatio = 0.9;

    // --- focus indicator --------------------------------------------------
    ColorRole focusIndicator = ColorRole::Secondary;
    qreal focusIndicatorThickness = 3.0;
    qreal focusIndicatorOffset = 2.0;

    // --- colours ----------------------------------------------------------
    MdIconButtonFamily plain;
    MdIconButtonFamily selected;
    MdIconButtonFamily unselected;

    /// The family painting should use for `toggleable && selected`.
    const MdIconButtonFamily &familyFor(bool toggleable, bool selectedNow) const;

    /// Build the published set for one combination.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace, with
    /// the more qualified key winning:
    ///
    ///   md.comp.icon-button.<size>.container.height        (wins)
    ///   md.comp.icon-button.container.height               (fallback)
    ///   published value                                    (default)
    ///
    /// Colours are not overridable — same rule as the common button, recorded
    /// in docs/porting-todo.md. Shape values accept both spellings ("full" /
    /// "corner-full").
    static MdIconButtonTokens resolve(IconButtonVariant variant,
                                      ButtonSize size,
                                      ButtonShape shape,
                                      IconButtonSpaceTrack track,
                                      const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_ICON_BUTTON_TOKENS_H
