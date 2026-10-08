#include "MdCarouselTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// The `md.comp.*` override parsing itself lives in MdCompTokenParse.h, shared
// with the other component resolvers so a length or a shape name can only be
// parsed one way across the whole library.
using namespace comptoken;

} // namespace

MdCarouselTokens MdCarouselTokens::resolve(const MdComponentTokens *overrides)
{
    const char *component = "carousel-item";
    MdCarouselTokens tokens;

    // --- container metrics ---------------------------------------------------
    tokens.containerShapeRadius =
        lengthOverride(overrides, plainKeys(component, "container.shape.corner-radius"), 28.0);

    // --- the with-outline row set -------------------------------------------
    tokens.outlineWidth =
        lengthOverride(overrides, plainKeys(component, "with-outline.outline.width"), 1.0);

    // --- disabled compositing -------------------------------------------------
    tokens.disabledContainerOpacity =
        lengthOverride(overrides, plainKeys(component, "disabled.container.opacity"), 0.38);
    tokens.disabledOutlineOpacity =
        lengthOverride(overrides, plainKeys(component, "with-outline.disabled.outline.opacity"),
                       0.12);

    // --- scroll constants -----------------------------------------------------
    tokens.itemSpacing =
        lengthOverride(overrides, plainKeys(component, "container.item-spacing"), 8.0);
    tokens.minSmallItemSize =
        lengthOverride(overrides, plainKeys(component, "container.min-small-item-size"), 40.0);
    tokens.maxSmallItemSize =
        lengthOverride(overrides, plainKeys(component, "container.max-small-item-size"), 56.0);

    // The colour roles, the elevation levels, the focus-indicator metrics and
    // the state-layer opacities are not overridable — same rule as the other
    // families, recorded in docs/porting-todo.md.
    return tokens;
}

} // namespace md
