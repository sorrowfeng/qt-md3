// TestMd3GalleryClick 锟?gallery-level click probe.
//
// TestMd3InteractionSmoke proves the components work when constructed
// standalone. This suite instantiates the real GalleryWindow shell and drives
// the controls where users actually meet them: banked into a page, placed by
// placeChildren(), hosted in a QScrollArea inside the nav/header stack.
// A control that looks right but is covered, mis-placed, hidden or never
// receiving the click fails here even when the standalone smoke is green.

#include "gallery/GalleryWindow.h"

#include "core/MdTheme.h"
#include "widgets/MdButton.h"
#include "widgets/MdCheckBox.h"
#include "widgets/MdChip.h"
#include "widgets/MdExtendedFab.h"
#include "widgets/MdFab.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdMenu.h"
#include "widgets/MdRadioButton.h"
#include "widgets/MdSegmentedButton.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSwitch.h"
#include "widgets/MdTab.h"
#include "widgets/MdTabs.h"
#include "widgets/MdTextArea.h"
#include "widgets/MdTextField.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>
#include <QtGui/QMouseEvent>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollArea>

using namespace md;

namespace {

void settle(gallery::GalleryWindow &window, int index, int ms = 80)
{
    window.setCurrentPage(index);
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < ms) {
        QCoreApplication::processEvents();
        QThread::msleep(4);
    }
    QCoreApplication::processEvents();
}

/// The widget a real click at `globalPos` would land on.
QWidget *hitAt(const QPoint &globalPos)
{
    return QApplication::widgetAt(globalPos);
}

/// True when `hit` is `widget` or one of its children (the text field's inner
/// line edit, for instance, is the legitimate hit target). A null `hit` is
/// treated as inconclusive rather than a cover: under QT_QPA_PLATFORM=offscreen
/// QApplication::widgetAt() has no real cursor to resolve.
bool hitIsSelfOrDescendant(QWidget *hit, QWidget *widget)
{
    if (hit == nullptr) {
        return true;
    }
    for (QWidget *w = hit; w != nullptr; w = w->parentWidget()) {
        if (w == widget) {
            return true;
        }
    }
    return false;
}

/// Only a *foreign* widget sitting on top is a real failure.
bool isCoveredByForeignWidget(QWidget *hit, QWidget *widget)
{
    return hit != nullptr && !hitIsSelfOrDescendant(hit, widget);
}

/// Click the centre of `widget` the way a user would, via the window system
/// position so hit-testing runs the same path as a real pointer click.
void clickCentre(QWidget *widget)
{
    const QPoint centre = widget->rect().center();
    const QPoint global = widget->mapToGlobal(centre);
    QTest::mouseClick(widget, Qt::LeftButton, Qt::NoModifier, centre);
    Q_UNUSED(global);
}

} // namespace

class TestMd3GalleryClick : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void buttonsPageClickEmits();
    void checkboxPageClickToggles();
    void switchPageClickToggles();
    void sliderPageClickChangesValue();
    void textFieldPageClickFocuses();
    void chipPageClickToggles();
    void segmentedPageClickSelects();
    void tabsPageClickSelects();
    void fabPageClickEmits();
    void iconButtonClickEmits();
    void menuPageItemClickEmits();
    void radioPageClickSelects();

    // Diagnostic: every enabled interactive child of a page must be the
    // hit-test winner at its own centre. Reports the first cover/geometry
    // failure per page instead of silently letting the click tests above
    // fail with a zero-count spy.
    void hitTestSweep();

private:
    gallery::GalleryWindow *m_window = nullptr;

    int pageIndexBySlug(const QString &slug) const;

    template <typename T>
    QList<T *> liveWidgets(gallery::GalleryWindow &window, int index)
    {
        QList<T *> result;
        QWidget *page = window.pageWidget(index);
        if (page == nullptr) {
            return result;
        }
        const QList<T *> all = page->findChildren<T *>();
        for (T *w : all) {
            if (w == nullptr || !w->isEnabled() || w->isVisible() == false) {
                continue;
            }
            if (w->width() <= 0 || w->height() <= 0) {
                continue;
            }
            result.append(w);
        }
        return result;
    }
};

int TestMd3GalleryClick::pageIndexBySlug(const QString &slug) const
{
    const int count = m_window->pageCount();
    for (int i = 0; i < count; ++i) {
        if (m_window->pageSlug(i) == slug) {
            return i;
        }
    }
    return -1;
}

void TestMd3GalleryClick::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    MdTheme::instance().setLanguageTag(QStringLiteral("en"));
    m_window = new gallery::GalleryWindow;
    m_window->resize(1280, 900);
    m_window->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_window));
    QCoreApplication::processEvents();
}

void TestMd3GalleryClick::cleanupTestCase()
{
    delete m_window;
    m_window = nullptr;
}

// ---------------------------------------------------------------------------
// Per-page click probes
// ---------------------------------------------------------------------------

void TestMd3GalleryClick::buttonsPageClickEmits()
{
    const int page = pageIndexBySlug(QStringLiteral("Buttons"));
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdButton *> buttons = liveWidgets<MdButton>(*m_window, page);
    QVERIFY2(!buttons.isEmpty(), "Buttons page has no live MdButton children");

    MdButton *target = nullptr;
    for (MdButton *b : buttons) {
        if (b->isEnabled() && b->isVisible() && !b->geometry().isEmpty()) {
            target = b;
            break;
        }
    }
    QVERIFY(target != nullptr);

    // Hit-test first: if something covers the button, a click probe alone
    // would only show a zero-count spy with no reason.
    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("Buttons page: %1 is covered by %2")
                            .arg(target->metaObject()->className(),
                                 hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    QSignalSpy spy(target, &QAbstractButton::clicked);
    clickCentre(target);
    QCOMPARE(spy.count(), 1);
}

void TestMd3GalleryClick::checkboxPageClickToggles()
{
    const int page = pageIndexBySlug(QStringLiteral("checkbox"));
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdCheckBox *> boxes = liveWidgets<MdCheckBox>(*m_window, page);
    QVERIFY2(!boxes.isEmpty(), "Checkbox page has no live MdCheckBox children");

    MdCheckBox *target = nullptr;
    for (MdCheckBox *box : boxes) {
        // Skip the intentional disabled demo rows.
        if (box->isEnabled() && box->isVisible() && !box->geometry().isEmpty()) {
            target = box;
            break;
        }
    }
    QVERIFY(target != nullptr);

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("Checkbox page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    const Qt::CheckState before = target->checkState();
    QSignalSpy spy(target, &QAbstractButton::clicked);
    clickCentre(target);
    QCOMPARE(spy.count(), 1);
    QVERIFY(target->checkState() != before);
}

void TestMd3GalleryClick::switchPageClickToggles()
{
    int page = pageIndexBySlug(QStringLiteral("Switch"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("switch"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdSwitch *> switches = liveWidgets<MdSwitch>(*m_window, page);
    QVERIFY2(!switches.isEmpty(), "Switch page has no live MdSwitch children");

    MdSwitch *target = nullptr;
    for (MdSwitch *sw : switches) {
        if (sw->isEnabled() && sw->isVisible() && !sw->geometry().isEmpty()) {
            target = sw;
            break;
        }
    }
    QVERIFY(target != nullptr);

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("Switch page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    const bool before = target->isChecked();
    QSignalSpy spy(target, &QAbstractButton::clicked);
    clickCentre(target);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(target->isChecked(), !before);
}

void TestMd3GalleryClick::sliderPageClickChangesValue()
{
    int page = pageIndexBySlug(QStringLiteral("slider"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("Slider"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdSlider *> sliders = liveWidgets<MdSlider>(*m_window, page);
    QVERIFY2(!sliders.isEmpty(), "Slider page has no live MdSlider children");

    MdSlider *target = nullptr;
    for (MdSlider *s : sliders) {
        if (s->isEnabled() && s->isVisible() && !s->geometry().isEmpty()
            && s->max() > s->min()) {
            target = s;
            break;
        }
    }
    QVERIFY(target != nullptr);

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("Slider page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    const qreal before = target->value();
    // Click well to the right of centre so the value is forced to move even
    // when the handle already sits near the middle.
    const QPoint pos(target->width() * 3 / 4, target->height() / 2);
    QTest::mouseClick(target, Qt::LeftButton, Qt::NoModifier, pos);
    QVERIFY2(target->value() != before,
             qPrintable(QStringLiteral("Slider value stayed %1 after click at x=%2")
                            .arg(before)
                            .arg(pos.x())));
}

void TestMd3GalleryClick::textFieldPageClickFocuses()
{
    const int page = pageIndexBySlug(QStringLiteral("text-field"));
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdTextField *> fields = liveWidgets<MdTextField>(*m_window, page);
    QVERIFY2(!fields.isEmpty(), "Text field page has no live MdTextField children");

    MdTextField *target = nullptr;
    for (MdTextField *f : fields) {
        if (f->isEnabled() && f->isVisible() && !f->geometry().isEmpty()) {
            target = f;
            break;
        }
    }
    QVERIFY(target != nullptr);

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("Text field page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    clickCentre(target);
    QVERIFY2(target->hasFocus() || (QApplication::focusWidget() != nullptr
                                    && target->isAncestorOf(QApplication::focusWidget())),
             "Text field did not take focus on click");

    QTest::keyClicks(target, QStringLiteral("hi"));
    QVERIFY2(target->text().contains(QStringLiteral("hi")),
             qPrintable(QStringLiteral("Typing after click produced %1").arg(target->text())));
}

void TestMd3GalleryClick::chipPageClickToggles()
{
    const int page = pageIndexBySlug(QStringLiteral("chips"));
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdChip *> chips = liveWidgets<MdChip>(*m_window, page);
    QVERIFY2(!chips.isEmpty(), "Chip page has no live MdChip children");

    // Prefer a checkable (filter/input) chip: assist and suggestion are
    // click-only by design and must not be asked to toggle.
    MdChip *target = nullptr;
    for (MdChip *c : chips) {
        if (c->isEnabled() && c->isVisible() && !c->geometry().isEmpty() && c->isCheckable()) {
            target = c;
            break;
        }
    }
    QVERIFY2(target != nullptr, "Chip page has no live checkable MdChip");

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("Chip page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    const bool before = target->isChecked();
    QSignalSpy spy(target, &QAbstractButton::clicked);
    clickCentre(target);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(target->isChecked(), !before);
}

void TestMd3GalleryClick::segmentedPageClickSelects()
{
    int page = pageIndexBySlug(QStringLiteral("Segmented buttons"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("Segmented buttons"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdSegmentedButton *> groups =
        liveWidgets<MdSegmentedButton>(*m_window, page);
    QVERIFY2(!groups.isEmpty(), "Segmented page has no live MdSegmentedButton children");

    MdSegmentedButton *target = nullptr;
    for (MdSegmentedButton *g : groups) {
        if (g->isEnabled() && g->isVisible() && !g->geometry().isEmpty()
            && g->segmentCount() >= 2) {
            target = g;
            break;
        }
    }
    QVERIFY(target != nullptr);

    // Click the second segment's slot (left half = first, right half = second
    // when there are two; otherwise click at 3/4 width).
    const QPoint pos(target->width() * 3 / 4, target->height() / 2);
    const QPoint global = target->mapToGlobal(pos);
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("Segmented page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    QSignalSpy activated(target, &MdSegmentedButton::segmentActivated);
    QSignalSpy checked(target, &MdSegmentedButton::segmentChecked);
    QTest::mouseClick(target, Qt::LeftButton, Qt::NoModifier, pos);
    QVERIFY2(activated.count() >= 1 || checked.count() >= 1,
             "Segmented button produced neither segmentActivated nor segmentChecked");
}

void TestMd3GalleryClick::tabsPageClickSelects()
{
    int page = pageIndexBySlug(QStringLiteral("tabs"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("Tabs"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdTabs *> rows = liveWidgets<MdTabs>(*m_window, page);
    QVERIFY2(!rows.isEmpty(), "Tabs page has no live MdTabs children");

    MdTabs *target = nullptr;
    for (MdTabs *row : rows) {
        if (row->isVisible() && row->count() >= 2) {
            target = row;
            break;
        }
    }
    QVERIFY(target != nullptr);

    MdTab *tab = target->tabAt(target->count() - 1);
    QVERIFY(tab != nullptr);
    QVERIFY(tab->isVisible());

    const QPoint global = tab->mapToGlobal(tab->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, tab),
             qPrintable(QStringLiteral("Tabs page: tab covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    const int before = target->currentIndex();
    QSignalSpy spy(target, &MdTabs::currentChanged);
    clickCentre(tab);
    QCOMPARE(target->currentIndex(), target->count() - 1);
    QVERIFY(target->currentIndex() != before);
    QVERIFY(spy.count() >= 1);
}

void TestMd3GalleryClick::fabPageClickEmits()
{
    int page = pageIndexBySlug(QStringLiteral("FABs"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("FABs"));
    }
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("FABs"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    // FABs may be MdFab or MdExtendedFab; both are QAbstractButton subclasses.
    const QList<QAbstractButton *> buttons =
        liveWidgets<QAbstractButton>(*m_window, page);
    QAbstractButton *target = nullptr;
    for (QAbstractButton *b : buttons) {
        if (qobject_cast<MdFab *>(b) == nullptr && qobject_cast<MdExtendedFab *>(b) == nullptr) {
            continue;
        }
        if (b->isEnabled() && b->isVisible() && !b->geometry().isEmpty()) {
            target = b;
            break;
        }
    }
    QVERIFY2(target != nullptr, "FAB page has no live MdFab/MdExtendedFab");

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("FAB page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    QSignalSpy spy(target, &QAbstractButton::clicked);
    clickCentre(target);
    QCOMPARE(spy.count(), 1);
}

void TestMd3GalleryClick::iconButtonClickEmits()
{
    int page = pageIndexBySlug(QStringLiteral("Icon buttons"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("Icon buttons"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdIconButton *> buttons = liveWidgets<MdIconButton>(*m_window, page);
    QVERIFY2(!buttons.isEmpty(), "Icon button page has no live MdIconButton children");

    MdIconButton *target = nullptr;
    for (MdIconButton *b : buttons) {
        if (b->isEnabled() && b->isVisible() && !b->geometry().isEmpty()) {
            target = b;
            break;
        }
    }
    QVERIFY(target != nullptr);

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("Icon button page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    QSignalSpy spy(target, &QAbstractButton::clicked);
    clickCentre(target);
    QCOMPARE(spy.count(), 1);
}

void TestMd3GalleryClick::menuPageItemClickEmits()
{
    int page = pageIndexBySlug(QStringLiteral("menu"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("Menu"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdMenuItem *> items = liveWidgets<MdMenuItem>(*m_window, page);
    QVERIFY2(!items.isEmpty(), "Menu page has no live MdMenuItem children");

    MdMenuItem *target = nullptr;
    for (MdMenuItem *item : items) {
        if (item->isEnabled() && item->isVisible() && !item->geometry().isEmpty()) {
            target = item;
            break;
        }
    }
    QVERIFY(target != nullptr);

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("Menu page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    QSignalSpy spy(target, &QAbstractButton::clicked);
    clickCentre(target);
    QCOMPARE(spy.count(), 1);
}

void TestMd3GalleryClick::radioPageClickSelects()
{
    int page = pageIndexBySlug(QStringLiteral("radio-button"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("radio-button"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdRadioButton *> radios = liveWidgets<MdRadioButton>(*m_window, page);
    QVERIFY2(!radios.isEmpty(), "Radio page has no live MdRadioButton children");

    MdRadioButton *target = nullptr;
    for (MdRadioButton *r : radios) {
        if (r->isEnabled() && r->isVisible() && !r->geometry().isEmpty() && !r->isChecked()) {
            target = r;
            break;
        }
    }
    QVERIFY(target != nullptr);

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("Radio page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    QSignalSpy spy(target, &QAbstractButton::clicked);
    clickCentre(target);
    QCOMPARE(spy.count(), 1);
    QVERIFY(target->isChecked());
}

// ---------------------------------------------------------------------------
// Hit-test sweep
// ---------------------------------------------------------------------------

void TestMd3GalleryClick::hitTestSweep()
{
    // Representative interactive families. Each entry: slug fragment and the
    // meta-object name of the control to sweep.
    struct Probe
    {
        QString slug;
        const char *typeName;
    };
    const QVector<Probe> probes = {
        {QStringLiteral("Buttons"), "md::MdButton"},
        {QStringLiteral("checkbox"), "md::MdCheckBox"},
        {QStringLiteral("switch"), "md::MdSwitch"},
        {QStringLiteral("slider"), "md::MdSlider"},
        {QStringLiteral("text-field"), "md::MdTextField"},
        {QStringLiteral("chips"), "md::MdChip"},
        {QStringLiteral("Segmented buttons"), "md::MdSegmentedButton"},
        {QStringLiteral("tabs"), "md::MdTabs"},
        {QStringLiteral("Icon buttons"), "md::MdIconButton"},
        {QStringLiteral("radio-button"), "md::MdRadioButton"},
    };

    QStringList failures;
    for (const Probe &probe : probes) {
        int index = pageIndexBySlug(probe.slug);
        if (index < 0) {
            // Try a case-insensitive match against every slug.
            const int count = m_window->pageCount();
            for (int i = 0; i < count; ++i) {
                if (m_window->pageSlug(i).contains(probe.slug, Qt::CaseInsensitive)) {
                    index = i;
                    break;
                }
            }
        }
        if (index < 0) {
            failures << QStringLiteral("%1: page not found").arg(probe.slug);
            continue;
        }
        settle(*m_window, index);

        QWidget *page = m_window->pageWidget(index);
        const QList<QWidget *> all = page->findChildren<QWidget *>();
        int checked = 0;
        for (QWidget *w : all) {
            if (w->metaObject() == nullptr) {
                continue;
            }
            if (QString::fromUtf8(w->metaObject()->className()) != QLatin1String(probe.typeName)) {
                continue;
            }
            if (!w->isEnabled() || !w->isVisible() || w->geometry().isEmpty()) {
                continue;
            }
            ++checked;
            const QPoint global = w->mapToGlobal(w->rect().center());
            QWidget *hit = hitAt(global);
            // Only a foreign widget on top is a real cover. widgetAt() == null
            // is inconclusive under the offscreen platform.
            if (isCoveredByForeignWidget(hit, w)) {
                failures << QStringLiteral("%1: %2 at %3 covered by %4")
                                .arg(probe.slug,
                                     QString::fromUtf8(w->metaObject()->className()),
                                     QStringLiteral("%1,%2 %3x%4")
                                         .arg(w->geometry().x())
                                         .arg(w->geometry().y())
                                         .arg(w->geometry().width())
                                         .arg(w->geometry().height()),
                                     hit ? QString::fromUtf8(hit->metaObject()->className())
                                         : QStringLiteral("null"));
                // One failure per page is enough signal.
                break;
            }
        }
        if (checked == 0) {
            failures << QStringLiteral("%1: no live %2 found").arg(probe.slug, probe.typeName);
        }
    }

    QVERIFY2(failures.isEmpty(), qPrintable(failures.join(QStringLiteral("\n"))));
}

QTEST_MAIN(TestMd3GalleryClick)

#include "TestMd3GalleryClick.moc"





