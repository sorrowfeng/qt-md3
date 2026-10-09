#include "MdDividerTokens.h"

#include "MdCompTokenParse.h"

namespace md {

MdDividerTokens MdDividerTokens::resolve(const MdComponentTokens *overrides)
{
    MdDividerTokens tokens;

    using namespace comptoken;
    tokens.thickness = lengthOverride(overrides, plainKeys("divider", "thickness"),
                                      tokens.thickness);
    tokens.inset = lengthOverride(overrides, plainKeys("divider", "inset"), tokens.inset);

    tokens.color = ColorRole::OutlineVariant;
    return tokens;
}

} // namespace md
