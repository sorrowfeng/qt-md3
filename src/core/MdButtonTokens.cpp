#include "MdButtonTokens.h"

#include <QtCore/QStringList>

namespace md {

namespace {

// ---------------------------------------------------------------------------
// Small parsing helpers for the md.comp.* override namespace
// ---------------------------------------------------------------------------

/// `md.comp.button.<size>.x` wins over `md.comp.button.x`.
///
/// `bag` may be null, in which case only the application-wide store is
/// consulted — which is what makes a per-instance override optional rather
/// than mandatory, matching the brief's two-level override requirement.
QString overrideFor(const MdComponentTokens *bag, const QStringList &keys)
{
    const MdComponentTokens &global = MdComponentTokens::global();
    for (const QString &key : keys) {
        if (bag) {
            // resolve() already walks instance-then-global, so an instance bag
            // never shadows the application-wide store by accident.
            const QString value = bag->resolve(key);
            if (!value.isEmpty()) {
                return value;
            }
        } else {
            const QString value = global.value(key);
            if (!value.isEmpty()) {
                return value;
            }
        }
    }
    return QString();
}

/// A length in logical px. Accepts a bare number and the `12px` form the SCSS
/// export uses, so a value can be pasted straight out of the token file.
bool parseLength(const QString &text, qreal *out)
{
    QString trimmed = text.trimmed();
    if (trimmed.endsWith(QLatin1String("px"), Qt::CaseInsensitive)) {
        trimmed.chop(2);
    }
    bool ok = false;
    const qreal value = trimmed.trimmed().toDouble(&ok);
    if (!ok) {
        return false;
    }
    *out = value;
    return true;
}

qreal lengthOverride(const MdComponentTokens *bag, const QStringList &keys, qreal fallback)
{
    qreal value = 0.0;
    if (parseLength(overrideFor(bag, keys), &value)) {
        return value;
    }
    return fallback;
}

/// Inverse of `shapeCornerName()`. Returns false for an unrecognised name so
/// a typo'd override falls back to the published value instead of silently
/// becoming `None`.
bool parseShapeCorner(const QString &text, ShapeCorner *out)
{
    QString name = text.trimmed().toLower();
    // Published token names are `md.sys.shape.corner.<name>`, but the override
    // key already ends in `.shape.round` / `.shape.square`, so repeating the
    // word is noise. Both "full" and "corner-full" are accepted.
    if (name.startsWith(QLatin1String("corner-"))) {
        name = name.mid(7);
    }

    for (int i = 0; i < int(ShapeCorner::Count); ++i) {
        const auto corner = ShapeCorner(i);
        QString candidate = shapeCornerName(corner);
        if (candidate.startsWith(QLatin1String("corner-"))) {
            candidate = candidate.mid(7);
        }
        if (candidate == name) {
            *out = corner;
            return true;
        }
    }
    return false;
}

ShapeCorner shapeOverride(const MdComponentTokens *bag, const QStringList &keys, ShapeCorner fallback)
{
    ShapeCorner corner = fallback;
    if (parseShapeCorner(overrideFor(bag, keys), &corner)) {
        return corner;
    }
    return fallback;
}

QStringList sizeKeys(const char *size, const char *token)
{
    return {QStringLiteral("md.comp.button.%1.%2").arg(QLatin1String(size), QLatin1String(token)),
            QStringLiteral("md.comp.button.%1").arg(QLatin1String(token))};
}

QStringList plainKeys(const char *token)
{
    return {QStringLiteral("md.comp.button.%1").arg(QLatin1String(token))};
}

// ---------------------------------------------------------------------------
// md.comp.button.<size>
// ---------------------------------------------------------------------------

struct SizeRow
{
    ButtonSize size;
    const char *token;          ///< the `md.comp.button.<token>` segment
    qreal height;
    qreal iconSize;
    qreal iconLabelSpace;
    qreal leadingSpace;
    qreal trailingSpace;
    qreal outlineWidth;
    TypeStyle labelStyle;
    ShapeCorner roundResting;
    ShapeCorner squareResting;
    ShapeCorner pressed;
};

const SizeRow kSizeRows[] = {
    // md.comp.button.xsmall.*
    {ButtonSize::XSmall, "xsmall", 32.0, 20.0, 8.0, 12.0, 12.0, 1.0, TypeStyle::LabelLarge,
     ShapeCorner::Full, ShapeCorner::Medium, ShapeCorner::Small},
    // md.comp.button.small.*
    {ButtonSize::Small, "small", 40.0, 20.0, 8.0, 16.0, 16.0, 1.0, TypeStyle::LabelLarge,
     ShapeCorner::Full, ShapeCorner::Medium, ShapeCorner::Small},
    // md.comp.button.medium.* — the first size to step up the type scale.
    {ButtonSize::Medium, "medium", 56.0, 24.0, 8.0, 24.0, 24.0, 1.0, TypeStyle::TitleMedium,
     ShapeCorner::Full, ShapeCorner::Large, ShapeCorner::Medium},
    // md.comp.button.large.*
    {ButtonSize::Large, "large", 96.0, 32.0, 12.0, 48.0, 48.0, 2.0, TypeStyle::HeadlineSmall,
     ShapeCorner::Full, ShapeCorner::ExtraLarge, ShapeCorner::Large},
    // md.comp.button.xlarge.*
    {ButtonSize::XLarge, "xlarge", 136.0, 40.0, 16.0, 64.0, 64.0, 3.0, TypeStyle::HeadlineLarge,
     ShapeCorner::Full, ShapeCorner::ExtraLarge, ShapeCorner::Large},
};

const SizeRow &sizeRow(ButtonSize size)
{
    for (const SizeRow &row : kSizeRows) {
        if (row.size == size) {
            return row;
        }
    }
    return kSizeRows[1]; // Small, the 40 px default the base token set matches.
}

// ---------------------------------------------------------------------------
// md.comp.button.<style> — colours
// ---------------------------------------------------------------------------

/// Build one variant's five state rows from the published per-state roles.
///
/// `containerless` variants (outlined, text) paint no fill, `outline` is
/// `ColorRole::Count` for every variant except outlined, and a variant with
/// `outline` set never paints a container. That split is exactly how the
/// upstream files are shaped: each style file republishes a full state matrix.
struct VariantRow
{
    ButtonVariant variant;
    // enabled / hovered / focused / pressed container + content + state layer
    ColorRole container;      ///< Count for container-less variants
    ColorRole content;        ///< label-text and icon share it in all 5 variants
    ColorRole stateLayer;
    ColorRole outline;        ///< only the outlined variant has one
    ElevationLevel enabledElevation;
    ElevationLevel hoveredElevation;
    ElevationLevel focusedElevation;
    ElevationLevel pressedElevation;
};

const VariantRow kVariantRows[] = {
    // md.comp.button.elevated.* — surface-container-low, +1 dp on hover only.
    {ButtonVariant::Elevated, ColorRole::SurfaceContainerLow, ColorRole::Primary, ColorRole::Primary,
     ColorRole::Count, ElevationLevel::Level1, ElevationLevel::Level2, ElevationLevel::Level1,
     ElevationLevel::Level1},
    // md.comp.button.filled.* — the only variant whose enabled elevation is 0
    // and whose hovered elevation is 1.
    {ButtonVariant::Filled, ColorRole::Primary, ColorRole::OnPrimary, ColorRole::OnPrimary,
     ColorRole::Count, ElevationLevel::Level0, ElevationLevel::Level1, ElevationLevel::Level0,
     ElevationLevel::Level0},
    // md.comp.button.tonal.*
    {ButtonVariant::Tonal, ColorRole::SecondaryContainer, ColorRole::OnSecondaryContainer,
     ColorRole::OnSecondaryContainer, ColorRole::Count, ElevationLevel::Level0,
     ElevationLevel::Level1, ElevationLevel::Level0, ElevationLevel::Level0},
    // md.comp.button.outlined.* — no container, outline-variant stroke in every
    // state including disabled.
    {ButtonVariant::Outlined, ColorRole::Count, ColorRole::OnSurfaceVariant,
     ColorRole::OnSurfaceVariant, ColorRole::OutlineVariant, ElevationLevel::Level0,
     ElevationLevel::Level0, ElevationLevel::Level0, ElevationLevel::Level0},
    // md.comp.button.text.* — no container and no outline. The export does
    // publish `disabled.container.color`, but the variant has no *enabled*
    // container token, so nothing is painted; giving it a fill only while
    // disabled would be a visible jump no part of the spec describes.
    {ButtonVariant::Text, ColorRole::Count, ColorRole::Primary, ColorRole::Primary, ColorRole::Count,
     ElevationLevel::Level0, ElevationLevel::Level0, ElevationLevel::Level0, ElevationLevel::Level0},
};

const VariantRow &variantRow(ButtonVariant variant)
{
    for (const VariantRow &row : kVariantRows) {
        if (row.variant == variant) {
            return row;
        }
    }
    return kVariantRows[1]; // Filled.
}

} // namespace

const MdButtonStateColours &MdButtonTokens::state(MdButtonState which) const
{
    switch (which) {
    case MdButtonState::Enabled: return enabled;
    case MdButtonState::Hovered: return hovered;
    case MdButtonState::Focused: return focused;
    case MdButtonState::Pressed: return pressed;
    case MdButtonState::Disabled: return disabled;
    case MdButtonState::Count: break;
    }
    return enabled;
}

MdButtonTokens MdButtonTokens::resolve(ButtonVariant variant,
                                       ButtonSize size,
                                       ButtonShape shape,
                                       const MdComponentTokens *overrides)
{
    const SizeRow &row = sizeRow(size);
    const VariantRow &style = variantRow(variant);
    MdButtonTokens tokens;

    // --- metrics: md.comp.button.<size>.<token> --------------------------
    tokens.containerHeight =
        lengthOverride(overrides, sizeKeys(row.token, "container.height"), row.height);
    tokens.iconSize = lengthOverride(overrides, sizeKeys(row.token, "icon.size"), row.iconSize);
    tokens.iconLabelSpace =
        lengthOverride(overrides, sizeKeys(row.token, "icon-label-space"), row.iconLabelSpace);
    tokens.leadingSpace =
        lengthOverride(overrides, sizeKeys(row.token, "leading-space"), row.leadingSpace);
    tokens.trailingSpace =
        lengthOverride(overrides, sizeKeys(row.token, "trailing-space"), row.trailingSpace);
    tokens.outlineWidth = lengthOverride(overrides, sizeKeys(row.token, "outlined.outline.width"),
                                         row.outlineWidth);
    tokens.labelStyle = row.labelStyle;

    // --- shape: container.shape.round | .square, then pressed.shape ------
    const ShapeCorner resting = shape == ButtonShape::Square ? row.squareResting : row.roundResting;
    tokens.restingShape = shapeOverride(overrides,
                                        sizeKeys(row.token,
                                                 shape == ButtonShape::Square
                                                     ? "container.shape.square"
                                                     : "container.shape.round"),
                                        resting);
    tokens.pressedShape =
        shapeOverride(overrides, sizeKeys(row.token, "pressed.container.shape"), row.pressed);

    // --- press morph spring: the whole size scale shares spring-fast-spatial
    tokens.springStiffness = lengthOverride(
        overrides, plainKeys("pressed.container.corner-size.motion.spring.stiffness"), 1400.0);
    tokens.springDampingRatio = lengthOverride(
        overrides, plainKeys("pressed.container.corner-size.motion.spring.damping"), 0.9);

    // --- disabled opacities ----------------------------------------------
    tokens.disabledContainerOpacity =
        lengthOverride(overrides, plainKeys("disabled.container.opacity"), 0.10);
    tokens.disabledIconOpacity = lengthOverride(overrides, plainKeys("disabled.icon.opacity"), 0.38);
    tokens.disabledLabelOpacity =
        lengthOverride(overrides, plainKeys("disabled.label-text.opacity"), 0.38);

    // --- focus indicator --------------------------------------------------
    tokens.focusIndicator = ColorRole::Secondary;
    tokens.focusIndicatorThickness =
        lengthOverride(overrides, plainKeys("focus.indicator.thickness"), 3.0);
    tokens.focusIndicatorOffset =
        lengthOverride(overrides, plainKeys("focus.indicator.outline.offset"), 2.0);

    // --- colours ----------------------------------------------------------
    const MdButtonColourSlot container{style.container, 1.0};
    const MdButtonColourSlot content{style.content, 1.0};

    tokens.enabled.container = container;
    tokens.enabled.labelText = content;
    tokens.enabled.icon = content;
    tokens.enabled.stateLayer = style.stateLayer;
    tokens.enabled.outline = style.outline;
    tokens.enabled.elevation = style.enabledElevation;

    tokens.hovered.container = container;
    tokens.hovered.labelText = content;
    tokens.hovered.icon = content;
    tokens.hovered.stateLayer = style.stateLayer;
    tokens.hovered.outline = style.outline;
    tokens.hovered.elevation = style.hoveredElevation;

    tokens.focused.container = container;
    tokens.focused.labelText = content;
    tokens.focused.icon = content;
    tokens.focused.stateLayer = style.stateLayer;
    tokens.focused.outline = style.outline;
    tokens.focused.elevation = style.focusedElevation;

    tokens.pressed.container = container;
    tokens.pressed.labelText = content;
    tokens.pressed.icon = content;
    tokens.pressed.stateLayer = style.stateLayer;
    tokens.pressed.outline = style.outline;
    tokens.pressed.elevation = style.pressedElevation;

    // Disabled is the one state whose roles are *not* the variant's: every
    // variant publishes `disabled.container.color: on-surface` and
    // `disabled.{icon,label-text}.color: on-surface`, faded by the three
    // opacities above. The outlined variant additionally keeps its outline.
    const MdButtonColourSlot disabledContainer{ColorRole::OnSurface,
                                               tokens.disabledContainerOpacity};
    const MdButtonColourSlot disabledContent{ColorRole::OnSurface, tokens.disabledLabelOpacity};
    tokens.disabled.container =
        style.container == ColorRole::Count ? MdButtonColourSlot() : disabledContainer;
    tokens.disabled.labelText = disabledContent;
    tokens.disabled.icon = MdButtonColourSlot{ColorRole::OnSurface, tokens.disabledIconOpacity};
    tokens.disabled.stateLayer = ColorRole::Count; // a disabled button takes no state layer
    tokens.disabled.outline = style.outline;
    tokens.disabled.elevation = ElevationLevel::Level0;

    return tokens;
}

} // namespace md
