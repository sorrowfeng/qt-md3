#ifndef MD_ICON_H
#define MD_ICON_H

// MdIcon — MD3 icons.
//
// MD3 icons are Material Symbols: a *variable font* with four axes.
//
//   FILL 0..1      outlined glyph <-> filled glyph
//   wght 100..700  stroke weight
//   GRAD -50..200  grade (optical weight, does not change advance width)
//   opsz 20..48    optical size
//
// The brief is explicit that a static SVG collection is not an acceptable
// substitute, so MdIcon is built around the font first. Two things make that
// workable without shipping a 10 MB binary in the repository:
//
//   1. The authoritative codepoint table
//      (resources/icons/material-symbols-codepoints.txt, 4299 names, taken
//      verbatim from google/material-design-icons) is bundled, so name ->
//      glyph lookup is exact and complete.
//   2. The font file itself is loaded from the resource system *if present*.
//      When it is not, MdIcon degrades to the bundled classic Material Icons
//      SVG baseline instead of drawing nothing. See docs/resources-manifest.md
//      for the two supported ways to provide the font.
//
// Axes are applied through QFont::setVariableAxis, which needs Qt 6.7+. On
// Qt 6.5/6.6 the axes are recorded but only the named weights can be honoured,
// and MdIcon::axesSupported() returns false so callers can say so in the UI.

#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QPixmap>

class QPainter;
class QRectF;

namespace md {

/// The four Material Symbols variable axes.
struct QT_MD3_EXPORT MdIconStyle
{
    /// FILL, 0 (outlined) .. 1 (filled).
    qreal fill = 0.0;
    /// wght, 100..700. The Material Symbols default is 400.
    qreal weight = 400.0;
    /// GRAD, -50..200.
    qreal grade = 0.0;
    /// opsz, 20..48. Also the nominal optical size in dp.
    qreal opticalSize = 24.0;

    bool isDefault() const
    {
        return qFuzzyCompare(fill, 0.0) && qFuzzyCompare(weight, 400.0)
               && qFuzzyCompare(grade, 0.0) && qFuzzyCompare(opticalSize, 24.0);
    }
};

class QT_MD3_EXPORT MdIcon
{
public:
    // --- codepoint table -------------------------------------------------
    /// `:/qt-md3/icons/material-symbols-codepoints.txt`.
    static QString codepointsResourcePath();
    /// Parses the bundled table. Called automatically on first lookup.
    static bool loadCodepoints();
    static bool loadCodepoints(const QString &resourcePath);
    /// Parses "name codepoint" lines, as published upstream.
    static bool loadCodepointsFromData(const QByteArray &data);
    /// Add or replace one entry (for application-specific icon subsets).
    static void registerIcon(const QString &name, uint codepoint);

    static bool contains(const QString &name);
    static uint codepoint(const QString &name);
    /// The single character to draw, or an empty string when unknown.
    static QString glyph(const QString &name);
    /// Every known name, sorted. Empty until the table is loaded.
    static QStringList names();
    static int count();

    // --- Material Symbols font -------------------------------------------
    /// The exact font family name, e.g. "Material Symbols Outlined".
    static QString fontFamily(MdIconFamily family = MdIconFamily::Outlined);
    /// `md.comp.icon.size` — 24px.
    static qreal defaultSize();
    /// Builds the font for the family at `pixelSize`, with the style's axes
    /// applied. Callers normally let paint() do this.
    static QFont font(MdIconFamily family = MdIconFamily::Outlined,
                      const MdIconStyle &style = MdIconStyle(),
                      qreal pixelSize = 24.0);
    /// True when the platform actually resolved `family` to a real font.
    static bool isFontAvailable(MdIconFamily family = MdIconFamily::Outlined);
    /// True when variable axes can be applied (Qt 6.7+).
    static bool axesSupported();

    // --- classic Material Icons SVG baseline ------------------------------
    static QString classicResourceDir();
    static QStringList classicNames();
    static bool hasClassicIcon(const QString &name);
    static QString classicPath(const QString &name);
    /// The SVG, tinted with `color` and rasterised at `size` device pixels.
    /// Results are cached; the cache is dropped on a device-pixel-ratio change.
    static QPixmap classicPixmap(const QString &name, int size, const QColor &color);

    // --- unified entry points --------------------------------------------
    /// Which of the two back ends would be used for `set`.
    static MdIconSet resolveSet(MdIconSet set);

    /// Draw `name` centred inside `rect`.
    /// Returns false when neither back end could supply the icon, so callers
    /// can substitute a placeholder rather than paint an empty box.
    static bool paint(QPainter *painter,
                      const QRectF &rect,
                      const QString &name,
                      const QColor &color,
                      MdIconSet set = MdIconSet::Auto,
                      MdIconFamily family = MdIconFamily::Outlined,
                      const MdIconStyle &style = MdIconStyle());

    /// The square box an icon of `opticalSize` dp occupies.
    static QSizeF preferredSize(qreal opticalSize = 24.0);
};

} // namespace md

#endif // MD_ICON_H
