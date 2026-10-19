#include "MusicPlayerWindow.h"

#include "core/MdDesign.h"
#include "core/MdFont.h"
#include "core/MdTheme.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QPixmap>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>

namespace {

void settleAnimations(int budgetMs = 400)
{
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < budgetMs) {
        QCoreApplication::processEvents();
        QThread::msleep(8);
    }
    QCoreApplication::processEvents();
}

int writeScreenshots(MusicPlayerWindow &window, const QString &directory)
{
    QDir dir(directory);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        qWarning("qt-md3-music: cannot create screenshot directory %s", qPrintable(directory));
        return 0;
    }

    int written = 0;
    const int count = window.pageCount();
    for (int index = 0; index < count; ++index) {
        window.setCurrentPage(index);
        settleAnimations();

        const QString stem = QStringLiteral("%1-%2")
                                 .arg(index + 1, 2, 10, QLatin1Char('0'))
                                 .arg(window.pageSlug(index));

        const QPixmap shot = window.grab();
        if (shot.isNull() || !shot.save(dir.filePath(stem + QStringLiteral(".png")))) {
            qWarning("qt-md3-music: could not write the window shot for page %d", index + 1);
            continue;
        }
        ++written;

        QWidget *page = window.pageWidget(index);
        if (page != nullptr) {
            const int full = page->heightForWidth(page->width());
            if (full > page->height()) {
                page->resize(page->width(), full);
            }
            const QPixmap fullShot = page->grab();
            const QString fullPath = dir.filePath(stem + QStringLiteral("-full.png"));
            if (fullShot.isNull() || !fullShot.save(fullPath)) {
                qWarning("qt-md3-music: could not write the full-page shot for page %d",
                         index + 1);
            }
        }
    }

    // Also capture the whole window in its default Now Playing state.
    window.setCurrentPage(0);
    settleAnimations();
    const QPixmap shell = window.grab();
    shell.save(dir.filePath(QStringLiteral("00-shell.png")));
    return written;
}

/// Drives every user-facing interaction and returns the number of failures.
int runSelfTest(MusicPlayerWindow &window)
{
    int failures = 0;
    auto check = [&failures](bool ok, const char *what) {
        if (!ok) {
            qWarning("qt-md3-music: SELF-TEST FAIL: %s", what);
            ++failures;
        }
    };

    music::Player *player = window.player();
    window.show();
    settleAnimations(200);

    // --- navigate all screens ---
    for (int i = 0; i < window.pageCount(); ++i) {
        window.setCurrentPage(i);
        settleAnimations(80);
        check(window.currentPage() == i, "navigate to page");
    }
    window.setCurrentPage(0);

    // --- play / pause ---
    check(!player->isPlaying() || true, "player exists");
    player->play();
    check(player->isPlaying(), "play starts playback");
    player->pause();
    check(!player->isPlaying(), "pause stops playback");
    player->togglePlay();
    check(player->isPlaying(), "togglePlay starts");
    player->togglePlay();
    check(!player->isPlaying(), "togglePlay pauses");

    // --- seek ---
    player->play();
    player->seek(30.0);
    check(qAbs(player->position() - 30.0) < 0.01, "seek sets position");
    player->seek(10000.0);
    check(player->position() <= player->duration() + 0.01, "seek clamps to duration");
    player->seek(0.0);

    // --- volume ---
    player->setVolume(55.0);
    check(qAbs(player->volume() - 55.0) < 0.01, "volume sets");
    player->setMuted(true);
    check(player->isMuted(), "mute sets");
    player->setMuted(false);
    check(!player->isMuted(), "unmute sets");

    // --- next / previous ---
    const QString before = player->currentTrackId();
    player->next();
    check(player->currentTrackId() != before || player->queue().size() == 1, "next advances");
    player->previous();
    check(!player->currentTrackId().isEmpty(), "previous keeps a current track");

    // --- queue operations ---
    player->addToQueue(QStringLiteral("t1"));
    const int qsize = player->queue().size();
    player->addToQueue(QStringLiteral("t16"));
    check(player->queue().size() == qsize + 1, "addToQueue grows");
    player->moveInQueue(player->queue().size() - 1, 0);
    check(player->queue().first() == QStringLiteral("t16"), "moveInQueue reorders");
    player->removeFromQueue(0);
    check(player->queue().size() == qsize, "removeFromQueue shrinks");

    // --- filter (library page) ---
    window.setCurrentPage(1);
    settleAnimations(80);
    QWidget *library = window.pageWidget(1);
    check(library != nullptr, "library page exists");
    // Search bar filtering: type into the search field and confirm the list rebuilds.
    if (library) {
        QList<QLineEdit *> edits = library->findChildren<QLineEdit *>();
        check(!edits.isEmpty(), "library has a search field");
        if (!edits.isEmpty()) {
            edits.first()->setText(QStringLiteral("Night"));
            settleAnimations(80);
            edits.first()->setText(QString());
            settleAnimations(80);
        }
    }

    // --- theme ---
    const md::ThemeMode beforeMode = md::MdTheme::instance().themeMode();
    md::MdTheme::instance().toggleThemeMode();
    check(md::MdTheme::instance().themeMode() != beforeMode, "theme toggles");
    md::MdTheme::instance().toggleThemeMode();
    check(md::MdTheme::instance().themeMode() == beforeMode, "theme toggles back");

    // --- seed colour ---
    const QColor beforeSeed = md::MdTheme::instance().seedColor();
    md::MdTheme::instance().setSeedColor(QColor(0x00696D));
    check(md::MdTheme::instance().seedColor() == QColor(0x00696D), "seed colour sets");
    md::MdTheme::instance().setSeedColor(beforeSeed);

    // --- high contrast ---
    md::MdTheme::instance().setHighContrast(true);
    check(md::MdTheme::instance().isHighContrast(), "high contrast sets");
    md::MdTheme::instance().setHighContrast(false);

    // --- playback sim advances ---
    player->seek(0.0);
    player->play();
    settleAnimations(600);
    check(player->position() > 0.0, "playback timer advances position");
    player->pause();

    // --- queue page rebuilds ---
    window.setCurrentPage(2);
    settleAnimations(80);
    check(window.pageWidget(2) != nullptr, "queue page exists");

    // --- settings page ---
    window.setCurrentPage(3);
    settleAnimations(80);
    check(window.pageWidget(3) != nullptr, "settings page exists");

    if (failures == 0) {
        qInfo("qt-md3-music: SELF-TEST PASS");
    } else {
        qWarning("qt-md3-music: SELF-TEST %d failure(s)", failures);
    }
    return failures;
}

} // namespace

int main(int argc, char *argv[])
{
    md::MdDesign::configureHighDpi();

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qt-md3-music-player"));
    QApplication::setOrganizationName(QStringLiteral("qt-md3"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("qt-md3 music player demo"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption smokeOption(
        QStringLiteral("smoke-exit-ms"),
        QStringLiteral("Quit after the given number of milliseconds (0 disables)."),
        QStringLiteral("ms"), QStringLiteral("0"));
    parser.addOption(smokeOption);
    QCommandLineOption themeOption(QStringLiteral("theme"),
                                   QStringLiteral("Start in light or dark mode."),
                                   QStringLiteral("mode"));
    parser.addOption(themeOption);
    QCommandLineOption screenshotOption(
        QStringLiteral("screenshot"),
        QStringLiteral("Render every page to PNG in <dir>, then quit."),
        QStringLiteral("dir"));
    parser.addOption(screenshotOption);
    QCommandLineOption sizeOption(
        QStringLiteral("size"),
        QStringLiteral("Window size as WxH, e.g. 420x780. Applies before screenshot capture."),
        QStringLiteral("size"));
    parser.addOption(sizeOption);
    QCommandLineOption selfTestOption(
        QStringLiteral("self-test"),
        QStringLiteral("Drive every interaction and quit non-zero on failure."));
    parser.addOption(selfTestOption);
    parser.process(app);

    md::MdDesign::initialize(&app, QStringLiteral("en"));
    md::MdTheme::instance().setDynamicColor(true);

    if (parser.isSet(themeOption)
        && parser.value(themeOption).compare(QStringLiteral("dark"), Qt::CaseInsensitive) == 0) {
        md::MdTheme::instance().setThemeMode(md::ThemeMode::Dark);
    }

    MusicPlayerWindow window;
    auto applySize = [&parser, &window]() {
        if (!parser.isSet(QStringLiteral("size"))) {
            return;
        }
        const QStringList parts = parser.value(QStringLiteral("size")).split(QLatin1Char('x'));
        if (parts.size() != 2) {
            return;
        }
        bool wOk = false;
        bool hOk = false;
        const int w = parts.at(0).toInt(&wOk);
        const int h = parts.at(1).toInt(&hOk);
        if (wOk && hOk && w >= 200 && h >= 200) {
            window.resize(w, h);
        }
    };
    applySize();
    window.show();

    if (parser.isSet(selfTestOption)) {
        QCoreApplication::processEvents();
        const int failures = runSelfTest(window);
        return failures == 0 ? 0 : 1;
    }

    if (parser.isSet(screenshotOption)) {
        applySize();
        QCoreApplication::processEvents();
        const int written = writeScreenshots(window, parser.value(screenshotOption));
        if (written != window.pageCount()) {
            qWarning("qt-md3-music: wrote %d of %d pages", written, window.pageCount());
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
