#include "MdTokens.h"

#include <QtCore/QPair>

namespace md {

namespace {

// ---------------------------------------------------------------------------
// md.ref.palette.* — material-web tokens/versions/v0_192/_md-ref-palette.scss
// ---------------------------------------------------------------------------

struct ToneEntry {
    int tone;
    const char *hex;
};

struct PaletteEntry {
    const char *name;
    const ToneEntry *tones;
    int count;
};

// clang-format off
const ToneEntry kError[] = {
    {0, "#000000"}, {10, "#410e0b"}, {20, "#601410"}, {30, "#8c1d18"},
    {40, "#b3261e"}, {50, "#dc362e"}, {60, "#e46962"}, {70, "#ec928e"},
    {80, "#f2b8b5"}, {90, "#f9dedc"}, {95, "#fceeee"}, {99, "#fffbf9"},
    {100, "#ffffff"},
};

const ToneEntry kNeutralVariant[] = {
    {0, "#000000"}, {10, "#1d1a22"}, {20, "#322f37"}, {30, "#49454f"},
    {40, "#605d66"}, {50, "#79747e"}, {60, "#938f99"}, {70, "#aea9b4"},
    {80, "#cac4d0"}, {90, "#e7e0ec"}, {95, "#f5eefa"}, {99, "#fffbfe"},
    {100, "#ffffff"},
};

const ToneEntry kNeutral[] = {
    {0, "#000000"}, {4, "#0f0d13"}, {6, "#141218"}, {10, "#1d1b20"},
    {12, "#211f26"}, {17, "#2b2930"}, {20, "#322f35"}, {22, "#36343b"},
    {24, "#3b383e"}, {30, "#48464c"}, {40, "#605d64"}, {50, "#79767d"},
    {60, "#938f96"}, {70, "#aea9b1"}, {80, "#cac5cd"}, {87, "#ded8e1"},
    {90, "#e6e0e9"}, {92, "#ece6f0"}, {94, "#f3edf7"}, {95, "#f5eff7"},
    {96, "#f7f2fa"}, {98, "#fef7ff"}, {99, "#fffbff"}, {100, "#ffffff"},
};

const ToneEntry kPrimary[] = {
    {0, "#000000"}, {10, "#21005d"}, {20, "#381e72"}, {30, "#4f378b"},
    {40, "#6750a4"}, {50, "#7f67be"}, {60, "#9a82db"}, {70, "#b69df8"},
    {80, "#d0bcff"}, {90, "#eaddff"}, {95, "#f6edff"}, {99, "#fffbfe"},
    {100, "#ffffff"},
};

const ToneEntry kSecondary[] = {
    {0, "#000000"}, {10, "#1d192b"}, {20, "#332d41"}, {30, "#4a4458"},
    {40, "#625b71"}, {50, "#7a7289"}, {60, "#958da5"}, {70, "#b0a7c0"},
    {80, "#ccc2dc"}, {90, "#e8def8"}, {95, "#f6edff"}, {99, "#fffbfe"},
    {100, "#ffffff"},
};

const ToneEntry kTertiary[] = {
    {0, "#000000"}, {10, "#31111d"}, {20, "#492532"}, {30, "#633b48"},
    {40, "#7d5260"}, {50, "#986977"}, {60, "#b58392"}, {70, "#d29dac"},
    {80, "#efb8c8"}, {90, "#ffd8e4"}, {95, "#ffecf1"}, {99, "#fffbfa"},
    {100, "#ffffff"},
};

const PaletteEntry kPalettes[] = {
    {"primary", kPrimary, int(sizeof(kPrimary) / sizeof(kPrimary[0]))},
    {"secondary", kSecondary, int(sizeof(kSecondary) / sizeof(kSecondary[0]))},
    {"tertiary", kTertiary, int(sizeof(kTertiary) / sizeof(kTertiary[0]))},
    {"neutral", kNeutral, int(sizeof(kNeutral) / sizeof(kNeutral[0]))},
    {"neutral-variant", kNeutralVariant,
     int(sizeof(kNeutralVariant) / sizeof(kNeutralVariant[0]))},
    {"error", kError, int(sizeof(kError) / sizeof(kError[0]))},
};
// clang-format on

const PaletteEntry *findPalette(const QString &name)
{
    for (const PaletteEntry &entry : kPalettes) {
        if (name == QLatin1String(entry.name)) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace

QString tokenSourceVersion()
{
    // material-web token export + Compose Material3 Expressive motion/shape.
    return QStringLiteral("md3-tokens/material-web-v0_192+compose-m3-expressive");
}

// ---------------------------------------------------------------------------
// MdReferenceTokens
// ---------------------------------------------------------------------------

QStringList MdReferenceTokens::paletteNames()
{
    QStringList names;
    for (const PaletteEntry &entry : kPalettes) {
        names << QString::fromUtf8(entry.name);
    }
    return names;
}

QList<int> MdReferenceTokens::tones(const QString &palette)
{
    QList<int> result;
    const PaletteEntry *entry = findPalette(palette);
    if (!entry) {
        return result;
    }
    for (int i = 0; i < entry->count; ++i) {
        result << entry->tones[i].tone;
    }
    return result;
}

QColor MdReferenceTokens::toneColor(const QString &palette, int tone)
{
    const PaletteEntry *entry = findPalette(palette);
    if (!entry) {
        return QColor();
    }
    for (int i = 0; i < entry->count; ++i) {
        if (entry->tones[i].tone == tone) {
            return QColor(QString::fromUtf8(entry->tones[i].hex));
        }
    }
    return QColor();
}

QColor MdReferenceTokens::referenceColor(const QString &token)
{
    if (token == QLatin1String("black")) {
        return QColor(QStringLiteral("#000000"));
    }
    if (token == QLatin1String("white")) {
        return QColor(QStringLiteral("#ffffff"));
    }
    // Longest palette names first so "neutral-variant90" is not mistaken for
    // "neutral".
    static const QStringList ordered = {
        QStringLiteral("neutral-variant"),
        QStringLiteral("secondary"),
        QStringLiteral("tertiary"),
        QStringLiteral("primary"),
        QStringLiteral("neutral"),
        QStringLiteral("error"),
    };
    for (const QString &palette : ordered) {
        if (!token.startsWith(palette)) {
            continue;
        }
        bool ok = false;
        const int tone = token.mid(palette.size()).toInt(&ok);
        if (ok) {
            return toneColor(palette, tone);
        }
    }
    return QColor();
}

QStringList MdReferenceTokens::referenceTokenNames()
{
    QStringList names;
    for (const PaletteEntry &entry : kPalettes) {
        const QString palette = QString::fromUtf8(entry.name);
        for (int i = 0; i < entry.count; ++i) {
            names << palette + QString::number(entry.tones[i].tone);
        }
    }
    names << QStringLiteral("black") << QStringLiteral("white");
    return names;
}

// ---------------------------------------------------------------------------
// MdSystemTokens — shape
// ---------------------------------------------------------------------------

qreal MdSystemTokens::cornerRadius(ShapeCorner corner)
{
    switch (corner) {
    // v0_192 md-sys-shape: none 0, extra-small 4, small 8, medium 12,
    // large 16, extra-large 28, full 9999.
    case ShapeCorner::None: return 0.0;
    case ShapeCorner::ExtraSmall: return 4.0;
    case ShapeCorner::Small: return 8.0;
    case ShapeCorner::Medium: return 12.0;
    case ShapeCorner::Large: return 16.0;
    case ShapeCorner::ExtraLarge: return 28.0;

    // M3 Expressive additions (Compose ShapeTokens.kt).
    case ShapeCorner::LargeIncreased: return 20.0;
    case ShapeCorner::ExtraLargeIncreased: return 32.0;
    case ShapeCorner::ExtraExtraLarge: return 48.0;

    // "full" is a pill: the painter resolves it against the shorter side.
    case ShapeCorner::Full: return -1.0;

    case ShapeCorner::Count: break;
    }
    return 0.0;
}

QList<qreal> MdSystemTokens::cornerRadii(ShapeCorner corner)
{
    const qreal radius = cornerRadius(corner);
    return {radius, radius, radius, radius};
}

QList<QPair<ShapeCorner, qreal>> MdSystemTokens::shapeScale()
{
    return {
        {ShapeCorner::None, 0.0},
        {ShapeCorner::ExtraSmall, 4.0},
        {ShapeCorner::Small, 8.0},
        {ShapeCorner::Medium, 12.0},
        {ShapeCorner::Large, 16.0},
        {ShapeCorner::LargeIncreased, 20.0},
        {ShapeCorner::ExtraLarge, 28.0},
        {ShapeCorner::ExtraLargeIncreased, 32.0},
        {ShapeCorner::ExtraExtraLarge, 48.0},
        {ShapeCorner::Full, -1.0},
    };
}

// ---------------------------------------------------------------------------
// MdSystemTokens — elevation
// ---------------------------------------------------------------------------

qreal MdSystemTokens::elevationDp(ElevationLevel level)
{
    switch (level) {
    case ElevationLevel::Level0: return 0.0;
    case ElevationLevel::Level1: return 1.0;
    case ElevationLevel::Level2: return 3.0;
    case ElevationLevel::Level3: return 6.0;
    case ElevationLevel::Level4: return 8.0;
    case ElevationLevel::Level5: return 12.0;
    case ElevationLevel::Count: break;
    }
    return 0.0;
}

// ---------------------------------------------------------------------------
// MdSystemTokens — motion
// ---------------------------------------------------------------------------

int MdSystemTokens::durationMs(MotionDuration duration)
{
    switch (duration) {
    case MotionDuration::Short1: return 50;
    case MotionDuration::Short2: return 100;
    case MotionDuration::Short3: return 150;
    case MotionDuration::Short4: return 200;
    case MotionDuration::Medium1: return 250;
    case MotionDuration::Medium2: return 300;
    case MotionDuration::Medium3: return 350;
    case MotionDuration::Medium4: return 400;
    case MotionDuration::Long1: return 450;
    case MotionDuration::Long2: return 500;
    case MotionDuration::Long3: return 550;
    case MotionDuration::Long4: return 600;
    case MotionDuration::ExtraLong1: return 700;
    case MotionDuration::ExtraLong2: return 800;
    case MotionDuration::ExtraLong3: return 900;
    case MotionDuration::ExtraLong4: return 1000;
    case MotionDuration::Count: break;
    }
    return 0;
}

QList<qreal> MdSystemTokens::easingBezier(MotionEasing easing)
{
    switch (easing) {
    case MotionEasing::Linear: return {0.0, 0.0, 1.0, 1.0};
    case MotionEasing::Standard: return {0.2, 0.0, 0.0, 1.0};
    case MotionEasing::StandardAccelerate: return {0.3, 0.0, 1.0, 1.0};
    case MotionEasing::StandardDecelerate: return {0.0, 0.0, 0.0, 1.0};
    case MotionEasing::Emphasized: return {0.2, 0.0, 0.0, 1.0};
    case MotionEasing::EmphasizedAccelerate: return {0.3, 0.0, 0.8, 0.15};
    case MotionEasing::EmphasizedDecelerate: return {0.05, 0.7, 0.1, 1.0};
    case MotionEasing::Legacy: return {0.4, 0.0, 0.2, 1.0};
    case MotionEasing::LegacyAccelerate: return {0.4, 0.0, 1.0, 1.0};
    case MotionEasing::LegacyDecelerate: return {0.0, 0.0, 0.2, 1.0};
    case MotionEasing::Count: break;
    }
    return {0.0, 0.0, 1.0, 1.0};
}

void MdSystemTokens::springParameters(MotionSpring spring,
                                      qreal *stiffness,
                                      qreal *dampingRatio)
{
    qreal s = 380.0;
    qreal d = 0.8;
    switch (spring) {
    case MotionSpring::SpatialFast: s = 800.0; d = 0.6; break;
    case MotionSpring::SpatialDefault: s = 380.0; d = 0.8; break;
    case MotionSpring::SpatialSlow: s = 200.0; d = 0.8; break;
    case MotionSpring::EffectsFast: s = 3800.0; d = 1.0; break;
    case MotionSpring::EffectsDefault: s = 1600.0; d = 1.0; break;
    case MotionSpring::EffectsSlow: s = 800.0; d = 1.0; break;
    case MotionSpring::Count: break;
    }
    if (stiffness) {
        *stiffness = s;
    }
    if (dampingRatio) {
        *dampingRatio = d;
    }
}

// ---------------------------------------------------------------------------
// MdSystemTokens — state layers
// ---------------------------------------------------------------------------

qreal MdSystemTokens::stateLayerOpacity(StateLayerKind kind)
{
    switch (kind) {
    case StateLayerKind::Hover: return 0.08;
    case StateLayerKind::Focus: return 0.12;
    case StateLayerKind::Pressed: return 0.12;
    case StateLayerKind::Dragged: return 0.16;
    case StateLayerKind::Count: break;
    }
    return 0.0;
}

// ---------------------------------------------------------------------------
// MdSystemTokens — typeface
// ---------------------------------------------------------------------------

QString MdSystemTokens::typefaceFamily(const QString &token)
{
    if (token == QLatin1String("brand")) {
        return QStringLiteral("Roboto");
    }
    if (token == QLatin1String("plain")) {
        return QStringLiteral("Roboto");
    }
    return QString();
}

int MdSystemTokens::typefaceWeight(const QString &token)
{
    if (token == QLatin1String("weight-regular")) {
        return 400;
    }
    if (token == QLatin1String("weight-medium")) {
        return 500;
    }
    if (token == QLatin1String("weight-bold")) {
        return 700;
    }
    return 400;
}

// ---------------------------------------------------------------------------
// MdComponentTokens
// ---------------------------------------------------------------------------

MdComponentTokens &MdComponentTokens::global()
{
    static MdComponentTokens instance;
    return instance;
}

QString MdComponentTokens::value(const QString &key) const
{
    return m_values.value(key);
}

QString MdComponentTokens::value(const QString &key, const QString &fallback) const
{
    const auto it = m_values.constFind(key);
    return it == m_values.constEnd() ? fallback : it.value();
}

void MdComponentTokens::setValue(const QString &key, const QString &value)
{
    if (key.isEmpty()) {
        return;
    }
    m_values.insert(key, value);
}

void MdComponentTokens::remove(const QString &key)
{
    m_values.remove(key);
}

bool MdComponentTokens::contains(const QString &key) const
{
    return m_values.contains(key);
}

void MdComponentTokens::clear()
{
    m_values.clear();
}

QStringList MdComponentTokens::keys() const
{
    QStringList result = m_values.keys();
    result.sort();
    return result;
}

QString MdComponentTokens::resolve(const QString &key, const QString &fallback) const
{
    const auto it = m_values.constFind(key);
    if (it != m_values.constEnd()) {
        return it.value();
    }
    const auto globalIt = global().m_values.constFind(key);
    if (globalIt != global().m_values.constEnd()) {
        return globalIt.value();
    }
    return fallback;
}

} // namespace md
