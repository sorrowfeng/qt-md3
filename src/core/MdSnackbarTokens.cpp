#include "MdSnackbarTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// md.comp.snackbar.action.<state>.{label-text,state-layer}.*
MdSnackbarElementRow actionRowFor(MdSnackbarState state)
{
    // The enabled row carries the colours; the interactive states only swap
    // the state layer in (the label text keeps inverse-primary throughout —
    // the export pins every state's label-text.color to inverse-primary).
    MdSnackbarElementRow row;
    row.content = ColorRole::InversePrimary;
    row.stateLayer = ColorRole::InversePrimary;
    switch (state) {
    case MdSnackbarState::Enabled:
        break;
    case MdSnackbarState::Hovered:
        row.stateLayerOpacity = 0.08; // md.sys.state.hover-state-layer-opacity
        break;
    case MdSnackbarState::Focused:
        row.stateLayerOpacity = 0.12; // focus-state-layer-opacity
        break;
    case MdSnackbarState::Pressed:
        row.stateLayerOpacity = 0.12; // pressed-state-layer-opacity
        break;
    case MdSnackbarState::Count:
        break;
    }
    return row;
}

// md.comp.snackbar.icon.<state>.{icon,state-layer}.* — the icon keeps
// inverse-on-surface through every state, same shape as the action's rows.
MdSnackbarElementRow iconRowFor(MdSnackbarState state)
{
    MdSnackbarElementRow row;
    row.content = ColorRole::InverseOnSurface;
    row.stateLayer = ColorRole::InverseOnSurface;
    switch (state) {
    case MdSnackbarState::Enabled:
        break;
    case MdSnackbarState::Hovered:
        row.stateLayerOpacity = 0.08;
        break;
    case MdSnackbarState::Focused:
        row.stateLayerOpacity = 0.12;
        break;
    case MdSnackbarState::Pressed:
        row.stateLayerOpacity = 0.12;
        break;
    case MdSnackbarState::Count:
        break;
    }
    return row;
}

} // namespace

MdSnackbarElementRow MdSnackbarTokens::actionRow(MdSnackbarState state) const
{
    return actionRowFor(state);
}

MdSnackbarElementRow MdSnackbarTokens::iconRow(MdSnackbarState state) const
{
    return iconRowFor(state);
}

MdSnackbarTokens MdSnackbarTokens::resolve(const MdComponentTokens *overrides)
{
    MdSnackbarTokens tokens;

    // --- container: md.comp.snackbar.container.* ---------------------------
    tokens.containerElevation = ElevationLevel::Level3; // no elevation override path yet
    tokens.containerShape = comptoken::shapeOverride(
        overrides,
        QStringList{QStringLiteral("md.comp.snackbar.container.shape"),
                    QStringLiteral("md.comp.snackbar.container.corner-size")},
        tokens.containerShape);
    tokens.singleLineHeight = comptoken::lengthOverride(
        overrides,
        QStringList{QStringLiteral("md.comp.snackbar.with-single-line.container.height")},
        tokens.singleLineHeight);
    tokens.twoLinesHeight = comptoken::lengthOverride(
        overrides,
        QStringList{QStringLiteral("md.comp.snackbar.with-two-lines.container.height")},
        tokens.twoLinesHeight);

    // --- icon: md.comp.snackbar.icon.size ----------------------------------
    tokens.iconSize = comptoken::lengthOverride(
        overrides, QStringList{QStringLiteral("md.comp.snackbar.icon.size")}, tokens.iconSize);

    return tokens;
}

qint64 MdSnackbarTokens::durationMs(MdSnackbarDuration duration, bool hasAction)
{
    // SnackbarHost.kt toMillis: Short 4000 / Long 10000 / Indefinite never;
    // the Auto default resolves caller-side: an action pins Indefinite.
    switch (duration) {
    case MdSnackbarDuration::Short:
        return 4000;
    case MdSnackbarDuration::Long:
        return 10000;
    case MdSnackbarDuration::Indefinite:
        return -1; // never self-dismisses
    case MdSnackbarDuration::Auto:
        return hasAction ? -1 : 4000;
    case MdSnackbarDuration::Count:
        break;
    }
    return 4000;
}

} // namespace md
