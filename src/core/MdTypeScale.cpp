#include "MdTypeScale.h"

#include "MdTokens.h"

#include <QtCore/QHash>
#include <QtCore/QMutex>

namespace md {

namespace {

struct TypeRow {
    TypeStyle style;
    // baseline (material-web): family token, size, lineHeight, tracking, weight
    const char *baselineFamily;
    qreal baselineSize;
    qreal baselineLineHeight;
    qreal baselineTracking;
    int baselineWeight;
    // emphasized (Compose Material3)
    const char *emphasizedFamily;
    qreal emphasizedSize;
    qreal emphasizedLineHeight;
    qreal emphasizedTracking;
    int emphasizedWeight;
};

// material-web: 1rem = 16px, so tracking in rem is multiplied by 16.
//   body-large   0.03125rem -> 0.5px
//   body-medium  0.015625rem -> 0.25px
//   body-small   0.025rem -> 0.4px
//   label-large  0.00625rem -> 0.1px
//   label-medium 0.03125rem -> 0.5px
//   label-small  0.03125rem -> 0.5px
//   title-medium 0.009375rem -> 0.15px
//   title-small  0.00625rem -> 0.1px
//   display-large -0.015625rem -> -0.25px
// clang-format off
const TypeRow kRows[] = {
    {TypeStyle::DisplayLarge,  "brand", 57.0, 64.0, -0.25, 400, "brand", 57.0, 64.0, 0.0,  500},
    {TypeStyle::DisplayMedium, "brand", 45.0, 52.0,  0.0,  400, "brand", 45.0, 52.0, 0.0,  500},
    {TypeStyle::DisplaySmall,  "brand", 36.0, 44.0,  0.0,  400, "brand", 36.0, 44.0, 0.0,  500},
    {TypeStyle::HeadlineLarge, "brand", 32.0, 40.0,  0.0,  400, "brand", 32.0, 40.0, 0.0,  500},
    {TypeStyle::HeadlineMedium,"brand", 28.0, 36.0,  0.0,  400, "brand", 28.0, 36.0, 0.0,  500},
    {TypeStyle::HeadlineSmall, "brand", 24.0, 32.0,  0.0,  400, "brand", 24.0, 32.0, 0.0,  500},
    {TypeStyle::TitleLarge,    "brand", 22.0, 28.0,  0.0,  400, "brand", 22.0, 28.0, 0.0,  500},
    {TypeStyle::TitleMedium,   "plain", 16.0, 24.0,  0.15, 500, "plain", 16.0, 24.0, 0.15, 700},
    {TypeStyle::TitleSmall,    "plain", 14.0, 20.0,  0.1,  500, "plain", 14.0, 20.0, 0.1,  700},
    {TypeStyle::BodyLarge,     "plain", 16.0, 24.0,  0.5,  400, "plain", 16.0, 24.0, 0.15, 500},
    {TypeStyle::BodyMedium,    "plain", 14.0, 20.0,  0.25, 400, "plain", 14.0, 20.0, 0.25, 500},
    {TypeStyle::BodySmall,     "plain", 12.0, 16.0,  0.4,  400, "plain", 12.0, 16.0, 0.4,  500},
    {TypeStyle::LabelLarge,    "plain", 14.0, 20.0,  0.1,  500, "plain", 14.0, 20.0, 0.1,  700},
    {TypeStyle::LabelMedium,   "plain", 12.0, 16.0,  0.5,  500, "plain", 12.0, 16.0, 0.5,  700},
    {TypeStyle::LabelSmall,    "plain", 11.0, 16.0,  0.5,  500, "plain", 11.0, 16.0, 0.5,  700},
};
// clang-format on

QHash<QString, QString> &familyOverrides()
{
    static QHash<QString, QString> overrides;
    return overrides;
}

QStringList &cjkFallbacks()
{
    static QStringList fallbacks;
    return fallbacks;
}

QMutex &familyMutex()
{
    static QMutex mutex;
    return mutex;
}

QFont::Weight qtWeight(int weight)
{
    if (weight >= 700) {
        return QFont::Bold;
    }
    if (weight >= 500) {
        return QFont::Medium;
    }
    return QFont::Normal;
}

} // namespace

MdTypeStyleSpec MdTypeScale::spec(TypeStyle style, TypeEmphasis emphasis)
{
    MdTypeStyleSpec result;
    result.style = style;
    result.emphasis = emphasis;

    for (const TypeRow &row : kRows) {
        if (row.style != style) {
            continue;
        }
        const bool isEmphasized = emphasis == TypeEmphasis::Emphasized;
        result.familyToken = QString::fromUtf8(isEmphasized ? row.emphasizedFamily
                                                              : row.baselineFamily);
        result.size = isEmphasized ? row.emphasizedSize : row.baselineSize;
        result.lineHeight = isEmphasized ? row.emphasizedLineHeight : row.baselineLineHeight;
        result.tracking = isEmphasized ? row.emphasizedTracking : row.baselineTracking;
        result.weight = isEmphasized ? row.emphasizedWeight : row.baselineWeight;
        break;
    }
    return result;
}

qreal MdTypeScale::lineHeightMultiplier(ScriptCategory category)
{
    switch (category) {
    case ScriptCategory::Small:
        return 1.00;
    case ScriptCategory::Medium:
        return 1.07;
    case ScriptCategory::Large:
    case ScriptCategory::ExtraLarge:
        // Not yet sourced; see the header comment and docs/porting-todo.md.
        return 1.07;
    }
    return 1.00;
}

qreal MdTypeScale::lineHeight(TypeStyle style, TypeEmphasis emphasis, ScriptCategory category)
{
    const MdTypeStyleSpec s = spec(style, emphasis);
    return s.lineHeight * lineHeightMultiplier(category);
}

QString MdTypeScale::family(const QString &token)
{
    QMutexLocker locker(&familyMutex());
    const auto it = familyOverrides().constFind(token);
    if (it != familyOverrides().constEnd()) {
        return it.value();
    }
    // Default: Roboto for both slots, straight from md.ref.typeface.
    return MdSystemTokens::typefaceFamily(token);
}

void MdTypeScale::setFamily(const QString &token, const QString &familyName)
{
    QMutexLocker locker(&familyMutex());
    familyOverrides().insert(token, familyName);
}

QStringList MdTypeScale::cjkFallbackFamilies()
{
    QMutexLocker locker(&familyMutex());
    if (cjkFallbacks().isEmpty()) {
        return {QStringLiteral("Noto Sans CJK SC"),
                QStringLiteral("Source Han Sans SC"),
                QStringLiteral("Microsoft YaHei"),
                QStringLiteral("PingFang SC")};
    }
    return cjkFallbacks();
}

void MdTypeScale::setCjkFallbackFamilies(const QStringList &families)
{
    QMutexLocker locker(&familyMutex());
    cjkFallbacks() = families;
}

QStringList MdTypeScale::familiesFor(const MdTypeStyleSpec &spec, ScriptCategory category)
{
    QStringList families;
    const QString primary = family(spec.familyToken);
    if (!primary.isEmpty()) {
        families << primary;
    }
    if (category != ScriptCategory::Small) {
        families << cjkFallbackFamilies();
    }
    return families;
}

QFont MdTypeScale::fontFor(const MdTypeStyleSpec &spec, ScriptCategory category)
{
    QFont result;
    const QStringList families = familiesFor(spec, category);
    if (!families.isEmpty()) {
        result.setFamilies(families);
        result.setFamily(families.first());
    }
    result.setPixelSize(qRound(spec.size));
    result.setWeight(qtWeight(spec.weight));
    result.setLetterSpacing(QFont::AbsoluteSpacing, spec.tracking);
    return result;
}

QFont MdTypeScale::font(TypeStyle style, TypeEmphasis emphasis, ScriptCategory category)
{
    return fontFor(spec(style, emphasis), category);
}

ScriptCategory MdTypeScale::scriptCategoryForLanguage(const QString &languageTag)
{
    const QString tag = languageTag.toLower();
    // CJK and Hangul.
    static const QStringList medium = {QStringLiteral("zh"), QStringLiteral("ja"),
                                       QStringLiteral("ko")};
    for (const QString &prefix : medium) {
        if (tag.startsWith(prefix)) {
            return ScriptCategory::Medium;
        }
    }
    // Devanagari / Bengali / Tamil and neighbours.
    static const QStringList large = {QStringLiteral("hi"), QStringLiteral("bn"),
                                      QStringLiteral("ta"), QStringLiteral("te"),
                                      QStringLiteral("kn"), QStringLiteral("ml"),
                                      QStringLiteral("mr"), QStringLiteral("gu")};
    for (const QString &prefix : large) {
        if (tag.startsWith(prefix)) {
            return ScriptCategory::Large;
        }
    }
    if (tag.startsWith(QStringLiteral("bo")) || tag.startsWith(QStringLiteral("mn"))) {
        return ScriptCategory::ExtraLarge;
    }
    return ScriptCategory::Small;
}

} // namespace md
