#include "MdDockedToolbarTokens.h"

#include "MdCompTokenParse.h"

namespace md {

MdDockedToolbarTokens MdDockedToolbarTokens::resolve(const MdComponentTokens *overrides)
{
    MdDockedToolbarTokens tokens;

    using namespace comptoken;

    tokens.containerHeight = lengthOverride(
        overrides, plainKeys("toolbar", "docked.container.height"), tokens.containerHeight);
    tokens.containerLeadingSpace =
        lengthOverride(overrides, plainKeys("toolbar", "docked.container.leading-space"),
                       tokens.containerLeadingSpace);
    tokens.containerTrailingSpace =
        lengthOverride(overrides, plainKeys("toolbar", "docked.container.trailing-space"),
                       tokens.containerTrailingSpace);
    tokens.containerMaxSpacing =
        lengthOverride(overrides, plainKeys("toolbar", "docked.container.max-spacing"),
                       tokens.containerMaxSpacing);
    tokens.containerMinSpacing =
        lengthOverride(overrides, plainKeys("toolbar", "docked.container.min-spacing"),
                       tokens.containerMinSpacing);
    tokens.containerShape = shapeOverride(
        overrides, plainKeys("toolbar", "docked.container.shape"), tokens.containerShape);

    // Not token rows: `BottomAppBarLayout` centres the content in a fixed-height
    // row, so the vertical content padding is zero and only the two spacings
    // above exist. The keys are invented here so a theme can still retune them,
    // the same treatment the divider's non-token 16 px inset receives.
    tokens.contentTopSpace = lengthOverride(
        overrides, plainKeys("toolbar", "docked.content.top-space"), tokens.contentTopSpace);
    tokens.contentBottomSpace =
        lengthOverride(overrides, plainKeys("toolbar", "docked.content.bottom-space"),
                       tokens.contentBottomSpace);

    return tokens;
}

} // namespace md
