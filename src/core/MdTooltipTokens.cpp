#include "MdTooltipTokens.h"

#include "MdCompTokenParse.h"

namespace md {

MdTooltipActionRow MdTooltipTokens::actionRow(MdTooltipActionState state) const
{
    // md.comp.rich-tooltip.action.<state>.{label-text,state-layer}.* — the
    // label text stays primary through every state; the interactive states
    // only swap the primary state layer in at the md.sys.state opacities.
    MdTooltipActionRow row;
    row.content = ColorRole::Primary;
    row.stateLayer = ColorRole::Primary;
    switch (state) {
    case MdTooltipActionState::Enabled:
        break;
    case MdTooltipActionState::Hovered:
        row.stateLayerOpacity = 0.08; // md.sys.state.hover-state-layer-opacity
        break;
    case MdTooltipActionState::Focused:
        row.stateLayerOpacity = 0.12; // focus-state-layer-opacity
        break;
    case MdTooltipActionState::Pressed:
        row.stateLayerOpacity = 0.12; // pressed-state-layer-opacity
        break;
    case MdTooltipActionState::Count:
        break;
    }
    return row;
}

MdTooltipTokens MdTooltipTokens::resolve(MdTooltipVariant variant,
                                         const MdComponentTokens *overrides)
{
    MdTooltipTokens tokens;
    tokens.plainContainerShape = comptoken::shapeOverride(
        overrides,
        QStringList{QStringLiteral("md.comp.plain-tooltip.container.shape"),
                    QStringLiteral("md.comp.plain-tooltip.container.corner-size")},
        tokens.plainContainerShape);
    tokens.richContainerShape = comptoken::shapeOverride(
        overrides,
        QStringList{QStringLiteral("md.comp.rich-tooltip.container.shape"),
                    QStringLiteral("md.comp.rich-tooltip.container.corner-size")},
        tokens.richContainerShape);
    Q_UNUSED(variant);
    return tokens;
}

} // namespace md
