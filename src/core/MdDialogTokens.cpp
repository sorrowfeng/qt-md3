#include "MdDialogTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// The `md.comp.*` override parsing itself lives in MdCompTokenParse.h, shared
// with the other component resolvers so a length or a shape name can only be
// parsed one way across the whole library.
using namespace comptoken;

// Transcribed from _md-comp-dialog.scss (34.0.21). The action's four state
// rows repeat the primary label colour; only the state-layer opacity moves
// (0 / 0.08 / 0.12 / 0.12, resolved by md.sys.state in the export).
struct MdDialogActionStateRow
{
    MdDialogActionState state;
    qreal stateLayerOpacity;
};

const MdDialogActionStateRow kActionRows[] = {
    {MdDialogActionState::Enabled, 0.0},
    {MdDialogActionState::Hovered, 0.08},
    {MdDialogActionState::Focused, 0.12},
    {MdDialogActionState::Pressed, 0.12},
};

} // namespace

MdDialogActionRow MdDialogTokens::actionRow(MdDialogActionState state) const
{
    MdDialogActionRow row;
    row.content = actionLabelColor;
    row.stateLayer = actionLabelColor;
    for (const MdDialogActionStateRow &published : kActionRows) {
        if (published.state == state) {
            row.stateLayerOpacity = published.stateLayerOpacity;
            break;
        }
    }
    return row;
}

MdDialogTokens MdDialogTokens::resolve(const MdComponentTokens *overrides)
{
    const char *component = "dialog";
    MdDialogTokens tokens;

    // --- shape / metrics ---------------------------------------------------
    tokens.containerShape =
        shapeOverride(overrides, plainKeys(component, "container.shape"),
                      ShapeCorner::ExtraLarge);
    tokens.iconSize =
        lengthOverride(overrides, plainKeys(component, "icon.size"), 24.0);

    // --- deprecated divider rows, still published --------------------------
    tokens.dividerHeight =
        lengthOverride(overrides, plainKeys(component, "divider.height"), 1.0);

    // The colour and type rows are not overridable — same rule as the other
    // families, recorded in docs/porting-todo.md.
    return tokens;
}

} // namespace md
