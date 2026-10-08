#include "gallery/GalleryWindow.h"

#include "core/MdCore.h"
#include "core/MdDesign.h"
#include "core/MdIcon.h"
#include "core/MdTheme.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>

namespace {

/// Turns a page title into a file-name-safe slug: "Colour & type" -> "colour-type".
QString slugify(const QString &title)
{
    QString slug;
    slug.reserve(title.size());
    for (const QChar &character : title) {
        if (character.isLetterOrNumber()) {
            slug.append(character.toLower());
        } else if (!slug.endsWith(QLatin1Char('-')) && !slug.isEmpty()) {
            slug.append(QLatin1Char('-'));
        }
    }
    while (slug.endsWith(QLatin1Char('-'))) {
        slug.chop(1);
    }
    return slug.isEmpty() ? QStringLiteral("page") : slug;
}

/// Renders every page to `directory` and returns the number written.
///
/// This is a verification hook, not a feature of the gallery. A page whose
/// paintEvent silently draws nothing still exits 0, so "the window opened" is
/// not evidence that anything rendered; a PNG per page is.
///
/// Two images per page:
///
///   * `<nn>-<slug>.png`       the whole window, i.e. what a user sees on
///                             opening the page — nav, header and all.
///   * `<nn>-<slug>-full.png`  the page widget alone, grown to its own
///                             heightForWidth(). A page is routinely taller
///                             than the viewport, so the window shot can only
///                             ever prove that the *top* of a page draws. This
///                             one proves the whole page does.
int writeScreenshots(gallery::GalleryWindow &window, const QString &directory)
{
    QDir dir(directory);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        qWarning("qt-md3: cannot create screenshot directory %s", qPrintable(directory));
        return 0;
    }

    int written = 0;
    const int count = window.pageCount();
    for (int index = 0; index < count; ++index) {
        window.setCurrentPage(index);
        // Let the stack swap and the scroll area relayout before grabbing.
        QCoreApplication::processEvents();

        const QString stem = QStringLiteral("%1-%2")
                                 .arg(index + 1, 2, 10, QLatin1Char('0'))
                                 .arg(slugify(window.pageSlug(index)));

        const QPixmap shot = window.grab();
        if (shot.isNull() || !shot.save(dir.filePath(stem + QStringLiteral(".png")))) {
            qWarning("qt-md3: could not write the window shot for page %d", index + 1);
            continue;
        }
        ++written;

        // Grow the page to its full height and grab it directly. No
        // processEvents() in between: the scroll area would immediately shrink
        // it back to the viewport, and resize() has already delivered the
        // QResizeEvent that repositions the page's children.
        QWidget *page = window.pageWidget(index);
        if (page != nullptr) {
            const int full = page->heightForWidth(page->width());
            if (full > page->height()) {
                page->resize(page->width(), full);
            }
            const QPixmap fullShot = page->grab();
            const QString fullPath = dir.filePath(stem + QStringLiteral("-full.png"));
            if (fullShot.isNull() || !fullShot.save(fullPath)) {
                qWarning("qt-md3: could not write the full-page shot for page %d", index + 1);
            }
        }
    }
    return written;
}

} // namespace

// qt-md3 example application: the component gallery.
//
// The gallery is the inspection surface for the library. It is built into the
// project's build/ directory and is never committed. It performs no styling of
// its own — every colour, font, radius and duration comes from the theme — so
// if something looks wrong here, the fault is in the tokens or the base
// modules rather than in the example.

int main(int argc, char *argv[])
{
    // High-DPI policy first, before QApplication exists.
    md::MdDesign::configureHighDpi();

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qt-md3-gallery"));
    QApplication::setApplicationVersion(QString::fromUtf8(md::libraryVersion()));
    QApplication::setOrganizationName(QStringLiteral("qt-md3"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("qt-md3 component gallery"));
    parser.addHelpOption();
    parser.addVersionOption();

    // Smoke hook for CI: quit automatically after N milliseconds.
    QCommandLineOption smokeOption(
        QStringLiteral("smoke-exit-ms"),
        QStringLiteral("Quit after the given number of milliseconds (0 disables)."),
        QStringLiteral("ms"), QStringLiteral("0"));
    parser.addOption(smokeOption);
    QCommandLineOption themeOption(QStringLiteral("theme"),
                                   QStringLiteral("Start in light or dark mode."),
                                   QStringLiteral("mode"));
    parser.addOption(themeOption);
    parser.addOption(QCommandLineOption(QStringLiteral("dynamic"),
                                        QStringLiteral("Start with dynamic colour enabled.")));
    QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Render every page to PNG in <dir>, then quit."),
        QStringLiteral("dir"));
    parser.addOption(screenshotOption);
    parser.process(app);

    // Registers bundled fonts, sets the language and the base font, applies
    // the layout direction. This is the one entry point every consumer calls.
    // The gallery defaults to Simplified Chinese; the header's language button
    // toggles between zh-Hans and en.
    const QString languageTag = QStringLiteral("zh-Hans");
    md::MdDesign::initialize(&app, languageTag);

    // The icon system works with or without the Material Symbols font, but it
    // is worth knowing which one this build resolved to.
    if (!md::MdIcon::isFontAvailable(md::MdIconFamily::Outlined)) {
        qInfo("qt-md3: Material Symbols is not installed; icons fall back to the bundled "
              "classic Material Icons SVG baseline.");
    }

    if (parser.isSet(themeOption)
        && parser.value(themeOption).compare(QStringLiteral("dark"), Qt::CaseInsensitive) == 0) {
        md::MdTheme::instance().setThemeMode(md::ThemeMode::Dark);
    }
    if (parser.isSet(QStringLiteral("dynamic"))) {
        md::MdTheme::instance().setDynamicColor(true);
    }

    gallery::GalleryWindow window;
    window.show();

    if (parser.isSet(screenshotOption)) {
        // Deliberately after show(): the pages size themselves from the real
        // viewport, so grabbing before the window is mapped would capture a
        // layout that never existed.
        QCoreApplication::processEvents();
        const int written = writeScreenshots(window, parser.value(screenshotOption));
        if (written != window.pageCount()) {
            qWarning("qt-md3: wrote %d of %d pages", written, window.pageCount());
            return 1;
        }
        return 0;
    }

    const int smokeMs = parser.value(smokeOption).toInt();
    if (smokeMs > 0) {
        QTimer::singleShot(smokeMs, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
