#ifndef MD_SNACKBAR_TOKENS_H
#define MD_SNACKBAR_TOKENS_H

// MdSnackbarTokens — the `md.comp.snackbar.*` export (34.0.21).
//
// One published set, no variants. The container is inverse-surface at
// elevation level 3 with the extra-small corner, supporting text is
// body-medium in inverse-on-surface, the optional action label is label-large
// in inverse-primary and the optional dismiss icon is 24 px in
// inverse-on-surface — the inverse roles are what makes a snackbar readable
// on any surface without a variant system.
//
// The export publishes hover / focus / pressed rows for BOTH interactive
// elements (action label-text + state-layer, icon + state-layer), each at the
// standard md.sys.state opacities; they are the only state rows the family
// has, and the ripple colours follow the project's "ripple = pressed state
// layer" rule from them.
//
// Layout constants the export does NOT publish (Compose Snackbar.kt private
// vals, transcribed and labelled): container max width 600, horizontal
// spacing 16, the text-end extra spacing 8, the text's 14 px vertical
// padding, the 30 px first-line height of a wrapped one-row snackbar and the
// new-line action's 4 px bottom padding. The one-row height behaviour is the
// export's own: max(48, content) for a single line, first-line top at 30 px
// and max(68, …) when the message wraps.
//
// The `container.color` row is deprecated upstream (34.0.21 keeps publishing
// it; the merged inverse-surface value is unchanged), so it is transcribed
// here and read like any other row.

#include "MdTypes.h"
#include "MdTokens.h"
#include "QtMd3Export.h"

namespace md {

/// The interactive states the export publishes rows for, per element.
enum class MdSnackbarState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Count,
};

/// Which of the two interactive elements a state row describes.
enum class MdSnackbarElement {
    Action,
    Icon,
    Count,
};

/// One state row of one element: the content colour (label-text or icon) and
/// the state layer painted on the element's region.
struct QT_MD3_EXPORT MdSnackbarElementRow
{
    ColorRole content = ColorRole::Count;
    ColorRole stateLayer = ColorRole::Count;
    qreal stateLayerOpacity = 0.0;
};

/// Host-side display durations — Compose `SnackbarDuration.toMillis`, the
/// material-web export has no duration rows. `Auto` is the Compose default:
/// Indefinite when the snackbar has an action (an actionable snackbar must
/// not self-dismiss), Short otherwise.
enum class MdSnackbarDuration {
    Auto,
    Short,
    Long,
    Indefinite,
    Count,
};

struct QT_MD3_EXPORT MdSnackbarTokens
{
    // --- container: md.comp.snackbar.container.* ---------------------------
    ColorRole containerColor = ColorRole::InverseSurface;
    ElevationLevel containerElevation = ElevationLevel::Level3;
    ColorRole containerShadowColor = ColorRole::Shadow;
    ShapeCorner containerShape = ShapeCorner::ExtraSmall;
    /// `with-single-line.container.height`.
    qreal singleLineHeight = 48.0;
    /// `with-two-lines.container.height` — the minimum for a wrapped message.
    qreal twoLinesHeight = 68.0;

    // --- supporting text: md.comp.snackbar.supporting-text.* ---------------
    TypeStyle supportingTextStyle = TypeStyle::BodyMedium;
    ColorRole supportingTextColor = ColorRole::InverseOnSurface;

    // --- icon: md.comp.snackbar.icon.* --------------------------------------
    qreal iconSize = 24.0;
    ColorRole iconColor = ColorRole::InverseOnSurface;

    // --- action: md.comp.snackbar.action.label-text.* -----------------------
    TypeStyle actionLabelStyle = TypeStyle::LabelLarge;
    ColorRole actionLabelColor = ColorRole::InversePrimary;

    // --- state rows ---------------------------------------------------------
    MdSnackbarElementRow actionRow(MdSnackbarState state) const;
    MdSnackbarElementRow iconRow(MdSnackbarState state) const;

    // --- resolution ---------------------------------------------------------
    /// The resolved `md.comp.snackbar.*` set, after the application-wide and
    /// per-instance `md.comp.*` overrides (lengths and the container shape).
    static MdSnackbarTokens resolve(const MdComponentTokens *overrides = nullptr);

    // --- Compose-port layout constants (not export rows) --------------------
    /// ContainerMaxWidth — a snackbar never grows past this.
    static constexpr qreal kContainerMaxWidth = 600.0;
    /// HorizontalSpacing — the text's inline-start inset.
    static constexpr qreal kHorizontalSpacing = 16.0;
    /// TextEndExtraSpacing — extra room after the text when no dismiss icon
    /// follows it (one-row layout).
    static constexpr qreal kTextEndExtraSpacing = 8.0;
    /// SnackbarVerticalPadding — the text box's vertical padding (one-row).
    static constexpr qreal kTextVerticalPadding = 14.0;
    /// HeightToFirstLine — a wrapped one-row message starts 30 px from the top.
    static constexpr qreal kHeightToFirstLine = 30.0;
    /// ActionButtonBottomPadding — the new-line action row sits 4 px above the
    /// container's bottom edge.
    static constexpr qreal kActionButtonBottomPadding = 4.0;
    /// HorizontalSpacingButtonSide — the new-line action row's end inset when
    /// no dismiss icon follows it.
    static constexpr qreal kHorizontalSpacingButtonSide = 8.0;
    /// Compose SnackbarHost's 12 dp padding around the snackbar.
    static constexpr qreal kHostMargin = 12.0;

    /// The widget's shadow headroom on every side: MdElevation's level-3
    /// rings spread up to 3 px sideways / 6 px downward, and a Qt child widget
    /// clips its own rect — so the widget rectangle is the container grown by
    /// this margin and the container itself sits inset. Not a token row; the
    /// export expresses the same thing by drawing outside its bounds.
    static constexpr qreal kShadowMargin = 8.0;

    /// Duration in ms (`SnackbarDuration.toMillis`); Auto resolves like Compose's
    /// caller-side default. `hasAction` decides the Auto branch.
    static qint64 durationMs(MdSnackbarDuration duration, bool hasAction);
};

} // namespace md

#endif // MD_SNACKBAR_TOKENS_H
