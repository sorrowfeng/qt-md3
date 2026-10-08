#ifndef MD_TOOLTIP_TOKENS_H
#define MD_TOOLTIP_TOKENS_H

// MdTooltipTokens — the `md.comp.plain-tooltip.*` and
// `md.comp.rich-tooltip.*` exports (34.0.21), the Communication family's
// closing pair.
//
// material-web ships only the token exports for this family (no component
// source in the current repository), so the visuals come straight from the
// SCSS and the behaviour from Compose M3's Tooltip.kt — the same split as the
// loading indicator, recorded in docs/porting-todo.md.
//
// Plain tooltip: an inverse-surface container with the extra-small corner and
// NO elevation, carrying a body-small supporting text in inverse-on-surface.
// Rich tooltip: a surface-container container at elevation level 2 with the
// medium corner, a title-small subhead and a body-medium supporting text both
// in on-surface-variant, and a label-large action in primary.
//
// The rich tooltip's action is the family's only interactive element and the
// export publishes hover / focus / pressed rows for it (label-text stays
// primary, the state layer is primary at the md.sys.state opacities); the
// ripple colour follows the pressed row per the project's rule.
//
// The layout constants below are Compose Tooltip.kt private vals — neither
// export publishes layout rows. `kDurationMs` is
// BasicTooltipDefaults.TooltipDuration: a NON-persistent tooltip self-dismisses
// after 1.5 s — unless it was shown by mouse hover (Compose's UserInput
// priority skips the timeout; it dismisses when the pointer leaves instead).

#include "MdTypes.h"
#include "MdTokens.h"
#include "QtMd3Export.h"

namespace md {

/// The two families. Compose ships them as two composables (PlainTooltip /
/// RichTooltip) sharing one anchor machinery.
enum class MdTooltipVariant {
    Plain,
    Rich,
    Count,
};

/// The rich action's interaction states — the export's state-row axis.
enum class MdTooltipActionState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Count,
};

/// One state row of the rich action: the label-text colour and the state
/// layer painted on the action's region.
struct QT_MD3_EXPORT MdTooltipActionRow
{
    ColorRole content = ColorRole::Count;
    ColorRole stateLayer = ColorRole::Count;
    qreal stateLayerOpacity = 0.0;
};

/// Where the caret protrudes: Bottom when the tooltip sits above the anchor
/// (pointing down at it), Top when below. Shared by the widget and the style
/// like the other component-wide enums.
enum class MdTooltipCaretSide
{
    None,
    Top,
    Bottom,
    Count,
};

struct QT_MD3_EXPORT MdTooltipTokens
{
    // --- plain: md.comp.plain-tooltip.* -------------------------------------
    ColorRole plainContainerColor = ColorRole::InverseSurface;
    ShapeCorner plainContainerShape = ShapeCorner::ExtraSmall;
    TypeStyle plainTextStyle = TypeStyle::BodySmall;
    ColorRole plainTextColor = ColorRole::InverseOnSurface;

    // --- rich: md.comp.rich-tooltip.* ---------------------------------------
    ColorRole richContainerColor = ColorRole::SurfaceContainer;
    ElevationLevel richContainerElevation = ElevationLevel::Level2;
    ColorRole richContainerShadowColor = ColorRole::Shadow;
    ShapeCorner richContainerShape = ShapeCorner::Medium;
    TypeStyle richSubheadStyle = TypeStyle::TitleSmall;
    ColorRole richSubheadColor = ColorRole::OnSurfaceVariant;
    TypeStyle richTextStyle = TypeStyle::BodyMedium;
    ColorRole richTextColor = ColorRole::OnSurfaceVariant;
    TypeStyle richActionLabelStyle = TypeStyle::LabelLarge;
    ColorRole richActionLabelColor = ColorRole::Primary;

    // --- state rows (rich action) -------------------------------------------
    MdTooltipActionRow actionRow(MdTooltipActionState state) const;

    // --- resolution ---------------------------------------------------------
    /// The resolved `md.comp.*` set for `variant`, after the application-wide
    /// and per-instance `md.comp.*` overrides (the container shapes).
    static MdTooltipTokens resolve(MdTooltipVariant variant,
                                   const MdComponentTokens *overrides = nullptr);

    // --- Compose-port layout constants (not export rows) --------------------
    /// TooltipMinWidth / TooltipMinHeight — shared by both composables.
    static constexpr qreal kMinWidth = 40.0;
    static constexpr qreal kMinHeight = 24.0;
    /// TooltipDefaults.plainTooltipMaxWidth.
    static constexpr qreal kMaxPlainWidth = 200.0;
    /// TooltipDefaults.richTooltipMaxWidth.
    static constexpr qreal kMaxRichWidth = 320.0;
    /// PlainTooltipContentPadding — horizontal 8, vertical 4.
    static constexpr qreal kPlainHorizontalPadding = 8.0;
    static constexpr qreal kPlainVerticalPadding = 4.0;
    /// RichTooltipHorizontalPadding.
    static constexpr qreal kRichHorizontalPadding = 16.0;
    /// HeightToSubheadFirstLine — the subhead's first baseline sits 28 px from
    /// the container top.
    static constexpr qreal kHeightToSubheadFirstLine = 28.0;
    /// HeightFromSubheadToTextFirstLine — the text's first baseline sits 24 px
    /// below the subhead box.
    static constexpr qreal kHeightFromSubheadToTextFirstLine = 24.0;
    /// TextBottomPadding.
    static constexpr qreal kTextBottomPadding = 16.0;
    /// ActionLabelMinHeight / ActionLabelBottomPadding.
    static constexpr qreal kActionLabelMinHeight = 36.0;
    static constexpr qreal kActionLabelBottomPadding = 8.0;
    /// SpacingBetweenTooltipAndAnchor — the surface keeps this far from the
    /// anchor (the caret overlaps into it; see MdTooltip's caret margin).
    static constexpr qreal kAnchorSpacing = 4.0;
    /// TooltipDefaults.caretSize — 16 x 8.
    static constexpr qreal kCaretWidth = 16.0;
    static constexpr qreal kCaretHeight = 8.0;
    /// BasicTooltipDefaults.TooltipDuration — a non-persistent tooltip
    /// self-dismisses after this, except when mouse hover showed it.
    static constexpr qint64 kDurationMs = 1500;

    /// The rich container's shadow headroom on every side: MdElevation's
    /// level-2 rings spread past the container and a Qt child widget clips its
    /// own rect, so the widget rectangle grows by this margin the same way the
    /// snackbar's does. Not a token row. Plain has no elevation and no margin.
    static constexpr qreal kShadowMargin = 8.0;

    /// The caret protrudes past the container on the anchor-facing side; the
    /// widget rectangle reserves this much there (== kCaretHeight, rounded up
    /// into the shared margin idiom). Divergence recorded: Compose draws the
    /// caret OUTSIDE its surface into the 4 px anchor gap — overlapping the
    /// anchor by caret − spacing — while this port keeps the tip exactly
    /// kAnchorSpacing above (below) the anchor.
    static constexpr qreal kCaretMargin = 8.0;

    /// The action label's interactive region grows by this much around the
    /// text — the text-button chrome, the same convention the snackbar uses.
    static constexpr qreal kActionHitPadding = 12.0;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdTooltipCaretSide)

#endif // MD_TOOLTIP_TOKENS_H
