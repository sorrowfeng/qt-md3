#include "MdColorScheme.h"

#include "MdDynamicColor.h"
#include "MdTokens.h"

namespace md {

namespace {

using R = ColorRole;

/// Baseline light mapping — material-web
/// tokens/versions/v0_192/_md-sys-color.scss, values-light().
struct RoleMapping {
    R role;
    const char *reference; // md.ref.palette token name
};

// clang-format off
const RoleMapping kLight[] = {
    {R::Background,               "neutral98"},
    {R::Error,                    "error40"},
    {R::ErrorContainer,           "error90"},
    {R::InverseOnSurface,         "neutral95"},
    {R::InversePrimary,           "primary80"},
    {R::InverseSurface,           "neutral20"},
    {R::OnBackground,             "neutral10"},
    {R::OnError,                  "error100"},
    {R::OnErrorContainer,         "error10"},
    {R::OnPrimary,                "primary100"},
    {R::OnPrimaryContainer,       "primary10"},
    {R::OnPrimaryFixed,           "primary10"},
    {R::OnPrimaryFixedVariant,    "primary30"},
    {R::OnSecondary,              "secondary100"},
    {R::OnSecondaryContainer,     "secondary10"},
    {R::OnSecondaryFixed,         "secondary10"},
    {R::OnSecondaryFixedVariant,  "secondary30"},
    {R::OnSurface,                "neutral10"},
    {R::OnSurfaceVariant,         "neutral-variant30"},
    {R::OnTertiary,               "tertiary100"},
    {R::OnTertiaryContainer,      "tertiary10"},
    {R::OnTertiaryFixed,          "tertiary10"},
    {R::OnTertiaryFixedVariant,   "tertiary30"},
    {R::Outline,                  "neutral-variant50"},
    {R::OutlineVariant,           "neutral-variant80"},
    {R::Primary,                  "primary40"},
    {R::PrimaryContainer,         "primary90"},
    {R::PrimaryFixed,             "primary90"},
    {R::PrimaryFixedDim,          "primary80"},
    {R::Scrim,                    "neutral0"},
    {R::Secondary,                "secondary40"},
    {R::SecondaryContainer,       "secondary90"},
    {R::SecondaryFixed,           "secondary90"},
    {R::SecondaryFixedDim,        "secondary80"},
    {R::Shadow,                   "neutral0"},
    {R::Surface,                  "neutral98"},
    {R::SurfaceBright,            "neutral98"},
    {R::SurfaceContainer,         "neutral94"},
    {R::SurfaceContainerHigh,     "neutral92"},
    {R::SurfaceContainerHighest,  "neutral90"},
    {R::SurfaceContainerLow,      "neutral96"},
    {R::SurfaceContainerLowest,   "neutral100"},
    {R::SurfaceDim,               "neutral87"},
    {R::SurfaceTint,              "primary40"},
    {R::SurfaceVariant,           "neutral-variant90"},
    {R::Tertiary,                 "tertiary40"},
    {R::TertiaryContainer,        "tertiary90"},
    {R::TertiaryFixed,            "tertiary90"},
    {R::TertiaryFixedDim,         "tertiary80"},
};

const RoleMapping kDark[] = {
    {R::Background,               "neutral6"},
    {R::Error,                    "error80"},
    {R::ErrorContainer,           "error30"},
    {R::InverseOnSurface,         "neutral20"},
    {R::InversePrimary,           "primary40"},
    {R::InverseSurface,           "neutral90"},
    {R::OnBackground,             "neutral90"},
    {R::OnError,                  "error20"},
    {R::OnErrorContainer,         "error90"},
    {R::OnPrimary,                "primary20"},
    {R::OnPrimaryContainer,       "primary90"},
    {R::OnPrimaryFixed,           "primary10"},
    {R::OnPrimaryFixedVariant,    "primary30"},
    {R::OnSecondary,              "secondary20"},
    {R::OnSecondaryContainer,     "secondary90"},
    {R::OnSecondaryFixed,         "secondary10"},
    {R::OnSecondaryFixedVariant,  "secondary30"},
    {R::OnSurface,                "neutral90"},
    {R::OnSurfaceVariant,         "neutral-variant80"},
    {R::OnTertiary,               "tertiary20"},
    {R::OnTertiaryContainer,      "tertiary90"},
    {R::OnTertiaryFixed,          "tertiary10"},
    {R::OnTertiaryFixedVariant,   "tertiary30"},
    {R::Outline,                  "neutral-variant60"},
    {R::OutlineVariant,           "neutral-variant30"},
    {R::Primary,                  "primary80"},
    {R::PrimaryContainer,         "primary30"},
    {R::PrimaryFixed,             "primary90"},
    {R::PrimaryFixedDim,          "primary80"},
    {R::Scrim,                    "neutral0"},
    {R::Secondary,                "secondary80"},
    {R::SecondaryContainer,       "secondary30"},
    {R::SecondaryFixed,           "secondary90"},
    {R::SecondaryFixedDim,        "secondary80"},
    {R::Shadow,                   "neutral0"},
    {R::Surface,                  "neutral6"},
    {R::SurfaceBright,            "neutral24"},
    {R::SurfaceContainer,         "neutral12"},
    {R::SurfaceContainerHigh,     "neutral17"},
    {R::SurfaceContainerHighest,  "neutral22"},
    {R::SurfaceContainerLow,      "neutral10"},
    {R::SurfaceContainerLowest,   "neutral4"},
    {R::SurfaceDim,               "neutral6"},
    {R::SurfaceTint,              "primary80"},
    {R::SurfaceVariant,           "neutral-variant30"},
    {R::Tertiary,                 "tertiary80"},
    {R::TertiaryContainer,        "tertiary30"},
    {R::TertiaryFixed,            "tertiary90"},
    {R::TertiaryFixedDim,         "tertiary80"},
};
// clang-format on

} // namespace

MdColorScheme::MdColorScheme() = default;

MdColorScheme MdColorScheme::baseline(ThemeMode mode)
{
    MdColorScheme scheme;
    scheme.m_mode = mode;
    scheme.m_dynamic = false;

    const RoleMapping *table = mode == ThemeMode::Dark ? kDark : kLight;
    const int count = mode == ThemeMode::Dark ? int(sizeof(kDark) / sizeof(kDark[0]))
                                              : int(sizeof(kLight) / sizeof(kLight[0]));
    for (int i = 0; i < count; ++i) {
        scheme.m_colors.insert(table[i].role,
                               MdReferenceTokens::referenceColor(
                                   QString::fromUtf8(table[i].reference)));
    }
    return scheme;
}

MdColorScheme MdColorScheme::dynamic(const QColor &seed,
                                     ThemeMode mode,
                                     SchemeVariant variant,
                                     ContrastLevel contrast)
{
    const Argb seedArgb = MdColorMath::argbFromHex(seed.name(QColor::HexRgb));
    const MdDynamicScheme dynamicScheme = MdDynamicScheme::create(seedArgb, mode, variant, contrast);

    MdColorScheme scheme;
    scheme.m_mode = mode;
    scheme.m_dynamic = true;
    scheme.m_variant = variant;
    scheme.m_contrast = contrast;
    scheme.m_seed = seed;
    for (int i = 0; i < int(ColorRole::Count); ++i) {
        const ColorRole role = static_cast<ColorRole>(i);
        scheme.m_colors.insert(role, QColor::fromRgba(QRgb(dynamicScheme.color(role))));
    }
    return scheme;
}

QColor MdColorScheme::color(ColorRole role) const
{
    return m_colors.value(role);
}

void MdColorScheme::setColor(ColorRole role, const QColor &color)
{
    m_colors.insert(role, color);
}

QString MdColorScheme::hex(ColorRole role) const
{
    const QColor value = color(role);
    return value.isValid() ? value.name(QColor::HexRgb) : QStringLiteral("#000000");
}

} // namespace md
