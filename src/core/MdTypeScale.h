#ifndef MD_TYPE_SCALE_H
#define MD_TYPE_SCALE_H

// Type scale: the 30 MD3 type styles (15 baseline + 15 emphasized).
//
// Baseline sizes / line heights / tracking / weights are transcribed from
// material-web tokens/versions/v0_192/_md-sys-typescale.scss. The emphasized
// set comes from Compose Material3's TypeScaleTokens.kt, because material-web
// does not implement M3 Expressive.
//
// Line height adapts to the language script category: CJK needs roughly 7%
// more vertical room than Latin, so a Chinese label must not reuse a Latin
// line height.

#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QFont>

namespace md {

/// One resolved type style.
struct MdTypeStyleSpec
{
    TypeStyle style = TypeStyle::BodyMedium;
    TypeEmphasis emphasis = TypeEmphasis::Baseline;
    QString familyToken; ///< "brand" or "plain"
    qreal size = 14.0;   ///< logical px
    qreal lineHeight = 20.0;
    qreal tracking = 0.25;
    int weight = 400;
};

class QT_MD3_EXPORT MdTypeScale
{
public:
    /// Resolved spec, before font-family substitution.
    static MdTypeStyleSpec spec(TypeStyle style, TypeEmphasis emphasis = TypeEmphasis::Baseline);

    /// A ready-to-use QFont. The family is resolved through familyFor().
    static QFont font(TypeStyle style,
                      TypeEmphasis emphasis = TypeEmphasis::Baseline,
                      ScriptCategory category = ScriptCategory::Small);

    /// Build a QFont from an explicit spec.
    static QFont fontFor(const MdTypeStyleSpec &spec, ScriptCategory category = ScriptCategory::Small);

    /// Line height in logical px, including the script-category adjustment.
    static qreal lineHeight(TypeStyle style,
                           TypeEmphasis emphasis = TypeEmphasis::Baseline,
                           ScriptCategory category = ScriptCategory::Small);

    /// Line-height multiplier for a script category.
    ///
    /// Small = 1.00 and Medium = 1.07 come from the MD3 guidance quoted in the
    /// project brief. The Large / ExtraLarge multipliers are not yet sourced
    /// from the official type-scale page and currently fall back to the Medium
    /// value; see docs/porting-todo.md.
    static qreal lineHeightMultiplier(ScriptCategory category);

    /// Family name for a "brand" / "plain" slot.
    static QString family(const QString &token);
    /// Override a family slot (e.g. to install Roboto Flex or a Chinese face).
    static void setFamily(const QString &token, const QString &familyName);

    /// Fallback families appended for CJK text, in priority order.
    static QStringList cjkFallbackFamilies();
    static void setCjkFallbackFamilies(const QStringList &families);

    /// The family list actually handed to QFont for a spec + script category.
    static QStringList familiesFor(const MdTypeStyleSpec &spec, ScriptCategory category);

    /// Best-effort script category from a language tag ("zh-Hans" -> Medium).
    static ScriptCategory scriptCategoryForLanguage(const QString &languageTag);
};

} // namespace md

#endif // MD_TYPE_SCALE_H
