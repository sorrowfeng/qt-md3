#ifndef MD_COLOR_SCHEME_H
#define MD_COLOR_SCHEME_H

// A resolved set of MD3 colour roles.
//
// Two ways to obtain one:
//   * baseline()  — the static mapping published in material-web's
//                   _md-sys-color*.scss: six authored token sets, one per
//                   (mode, contrast level) pair.
//   * dynamic()   — generated from a seed colour via MdDynamicColor, which
//                   ports material-color-utilities' HCT pipeline.
//
// These are two different layers and deliberately do not agree. The published
// sets are authored on the M3 reference palette, where the primary family
// carries the seed's own chroma; the dynamic solver rebuilds the family at the
// chroma ColorSpec2021 prescribes for the variant. For the default purple that
// is #6750a4 (baseline) versus #65558f (tonal-spot). Neither is the "right"
// answer — pick the layer you want and stay in it.

#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QHash>
#include <QtGui/QColor>

namespace md {

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
/// Qt 5's QHash has no built-in enum support; Qt 6 does, so the overload
/// is only needed on the Qt 5 path.
inline uint qHash(ColorRole key, uint seed = 0) noexcept
{
    return ::qHash(static_cast<int>(key), seed);
}
#endif

class QT_MD3_EXPORT MdColorScheme
{
public:
    MdColorScheme();

    /// Static baseline scheme from the published reference palette.
    ///
    /// Six token sets are published — light/dark × standard/medium/high — and
    /// each is returned verbatim. `ContrastLevel::Reduced` has no published
    /// counterpart because it only exists in the dynamic pipeline, so it
    /// resolves to the standard set.
    static MdColorScheme baseline(ThemeMode mode,
                                  ContrastLevel level = ContrastLevel::Standard);

    /// Scheme generated from a seed colour (dynamic colour).
    static MdColorScheme dynamic(const QColor &seed,
                                 ThemeMode mode,
                                 SchemeVariant variant = SchemeVariant::TonalSpot,
                                 ContrastLevel contrast = ContrastLevel::Standard);

    QColor color(ColorRole role) const;
    void setColor(ColorRole role, const QColor &color);

    ThemeMode mode() const { return m_mode; }
    bool isDark() const { return m_mode == ThemeMode::Dark; }
    bool isDynamic() const { return m_dynamic; }
    SchemeVariant variant() const { return m_variant; }
    ContrastLevel contrastLevel() const { return m_contrast; }
    QColor seedColor() const { return m_seed; }

    /// Hex string for a role, e.g. "#6750a4" — used by the token audit test.
    QString hex(ColorRole role) const;

    const QHash<ColorRole, QColor> &colors() const { return m_colors; }

private:
    QHash<ColorRole, QColor> m_colors;
    ThemeMode m_mode = ThemeMode::Light;
    SchemeVariant m_variant = SchemeVariant::TonalSpot;
    ContrastLevel m_contrast = ContrastLevel::Standard;
    QColor m_seed;
    bool m_dynamic = false;
};

} // namespace md

#endif // MD_COLOR_SCHEME_H
