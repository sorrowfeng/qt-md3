#ifndef MD_TYPES_H
#define MD_TYPES_H

// Shared enums and small value types for qt-md3.
//
// Per the project brief, every component-wide enum lives here so that the
// public API stays consistent across the library. Enumerators are named after
// their MD3 token counterparts (md.sys.color.*, md.sys.shape.*,
// md.sys.motion.*, md.sys.typescale.*) so the mapping is obvious at a glance.

#include "QtMd3Export.h"

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace md {

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------

/// Light / dark is not a separate design; it is the same scheme evaluated at
/// different tone mappings (see MdColorScheme).
enum class ThemeMode {
    Light,
    Dark,
    Count,
};

/// Dynamic-color scheme variants (MaterialColorUtilities `Scheme`).
enum class SchemeVariant {
    TonalSpot,
    Vibrant,
    Expressive,
    Content,
    Fidelity,
    Monochrome,
    Neutral,
    Rainbow,
    FruitSalad,
    Count,
};

/// Contrast levels defined by MD3 (standard / medium / high), plus the
/// material-color-utilities level below standard.
///
/// `Reduced` is not a published `md.sys.*` token: MD3 names three levels.
/// material-color-utilities nevertheless defines a fourth at -1.0 and its own
/// test suite covers it, and Android ships a "reduce contrast" accessibility
/// setting that maps onto it, so the machinery is exposed rather than hidden.
/// See `contrastLevelValue()`.
enum class ContrastLevel {
    Standard,
    Medium,
    High,
    Reduced,
    Count,
};

/// Every enum in this header ends with a `Count` sentinel so callers can
/// enumerate the full set without hard-coding a size. Keep that invariant.

/// Layout direction, mirroring Qt::LayoutDirection but kept inside the token
/// layer so the theme can expose it without pulling in QtWidgets.
enum class TextDirection {
    LeftToRight,
    RightToLeft,
    Count,
};

/// Element density. MD3 documents comfortable / compact densities in several
/// component specs (lists, menus, text fields).
enum class Density {
    Default,
    Comfortable,
    Compact,
    Count,
};

// ---------------------------------------------------------------------------
// Color roles
// ---------------------------------------------------------------------------

/// The complete MD3 color-role set, including the `-fixed` family. Ordering
/// follows the official `md.sys.color.*` token list.
enum class ColorRole {
    Primary,
    OnPrimary,
    PrimaryContainer,
    OnPrimaryContainer,

    Secondary,
    OnSecondary,
    SecondaryContainer,
    OnSecondaryContainer,

    Tertiary,
    OnTertiary,
    TertiaryContainer,
    OnTertiaryContainer,

    Error,
    OnError,
    ErrorContainer,
    OnErrorContainer,

    Background,
    OnBackground,

    Surface,
    OnSurface,
    SurfaceVariant,
    OnSurfaceVariant,
    SurfaceDim,
    SurfaceBright,
    SurfaceContainerLowest,
    SurfaceContainerLow,
    SurfaceContainer,
    SurfaceContainerHigh,
    SurfaceContainerHighest,

    InverseSurface,
    InverseOnSurface,
    InversePrimary,

    Outline,
    OutlineVariant,
    Scrim,
    Shadow,
    SurfaceTint,

    PrimaryFixed,
    PrimaryFixedDim,
    OnPrimaryFixed,
    OnPrimaryFixedVariant,

    SecondaryFixed,
    SecondaryFixedDim,
    OnSecondaryFixed,
    OnSecondaryFixedVariant,

    TertiaryFixed,
    TertiaryFixedDim,
    OnTertiaryFixed,
    OnTertiaryFixedVariant,

    Count,
};

// ---------------------------------------------------------------------------
// Type scale
// ---------------------------------------------------------------------------

/// The 15 MD3 type styles. Each has a baseline and an emphasized variant
/// (M3 Expressive), see TypeEmphasis.
enum class TypeStyle {
    DisplayLarge,
    DisplayMedium,
    DisplaySmall,
    HeadlineLarge,
    HeadlineMedium,
    HeadlineSmall,
    TitleLarge,
    TitleMedium,
    TitleSmall,
    BodyLarge,
    BodyMedium,
    BodySmall,
    LabelLarge,
    LabelMedium,
    LabelSmall,
    Count,
};

/// M3 Expressive ships a heavier emphasised alternative for every type style.
enum class TypeEmphasis {
    Baseline,
    Emphasized,
    Count,
};

/// Language-script height category. Line height is derived per category
/// because CJK glyphs need ~7% more vertical room than Latin.
enum class ScriptCategory {
    Small,      ///< Latin, Cyrillic, Greek
    Medium,     ///< CJK, Hangul, Kana
    Large,      ///< Devanagari, Bengali, Tamil
    ExtraLarge, ///< Tibetan, Mongolian
    Count,
};

// ---------------------------------------------------------------------------
// Shape
// ---------------------------------------------------------------------------

/// Shape scale corners. `*Increased` steps and `ExtraExtraLarge` are M3
/// Expressive additions.
enum class ShapeCorner {
    None,
    ExtraSmall,
    Small,
    Medium,
    Large,
    LargeIncreased,
    ExtraLarge,
    ExtraLargeIncreased,
    ExtraExtraLarge,
    Full,
    Count,
};

// ---------------------------------------------------------------------------
// Elevation
// ---------------------------------------------------------------------------

enum class ElevationLevel {
    Level0,
    Level1,
    Level2,
    Level3,
    Level4,
    Level5,
    Count,
};

// ---------------------------------------------------------------------------
// State layers
// ---------------------------------------------------------------------------

/// Static interaction overlays. Kept strictly separate from the ripple, which
/// only animates on press.
enum class StateLayerKind {
    Hover,
    Focus,
    Pressed,
    Dragged,
    Count,
};

// ---------------------------------------------------------------------------
// Motion
// ---------------------------------------------------------------------------

enum class MotionEasing {
    Linear,
    Standard,
    StandardAccelerate,
    StandardDecelerate,
    Emphasized,
    EmphasizedAccelerate,
    EmphasizedDecelerate,
    Legacy,
    LegacyAccelerate,
    LegacyDecelerate,
    Count,
};

enum class MotionDuration {
    Short1,
    Short2,
    Short3,
    Short4,
    Medium1,
    Medium2,
    Medium3,
    Medium4,
    Long1,
    Long2,
    Long3,
    Long4,
    ExtraLong1,
    ExtraLong2,
    ExtraLong3,
    ExtraLong4,
    Count,
};

/// M3 Expressive spring slots: spatial / effects x fast / default / slow.
enum class MotionSpring {
    SpatialFast,
    SpatialDefault,
    SpatialSlow,
    EffectsFast,
    EffectsDefault,
    EffectsSlow,
    Count,
};

// ---------------------------------------------------------------------------
// Icons
// ---------------------------------------------------------------------------

/// Material Symbols ships as three variable fonts, plus classic Material Icons
/// as SVGs. See MdIcon.
enum class MdIconFamily {
    Outlined,
    Rounded,
    Sharp,
    Count,
};

/// The classic (pre-Symbols) SVG baseline bundled in resources/icons/classic.
enum class MdIconSet {
    MaterialSymbols,
    Classic,
    /// Prefer Symbols, fall back to Classic when the font is unavailable.
    Auto,
    Count,
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Stable lowercase token name, e.g. "surface-container-high".
QT_MD3_EXPORT QString colorRoleName(ColorRole role);
QT_MD3_EXPORT QString typeStyleName(TypeStyle style);
QT_MD3_EXPORT QString shapeCornerName(ShapeCorner corner);
QT_MD3_EXPORT QString easingName(MotionEasing easing);
QT_MD3_EXPORT QString durationName(MotionDuration duration);
QT_MD3_EXPORT QString springName(MotionSpring spring);
QT_MD3_EXPORT QString iconFamilyName(MdIconFamily family);
QT_MD3_EXPORT QString iconSetName(MdIconSet set);
QT_MD3_EXPORT QString themeModeName(ThemeMode mode);
QT_MD3_EXPORT QString schemeVariantName(SchemeVariant variant);
QT_MD3_EXPORT QString contrastLevelName(ContrastLevel level);
QT_MD3_EXPORT QString typeEmphasisName(TypeEmphasis emphasis);
QT_MD3_EXPORT QString scriptCategoryName(ScriptCategory category);
QT_MD3_EXPORT QString stateLayerKindName(StateLayerKind kind);
QT_MD3_EXPORT QString elevationLevelName(ElevationLevel level);
QT_MD3_EXPORT QString densityName(Density density);

} // namespace md

Q_DECLARE_METATYPE(md::ThemeMode)
Q_DECLARE_METATYPE(md::SchemeVariant)
Q_DECLARE_METATYPE(md::ContrastLevel)
Q_DECLARE_METATYPE(md::ColorRole)
Q_DECLARE_METATYPE(md::TypeStyle)
Q_DECLARE_METATYPE(md::TypeEmphasis)
Q_DECLARE_METATYPE(md::ShapeCorner)
Q_DECLARE_METATYPE(md::ElevationLevel)
Q_DECLARE_METATYPE(md::MotionEasing)
Q_DECLARE_METATYPE(md::MotionDuration)
Q_DECLARE_METATYPE(md::MotionSpring)
Q_DECLARE_METATYPE(md::Density)
Q_DECLARE_METATYPE(md::TextDirection)
Q_DECLARE_METATYPE(md::MdIconFamily)
Q_DECLARE_METATYPE(md::MdIconSet)

#endif // MD_TYPES_H
