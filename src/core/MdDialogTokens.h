#ifndef MD_DIALOG_TOKENS_H
#define MD_DIALOG_TOKENS_H

// MdDialogTokens — the `md.comp.dialog.*` export (34.0.21).
//
// One published set, no variants. The container is surface-container-high at
// elevation level 3 with the extra-large corner, the headline is
// headline-small in on-surface, the supporting text is body-medium in
// on-surface-variant, the optional icon is 24 px in secondary and the
// optional action labels are label-large in primary. The deprecated
// `with-divider.*` rows are still published (1 px, outline) and transcribed
// for completeness; the subhead.* rows are deprecated aliases of the
// headline.* rows and are NOT carried (the headline is the live taxonomy).
//
// The export publishes hover / focus / pressed rows for the action
// label-text + state-layer, each at the standard md.sys.state opacities.
// They are transcribed into `MdDialogTokens` as the family's own record, and
// the ripple colour follows the project's "ripple = pressed state layer"
// rule from the pressed row.
//
// Layout constants the export does NOT publish (Compose AlertDialog.kt /
// AlertDialogDefaults private vals, transcribed and labelled): the 24 px
// all-around container padding, the text's extra 24 px bottom padding, the
// icon's and title's 16 px bottom padding, the 8 px action spacing, the
// 280..560 width clamp and the 0.32 scrim opacity (md.sys.color.scrim, not a
// dialog row). Compose's "precision pointer component sizing" flag swaps the
// paddings to 20/16 and the title to 20 sp — flag-gated upstream and NOT
// ported (recorded in docs/porting-todo.md).

#include "MdTypes.h"
#include "MdTokens.h"
#include "QtMd3Export.h"

namespace md {

/// The interactive states the export publishes action rows for.
enum class MdDialogActionState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Count,
};

/// One state row of the action label: the content colour and the state layer
/// painted on the action's region.
struct QT_MD3_EXPORT MdDialogActionRow
{
    ColorRole content = ColorRole::Count;
    ColorRole stateLayer = ColorRole::Count;
    qreal stateLayerOpacity = 0.0;
};

struct QT_MD3_EXPORT MdDialogTokens
{
    // --- container: md.comp.dialog.container.* -----------------------------
    ColorRole containerColor = ColorRole::SurfaceContainerHigh;
    ElevationLevel containerElevation = ElevationLevel::Level3;
    ColorRole containerShadowColor = ColorRole::Shadow;
    ShapeCorner containerShape = ShapeCorner::ExtraLarge;

    // --- headline: md.comp.dialog.headline.* --------------------------------
    TypeStyle headlineStyle = TypeStyle::HeadlineSmall;
    ColorRole headlineColor = ColorRole::OnSurface;

    // --- supporting text: md.comp.dialog.supporting-text.* ------------------
    TypeStyle supportingTextStyle = TypeStyle::BodyMedium;
    ColorRole supportingTextColor = ColorRole::OnSurfaceVariant;

    // --- icon: md.comp.dialog.with-icon.icon.* ------------------------------
    qreal iconSize = 24.0;
    ColorRole iconColor = ColorRole::Secondary;

    // --- action: md.comp.dialog.action.label-text.* -------------------------
    TypeStyle actionLabelStyle = TypeStyle::LabelLarge;
    ColorRole actionLabelColor = ColorRole::Primary;

    // --- divider (deprecated, still published) ------------------------------
    qreal dividerHeight = 1.0;
    ColorRole dividerColor = ColorRole::Outline;

    // --- state rows ---------------------------------------------------------
    MdDialogActionRow actionRow(MdDialogActionState state) const;

    // --- resolution ---------------------------------------------------------
    /// The resolved `md.comp.dialog.*` set, after the application-wide and
    /// per-instance `md.comp.*` overrides (lengths and the container shape).
    static MdDialogTokens resolve(const MdComponentTokens *overrides = nullptr);

    // --- Compose-port layout constants (not export rows) --------------------
    /// AlertDialogDefaults.dialogPadding — the all-around container padding.
    static constexpr qreal kContainerPadding = 24.0;
    /// AlertDialogDefaults.textPadding — the supporting text's extra bottom.
    static constexpr qreal kTextBottomPadding = 24.0;
    /// IconPadding — the icon slot's bottom padding.
    static constexpr qreal kIconBottomPadding = 16.0;
    /// TitlePadding — the title's bottom padding.
    static constexpr qreal kTitleBottomPadding = 16.0;
    /// ButtonsMainAxisSpacing / ButtonsCrossAxisSpacing.
    static constexpr qreal kActionsSpacing = 8.0;
    /// DialogMinWidth.
    static constexpr qreal kMinWidth = 280.0;
    /// DialogMaxWidth.
    static constexpr qreal kMaxWidth = 560.0;
    /// md.sys.color.scrim — the dimming layer's opacity (black at 32%). Not a
    /// dialog row; the export has no scrim namespace.
    static constexpr qreal kScrimOpacity = 0.32;

    /// The widget's shadow headroom on every side — same idiom as the
    /// snackbar: MdElevation's level-3 rings spread up to 3 px sideways /
    /// 6 px downward and a Qt child widget clips its own rect, so the widget
    /// rectangle is the container grown by this margin. Not a token row.
    static constexpr qreal kShadowMargin = 8.0;
};

} // namespace md

#endif // MD_DIALOG_TOKENS_H
