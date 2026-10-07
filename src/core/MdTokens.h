#ifndef MD_TOKENS_H
#define MD_TOKENS_H

// Three-layer MD3 token system.
//
//   reference (md.ref.*)     — the fixed baseline tonal palettes
//   system    (md.sys.*)     — colour roles, type, shape, elevation, motion
//   component (md.comp.*)    — per-component values, global + per-instance
//
// Every numeric value in this file was transcribed from the authoritative
// sources, never from memory:
//
//   * ref palette / sys shape / elevation / motion / state / typeface
//     -> material-components/material-web, tokens/versions/v0_192/*.scss
//        (design system "Google Material 3", version v0.192, Apache-2.0)
//   * the four M3 Expressive-only shape steps and the six spring slots
//     -> androidx Compose Material3 ShapeTokens.kt / ExpressiveMotionTokens.kt
//
// Values are logical pixels (CSS px == Qt logical px at 100% scaling).

#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QColor>

namespace md {

/// Token-table version this build is pinned to, for auditability.
QT_MD3_EXPORT QString tokenSourceVersion();

// ---------------------------------------------------------------------------
// Reference layer — md.ref.*
// ---------------------------------------------------------------------------

/// Access to the reference tonal palettes.
class QT_MD3_EXPORT MdReferenceTokens
{
public:
    /// Palette names: primary, secondary, tertiary, neutral, neutral-variant,
    /// error.
    static QStringList paletteNames();

    /// Every tone published for a palette, ascending (e.g. primary:
    /// 0 10 20 30 40 50 60 70 80 90 95 99 100). Empty for an unknown palette.
    static QList<int> tones(const QString &palette);

    /// Colour for palette+tone, e.g. toneColor("primary", 40) == #6750a4.
    /// Returns an invalid QColor for an unpublished tone.
    static QColor toneColor(const QString &palette, int tone);

    /// Colour by reference token name, e.g. "primary40" -> #6750a4,
    /// "neutral-variant90" -> #e7e0ec. Also accepts the literals "black"
    /// and "white". Returns an invalid QColor when unknown.
    static QColor referenceColor(const QString &token);

    /// Every reference token name ("primary40", "neutral98", ...).
    static QStringList referenceTokenNames();
};

// ---------------------------------------------------------------------------
// System layer — md.sys.*
// ---------------------------------------------------------------------------

/// Access to the system token tables.
class QT_MD3_EXPORT MdSystemTokens
{
public:
    // --- shape: md.sys.shape.corner.* ------------------------------------
    /// Rounded corner radius in logical px. ShapeCorner::Full returns -1,
    /// meaning "use half of the shorter side" (a pill / circle).
    static qreal cornerRadius(ShapeCorner corner);

    /// The four corner radii {top-left, top-right, bottom-right, bottom-left}.
    static QList<qreal> cornerRadii(ShapeCorner corner);

    /// Token name -> radius, in scale order. Includes the four M3 Expressive
    /// steps that are absent from the v0_192 static token export.
    static QList<QPair<ShapeCorner, qreal>> shapeScale();

    // --- elevation: md.sys.elevation.level* ------------------------------
    /// Elevation in dp (0, 1, 3, 6, 8, 12).
    static qreal elevationDp(ElevationLevel level);

    // --- motion: md.sys.motion.* -----------------------------------------
    /// Duration in milliseconds.
    static int durationMs(MotionDuration duration);

    /// Cubic-bezier control points {x1, y1, x2, y2}.
    static QList<qreal> easingBezier(MotionEasing easing);

    /// M3 Expressive spring parameters. `stiffness` is in the same unit as
    /// Compose's `Spring.Stiffness*`; `dampingRatio` is dimensionless.
    static void springParameters(MotionSpring spring, qreal *stiffness, qreal *dampingRatio);

    // --- state: md.sys.state.* -------------------------------------------
    /// Overlay opacity for hover / focus / pressed / dragged.
    static qreal stateLayerOpacity(StateLayerKind kind);

    // --- typeface: md.ref.typeface.* -------------------------------------
    /// Font family token: "brand" (display / headline / title-large) or
    /// "plain" (everything else).
    static QString typefaceFamily(const QString &token);
    /// Numeric weight for "weight-regular" / "weight-medium" / "weight-bold".
    static int typefaceWeight(const QString &token);
};

// ---------------------------------------------------------------------------
// Component layer — md.comp.*
// ---------------------------------------------------------------------------

/// A key/value bag for `md.comp.<component>.<part>.<property>` tokens.
///
/// Two override levels are supported, matching the brief: an application-wide
/// store (`global()`) and any number of per-instance copies that a single
/// component may hold to override just itself.
class QT_MD3_EXPORT MdComponentTokens
{
public:
    MdComponentTokens() = default;

    /// The application-wide store. Components fall back to this after their
    /// own instance-level overrides.
    static MdComponentTokens &global();

    /// Raw string value for an exact key, e.g.
    /// "md.comp.filled-button.container.height". Empty when unset.
    QString value(const QString &key) const;
    /// Value with a fallback, for convenience.
    QString value(const QString &key, const QString &fallback) const;

    void setValue(const QString &key, const QString &value);
    void remove(const QString &key);
    bool contains(const QString &key) const;
    void clear();

    /// All keys, sorted, for diagnostics and the token audit test.
    QStringList keys() const;

    /// Instance override, then global override, then `fallback`.
    QString resolve(const QString &key, const QString &fallback = QString()) const;

private:
    QHash<QString, QString> m_values;
};

} // namespace md

#endif // MD_TOKENS_H
