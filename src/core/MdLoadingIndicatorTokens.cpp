#include "MdLoadingIndicatorTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

using namespace comptoken;

} // namespace

MdLoadingIndicatorTokens MdLoadingIndicatorTokens::resolve(
    LoadingIndicatorVariant variant, const MdComponentTokens *overrides)
{
    MdLoadingIndicatorTokens tokens;

    if (variant == LoadingIndicatorVariant::Plain) {
        // --- md.comp.loading-indicator.* ------------------------------------
        tokens.activeIndicatorSize =
            lengthOverride(overrides, plainKeys("loading-indicator", "active-indicator.size"),
                           tokens.activeIndicatorSize);
        tokens.containerWidth =
            lengthOverride(overrides, plainKeys("loading-indicator", "container.width"),
                           tokens.containerWidth);
        tokens.containerHeight =
            lengthOverride(overrides, plainKeys("loading-indicator", "container.height"),
                           tokens.containerHeight);
        return tokens;
    }

    // --- md.comp.loading-indicator.contained.* ------------------------------
    // The contained variant reuses the base metrics unless a
    // variant-qualified row overrides them (the export publishes no
    // contained metric rows of its own, so the base values stand).
    tokens.activeIndicatorSize =
        lengthOverride(overrides, plainKeys("loading-indicator", "active-indicator.size"),
                       tokens.activeIndicatorSize);
    tokens.containerWidth =
        lengthOverride(overrides,
                       plainKeys("loading-indicator.contained", "container.width"),
                       tokens.containerWidth);
    tokens.containerHeight =
        lengthOverride(overrides,
                       plainKeys("loading-indicator.contained", "container.height"),
                       tokens.containerHeight);
    return tokens;
}

} // namespace md
