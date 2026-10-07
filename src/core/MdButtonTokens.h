#ifndef MD_BUTTON_TOKENS_H
#define MD_BUTTON_TOKENS_H

// The published `md.comp.button.*` token set, resolved for one
// (variant, size, shape) combination.
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-button.scss            (base)
//     tokens/versions/latest/sass/_md-comp-button-<style>.scss    (5 variants)
//     tokens/versions/latest/sass/_md-comp-button-<size>.scss     (5 sizes)
//
// Token export version 34.0.21. The keys are carried through verbatim so a
// reader can grep the upstream file for any field.
//
// Why `latest` and not the v0_192 export the rest of the token layer is pinned
// to: the five-step button size scale is an M3 Expressive addition, and v0_192
// publishes exactly one button height with no size sets at all. The divergence
// that opens up is recorded in docs/porting-todo.md rather than papered over.
//
// Two upstream inconsistencies are deliberately *not* smoothed out here; they
// are recorded in the same document and pinned by TestMd3Button:
//
//   * `md.comp.button.leading-space` is 24px while
//     `md.comp.button.small.leading-space` is 16px for the same 40px height.
//     The size-specific set wins, because a caller who asked for `Small`
//     should get what `md.comp.button.small` says.
//   * the container/shadow roles are carried as ColorRole values rather than
//     resolved colours, so that a theme change re-resolves them without the
//     token table having to be rebuilt.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace md {

/// One colour slot of a button.
///
/// A slot may be *absent*, which is how the published table records "this
/// variant does not paint this part": a text button has no container, a filled
/// button has no outline, an outlined button has no container. Absence is a
/// real value here, not a zero — painting a transparent rectangle instead
/// would still consume an anti-aliased edge.
struct QT_MD3_EXPORT MdButtonColourSlot
{
    /// `ColorRole::Count` means "the variant does not paint this part".
    ColorRole role = ColorRole::Count;
    /// Multiplier applied on top of the role's colour, used for the disabled
    /// opacities (`md.comp.button.disabled.*.opacity`).
    qreal opacity = 1.0;

    bool isPresent() const { return role != ColorRole::Count; }
};

/// The resolved colours for one interaction state.
struct QT_MD3_EXPORT MdButtonStateColours
{
    /// `container.color`. Absent for the container-less variants.
    MdButtonColourSlot container;
    /// `label-text.color`. Present in every published state.
    MdButtonColourSlot labelText;
    /// `icon.color`. Present in every published state.
    MdButtonColourSlot icon;
    /// `state-layer.color`. The *opacity* is `md.sys.state.*` and is therefore
    /// not stored per component — see MdStateLayer.
    ColorRole stateLayer = ColorRole::Count;
    /// `outline.color`. Only the outlined variant publishes one.
    ColorRole outline = ColorRole::Count;
    /// `container.elevation`.
    ///
    /// The `hovered` elevation is marked `@deprecated No longer part of the
    /// design spec` upstream but is still published, so it is honoured; see
    /// docs/porting-todo.md.
    ElevationLevel elevation = ElevationLevel::Level0;

    bool paintsContainer() const { return container.isPresent(); }
    bool paintsOutline() const { return outline != ColorRole::Count; }
};

/// The five states a button's token table is published for.
enum class MdButtonState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Disabled,
    Count,
};

/// Everything needed to paint and lay out one button.
struct QT_MD3_EXPORT MdButtonTokens
{
    // --- metrics (md.comp.button.<size>.*) -------------------------------
    qreal containerHeight = 40.0;
    qreal leadingSpace = 16.0;
    qreal trailingSpace = 16.0;
    qreal iconLabelSpace = 8.0;
    qreal iconSize = 20.0;
    qreal outlineWidth = 1.0;

    // --- shape (container.shape.<shape> / pressed.container.shape) --------
    ShapeCorner restingShape = ShapeCorner::Full;
    ShapeCorner pressedShape = ShapeCorner::Small;

    // --- press shape morph
    //     (pressed.container.corner-size.motion.spring.*) -----------------
    qreal springStiffness = 1400.0;
    qreal springDampingRatio = 0.9;

    // --- type (label-text) ------------------------------------------------
    TypeStyle labelStyle = TypeStyle::LabelLarge;

    // --- disabled opacities -----------------------------------------------
    qreal disabledContainerOpacity = 0.10;
    qreal disabledLabelOpacity = 0.38;
    qreal disabledIconOpacity = 0.38;

    // --- focus indicator (focus.indicator.*) ------------------------------
    ColorRole focusIndicator = ColorRole::Secondary;
    qreal focusIndicatorThickness = 3.0;
    qreal focusIndicatorOffset = 2.0;

    // --- colours ----------------------------------------------------------
    MdButtonStateColours enabled;
    MdButtonStateColours hovered;
    MdButtonStateColours focused;
    MdButtonStateColours pressed;
    MdButtonStateColours disabled;

    /// The colours for one state. Never returns a dangling reference.
    const MdButtonStateColours &state(MdButtonState which) const;

    /// Resting corner token and the radius it resolves to for a box of `size`.
    ShapeCorner restingShapeToken() const { return restingShape; }
    ShapeCorner pressedShapeToken() const { return pressedShape; }

    /// Build the published set for one combination.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace, with the
    /// size-qualified key winning over the generic one:
    ///
    ///   md.comp.button.<size>.container.height   (wins)
    ///   md.comp.button.container.height          (fallback)
    ///   published value                          (default)
    ///
    /// Only the numeric and shape tokens are overridable. Colours are not:
    /// MD3 theming re-seeds the colour scheme, which every role here follows,
    /// so a component-local colour override would be a second, competing
    /// theming mechanism. Recorded in docs/porting-todo.md.
    ///
    /// Shape values accept both spellings of the token name — "full" and
    /// "corner-full" — because the key already says `.shape.`.
    static MdButtonTokens resolve(ButtonVariant variant,
                                  ButtonSize size,
                                  ButtonShape shape,
                                  const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_BUTTON_TOKENS_H
