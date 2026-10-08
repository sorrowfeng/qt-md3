#include "MdBottomSheetTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// The `md.comp.*` override parsing itself lives in MdCompTokenParse.h, shared
// with the other component resolvers so a length or a shape name can only be
// parsed one way across the whole library.
using namespace comptoken;

} // namespace

MdBottomSheetTokens MdBottomSheetTokens::resolve(const MdComponentTokens *overrides)
{
    const char *component = "sheet.bottom";
    MdBottomSheetTokens tokens;

    // --- shapes -------------------------------------------------------------
    // The docked container shape (extra-large-top) and the minimized shape
    // (none) are both published and both overridable.
    tokens.containerShape =
        shapeOverride(overrides, plainKeys(component, "docked.container.shape"),
                      ShapeCorner::ExtraLarge);
    tokens.minimizedShape =
        shapeOverride(overrides, plainKeys(component, "docked.minimized.container.shape"),
                      ShapeCorner::None);

    // --- drag handle metrics ------------------------------------------------
    tokens.dragHandleWidth =
        lengthOverride(overrides, plainKeys(component, "docked.drag-handle.width"), 32.0);
    tokens.dragHandleHeight =
        lengthOverride(overrides, plainKeys(component, "docked.drag-handle.height"), 4.0);

    // The colour rows, the elevation level and the deprecated opacity row are
    // not overridable — same rule as the other families, recorded in
    // docs/porting-todo.md.
    return tokens;
}

} // namespace md
