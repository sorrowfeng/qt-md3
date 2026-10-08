#include "MdSideSheetTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// The `md.comp.*` override parsing itself lives in MdCompTokenParse.h, shared
// with the other component resolvers so a length or a shape name can only be
// parsed one way across the whole library.
using namespace comptoken;

} // namespace

MdSideSheetTokens MdSideSheetTokens::resolve(const MdComponentTokens *overrides)
{
    const char *component = "sheet.side";
    MdSideSheetTokens tokens;

    // --- shapes -------------------------------------------------------------
    // The docked standard shape (corner-none) and the modal shape
    // (corner-large-start) are both published and both overridable. The
    // detached shape (corner-large) belongs to the un-ported detached
    // presentation but is overridable with the rest.
    tokens.standardContainerShape =
        shapeOverride(overrides, plainKeys(component, "docked.standard.container.shape"),
                      ShapeCorner::None);
    tokens.modalContainerShape =
        shapeOverride(overrides, plainKeys(component, "docked.modal.container.shape"),
                      ShapeCorner::Large);
    tokens.detachedShape = shapeOverride(overrides, plainKeys(component, "detached.container.shape"),
                                         ShapeCorner::Large);

    // --- container metrics ---------------------------------------------------
    tokens.containerWidth =
        lengthOverride(overrides, plainKeys(component, "docked.container.width"), 256.0);

    // The colour rows and the elevation levels are not overridable — same
    // rule as the other families, recorded in docs/porting-todo.md.
    return tokens;
}

} // namespace md
