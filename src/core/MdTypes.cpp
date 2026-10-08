#include "MdTypes.h"

namespace md {

QString colorRoleName(ColorRole role)
{
    switch (role) {
    case ColorRole::Primary: return QStringLiteral("primary");
    case ColorRole::OnPrimary: return QStringLiteral("on-primary");
    case ColorRole::PrimaryContainer: return QStringLiteral("primary-container");
    case ColorRole::OnPrimaryContainer: return QStringLiteral("on-primary-container");

    case ColorRole::Secondary: return QStringLiteral("secondary");
    case ColorRole::OnSecondary: return QStringLiteral("on-secondary");
    case ColorRole::SecondaryContainer: return QStringLiteral("secondary-container");
    case ColorRole::OnSecondaryContainer: return QStringLiteral("on-secondary-container");

    case ColorRole::Tertiary: return QStringLiteral("tertiary");
    case ColorRole::OnTertiary: return QStringLiteral("on-tertiary");
    case ColorRole::TertiaryContainer: return QStringLiteral("tertiary-container");
    case ColorRole::OnTertiaryContainer: return QStringLiteral("on-tertiary-container");

    case ColorRole::Error: return QStringLiteral("error");
    case ColorRole::OnError: return QStringLiteral("on-error");
    case ColorRole::ErrorContainer: return QStringLiteral("error-container");
    case ColorRole::OnErrorContainer: return QStringLiteral("on-error-container");

    case ColorRole::Background: return QStringLiteral("background");
    case ColorRole::OnBackground: return QStringLiteral("on-background");

    case ColorRole::Surface: return QStringLiteral("surface");
    case ColorRole::OnSurface: return QStringLiteral("on-surface");
    case ColorRole::SurfaceVariant: return QStringLiteral("surface-variant");
    case ColorRole::OnSurfaceVariant: return QStringLiteral("on-surface-variant");
    case ColorRole::SurfaceDim: return QStringLiteral("surface-dim");
    case ColorRole::SurfaceBright: return QStringLiteral("surface-bright");
    case ColorRole::SurfaceContainerLowest: return QStringLiteral("surface-container-lowest");
    case ColorRole::SurfaceContainerLow: return QStringLiteral("surface-container-low");
    case ColorRole::SurfaceContainer: return QStringLiteral("surface-container");
    case ColorRole::SurfaceContainerHigh: return QStringLiteral("surface-container-high");
    case ColorRole::SurfaceContainerHighest: return QStringLiteral("surface-container-highest");

    case ColorRole::InverseSurface: return QStringLiteral("inverse-surface");
    case ColorRole::InverseOnSurface: return QStringLiteral("inverse-on-surface");
    case ColorRole::InversePrimary: return QStringLiteral("inverse-primary");

    case ColorRole::Outline: return QStringLiteral("outline");
    case ColorRole::OutlineVariant: return QStringLiteral("outline-variant");
    case ColorRole::Scrim: return QStringLiteral("scrim");
    case ColorRole::Shadow: return QStringLiteral("shadow");
    case ColorRole::SurfaceTint: return QStringLiteral("surface-tint");

    case ColorRole::PrimaryFixed: return QStringLiteral("primary-fixed");
    case ColorRole::PrimaryFixedDim: return QStringLiteral("primary-fixed-dim");
    case ColorRole::OnPrimaryFixed: return QStringLiteral("on-primary-fixed");
    case ColorRole::OnPrimaryFixedVariant: return QStringLiteral("on-primary-fixed-variant");

    case ColorRole::SecondaryFixed: return QStringLiteral("secondary-fixed");
    case ColorRole::SecondaryFixedDim: return QStringLiteral("secondary-fixed-dim");
    case ColorRole::OnSecondaryFixed: return QStringLiteral("on-secondary-fixed");
    case ColorRole::OnSecondaryFixedVariant:
        return QStringLiteral("on-secondary-fixed-variant");

    case ColorRole::TertiaryFixed: return QStringLiteral("tertiary-fixed");
    case ColorRole::TertiaryFixedDim: return QStringLiteral("tertiary-fixed-dim");
    case ColorRole::OnTertiaryFixed: return QStringLiteral("on-tertiary-fixed");
    case ColorRole::OnTertiaryFixedVariant:
        return QStringLiteral("on-tertiary-fixed-variant");

    case ColorRole::Count: break;
    }
    return QStringLiteral("unknown");
}

QString typeStyleName(TypeStyle style)
{
    switch (style) {
    case TypeStyle::DisplayLarge: return QStringLiteral("display-large");
    case TypeStyle::DisplayMedium: return QStringLiteral("display-medium");
    case TypeStyle::DisplaySmall: return QStringLiteral("display-small");
    case TypeStyle::HeadlineLarge: return QStringLiteral("headline-large");
    case TypeStyle::HeadlineMedium: return QStringLiteral("headline-medium");
    case TypeStyle::HeadlineSmall: return QStringLiteral("headline-small");
    case TypeStyle::TitleLarge: return QStringLiteral("title-large");
    case TypeStyle::TitleMedium: return QStringLiteral("title-medium");
    case TypeStyle::TitleSmall: return QStringLiteral("title-small");
    case TypeStyle::BodyLarge: return QStringLiteral("body-large");
    case TypeStyle::BodyMedium: return QStringLiteral("body-medium");
    case TypeStyle::BodySmall: return QStringLiteral("body-small");
    case TypeStyle::LabelLarge: return QStringLiteral("label-large");
    case TypeStyle::LabelMedium: return QStringLiteral("label-medium");
    case TypeStyle::LabelSmall: return QStringLiteral("label-small");
    case TypeStyle::Count: break;
    }
    return QStringLiteral("unknown");
}

QString shapeCornerName(ShapeCorner corner)
{
    switch (corner) {
    case ShapeCorner::None: return QStringLiteral("corner-none");
    case ShapeCorner::ExtraSmall: return QStringLiteral("corner-extra-small");
    case ShapeCorner::Small: return QStringLiteral("corner-small");
    case ShapeCorner::Medium: return QStringLiteral("corner-medium");
    case ShapeCorner::Large: return QStringLiteral("corner-large");
    case ShapeCorner::LargeIncreased: return QStringLiteral("corner-large-increased");
    case ShapeCorner::ExtraLarge: return QStringLiteral("corner-extra-large");
    case ShapeCorner::ExtraLargeIncreased: return QStringLiteral("corner-extra-large-increased");
    case ShapeCorner::ExtraExtraLarge: return QStringLiteral("corner-extra-extra-large");
    case ShapeCorner::Full: return QStringLiteral("corner-full");
    case ShapeCorner::Count: break;
    }
    return QStringLiteral("unknown");
}

QString easingName(MotionEasing easing)
{
    switch (easing) {
    case MotionEasing::Linear: return QStringLiteral("easing-linear");
    case MotionEasing::Standard: return QStringLiteral("easing-standard");
    case MotionEasing::StandardAccelerate: return QStringLiteral("easing-standard-accelerate");
    case MotionEasing::StandardDecelerate: return QStringLiteral("easing-standard-decelerate");
    case MotionEasing::Emphasized: return QStringLiteral("easing-emphasized");
    case MotionEasing::EmphasizedAccelerate:
        return QStringLiteral("easing-emphasized-accelerate");
    case MotionEasing::EmphasizedDecelerate:
        return QStringLiteral("easing-emphasized-decelerate");
    case MotionEasing::Legacy: return QStringLiteral("easing-legacy");
    case MotionEasing::LegacyAccelerate: return QStringLiteral("easing-legacy-accelerate");
    case MotionEasing::LegacyDecelerate: return QStringLiteral("easing-legacy-decelerate");
    case MotionEasing::Count: break;
    }
    return QStringLiteral("unknown");
}

QString durationName(MotionDuration duration)
{
    switch (duration) {
    case MotionDuration::Short1: return QStringLiteral("duration-short1");
    case MotionDuration::Short2: return QStringLiteral("duration-short2");
    case MotionDuration::Short3: return QStringLiteral("duration-short3");
    case MotionDuration::Short4: return QStringLiteral("duration-short4");
    case MotionDuration::Medium1: return QStringLiteral("duration-medium1");
    case MotionDuration::Medium2: return QStringLiteral("duration-medium2");
    case MotionDuration::Medium3: return QStringLiteral("duration-medium3");
    case MotionDuration::Medium4: return QStringLiteral("duration-medium4");
    case MotionDuration::Long1: return QStringLiteral("duration-long1");
    case MotionDuration::Long2: return QStringLiteral("duration-long2");
    case MotionDuration::Long3: return QStringLiteral("duration-long3");
    case MotionDuration::Long4: return QStringLiteral("duration-long4");
    case MotionDuration::ExtraLong1: return QStringLiteral("duration-extra-long1");
    case MotionDuration::ExtraLong2: return QStringLiteral("duration-extra-long2");
    case MotionDuration::ExtraLong3: return QStringLiteral("duration-extra-long3");
    case MotionDuration::ExtraLong4: return QStringLiteral("duration-extra-long4");
    case MotionDuration::Count: break;
    }
    return QStringLiteral("unknown");
}

QString springName(MotionSpring spring)
{
    switch (spring) {
    case MotionSpring::SpatialFast: return QStringLiteral("spring-spatial-fast");
    case MotionSpring::SpatialDefault: return QStringLiteral("spring-spatial-default");
    case MotionSpring::SpatialSlow: return QStringLiteral("spring-spatial-slow");
    case MotionSpring::EffectsFast: return QStringLiteral("spring-effects-fast");
    case MotionSpring::EffectsDefault: return QStringLiteral("spring-effects-default");
    case MotionSpring::EffectsSlow: return QStringLiteral("spring-effects-slow");
    case MotionSpring::Count: break;
    }
    return QStringLiteral("unknown");
}

QString themeModeName(ThemeMode mode)
{
    switch (mode) {
    case ThemeMode::Light: return QStringLiteral("light");
    case ThemeMode::Dark: return QStringLiteral("dark");
    case ThemeMode::Count: break;
    }
    return QStringLiteral("unknown");
}

QString schemeVariantName(SchemeVariant variant)
{
    switch (variant) {
    case SchemeVariant::TonalSpot: return QStringLiteral("tonal-spot");
    case SchemeVariant::Vibrant: return QStringLiteral("vibrant");
    case SchemeVariant::Expressive: return QStringLiteral("expressive");
    case SchemeVariant::Content: return QStringLiteral("content");
    case SchemeVariant::Fidelity: return QStringLiteral("fidelity");
    case SchemeVariant::Monochrome: return QStringLiteral("monochrome");
    case SchemeVariant::Neutral: return QStringLiteral("neutral");
    case SchemeVariant::Rainbow: return QStringLiteral("rainbow");
    case SchemeVariant::FruitSalad: return QStringLiteral("fruit-salad");
    case SchemeVariant::Count: break;
    }
    return QStringLiteral("unknown");
}

QString contrastLevelName(ContrastLevel level)
{
    switch (level) {
    case ContrastLevel::Standard: return QStringLiteral("standard");
    case ContrastLevel::Medium: return QStringLiteral("medium");
    case ContrastLevel::High: return QStringLiteral("high");
    case ContrastLevel::Reduced: return QStringLiteral("reduced");
    case ContrastLevel::Count: break;
    }
    return QStringLiteral("unknown");
}

QString typeEmphasisName(TypeEmphasis emphasis)
{
    switch (emphasis) {
    case TypeEmphasis::Baseline: return QStringLiteral("baseline");
    case TypeEmphasis::Emphasized: return QStringLiteral("emphasized");
    case TypeEmphasis::Count: break;
    }
    return QStringLiteral("unknown");
}

QString scriptCategoryName(ScriptCategory category)
{
    switch (category) {
    case ScriptCategory::Small: return QStringLiteral("small");
    case ScriptCategory::Medium: return QStringLiteral("medium");
    case ScriptCategory::Large: return QStringLiteral("large");
    case ScriptCategory::ExtraLarge: return QStringLiteral("extra-large");
    case ScriptCategory::Count: break;
    }
    return QStringLiteral("unknown");
}

QString stateLayerKindName(StateLayerKind kind)
{
    switch (kind) {
    case StateLayerKind::Hover: return QStringLiteral("hover");
    case StateLayerKind::Focus: return QStringLiteral("focus");
    case StateLayerKind::Pressed: return QStringLiteral("pressed");
    case StateLayerKind::Dragged: return QStringLiteral("dragged");
    case StateLayerKind::Count: break;
    }
    return QStringLiteral("unknown");
}

QString elevationLevelName(ElevationLevel level)
{
    switch (level) {
    case ElevationLevel::Level0: return QStringLiteral("level0");
    case ElevationLevel::Level1: return QStringLiteral("level1");
    case ElevationLevel::Level2: return QStringLiteral("level2");
    case ElevationLevel::Level3: return QStringLiteral("level3");
    case ElevationLevel::Level4: return QStringLiteral("level4");
    case ElevationLevel::Level5: return QStringLiteral("level5");
    case ElevationLevel::Count: break;
    }
    return QStringLiteral("unknown");
}

QString densityName(Density density)
{
    switch (density) {
    case Density::Default: return QStringLiteral("default");
    case Density::Comfortable: return QStringLiteral("comfortable");
    case Density::Compact: return QStringLiteral("compact");
    case Density::Count: break;
    }
    return QStringLiteral("unknown");
}

QString buttonVariantName(ButtonVariant variant)
{
    // These are the `md.comp.button.<name>` segments, not the legacy
    // `md.comp.<name>-button` family names. The two spell "tonal" differently
    // (`md.comp.filled-tonal-button` versus `md.comp.button.tonal`), and the
    // component token keys follow the newer form.
    switch (variant) {
    case ButtonVariant::Elevated: return QStringLiteral("elevated");
    case ButtonVariant::Filled: return QStringLiteral("filled");
    case ButtonVariant::Tonal: return QStringLiteral("tonal");
    case ButtonVariant::Outlined: return QStringLiteral("outlined");
    case ButtonVariant::Text: return QStringLiteral("text");
    case ButtonVariant::Count: break;
    }
    return QStringLiteral("unknown");
}

QString buttonSizeName(ButtonSize size)
{
    switch (size) {
    case ButtonSize::XSmall: return QStringLiteral("xsmall");
    case ButtonSize::Small: return QStringLiteral("small");
    case ButtonSize::Medium: return QStringLiteral("medium");
    case ButtonSize::Large: return QStringLiteral("large");
    case ButtonSize::XLarge: return QStringLiteral("xlarge");
    case ButtonSize::Count: break;
    }
    return QStringLiteral("unknown");
}

QString buttonShapeName(ButtonShape shape)
{
    switch (shape) {
    case ButtonShape::Round: return QStringLiteral("round");
    case ButtonShape::Square: return QStringLiteral("square");
    case ButtonShape::Count: break;
    }
    return QStringLiteral("unknown");
}

QString buttonGroupVariantName(ButtonGroupVariant variant)
{
    // The `md.comp.button-group.<name>` segments. The spec calls the two forms
    // "standard button group" and "connected button group"; the token keys use
    // the single word.
    switch (variant) {
    case ButtonGroupVariant::Standard: return QStringLiteral("standard");
    case ButtonGroupVariant::Connected: return QStringLiteral("connected");
    case ButtonGroupVariant::Count: break;
    }
    return QStringLiteral("unknown");
}

QString buttonGroupOrientationName(ButtonGroupOrientation orientation)
{
    switch (orientation) {
    case ButtonGroupOrientation::Horizontal: return QStringLiteral("horizontal");
    case ButtonGroupOrientation::Vertical: return QStringLiteral("vertical");
    case ButtonGroupOrientation::Count: break;
    }
    return QStringLiteral("unknown");
}

QString buttonGroupSelectionName(ButtonGroupSelection selection)
{
    // Worded as the spec's "Configurations" table words them.
    switch (selection) {
    case ButtonGroupSelection::None: return QStringLiteral("none");
    case ButtonGroupSelection::Single: return QStringLiteral("single-select");
    case ButtonGroupSelection::Multiple: return QStringLiteral("multi-select");
    case ButtonGroupSelection::Required: return QStringLiteral("selection-required");
    case ButtonGroupSelection::Count: break;
    }
    return QStringLiteral("unknown");
}

QString iconFamilyName(MdIconFamily family)
{
    switch (family) {
    case MdIconFamily::Outlined: return QStringLiteral("outlined");
    case MdIconFamily::Rounded: return QStringLiteral("rounded");
    case MdIconFamily::Sharp: return QStringLiteral("sharp");
    case MdIconFamily::Count: break;
    }
    return QStringLiteral("unknown");
}

QString iconSetName(MdIconSet set)
{
    switch (set) {
    case MdIconSet::MaterialSymbols: return QStringLiteral("material-symbols");
    case MdIconSet::Classic: return QStringLiteral("classic");
    case MdIconSet::Auto: return QStringLiteral("auto");
    case MdIconSet::Count: break;
    }
    return QStringLiteral("unknown");
}

} // namespace md
