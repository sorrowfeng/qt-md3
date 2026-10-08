#ifndef MD_COMP_TOKEN_PARSE_H
#define MD_COMP_TOKEN_PARSE_H

// Shared parsing for the `md.comp.*` override namespace.
//
// Every component token resolver reads the same two kinds of value out of the
// same key shape, so they are parsed once here rather than re-written per
// component. Two properties matter and both are about failing *loudly at the
// right level*:
//
//   * an override string can be written the way the SCSS export writes it
//     (`12px`, `cubic-bezier(...)`-free numbers), so `12px` and `12` both parse;
//   * an override string that does not parse is *ignored*, not coerced. A
//     typo'd shape name must not silently become `None`, and a typo'd length
//     must not silently become 0. The published value stands instead.
//
// This header is internal: it is not in the installed header list.

#include "MdTokens.h"
#include "MdTypes.h"

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace md {
namespace comptoken {

/// First key in `keys` that has a value, searching the instance bag and then
/// the application-wide store.
///
/// `bag` may be null, in which case only the global store is consulted — which
/// is what makes a per-instance override optional rather than mandatory.
inline QString overrideFor(const MdComponentTokens *bag, const QStringList &keys)
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
inline bool parseLength(const QString &text, qreal *out)
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

inline qreal lengthOverride(const MdComponentTokens *bag, const QStringList &keys, qreal fallback)
{
    qreal value = 0.0;
    if (parseLength(overrideFor(bag, keys), &value)) {
        return value;
    }
    return fallback;
}

/// Inverse of `shapeCornerName()`.
///
/// Published token names are `md.sys.shape.corner.<name>`, but an override key
/// already says `.shape.` or `.corner-size`, so repeating the word is noise.
/// Both "full" and "corner-full" are accepted. Returns false for an
/// unrecognised name so a typo falls back to the published value.
inline bool parseShapeCorner(const QString &text, ShapeCorner *out)
{
    QString name = text.trimmed().toLower();
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

inline ShapeCorner shapeOverride(const MdComponentTokens *bag,
                                 const QStringList &keys,
                                 ShapeCorner fallback)
{
    ShapeCorner corner = fallback;
    if (parseShapeCorner(overrideFor(bag, keys), &corner)) {
        return corner;
    }
    return fallback;
}

/// `md.comp.<component>.<size>.<token>`, then `md.comp.<component>.<token>`.
///
/// The `md.comp.button` layout: one generic set plus a set per size.
inline QStringList sizeKeys(const char *component, const char *size, const char *token)
{
    return {QStringLiteral("md.comp.%1.%2.%3").arg(QLatin1String(component), QLatin1String(size),
                                                   QLatin1String(token)),
            QStringLiteral("md.comp.%1.%2").arg(QLatin1String(component), QLatin1String(token))};
}

/// `md.comp.<component>.<variant>.<size>.<token>`, then
/// `md.comp.<component>.<variant>.<token>`.
///
/// The `md.comp.button-group` layout: one set per variant, and within a variant
/// one set per size.
inline QStringList variantSizeKeys(const char *component,
                                   const char *variant,
                                   const char *size,
                                   const char *token)
{
    return {QStringLiteral("md.comp.%1.%2.%3.%4")
                .arg(QLatin1String(component), QLatin1String(variant), QLatin1String(size),
                     QLatin1String(token)),
            QStringLiteral("md.comp.%1.%2.%3")
                .arg(QLatin1String(component), QLatin1String(variant), QLatin1String(token))};
}

/// A single `md.comp.<component>.<token>` key.
inline QStringList plainKeys(const char *component, const char *token)
{
    return {QStringLiteral("md.comp.%1.%2").arg(QLatin1String(component), QLatin1String(token))};
}

} // namespace comptoken
} // namespace md

#endif // MD_COMP_TOKEN_PARSE_H
