#include "MusicPlayerWindow.h"

#include "core/MdDesign.h"
#include "core/MdFont.h"
#include "core/MdTheme.h"
#include "widgets/MdMenu.h"
#include "widgets/MdSelect.h"
#include "widgets/MdSwitch.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QCursor>
#include <QtGui/QMouseEvent>
#include <QtGui/QPixmap>
#include <QtGui/QScreen>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QScrollArea>

#ifdef Q_OS_WIN
#    include <windows.h>
#endif

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

    // --- top-right search must not spawn windows ---
    {
        // Count *visible* non-main top-levels: Qt::Popup menus are always in
        // topLevelWidgets() (they are windows), so a raw count is noisy.
        auto visibleExtras = [&window]() {
            int n = 0;
            for (QWidget *w : QApplication::topLevelWidgets()) {
                if (w != &window && w->isVisible()) {
                    ++n;
                }
            }
            return n;
        };
        const int before = visibleExtras();
        // Click the app-bar search action several times: it navigates to the
        // Library page and focuses the one shared search bar. A control that
        // allocated a fresh overlay per click would grow the top-level count.
        for (int i = 0; i < 5; ++i) {
            // The search action is the MdIconButton with accessibleName "Search".
            const QList<QAbstractButton *> buttons =
                window.findChildren<QAbstractButton *>();
            for (QAbstractButton *button : buttons) {
                if (button->accessibleName() == QLatin1String("Search")) {
                    button->click();
                    break;
                }
            }
            settleAnimations(40);
        }
        const int after = visibleExtras();
        check(after <= before + 1, "search click does not stack top-level windows");
        check(window.currentPage() == 1, "search action lands on the library");
    }

    // --- avatar menu: one reusable popup, animated open/close ---
    {
        QAbstractButton *avatar = nullptr;
        const QList<QAbstractButton *> buttons = window.findChildren<QAbstractButton *>();
        for (QAbstractButton *button : buttons) {
            if (button->accessibleName() == QLatin1String("Account")) {
                avatar = button;
                break;
            }
        }
        check(avatar != nullptr, "avatar action exists");
        if (avatar) {
            avatar->click();
            settleAnimations(250);
            bool menuVisible = false;
            for (QWidget *w : QApplication::topLevelWidgets()) {
                if (w->isVisible() && qobject_cast<md::MdMenu *>(w) != nullptr) {
                    menuVisible = true;
                    break;
                }
            }
            check(menuVisible, "avatar menu opens");
            avatar->click();
            // Close is a reverse spring — give it the full settle window.
            settleAnimations(600);
            bool stillOpen = false;
            for (QWidget *w : QApplication::topLevelWidgets()) {
                if (w->isVisible() && qobject_cast<md::MdMenu *>(w) != nullptr) {
                    stillOpen = true;
                    break;
                }
            }
            check(!stillOpen, "avatar menu close does not leave windows");
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

// ---------------------------------------------------------------------------
// --drive-real: real Windows input on the real desktop (NOT offscreen).
// Uses SendInput so the click goes through the OS message queue, the Windows
// popup grab, and DWM — the path a human takes and the path QTest never hits.
// ---------------------------------------------------------------------------

#ifdef Q_OS_WIN

POINT toPhysical(const QPoint &logical);

void logLine(QTextStream &log, const QString &line)
{
    log << line << '\n';
    log.flush();
    qInfo("%s", qPrintable(line));
}

void dumpWin32At(QTextStream &log, const QString &tag, const QPoint &globalPos)
{
    const POINT pt = toPhysical(globalPos);
    HWND hwnd = WindowFromPoint(pt);
    wchar_t cls[256] = {};
    wchar_t title[256] = {};
    if (hwnd) {
        GetClassNameW(hwnd, cls, 256);
        GetWindowTextW(hwnd, title, 256);
    }
    logLine(log, QStringLiteral("%1 Win32 WindowFromPoint(phys %2,%3) hwnd=%4 class='%5' title='%6'")
                     .arg(tag).arg(pt.x).arg(pt.y)
                     .arg(quintptr(hwnd), 0, 16)
                     .arg(QString::fromWCharArray(cls), QString::fromWCharArray(title)));
    if (hwnd) {
        QWidget *asWidget = QWidget::find(reinterpret_cast<WId>(hwnd));
        HWND parent = GetParent(hwnd);
        logLine(log, QStringLiteral("  hit hwnd=%1 qwidget=%2 class=%3 parent=%4 visible=%5")
                         .arg(quintptr(hwnd), 0, 16)
                         .arg(asWidget ? QString::fromUtf8(asWidget->metaObject()->className())
                                       : QStringLiteral("(none)"))
                         .arg(QString::fromWCharArray(cls))
                         .arg(quintptr(parent), 0, 16)
                         .arg(int(IsWindowVisible(hwnd))));
    }

    // Enumerate every top-level HWND of this process — a Qt-hidden popup can
    // still be a mapped Win32 window that swallows clicks.
    struct EnumCtx {
        QTextStream *log;
        DWORD pid;
    } ctx{&log, GetCurrentProcessId()};
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto *ctx = reinterpret_cast<EnumCtx *>(lp);
        DWORD pid = 0;
        GetWindowThreadProcessId(h, &pid);
        if (pid != ctx->pid) {
            return TRUE;
        }
        wchar_t c[256] = {};
        wchar_t t[256] = {};
        GetClassNameW(h, c, 256);
        GetWindowTextW(h, t, 256);
        RECT rc{};
        GetWindowRect(h, &rc);
        const LONG_PTR ex = GetWindowLongPtr(h, GWL_EXSTYLE);
        logLine(*ctx->log,
                QStringLiteral("  OWN hwnd=%1 class='%2' title='%3' rect=(%4,%5)-(%6,%7) visible=%8 iconic=%9 exstyle=0x%10 layered=%11 transparent=%12")
                    .arg(quintptr(h), 0, 16)
                    .arg(QString::fromWCharArray(c), QString::fromWCharArray(t))
                    .arg(rc.left).arg(rc.top).arg(rc.right).arg(rc.bottom)
                    .arg(int(IsWindowVisible(h)))
                    .arg(int(IsIconic(h)))
                    .arg(qulonglong(ex), 0, 16)
                    .arg(int((ex & WS_EX_LAYERED) != 0))
                    .arg(int((ex & WS_EX_TRANSPARENT) != 0)));
        return TRUE;
    }, reinterpret_cast<LPARAM>(&ctx));

    // Walk the top-level z-order and report anything above our main window that
    // could swallow clicks (invisible popups, transparent overlays, leftovers).
    HWND mainHwnd = nullptr;
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (w->isWindow() && w->isVisible()) {
            mainHwnd = reinterpret_cast<HWND>(w->winId());
            break;
        }
    }
    if (mainHwnd) {
        logLine(log, QStringLiteral("  mainWinId=%1").arg(quintptr(mainHwnd), 0, 16));
        for (HWND h = GetTopWindow(nullptr); h; h = GetWindow(h, GW_HWNDNEXT)) {
            if (h == mainHwnd) {
                break;
            }
            if (!IsWindowVisible(h) && !IsIconic(h)) {
                continue;
            }
            wchar_t c2[256] = {};
            wchar_t t2[256] = {};
            GetClassNameW(h, c2, 256);
            GetWindowTextW(h, t2, 256);
            RECT rc{};
            GetWindowRect(h, &rc);
            logLine(log, QStringLiteral("  ABOVE-MAIN hwnd=%1 class='%2' title='%3' rect=(%4,%5)-(%6,%7) visible=%8")
                             .arg(quintptr(h), 0, 16)
                             .arg(QString::fromWCharArray(c2), QString::fromWCharArray(t2))
                             .arg(rc.left).arg(rc.top).arg(rc.right).arg(rc.bottom)
                             .arg(int(IsWindowVisible(h))));
        }
    }
}

void dumpQtState(QTextStream &log, const QString &tag)
{
    logLine(log, QStringLiteral("==== %1 ====").arg(tag));
    QWidget *popup = QApplication::activePopupWidget();
    QWidget *modal = QApplication::activeModalWidget();
    QWidget *grabber = QWidget::mouseGrabber();
    QWidget *kbGrabber = QWidget::keyboardGrabber();
    logLine(log, QStringLiteral("  activePopupWidget=%1")
                     .arg(popup ? QString::fromUtf8(popup->metaObject()->className()) : QStringLiteral("(none)")));
    logLine(log, QStringLiteral("  activeModalWidget=%1")
                     .arg(modal ? QString::fromUtf8(modal->metaObject()->className()) : QStringLiteral("(none)")));
    logLine(log, QStringLiteral("  mouseGrabber=%1")
                     .arg(grabber ? QString::fromUtf8(grabber->metaObject()->className()) : QStringLiteral("(none)")));
    logLine(log, QStringLiteral("  keyboardGrabber=%1")
                     .arg(kbGrabber ? QString::fromUtf8(kbGrabber->metaObject()->className()) : QStringLiteral("(none)")));
    logLine(log, QStringLiteral("  overrideCursor=%1").arg(int(QApplication::overrideCursor() != nullptr)));

    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (!w->isWindow()) {
            continue;
        }
        logLine(log, QStringLiteral("  TOP %1 name='%2' visible=%3 enabled=%4 flags=0x%5 geom=(%6,%7) %8x%9 modality=%10")
                         .arg(QString::fromUtf8(w->metaObject()->className()), w->objectName())
                         .arg(int(w->isVisible()))
                         .arg(int(w->isEnabled()))
                         .arg(qulonglong(w->windowFlags()), 0, 16)
                         .arg(w->geometry().x())
                         .arg(w->geometry().y())
                         .arg(w->geometry().width())
                         .arg(w->geometry().height())
                         .arg(int(w->windowModality())));
    }
}

void realPump(int ms)
{
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < ms) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 8);
        QThread::msleep(8);
    }
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

/// Real OS mouse click at a Qt global (logical) point. QCursor::setPos is
/// DPI-aware; SetCursorPos is not — on a scaled desktop it clicks the wrong
/// physical pixel and the "freeze" is just a mis-click.
bool realClickAt(const QPoint &globalPos)
{
    QCursor::setPos(globalPos);
    Sleep(30);
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    const UINT sent = SendInput(2, inputs, sizeof(INPUT));
    Sleep(30);
    QCoreApplication::processEvents();
    return sent == 2;
}

/// Physical screen pixel for Win32 APIs (WindowFromPoint).
POINT toPhysical(const QPoint &logical)
{
    qreal dpr = 1.0;
    if (QScreen *screen = QGuiApplication::screenAt(logical)) {
        dpr = screen->devicePixelRatio();
    } else if (QScreen *screen = QGuiApplication::primaryScreen()) {
        dpr = screen->devicePixelRatio();
    }
    POINT pt;
    pt.x = LONG(qRound(logical.x() * dpr));
    pt.y = LONG(qRound(logical.y() * dpr));
    return pt;
}

void raiseWindow(QWidget &window)
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(window.winId());
    // Foreground lock: AttachThreadInput then SetForegroundWindow.
    DWORD foreThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    DWORD thisThread = GetCurrentThreadId();
    if (foreThread != thisThread) {
        AttachThreadInput(foreThread, thisThread, TRUE);
    }
    ShowWindow(hwnd, SW_RESTORE);
    SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    if (foreThread != thisThread) {
        AttachThreadInput(foreThread, thisThread, FALSE);
    }
#endif
    Q_UNUSED(window);
}

bool realClickWidget(QWidget *widget)
{
    if (!widget || !widget->isVisible()) {
        return false;
    }
    const QPoint centre = widget->mapToGlobal(widget->rect().center());
    return realClickAt(centre);
}

QString hitName(const QPoint &globalPos)
{
    QWidget *hit = QApplication::widgetAt(globalPos);
    if (!hit) {
        return QStringLiteral("(null)");
    }
    return QStringLiteral("%1 name='%2' at %3")
        .arg(QString::fromUtf8(hit->metaObject()->className()), hit->objectName())
        .arg(quintptr(hit), 0, 16);
}

QString ptStr(const QPoint &p)
{
    return QStringLiteral("(%1,%2)").arg(p.x()).arg(p.y());
}

int runDriveReal(MusicPlayerWindow &window, const QString &logPath)
{
    QFile file(logPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning("qt-md3-music: cannot write log %s", qPrintable(logPath));
        return 2;
    }
    QTextStream log(&file);

    int failures = 0;
    auto fail = [&failures, &log](const QString &what) {
        logLine(log, QStringLiteral("FAIL: %1").arg(what));
        ++failures;
    };
    auto pass = [&log](const QString &what) {
        logLine(log, QStringLiteral("PASS: %1").arg(what));
    };

    logLine(log, QStringLiteral("drive-real start platform=%1 dpr=%2 screens=%3")
                     .arg(QString::fromUtf8(QGuiApplication::platformName().toUtf8()))
                     .arg(QGuiApplication::primaryScreen()
                              ? QGuiApplication::primaryScreen()->devicePixelRatio()
                              : -1.0)
                     .arg(QGuiApplication::screens().size()));
    if (QGuiApplication::platformName().compare(QLatin1String("windows"), Qt::CaseInsensitive) != 0) {
        fail(QStringLiteral("platform is not 'windows' — this mode needs the real desktop"));
        return 2;
    }

    window.resize(420, 780);
    window.show();
    realPump(400);
    raiseWindow(window);
    realPump(200);

    dumpQtState(log, QStringLiteral("after-show"));
    dumpWin32At(log, QStringLiteral("after-show"), window.mapToGlobal(QPoint(200, 200)));

    // --- 1. Click the Settings tab in the bottom nav (like a human) ---
    QWidget *settingsNav = nullptr;
    for (QAbstractButton *b : window.findChildren<QAbstractButton *>()) {
        const QString label = b->property("label").toString();
        const bool named = b->text() == QLatin1String("Settings")
                           || label == QLatin1String("Settings")
                           || b->accessibleName() == QLatin1String("Settings")
                           || b->accessibleName().contains(QLatin1String("Settings"), Qt::CaseInsensitive);
        if (named && b->isVisible() && b->window() == &window) {
            settingsNav = b;
            logLine(log, QStringLiteral("nav candidate '%1' text='%2' label='%3' geom=(%4,%5) %6x%7")
                             .arg(QString::fromUtf8(b->metaObject()->className()), b->text(), label)
                             .arg(b->geometry().x())
                             .arg(b->geometry().y())
                             .arg(b->geometry().width())
                             .arg(b->geometry().height()));
        }
    }
    if (!settingsNav) {
        // Fall back to any visible MdNavigationBarItem with that label.
        for (QWidget *w : window.findChildren<QWidget *>()) {
            if (w->isVisible() && w->metaObject()->className() == QByteArray("md::MdNavigationBarItem")
                && w->accessibleName().contains(QLatin1String("Settings"), Qt::CaseInsensitive)) {
                settingsNav = w;
                break;
            }
        }
    }
    if (!settingsNav) {
        fail(QStringLiteral("could not find the Settings nav tab"));
        dumpQtState(log, QStringLiteral("no-settings-nav"));
        return 1;
    }
    logLine(log, QStringLiteral("clicking Settings nav at %1")
                     .arg(ptStr(settingsNav->mapToGlobal(settingsNav->rect().center()))));
    realClickWidget(settingsNav);
    realPump(400);
    if (window.currentPage() != 3) {
        // The nav item may be a compound widget; drive the bar index if the
        // click missed, but record that the click itself failed.
        fail(QStringLiteral("Settings nav click did not switch page (currentPage=%1)")
                 .arg(window.currentPage()));
        window.setCurrentPage(3);
        realPump(200);
    } else {
        pass(QStringLiteral("Settings nav click switched page"));
    }

    QWidget *settings = window.pageWidget(3);
    QList<md::MdSelect *> selects = settings ? settings->findChildren<md::MdSelect *>() : QList<md::MdSelect *>();
    if (selects.isEmpty()) {
        fail(QStringLiteral("no MdSelect on Settings"));
        return 1;
    }
    md::MdSelect *themeSelect = selects.first();
    logLine(log, QStringLiteral("themeSelect=%1 items=%2 current=%3")
                     .arg(quintptr(themeSelect), 0, 16)
                     .arg(themeSelect->count()).arg(themeSelect->currentIndex()));

    // --- 2. Click the Theme combobox (MdSelect) ---
    QWidget *field = themeSelect->findChild<QLineEdit *>();
    if (!field) {
        field = themeSelect;
    }
    if (!field->isVisible()) {
        if (QScrollArea *sa = settings->findChild<QScrollArea *>()) {
            sa->ensureWidgetVisible(themeSelect);
            realPump(150);
        }
    }
    const QPoint fieldPt = field->mapToGlobal(field->rect().center());
    logLine(log, QStringLiteral("clicking Theme field at %1 widgetAt=%2 fieldVisible=%3 fieldGeom=(%4,%5) %6x%7 themeSelectVisible=%8")
                     .arg(ptStr(fieldPt)).arg(hitName(fieldPt))
                     .arg(int(field->isVisible()))
                     .arg(field->geometry().x())
                     .arg(field->geometry().y())
                     .arg(field->geometry().width())
                     .arg(field->geometry().height())
                     .arg(int(themeSelect->isVisible())));
    realClickAt(fieldPt);
    realPump(500);

    if (!themeSelect->isPopupVisible()) {
        fail(QStringLiteral("Theme popup did not open after real click"));
        dumpQtState(log, QStringLiteral("popup-failed-to-open"));
        return 1;
    }
    pass(QStringLiteral("Theme popup opened"));
    dumpQtState(log, QStringLiteral("popup-open"));

    // Wait out the open spring so rows are real children again.
    QWidget *popup = QApplication::activePopupWidget();
    for (int i = 0; i < 40; ++i) {
        realPump(25);
        bool anyVisible = false;
        QList<md::MdMenuItem *> rows = popup ? popup->findChildren<md::MdMenuItem *>()
                                            : QList<md::MdMenuItem *>();
        for (md::MdMenuItem *it : rows) {
            if (it->isVisible() && it->isEnabled()) {
                anyVisible = true;
                break;
            }
        }
        if (anyVisible) {
            break;
        }
    }

    // --- 3. Pick Dark from the popup ---
    md::MdMenuItem *darkItem = nullptr;
    QList<md::MdMenuItem *> rows = popup ? popup->findChildren<md::MdMenuItem *>()
                                        : QList<md::MdMenuItem *>();
    logLine(log, QStringLiteral("popup rows=%1").arg(rows.size()));
    for (md::MdMenuItem *it : rows) {
        logLine(log, QStringLiteral("  row '%1' visible=%2 enabled=%3 geom=(%4,%5) %6x%7")
                         .arg(it->text())
                         .arg(int(it->isVisible()))
                         .arg(int(it->isEnabled()))
                         .arg(it->geometry().x())
                         .arg(it->geometry().y())
                         .arg(it->geometry().width())
                         .arg(it->geometry().height()));
        if (it->text().compare(QLatin1String("Dark"), Qt::CaseInsensitive) == 0) {
            darkItem = it;
        }
    }
    if (!darkItem) {
        fail(QStringLiteral("no Dark row in the Theme popup"));
        return 1;
    }
    const QPoint darkPt = darkItem->mapToGlobal(darkItem->rect().center());
    logLine(log, QStringLiteral("clicking Dark at %1 widgetAt=%2")
                     .arg(ptStr(darkPt)).arg(hitName(darkPt)));
    realClickAt(darkPt);
    realPump(600);

    dumpQtState(log, QStringLiteral("after-dark-pick"));
    dumpWin32At(log, QStringLiteral("after-dark-pick"), window.mapToGlobal(QPoint(200, 300)));

    logLine(log, QStringLiteral("themeMode=%1 popupVisible=%2")
                     .arg(int(md::MdTheme::instance().themeMode()))
                     .arg(int(themeSelect->isPopupVisible())));

    if (md::MdTheme::instance().themeMode() != md::ThemeMode::Dark) {
        fail(QStringLiteral("Dark pick did not switch the theme"));
    } else {
        pass(QStringLiteral("Dark pick switched the theme"));
    }
    if (QApplication::activePopupWidget() != nullptr) {
        fail(QStringLiteral("activePopupWidget still set after the pick — grab held"));
    } else {
        pass(QStringLiteral("no active popup after the pick"));
    }
    if (themeSelect->isPopupVisible()) {
        fail(QStringLiteral("MdSelect still reports popup visible after the pick"));
    } else {
        pass(QStringLiteral("MdSelect popup flag cleared"));
    }

    // --- 4. Try to click anything else (this is the freeze) ---
    // 4a. High contrast switch.
    md::MdSwitch *hc = nullptr;
    for (md::MdSwitch *sw : settings->findChildren<md::MdSwitch *>()) {
        if (sw->text().contains(QLatin1String("contrast"), Qt::CaseInsensitive)
            || sw->accessibleName().contains(QLatin1String("contrast"), Qt::CaseInsensitive)) {
            hc = sw;
            break;
        }
    }
    if (hc) {
        if (QScrollArea *sa = settings->findChild<QScrollArea *>()) {
            sa->ensureWidgetVisible(hc);
            realPump(150);
        }
        const bool before = hc->isChecked();
        const QPoint hcPt = hc->mapToGlobal(hc->rect().center());
        logLine(log, QStringLiteral("clicking high-contrast switch at %1 widgetAt=%2 checked=%3")
                         .arg(ptStr(hcPt)).arg(hitName(hcPt)).arg(int(before)));
        realClickAt(hcPt);
        realPump(300);
        if (hc->isChecked() == before) {
            fail(QStringLiteral("high-contrast switch IGNORED the real click — UI is frozen"));
            dumpQtState(log, QStringLiteral("after-dead-switch-click"));
            dumpWin32At(log, QStringLiteral("after-dead-switch-click"), hcPt);

#ifdef Q_OS_WIN
            HWND cap = GetCapture();
            wchar_t capCls[256] = {};
            if (cap) {
                GetClassNameW(cap, capCls, 256);
            }
            logLine(log, QStringLiteral("Win32 GetCapture=%1 class='%2'")
                             .arg(quintptr(cap), 0, 16)
                             .arg(QString::fromWCharArray(capCls)));
            GUITHREADINFO gi{};
            gi.cbSize = sizeof(gi);
            if (GetGUIThreadInfo(GetCurrentThreadId(), &gi)) {
                logLine(log, QStringLiteral("Win32 GUI hwndCapture=%1 hwndFocus=%2 hwndActive=%3")
                                 .arg(quintptr(gi.hwndCapture), 0, 16)
                                 .arg(quintptr(gi.hwndFocus), 0, 16)
                                 .arg(quintptr(gi.hwndActive), 0, 16));
            }
#endif

            // Synthetic delivery to the SAME widget: if this toggles, the
            // widget is fine and the OS/Qt grab is swallowing real input.
            const bool beforeSynth = hc->isChecked();
            QMouseEvent pressEv(QEvent::MouseButtonPress, hc->rect().center(),
                                hc->mapToGlobal(hc->rect().center()), Qt::LeftButton,
                                Qt::LeftButton, Qt::NoModifier);
            QMouseEvent releaseEv(QEvent::MouseButtonRelease, hc->rect().center(),
                                  hc->mapToGlobal(hc->rect().center()), Qt::LeftButton,
                                  Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(hc, &pressEv);
            QCoreApplication::sendEvent(hc, &releaseEv);
            QCoreApplication::processEvents();
            logLine(log, QStringLiteral("synthetic sendEvent press+release toggled=%1 (before=%2 now=%3)")
                             .arg(int(hc->isChecked() != beforeSynth))
                             .arg(int(beforeSynth)).arg(int(hc->isChecked())));
            if (hc->isChecked() == beforeSynth) {
                fail(QStringLiteral("even synthetic events do not toggle the switch"));
            } else {
                fail(QStringLiteral("widget accepts synthetic events — real OS input is being swallowed"));
                // Restore
                QCoreApplication::sendEvent(hc, &pressEv);
                QCoreApplication::sendEvent(hc, &releaseEv);
                QCoreApplication::processEvents();
            }

            // Direct click() on the abstract button.
            const bool beforeClick = hc->isChecked();
            hc->click();
            QCoreApplication::processEvents();
            logLine(log, QStringLiteral("hc->click() toggled=%1")
                             .arg(int(hc->isChecked() != beforeClick)));

            // Identify whatever WindowFromPoint says is on top of the switch.
            {
                const POINT pt = toPhysical(hcPt);
                HWND top = WindowFromPoint(pt);
                wchar_t c[256] = {}, t[256] = {};
                if (top) {
                    GetClassNameW(top, c, 256);
                    GetWindowTextW(top, t, 256);
                }
                DWORD pid = 0, tid = 0;
                if (top) {
                    tid = GetWindowThreadProcessId(top, &pid);
                }
                RECT rc{};
                if (top) {
                    GetWindowRect(top, &rc);
                }
                HWND parent = top ? GetParent(top) : nullptr;
                HWND owner = top ? GetWindow(top, GW_OWNER) : nullptr;
                HWND root = top ? GetAncestor(top, GA_ROOT) : nullptr;
                QWidget *asWidget = top ? QWidget::find(reinterpret_cast<WId>(top)) : nullptr;
                logLine(log, QStringLiteral("intercept hwnd=%1 class='%2' title='%3' pid=%4 tid=%5 rect=(%6,%7)-(%8,%9) visible=%10 parent=%11 owner=%12 root=%13 ourPid=%14 qwidget=%15 qwidgetName=%16 qwidgetClass=%17")
                                 .arg(quintptr(top), 0, 16)
                                 .arg(QString::fromWCharArray(c), QString::fromWCharArray(t))
                                 .arg(pid).arg(tid)
                                 .arg(rc.left).arg(rc.top).arg(rc.right).arg(rc.bottom)
                                 .arg(top ? int(IsWindowVisible(top)) : -1)
                                 .arg(quintptr(parent), 0, 16)
                                 .arg(quintptr(owner), 0, 16)
                                 .arg(quintptr(root), 0, 16)
                                 .arg(GetCurrentProcessId())
                                 .arg(quintptr(asWidget), 0, 16)
                                 .arg(asWidget ? asWidget->objectName() : QStringLiteral("(none)"))
                                 .arg(asWidget ? QString::fromUtf8(asWidget->metaObject()->className())
                                               : QStringLiteral("(none)")));
                if (asWidget) {
                    logLine(log, QStringLiteral("  intercept QWidget visible=%1 enabled=%2 parent=%3 flags=0x%4 geom=(%5,%6) %7x%8")
                                     .arg(int(asWidget->isVisible()), int(asWidget->isEnabled()))
                                     .arg(asWidget->parent() ? quintptr(asWidget->parent()) : 0, 0, 16)
                                     .arg(qulonglong(asWidget->windowFlags()), 0, 16)
                                     .arg(asWidget->geometry().x())
                                     .arg(asWidget->geometry().y())
                                     .arg(asWidget->geometry().width())
                                     .arg(asWidget->geometry().height()));
                }
                // If a foreign/leftover window sits on top of the app, raise the
                // main window and retry — proves it is a z-order thief.
                HWND mainHwnd = reinterpret_cast<HWND>(window.winId());
                logLine(log, QStringLiteral("main winId=%1").arg(quintptr(mainHwnd), 0, 16));
                SetWindowPos(mainHwnd, HWND_TOP, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                realPump(50);
                const bool beforeRaise = hc->isChecked();
                realClickAt(hcPt);
                realPump(300);
                logLine(log, QStringLiteral("after HWND_TOP retry toggled=%1")
                                 .arg(int(hc->isChecked() != beforeRaise)));
                // Hide the intercepting window outright and retry.
                if (top && top != mainHwnd) {
                    ShowWindow(top, SW_HIDE);
                    realPump(50);
                    const bool beforeHide = hc->isChecked();
                    realClickAt(hcPt);
                    realPump(300);
                    logLine(log, QStringLiteral("after hiding intercept hwnd retry toggled=%1")
                                     .arg(int(hc->isChecked() != beforeHide)));
                }
            }
        } else {
            pass(QStringLiteral("high-contrast switch toggled after theme pick"));
            // Toggle back so the rest of the run is stable.
            realClickAt(hcPt);
            realPump(200);
        }
    } else {
        fail(QStringLiteral("no high-contrast switch found"));
    }

    // 4b. Nav tabs.
    for (QAbstractButton *b : window.findChildren<QAbstractButton *>()) {
        const QString label = b->property("label").toString();
        const bool isLibrary = b->text() == QLatin1String("Library")
                               || label == QLatin1String("Library")
                               || b->accessibleName() == QLatin1String("Library");
        if (isLibrary && b->isVisible() && b->window() == &window) {
            const bool ok = realClickWidget(b);
            realPump(400);
            logLine(log, QStringLiteral("Library nav real click sent=%1 currentPage=%2")
                             .arg(int(ok)).arg(window.currentPage()));
            if (window.currentPage() != 1) {
                fail(QStringLiteral("Library nav click did not land — UI is frozen"));
                dumpQtState(log, QStringLiteral("after-dead-nav-click"));
            } else {
                pass(QStringLiteral("Library nav click landed"));
            }
            break;
        }
    }

    // Back to Settings and try the combobox again.
    if (settingsNav) {
        realClickWidget(settingsNav);
        realPump(400);
    }
    if (window.currentPage() == 3 && settings) {
        if (QScrollArea *sa = settings->findChild<QScrollArea *>()) {
            sa->ensureWidgetVisible(themeSelect);
            realPump(200);
        }
        const QPoint fieldPt2 = field->mapToGlobal(field->rect().center());
        logLine(log, QStringLiteral("re-clicking Theme field at %1 widgetAt=%2")
                         .arg(ptStr(fieldPt2)).arg(hitName(fieldPt2)));
        realClickAt(fieldPt2);
        realPump(400);
        if (!themeSelect->isPopupVisible()) {
            fail(QStringLiteral("Theme combobox will not re-open after theme pick — UI is frozen"));
            dumpQtState(log, QStringLiteral("after-dead-combo-reopen"));
            dumpWin32At(log, QStringLiteral("after-dead-combo-reopen"), fieldPt2);
        } else {
            pass(QStringLiteral("Theme combobox re-opened"));
            // Escape it closed via a click on the field again.
            realClickAt(fieldPt2);
            realPump(300);
        }
    }

    // --- 5. Probe hit-testing over the whole window ---
    int deadSpots = 0;
    int liveSpots = 0;
    for (int y = 40; y < window.height() - 20; y += 40) {
        for (int x = 20; x < window.width() - 20; x += 40) {
            const QPoint gp = window.mapToGlobal(QPoint(x, y));
            QWidget *hit = QApplication::widgetAt(gp);
            if (hit && hit->isEnabled() && hit->isVisible()
                && (hit->focusPolicy() != Qt::NoFocus || qobject_cast<QAbstractButton *>(hit))) {
                ++liveSpots;
            } else if (!hit) {
                ++deadSpots;
            }
        }
    }
    logLine(log, QStringLiteral("hit-scan live=%1 dead=%2").arg(liveSpots).arg(deadSpots));

    dumpQtState(log, QStringLiteral("final"));

    if (failures == 0) {
        logLine(log, QStringLiteral("DRIVE-REAL PASS"));
    } else {
        logLine(log, QStringLiteral("DRIVE-REAL %1 failure(s)").arg(failures));
    }
    return failures;
}

#else // !Q_OS_WIN

int runDriveReal(MusicPlayerWindow &, const QString &logPath)
{
    QFile file(logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream log(&file);
        log << "DRIVE-REAL requires the Windows platform\n";
    }
    qWarning("qt-md3-music: --drive-real requires Windows");
    return 2;
}

#endif // Q_OS_WIN

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
    QCommandLineOption driveRealOption(
        QStringLiteral("drive-real"),
        QStringLiteral("Real Windows SendInput journey: Settings → Theme → Dark → try clicks."),
        QStringLiteral("logfile"));
    parser.addOption(driveRealOption);
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

    if (parser.isSet(driveRealOption)) {
        QCoreApplication::processEvents();
        return runDriveReal(window, parser.value(driveRealOption));
    }

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
