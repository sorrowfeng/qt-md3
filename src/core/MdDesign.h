#ifndef MD_DESIGN_H
#define MD_DESIGN_H

// MdDesign — the single startup entry point for an application that uses
// qt-md3.
//
//   md::MdDesign::configureHighDpi();   // BEFORE QApplication is constructed
//   QApplication app(argc, argv);
//   md::MdDesign::initialize(&app);     // fonts, base font, direction, theme

#include "QtMd3Export.h"

#include <QtCore/QString>

class QApplication;

namespace md {

class QT_MD3_EXPORT MdDesign
{
public:
    /// High-DPI attributes. Must run before QApplication exists; on Qt 6
    /// high-DPI scaling is always on, so this only sets the rounding policy.
    static void configureHighDpi();

    /// Load bundled fonts, install the MD3 base font, apply the theme's
    /// layout direction, and make sure the theme singleton exists.
    static void initialize(QApplication *app, const QString &languageTag = QString());

    /// Language tag applied by the last initialize() call.
    static QString languageTag();
};

} // namespace md

#endif // MD_DESIGN_H
