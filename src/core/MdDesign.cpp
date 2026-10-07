#include "MdDesign.h"

#include "MdFont.h"
#include "MdTheme.h"
#include "MdTypeScale.h"

#include <QtCore/QLocale>
#include <QtWidgets/QApplication>

namespace md {

namespace {
QString &storedLanguageTag()
{
    static QString tag;
    return tag;
}
} // namespace

void MdDesign::configureHighDpi()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif
    // Fractional scaling is the norm on Windows; PassThrough keeps the design
    // sizes honest at 125% / 150% instead of snapping to whole factors.
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
}

QString MdDesign::languageTag()
{
    return storedLanguageTag();
}

void MdDesign::initialize(QApplication *app, const QString &languageTag)
{
    MdFont::registerBundledFonts();

    QString tag = languageTag.trimmed();
    if (tag.isEmpty()) {
        tag = QLocale::system().name();
    }
    storedLanguageTag() = tag;

    MdTheme &theme = MdTheme::instance();
    theme.setLanguageTag(tag);

    // The base application font is body-medium, the MD3 default text style.
    const QFont baseFont = MdTypeScale::font(TypeStyle::BodyMedium,
                                            TypeEmphasis::Baseline,
                                            theme.scriptCategory());
    MdFont::applyApplicationFont(baseFont);

    if (app) {
        app->setLayoutDirection(theme.direction());
    }
}

} // namespace md
