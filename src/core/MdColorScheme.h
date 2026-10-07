#ifndef MD_COLOR_SCHEME_H
#define MD_COLOR_SCHEME_H

// A resolved set of MD3 colour roles.
//
// Two ways to obtain one:
//   * baseline()  — the static mapping published in material-web's
//                   _md-sys-color.scss (seed = the default purple).
//   * dynamic()   — generated from a seed colour via MdDynamicColor, which
//                   ports material-color-utilities' HCT pipeline.
//
// Light and dark are the same scheme evaluated at different tone mappings;
// they are not two hand-authored palettes.

#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QHash>
#include <QtGui/QColor>

namespace md {

class QT_MD3_EXPORT MdColorScheme
{
public:
    MdColorScheme();

    /// Static baseline scheme from the published reference palette.
    static MdColorScheme baseline(ThemeMode mode);

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
