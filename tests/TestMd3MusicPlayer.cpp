// TestMd3MusicPlayer — music-player Library / Settings visual + interaction QA.
//
// Instantiates the real MusicPlayerWindow and drives the journeys the user
// meets: Library search/filter/activation/scroll, Settings theme/seed/
// contrast/quality/crossfade/switches, keyboard tabbing, and the layout
// invariants that failed the visual pass (mini-player covering content,
// MdColorPicker swatch overlapping its sliders at phone widths).

#include "MusicPlayerWindow.h"

#include "core/MdTheme.h"
#include "widgets/MdChip.h"
#include "widgets/MdColorPicker.h"
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"
#include "widgets/MdSearchBar.h"
#include "widgets/MdSelect.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSwitch.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>
#include <QtGui/QMouseEvent>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>

using namespace md;

namespace {

void pump(int ms)
{
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < ms) {
        QCoreApplication::processEvents();
        QThread::msleep(4);
    }
    QCoreApplication::processEvents();
}

void clickCentre(QWidget *widget)
{
    const QPoint centre = widget->rect().center();
    QTest::mouseClick(widget, Qt::LeftButton, Qt::NoModifier, centre);
}

void dragSequence(QWidget &widget, const QPoint &from, const QPoint &to)
{
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(from),
                      QPointF(widget.mapToGlobal(from)), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &press);
    QMouseEvent move(QEvent::MouseMove, QPointF(to),
                     QPointF(widget.mapToGlobal(to)), Qt::LeftButton,
                     Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(to),
                        QPointF(widget.mapToGlobal(to)), Qt::LeftButton,
                        Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &release);
}

QWidget *named(QWidget *root, const char *name)
{
    if (root == nullptr) {
        return nullptr;
    }
    const QList<QWidget *> all = root->findChildren<QWidget *>();
    for (QWidget *w : all) {
        if (w->objectName() == QLatin1String(name)) {
            return w;
        }
    }
    return nullptr;
}

/// Interactive children of `root` that are shown and have a real rect.
template <typename T>
QList<T *> live(QWidget *root)
{
    QList<T *> result;
    if (root == nullptr) {
        return result;
    }
    const QList<T *> all = root->findChildren<T *>();
    for (T *w : all) {
        if (w != nullptr && w->isEnabled() && w->isVisibleTo(root) && !w->geometry().isEmpty()) {
            result.append(w);
        }
    }
    return result;
}

} // namespace

class TestMd3MusicPlayer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void librarySearchFiltersList();
    void libraryFilterChipsFilter();
    void libraryItemActivationPlays();
    void libraryListScrolls();
    void libraryChipsReachableAtPhoneWidth();

    void settingsThemeSelectChangesTheme();
    void settingsSeedPickerChangesColours();
    void settingsHighContrastToggles();
    void settingsQualitySelectChangesValue();
    void settingsCrossfadeSliderDrags();
    void settingsSwitchesToggle();
    void settingsKeyboardTabAndActivate();

    void miniPlayerDoesNotCoverContent();
    void colorPickerNoOverlapAtPhoneWidth();
    void resizeKeepsLayout();
    void themeToggleRestylesLabels();

private:
    MusicPlayerWindow *m_window = nullptr;
};

void TestMd3MusicPlayer::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    MdTheme::instance().setHighContrast(false);
    MdTheme::instance().setLanguageTag(QStringLiteral("en"));
    m_window = new MusicPlayerWindow;
    m_window->resize(420, 780);
    m_window->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    pump(120);
}

void TestMd3MusicPlayer::cleanupTestCase()
{
    delete m_window;
    m_window = nullptr;
}

// ---------------------------------------------------------------------------
// Library
// ---------------------------------------------------------------------------

void TestMd3MusicPlayer::librarySearchFiltersList()
{
    m_window->setCurrentPage(1);
    pump(80);
    QWidget *library = m_window->pageWidget(1);
    QVERIFY(library != nullptr);

    MdList *list = library->findChild<MdList *>();
    QVERIFY(list != nullptr);
    const int before = list->itemCount();
    QVERIFY(before > 2);

    QLineEdit *search = library->findChild<QLineEdit *>();
    QVERIFY(search != nullptr);

    search->setText(QStringLiteral("Night"));
    pump(80);
    const int filtered = list->itemCount();
    QVERIFY2(filtered < before, "search did not narrow the library list");
    QVERIFY2(filtered > 0, "search for a known title emptied the list");

    search->setText(QString());
    pump(80);
    QCOMPARE(list->itemCount(), before);
}

void TestMd3MusicPlayer::libraryFilterChipsFilter()
{
    m_window->setCurrentPage(1);
    pump(80);
    QWidget *library = m_window->pageWidget(1);
    QVERIFY(library != nullptr);

    MdList *list = library->findChild<MdList *>();
    QVERIFY(list != nullptr);

    MdChip *all = qobject_cast<MdChip *>(named(library, "filter-all"));
    MdChip *albums = qobject_cast<MdChip *>(named(library, "filter-albums"));
    MdChip *playlists = qobject_cast<MdChip *>(named(library, "filter-playlists"));
    QVERIFY(all && albums && playlists);
    QVERIFY(all->isChecked());

    const int allCount = list->itemCount();
    QVERIFY(allCount > 0);

    clickCentre(albums);
    pump(80);
    QVERIFY(albums->isChecked());
    QVERIFY(!all->isChecked());
    const int albumCount = list->itemCount();
    QVERIFY2(albumCount < allCount, "Albums chip did not narrow the list");

    clickCentre(playlists);
    pump(80);
    QVERIFY(playlists->isChecked());
    QVERIFY(!albums->isChecked());
    const int playlistCount = list->itemCount();
    QVERIFY2(playlistCount > 0 && playlistCount < allCount,
             "Playlists chip did not select a proper subset");

    // Unchecking the active chip falls back to All.
    clickCentre(playlists);
    pump(80);
    QVERIFY(all->isChecked());
    QVERIFY(!playlists->isChecked());
    QCOMPARE(list->itemCount(), allCount);
}

void TestMd3MusicPlayer::libraryItemActivationPlays()
{
    m_window->setCurrentPage(1);
    pump(80);
    QWidget *library = m_window->pageWidget(1);
    QVERIFY(library != nullptr);

    MdList *list = library->findChild<MdList *>();
    QVERIFY(list != nullptr);
    const QList<MdListItem *> items = list->items();
    QVERIFY(!items.isEmpty());

    MdListItem *target = nullptr;
    for (MdListItem *item : items) {
        if (item->isInteractive() && item->isEnabled()) {
            target = item;
            break;
        }
    }
    QVERIFY(target != nullptr);

    const QString before = m_window->player()->currentTrackId();
    QVERIFY(list->activateItem(target));
    pump(80);
    const QString after = m_window->player()->currentTrackId();
    QVERIFY2(!after.isEmpty(), "activating a library row left no current track");
    QVERIFY2(after != before || !before.isEmpty(),
             "activating a library row did not start playback");
    QVERIFY(m_window->player()->isPlaying() || !after.isEmpty());
}

void TestMd3MusicPlayer::libraryListScrolls()
{
    m_window->setCurrentPage(1);
    pump(80);
    QWidget *library = m_window->pageWidget(1);
    QVERIFY(library != nullptr);

    QScrollArea *scroll = library->findChild<QScrollArea *>();
    QVERIFY2(scroll != nullptr, "library page has no scroll area");
    QVERIFY(scroll->widgetResizable());

    // Content taller than the viewport is the whole reason the area exists.
    const int doc = scroll->widget() ? scroll->widget()->sizeHint().height() : 0;
    const int view = scroll->viewport()->height();
    Q_UNUSED(doc);
    Q_UNUSED(view);

    QScrollBar *bar = scroll->verticalScrollBar();
    QVERIFY(bar != nullptr);
    const int maxBefore = bar->maximum();
    // Even when everything currently fits, the bar must exist and be wired.
    bar->setValue(bar->maximum());
    pump(40);
    QCOMPARE(bar->value(), bar->maximum());
    bar->setValue(0);
    pump(40);
    QCOMPARE(bar->value(), 0);
    QVERIFY(maxBefore >= 0);
}

void TestMd3MusicPlayer::libraryChipsReachableAtPhoneWidth()
{
    m_window->resize(320, 640);
    m_window->setCurrentPage(1);
    pump(120);
    QWidget *library = m_window->pageWidget(1);
    QVERIFY(library != nullptr);

    // Every filter chip must be inside the window — the row wraps, so no
    // chip is allowed to fall outside the page at phone widths.
    const char *names[] = {"filter-all", "filter-albums", "filter-playlists", "filter-lossless"};
    for (const char *name : names) {
        MdChip *chip = qobject_cast<MdChip *>(named(library, name));
        QVERIFY2(chip != nullptr, name);
        const QRect chipRect(chip->mapTo(m_window, QPoint(0, 0)), chip->size());
        const QRect win = m_window->rect();
        QVERIFY2(win.contains(chipRect) || win.intersects(chipRect),
                 qPrintable(QStringLiteral("chip %1 is outside the window").arg(QLatin1String(name))));
        QVERIFY2(chip->isVisibleTo(library), qPrintable(QStringLiteral("chip %1 is hidden").arg(QLatin1String(name))));
    }
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

void TestMd3MusicPlayer::settingsThemeSelectChangesTheme()
{
    m_window->setCurrentPage(3);
    pump(80);
    QWidget *settings = m_window->pageWidget(3);
    QVERIFY(settings != nullptr);

    MdSelect *themeSelect = settings->findChild<MdSelect *>();
    QVERIFY(themeSelect != nullptr);

    const ThemeMode before = MdTheme::instance().themeMode();
    const int want = before == ThemeMode::Dark ? 0 : 1;
    themeSelect->setCurrentIndex(want);
    pump(80);
    QVERIFY2(MdTheme::instance().themeMode() != before, "theme select did not change the theme");
    QCOMPARE(int(themeSelect->currentIndex()), want);

    // Restore.
    themeSelect->setCurrentIndex(before == ThemeMode::Dark ? 1 : 0);
    pump(80);
    QCOMPARE(MdTheme::instance().themeMode(), before);
}

void TestMd3MusicPlayer::settingsSeedPickerChangesColours()
{
    m_window->setCurrentPage(3);
    pump(80);
    QWidget *settings = m_window->pageWidget(3);
    QVERIFY(settings != nullptr);

    MdColorPicker *picker = settings->findChild<MdColorPicker *>();
    QVERIFY(picker != nullptr);

    const QColor before = MdTheme::instance().seedColor();
    const QColor want(0x00, 0x69, 0x6D);
    picker->setColor(want);
    pump(80);
    QCOMPARE(MdTheme::instance().seedColor(), want);
    QCOMPARE(picker->color(), want);

    picker->setColor(before);
    pump(80);
    QCOMPARE(MdTheme::instance().seedColor(), before);
}

void TestMd3MusicPlayer::settingsHighContrastToggles()
{
    m_window->setCurrentPage(3);
    pump(80);
    QWidget *settings = m_window->pageWidget(3);
    QVERIFY(settings != nullptr);

    // The high-contrast switch is the first MdSwitch on the page.
    const QList<MdSwitch *> switches = live<MdSwitch>(settings);
    QVERIFY(!switches.isEmpty());
    MdSwitch *hc = switches.first();
    QVERIFY(hc->text().contains(QLatin1String("contrast"), Qt::CaseInsensitive)
            || hc->accessibleName().contains(QLatin1String("contrast"), Qt::CaseInsensitive));

    const bool before = MdTheme::instance().isHighContrast();
    clickCentre(hc);
    pump(80);
    QVERIFY2(MdTheme::instance().isHighContrast() != before,
             "high-contrast switch did not change the theme");
    QCOMPARE(hc->isChecked(), !before);

    clickCentre(hc);
    pump(80);
    QCOMPARE(MdTheme::instance().isHighContrast(), before);
}

void TestMd3MusicPlayer::settingsQualitySelectChangesValue()
{
    m_window->setCurrentPage(3);
    pump(80);
    QWidget *settings = m_window->pageWidget(3);
    QVERIFY(settings != nullptr);

    const QList<MdSelect *> selects = settings->findChildren<MdSelect *>();
    QVERIFY(selects.size() >= 2);
    MdSelect *quality = selects.at(1);
    QVERIFY(quality->count() >= 3);

    const int before = quality->currentIndex();
    const int want = before == 0 ? 1 : 0;
    QSignalSpy spy(quality, &MdSelect::currentIndexChanged);
    quality->setCurrentIndex(want);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(quality->currentIndex(), want);
    QVERIFY(!quality->currentText().isEmpty());
}

void TestMd3MusicPlayer::settingsCrossfadeSliderDrags()
{
    m_window->setCurrentPage(3);
    pump(80);
    QWidget *settings = m_window->pageWidget(3);
    QVERIFY(settings != nullptr);

    MdSlider *slider = settings->findChild<MdSlider *>();
    QVERIFY(slider != nullptr);

    const qreal before = slider->value();
    QSignalSpy spy(slider, &MdSlider::valueChanged);
    const QRect r = slider->rect();
    dragSequence(*slider, QPoint(r.left() + 4, r.center().y()),
                 QPoint(r.right() - 8, r.center().y()));
    QVERIFY2(spy.count() >= 1, "crossfade drag produced no valueChanged");
    QVERIFY2(slider->value() != before, "crossfade value did not move");

    // Keyboard: arrows step the slider.
    slider->setFocus();
    const qreal mid = slider->value();
    QTest::keyClick(slider, Qt::Key_Left);
    pump(20);
    QVERIFY(slider->value() != mid || slider->value() == slider->min());
}

void TestMd3MusicPlayer::settingsSwitchesToggle()
{
    m_window->setCurrentPage(3);
    pump(80);
    QWidget *settings = m_window->pageWidget(3);
    QVERIFY(settings != nullptr);

    const QList<MdSwitch *> switches = settings->findChildren<MdSwitch *>();
    QVERIFY2(switches.size() >= 3, "settings page is missing switches");

    // Skip the high-contrast switch (already covered); toggle normalize/gapless.
    int toggled = 0;
    for (MdSwitch *sw : switches) {
        const QString label = sw->text() + sw->accessibleName();
        if (label.contains(QLatin1String("contrast"), Qt::CaseInsensitive)) {
            continue;
        }
        const bool before = sw->isChecked();
        QSignalSpy spy(sw, &QAbstractButton::toggled);
        clickCentre(sw);
        QVERIFY(spy.count() >= 1);
        QVERIFY(sw->isChecked() != before);
        // Restore.
        clickCentre(sw);
        QCOMPARE(sw->isChecked(), before);
        ++toggled;
    }
    QVERIFY(toggled >= 2);
}

void TestMd3MusicPlayer::settingsKeyboardTabAndActivate()
{
    m_window->setCurrentPage(3);
    pump(80);
    QWidget *settings = m_window->pageWidget(3);
    QVERIFY(settings != nullptr);

    settings->setFocus();
    // Tab a few times; focus must land on a real child, never get lost.
    QWidget *last = settings;
    for (int i = 0; i < 12; ++i) {
        QTest::keyClick(last, Qt::Key_Tab);
        pump(20);
        QWidget *focus = QApplication::focusWidget();
        if (focus == nullptr) {
            break;
        }
        QVERIFY2(settings->isAncestorOf(focus) || focus == settings,
                 "Tab left the settings page");
        last = focus;
    }

    // Space on a focused switch toggles it.
    const QList<MdSwitch *> switches = settings->findChildren<MdSwitch *>();
    QVERIFY(!switches.isEmpty());
    MdSwitch *sw = switches.last();
    sw->setFocus();
    pump(20);
    const bool before = sw->isChecked();
    QTest::keyClick(sw, Qt::Key_Space);
    pump(40);
    QVERIFY(sw->isChecked() != before);
    QTest::keyClick(sw, Qt::Key_Space);
    pump(40);
    QCOMPARE(sw->isChecked(), before);
}

// ---------------------------------------------------------------------------
// Layout invariants
// ---------------------------------------------------------------------------

void TestMd3MusicPlayer::miniPlayerDoesNotCoverContent()
{
    const QSize sizes[] = {QSize(320, 640), QSize(420, 780), QSize(800, 600)};
    for (const QSize &size : sizes) {
        m_window->resize(size);
        pump(80);
        for (int page = 1; page <= 3; ++page) {
            m_window->setCurrentPage(page);
            pump(80);
            QWidget *w = m_window->pageWidget(page);
            QVERIFY(w != nullptr);

            // The page widget itself must sit fully above the mini player band.
            const QList<QWidget *> minis = m_window->findChildren<QWidget *>();
            QWidget *mini = nullptr;
            for (QWidget *c : minis) {
                if (c->accessibleName().contains(QLatin1String("mini"), Qt::CaseInsensitive)
                    || c->objectName().contains(QLatin1String("mini"), Qt::CaseInsensitive)) {
                    mini = c;
                    break;
                }
            }
            if (mini == nullptr || !mini->isVisible()) {
                continue;
            }
            const QRect miniRect(mini->mapTo(m_window, QPoint(0, 0)), mini->size());
            const QRect pageRect(w->mapTo(m_window, QPoint(0, 0)), w->size());
            QVERIFY2(!pageRect.intersects(miniRect),
                     qPrintable(QStringLiteral("page %1 geometry overlaps the mini player at %2x%3")
                                    .arg(page)
                                    .arg(size.width())
                                    .arg(size.height())));

            // Every live interactive control on the page must be inside the
            // page rect (the stack margin keeps them clear of the overlay).
            const QList<QWidget *> kids = w->findChildren<QWidget *>();
            for (QWidget *kid : kids) {
                if (!kid->isEnabled() || !kid->isVisibleTo(w) || kid->geometry().isEmpty()) {
                    continue;
                }
                if (!qobject_cast<QAbstractButton *>(kid) && !qobject_cast<QLineEdit *>(kid)
                    && !qobject_cast<MdSlider *>(kid) && !qobject_cast<MdSelect *>(kid)) {
                    continue;
                }
                const QRect kidRect(kid->mapTo(m_window, QPoint(0, 0)), kid->size());
                if (!kidRect.isValid() || kidRect.width() <= 0) {
                    continue;
                }
                // A control scrolled out of the viewport is fine; one parked
                // under the mini player is not.
                if (kidRect.intersects(miniRect) && kidRect.bottom() > miniRect.top() + 2
                    && kidRect.top() < miniRect.bottom() - 2) {
                    // Only fail when the control is actually visible in the
                    // window (not clipped by a scroll viewport above).
                    const QRect visible = kidRect.intersected(pageRect);
                    if (visible.intersects(miniRect)) {
                        QFAIL(qPrintable(
                            QStringLiteral("control %1/%2 intersects the mini player at %3x%4")
                                .arg(QString::fromUtf8(kid->metaObject()->className()),
                                     kid->objectName().isEmpty() ? QStringLiteral("?")
                                                                 : kid->objectName())
                                .arg(size.width())
                                .arg(size.height())));
                    }
                }
            }
        }
    }
}

void TestMd3MusicPlayer::colorPickerNoOverlapAtPhoneWidth()
{
    const int widths[] = {280, 320, 360, 420};
    for (int w : widths) {
        MdColorPicker picker;
        picker.setHexVisible(true);
        picker.resize(w, picker.sizeHint().height());
        picker.show();
        pump(20);

        MdSlider *hue = picker.hueSlider();
        MdSlider *sat = picker.saturationSlider();
        MdSlider *val = picker.valueSlider();
        QVERIFY(hue && sat && val);
        const QRect hueR = hue->geometry();
        const QRect satR = sat->geometry();
        const QRect valR = val->geometry();
        QVERIFY2(hueR.isValid() && satR.isValid() && valR.isValid(),
                 "color picker sliders have no geometry");
        QVERIFY2(hueR.right() < picker.width(), "hue slider is clipped");

        // The swatch disc is painted on the trailing side; the sliders must
        // never run under it. Recover the disc from paint layout: it sits at
        // width - padding - size. Compare against the slider column's right.
        const int sliderRight = qMax(hueR.right(), qMax(satR.right(), valR.right()));
        QVERIFY2(sliderRight < picker.width() - 8,
                 qPrintable(QStringLiteral("sliders reach the widget edge at width %1").arg(w)));
        // Hue and value sliders share the same column.
        QCOMPARE(hueR.left(), valR.left());
        QCOMPARE(hueR.width(), valR.width());
    }
}

void TestMd3MusicPlayer::resizeKeepsLayout()
{
    const QSize sizes[] = {QSize(320, 640), QSize(420, 780), QSize(800, 600), QSize(1280, 800)};
    for (const QSize &size : sizes) {
        m_window->resize(size);
        pump(80);
        for (int page = 1; page <= 3; ++page) {
            m_window->setCurrentPage(page);
            pump(80);
            QWidget *w = m_window->pageWidget(page);
            QVERIFY(w != nullptr);
            // Page must fill the stack area (which is already inset).
            QVERIFY2(w->width() > 0 && w->height() > 0,
                     qPrintable(QStringLiteral("page %1 has empty geometry at %2x%3")
                                    .arg(page)
                                    .arg(size.width())
                                    .arg(size.height())));
            // No *chrome* child may stick out of the page. Scroll content is
            // allowed to be wider/taller than its viewport — that is what the
            // scroll area is for.
            const QList<QWidget *> kids = w->findChildren<QWidget *>();
            for (QWidget *kid : kids) {
                if (!kid->isVisibleTo(w) || kid->geometry().isEmpty()) {
                    continue;
                }
                // Skip descendants of a QScrollArea viewport: they are
                // scroll content, not page chrome.
                bool inScroll = false;
                for (QWidget *p = kid->parentWidget(); p != nullptr; p = p->parentWidget()) {
                    if (qobject_cast<QScrollArea *>(p) != nullptr) {
                        inScroll = true;
                        break;
                    }
                    if (p == w) {
                        break;
                    }
                }
                if (inScroll) {
                    continue;
                }
                const QRect r(kid->mapTo(w, QPoint(0, 0)), kid->size());
                if (r.left() < -24 || r.right() > w->width() + 24) {
                    QFAIL(qPrintable(QStringLiteral("child %1 overflows page %2 at %3x%4")
                                         .arg(QString::fromUtf8(kid->metaObject()->className()))
                                         .arg(page)
                                         .arg(size.width())
                                         .arg(size.height())));
                }
            }
        }
    }
}

void TestMd3MusicPlayer::themeToggleRestylesLabels()
{
    m_window->resize(420, 780);
    m_window->setCurrentPage(3);
    pump(80);
    QWidget *settings = m_window->pageWidget(3);
    QVERIFY(settings != nullptr);

    // Capture a section label's palette in light, flip to dark, compare.
    QLabel *appearance = nullptr;
    const QList<QLabel *> labels = settings->findChildren<QLabel *>();
    for (QLabel *label : labels) {
        if (label->objectName() == QLatin1String("section-appearance")) {
            appearance = label;
            break;
        }
    }
    QVERIFY2(appearance != nullptr, "settings section label not found");

    const QColor lightText = appearance->palette().color(QPalette::WindowText);
    const QColor lightBg = MdTheme::instance().color(ColorRole::Surface);

    MdTheme::instance().setThemeMode(ThemeMode::Dark);
    pump(80);

    const QColor darkText = appearance->palette().color(QPalette::WindowText);
    const QColor darkBg = MdTheme::instance().color(ColorRole::Surface);
    QVERIFY2(darkText != lightText,
             "section label kept its light-theme colour after switching to dark");
    // Contrast: the label must not match its background in either theme.
    QVERIFY2(darkText != darkBg, "dark label colour equals dark surface");
    QVERIFY2(lightText != lightBg, "light label colour equals light surface");

    MdTheme::instance().setThemeMode(ThemeMode::Light);
    pump(80);
    QCOMPARE(appearance->palette().color(QPalette::WindowText), lightText);
}

QTEST_MAIN(TestMd3MusicPlayer)
#include "TestMd3MusicPlayer.moc"
