#include "MdBadgeTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

using namespace comptoken;

} // namespace

MdBadgeTokens MdBadgeTokens::resolve(bool large, const MdComponentTokens *overrides)
{
    MdBadgeTokens tokens;

    if (!large) {
        // --- the dot form: md.comp.badge.* ---------------------------------
        tokens.size = lengthOverride(overrides, plainKeys("badge", "size"), tokens.size);
        tokens.shape = shapeOverride(overrides, plainKeys("badge", "shape"), tokens.shape);
        tokens.color = ColorRole::Error;
        tokens.largeColor = ColorRole::Error;
        tokens.largeLabelTextColor = ColorRole::OnError;
        tokens.largeLabelTextType = TypeStyle::LabelSmall;
        return tokens;
    }

    // --- the content form: md.comp.badge.large.* ---------------------------
    // The more qualified `large.*` key wins over the base one, mirroring the
    // size-then-base lookup every sized family uses.
    const QStringList largeSizeKeys = {
        QStringLiteral("md.comp.badge.large.size"),
        QStringLiteral("md.comp.badge.size"),
    };
    tokens.size = lengthOverride(overrides, plainKeys("badge", "size"), tokens.size);
    tokens.largeSize = lengthOverride(overrides, largeSizeKeys, tokens.largeSize);

    const QStringList largeShapeKeys = {
        QStringLiteral("md.comp.badge.large.shape"),
        QStringLiteral("md.comp.badge.shape"),
    };
    tokens.shape = shapeOverride(overrides, plainKeys("badge", "shape"), tokens.shape);
    tokens.largeShape = shapeOverride(overrides, largeShapeKeys, tokens.largeShape);

    tokens.color = ColorRole::Error;
    tokens.largeColor = ColorRole::Error;
    tokens.largeLabelTextColor = ColorRole::OnError;
    tokens.largeLabelTextType = TypeStyle::LabelSmall;
    return tokens;
}

} // namespace md
