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
// Components — button family
// ---------------------------------------------------------------------------

/// The five MD3 common button types. Each one is a separate published token
/// set named `md.comp.button.<style>`.
enum class ButtonVariant {
    Elevated,
    Filled,
    Tonal,
    Outlined,
    Text,
    Count,
};

/// The M3 Expressive button size scale, `md.comp.button.<size>`.
///
/// The scale is Expressive-only: material-web's pinned v0_192 export knows a
/// single 40 px button and publishes no size sets at all, so these five come
/// from the current token export. See docs/porting-todo.md.
enum class ButtonSize {
    XSmall, ///< 32 px
    Small,  ///< 40 px
    Medium, ///< 56 px
    Large,  ///< 96 px
    XLarge, ///< 136 px
    Count,
};

/// The two container shapes every button publishes a resting corner for:
/// `container.shape.round` and `container.shape.square`.
///
/// The names describe the *design intent*, not the literal corner count — both
/// are still fully rounded rectangles, and the square form's radius grows with
/// the size (12 / 12 / 16 / 28 / 28 px).
enum class ButtonShape {
    Round,
    Square,
    Count,
};

// ---------------------------------------------------------------------------
// Components — button group family
// ---------------------------------------------------------------------------

/// The two M3 Expressive button-group forms, `md.comp.button-group.<form>`.
///
/// The two differ in *what happens to the neighbours* when an item is
/// selected or pressed, and that is the whole of the difference:
///
///   Standard   one item's press changes its own width, shape and padding,
///              which pushes the items either side of it.
///   Connected  one item's press changes only that item's shape. The rest of
///              the group does not move.
///
/// Both are invisible containers: a button group has no colour, radius or
/// surface of its own. It contributes spacing and shape overrides to the
/// buttons inside it. See MdButtonGroup.
enum class ButtonGroupVariant {
    Standard,
    Connected,
    Count,
};

/// The axis a button group stacks its items along. M3 documents the
/// horizontal form; the vertical form is the same rules rotated, and the
/// tokens are axis-free.
enum class ButtonGroupOrientation {
    Horizontal,
    Vertical,
    Count,
};

/// How many items a button group lets be selected at once.
enum class ButtonGroupSelection {
    /// No selection model: the group is an evenly spaced row of actions.
    None,
    /// At most one item selected, and it can be deselected by clicking it.
    Single,
    /// Any number of items selected, each toggled independently.
    Multiple,
    /// Exactly one item selected: clicking the selected item is a no-op.
    /// The "selection-required" configuration in the spec.
    Required,
    Count,
};

/// The four colour styles of an MD3 icon button,
/// `md.comp.icon-button.<style>`.
///
/// The names follow the export, not the common button's: there is no
/// *elevated* icon button, and the plain style is *standard* rather than
/// *text* — a standard icon button has no container the way a text button
/// has no container, but the token family is named for the style it is the
/// baseline of, so it is recorded here under its own name.
enum class IconButtonVariant {
    Standard,
    Filled,
    Tonal,
    Outlined,
    Count,
};

/// Which of the three published padding tracks an icon button sizes its
/// container with: `md.comp.icon-button.<track>-leading-space` and
/// `-trailing-space`. The default track makes the container exactly square at
/// every size; the narrow and wide tracks are published sets with their own
/// numbers, so they are a real axis here rather than a comment.
enum class IconButtonSpaceTrack {
    Default,
    Narrow,
    Wide,
    Count,
};

// ---------------------------------------------------------------------------
// FAB
// ---------------------------------------------------------------------------

/// The four published FAB colour sets, `md.comp.fab.<variant>.*`:
/// surface | primary | secondary | tertiary. (The latest export also carries
/// `primary-container`-style tonal sets; see docs/porting-todo.md for why they
/// are not a variant here yet.)
enum class FabVariant {
    Surface,
    Primary,
    Secondary,
    Tertiary,
    Count,
};

/// The three published FAB sizes, `md.comp.fab.<size>.*`:
/// small (40 px, corner-medium), medium (56 px, corner-large) and
/// large (96 px, corner-extra-large).
enum class FabSize {
    Small,
    Medium,
    Large,
    Count,
};

/// The six published extended-FAB colour sets, `md.comp.extended-fab.<set>.*`.
/// The export ships no surface set for this family — unlike the FAB family —
/// so there is no surface value here; recorded in docs/porting-todo.md.
enum class ExtendedFabVariant {
    Primary,
    Secondary,
    Tertiary,
    PrimaryContainer,
    SecondaryContainer,
    TertiaryContainer,
    Count,
};

/// The three published extended-FAB sizes, `md.comp.extended-fab.<size>.*`:
/// small (56 px, title-medium), medium (80 px, title-large) and
/// large (96 px, headline-small).
enum class ExtendedFabSize {
    Small,
    Medium,
    Large,
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
QT_MD3_EXPORT QString buttonVariantName(ButtonVariant variant);
QT_MD3_EXPORT QString buttonSizeName(ButtonSize size);
QT_MD3_EXPORT QString buttonShapeName(ButtonShape shape);
QT_MD3_EXPORT QString buttonGroupVariantName(ButtonGroupVariant variant);
QT_MD3_EXPORT QString iconButtonVariantName(IconButtonVariant variant);
QT_MD3_EXPORT QString iconButtonSpaceTrackName(IconButtonSpaceTrack track);
QT_MD3_EXPORT QString fabVariantName(FabVariant variant);
QT_MD3_EXPORT QString fabSizeName(FabSize size);
QT_MD3_EXPORT QString extendedFabVariantName(ExtendedFabVariant variant);
QT_MD3_EXPORT QString extendedFabSizeName(ExtendedFabSize size);
QT_MD3_EXPORT QString buttonGroupOrientationName(ButtonGroupOrientation orientation);
QT_MD3_EXPORT QString buttonGroupSelectionName(ButtonGroupSelection selection);

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
Q_DECLARE_METATYPE(md::ButtonVariant)
Q_DECLARE_METATYPE(md::ButtonSize)
Q_DECLARE_METATYPE(md::ButtonShape)
Q_DECLARE_METATYPE(md::ButtonGroupVariant)
Q_DECLARE_METATYPE(md::ButtonGroupOrientation)
Q_DECLARE_METATYPE(md::ButtonGroupSelection)
Q_DECLARE_METATYPE(md::IconButtonVariant)
Q_DECLARE_METATYPE(md::IconButtonSpaceTrack)
Q_DECLARE_METATYPE(md::FabVariant)
Q_DECLARE_METATYPE(md::FabSize)
Q_DECLARE_METATYPE(md::ExtendedFabVariant)
Q_DECLARE_METATYPE(md::ExtendedFabSize)

#endif // MD_TYPES_H
