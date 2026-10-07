#ifndef MD_FONT_H
#define MD_FONT_H

// Font loading and application-wide font installation.
//
// MD3's default families are Roboto (brand) and Roboto Flex; Material Symbols
// is the icon face with four variable axes (fill, weight, grade, optical
// size). Bundled faces live under resources/fonts and are exposed through the
// `:/qt-md3/fonts` resource prefix.

#include "QtMd3Export.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QFont>

namespace md {

class QT_MD3_EXPORT MdFont
{
public:
    /// Resource directory scanned by registerBundledFonts().
    static QString bundledResourceDir();

    /// Register every .ttf/.otf under the bundled resource directory.
    /// Returns the number of faces added.
    static int registerBundledFonts();

    /// Register a font file from disk or from a resource path.
    /// Returns the family names added, empty on failure.
    static QStringList registerFontFile(const QString &filePath);

    /// Apply a font as QApplication's default.
    static void applyApplicationFont(const QFont &font);

    /// Family name of the Material Symbols face once registered, else empty.
    static QString materialSymbolsFamily();

    /// An icon font configured for Material Symbols' variable axes.
    /// `fill` 0/1, `weight` 100..700, `grade` -25..200.
    static QFont iconFont(qreal pixelSize = 24.0, int fill = 0, int weight = 400, int grade = 0);

    /// Whether a family is actually installed on this system.
    static bool hasFamily(const QString &family);
};

} // namespace md

#endif // MD_FONT_H
