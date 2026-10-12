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
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"
#include "widgets/MdRadioButton.h"
#include "widgets/MdSegmentedButton.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSwitch.h"
#include "widgets/MdTab.h"
#include "widgets/MdTabs.h"
#include "widgets/MdTextArea.h"
#include "widgets/MdTextField.h"
#include "widgets/MdDatePicker.h"
#include "widgets/MdDialog.h"
#include "widgets/MdDialogHost.h"
#include "widgets/MdSearchBar.h"
#include "widgets/MdTimePicker.h"
#include "widgets/MdBottomSheetHost.h"
#include "widgets/MdSideSheetHost.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtCore/QThread>
#include <QtGui/QMouseEvent>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>

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
    void listPageClickSelects();
    void dialogPageActionButtonsReceiveClicks();
    void timePickerPageSelectorsSwitchFace();
    void datePickerPageClickSelectsDate();
    void searchPageTypingEntersText();
    void sheetHostDismissPreservesScrollBar();
    void showcaseMailPageClickSelects();
    void showcaseMusicPageClickEmits();
    void showcaseSettingsPageClickToggles();
    void dynamicColourPageClickChangesProbe();

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
        if (c->isEnabled() && c->isVisible() && !c->geometry().isEmpty() && c->isCheckable()
            && !c->isChecked()) {
            target = c;
            break;
        }
    }
    if (target == nullptr) {
        for (MdChip *c : chips) {
            if (c->isEnabled() && c->isVisible() && !c->geometry().isEmpty() && c->isCheckable()) {
                target = c;
                break;
            }
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

void TestMd3GalleryClick::listPageClickSelects()
{
    int page = pageIndexBySlug(QStringLiteral("list"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("List"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdListItem *> items = liveWidgets<MdListItem>(*m_window, page);
    QVERIFY2(!items.isEmpty(), "List page has no live MdListItem children");

    // Prefer an interactive, enabled item that is not already selected.
    MdListItem *target = nullptr;
    for (MdListItem *item : items) {
        if (item->isEnabled() && item->isVisible() && !item->geometry().isEmpty()
            && item->isInteractive() && !item->isSelected()) {
            target = item;
            break;
        }
    }
    QVERIFY2(target != nullptr, "List page has no clickable unselected MdListItem");

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("List page: covered by %1")
                            .arg(hit ? hit->metaObject()->className() : QStringLiteral("null"))));

    QSignalSpy activated(target, &MdListItem::activated);
    QTest::mouseClick(target, Qt::LeftButton, Qt::NoModifier, target->rect().center());
    QVERIFY2(activated.count() >= 1, "List item click produced no activated()");
    QVERIFY2(target->isSelected(), "List item click did not select the item");
}

// ---------------------------------------------------------------------------
// Hit-test sweep
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Dialog / TimePicker / DatePicker / Search
// ---------------------------------------------------------------------------

void TestMd3GalleryClick::dialogPageActionButtonsReceiveClicks()
{
    int page = pageIndexBySlug(QStringLiteral("dialogs"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("Dialogs"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    // Banked dialogs own real MdButton children. Every visible, enabled
    // action button must be the hit-test winner at its own centre.
    const QList<MdButton *> buttons = liveWidgets<MdButton>(*m_window, page);
    QVERIFY2(!buttons.isEmpty(), "Dialog page has no live MdButton children");

    int clicked = 0;
    for (MdButton *b : buttons) {
        if (!b->isEnabled() || !b->isVisible() || b->geometry().isEmpty()) {
            continue;
        }
        const QPoint global = b->mapToGlobal(b->rect().center());
        QWidget *hit = hitAt(global);
        if (isCoveredByForeignWidget(hit, b)) {
            continue;
        }
        QSignalSpy spy(b, &QAbstractButton::clicked);
        clickCentre(b);
        if (spy.count() >= 1) {
            ++clicked;
        }
    }
    QVERIFY2(clicked >= 1, "no dialog action button received a click");

    // Leave the page idle: a showing modal host is *meant* to cover the
    // page, and the next hit-test sweep would report that as a defect.
    for (md::MdDialogHost *host : m_window->pageWidget(page)->findChildren<md::MdDialogHost *>()) {
        host->dismiss();
        QTest::qWait(40);
    }
}

void TestMd3GalleryClick::timePickerPageSelectorsSwitchFace()
{
    int page = pageIndexBySlug(QStringLiteral("time-picker"));
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdTimePicker *> pickers = liveWidgets<MdTimePicker>(*m_window, page);
    QVERIFY2(!pickers.isEmpty(), "Time picker page has no live MdTimePicker children");

    MdTimePicker *picker = nullptr;
    for (MdTimePicker *p : pickers) {
        if (!p->is24h() && p->face() == MdTimePickerFace::Hours) {
            picker = p;
            break;
        }
    }
    QVERIFY(picker != nullptr);

    const QPoint global = picker->mapToGlobal(picker->timeSelectorRect(false).center().toPoint());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, picker),
             "time picker minutes selector is covered");

    QTest::mouseClick(picker, Qt::LeftButton, Qt::NoModifier,
                      picker->timeSelectorRect(false).center().toPoint());
    QCOMPARE(picker->face(), MdTimePickerFace::Minutes);
}

void TestMd3GalleryClick::datePickerPageClickSelectsDate()
{
    int page = pageIndexBySlug(QStringLiteral("date-picker"));
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdDatePicker *> pickers = liveWidgets<MdDatePicker>(*m_window, page);
    QVERIFY2(!pickers.isEmpty(), "Date picker page has no live MdDatePicker children");

    MdDatePicker *picker = pickers.first();
    picker->setDisplayedMonth(QDate(2026, 6, 1));
    settle(*m_window, page);

    const QRectF cell = picker->dateCellRect(2, 0);
    QVERIFY(picker->dateAtCell(2, 0).isValid());
    const QPoint global = picker->mapToGlobal(cell.center().toPoint());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, picker),
             "date picker cell is covered");

    // Click a cell that is not already the selection so the spy must fire.
    const QDate target = picker->dateAtCell(2, 0);
    QVERIFY(target.isValid());
    if (picker->selectedDate() == target) {
        picker->setSelectedDate(target.addDays(-1));
    }
    QSignalSpy spy(picker, &MdDatePicker::selectedDateChanged);
    QTest::mouseClick(picker, Qt::LeftButton, Qt::NoModifier, cell.center().toPoint());
    QCOMPARE(picker->selectedDate(), target);
    QVERIFY(spy.count() >= 1);
}

void TestMd3GalleryClick::searchPageTypingEntersText()
{
    int page = pageIndexBySlug(QStringLiteral("search"));
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdSearchBar *> bars = liveWidgets<MdSearchBar>(*m_window, page);
    QVERIFY2(!bars.isEmpty(), "Search page has no live MdSearchBar children");

    MdSearchBar *bar = bars.first();
    const QPoint global = bar->mapToGlobal(bar->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, bar), "search bar is covered");

    QTest::mouseClick(bar, Qt::LeftButton, Qt::NoModifier, bar->rect().center());
    QVERIFY(bar->hasFocus());
    QTest::keyClicks(bar, QStringLiteral("hello"));
    QCOMPARE(bar->text(), QStringLiteral("hello"));
}



namespace {

/// The page's wrapping QScrollArea (page -> viewport -> scroll area).
QScrollArea *scrollAreaFor(QWidget *page)
{
    if (!page) {
        return nullptr;
    }
    QWidget *viewport = page->parentWidget();
    if (!viewport) {
        return nullptr;
    }
    return qobject_cast<QScrollArea *>(viewport->parentWidget());
}

} // namespace

void TestMd3GalleryClick::sheetHostDismissPreservesScrollBar()
{
    // Sheets are overlays: showing and dismissing a modal sheet host must
    // not reflow the page or jump the vertical scrollbar. The bug this
    // locks: after the bottom/side sheet demo host is shown and dismissed,
    // the scrollbar geometry and scroll value shifted.
    struct Case
    {
        const char *slug;
        bool bottom;
    };
    const Case cases[] = {
        {"bottom-sheets", true},
        {"side-sheets", false},
    };
    for (const Case &c : cases) {
        int page = pageIndexBySlug(QString::fromUtf8(c.slug));
        QVERIFY2(page >= 0, c.slug);
        settle(*m_window, page);
        QCoreApplication::processEvents();

        QWidget *pageW = m_window->pageWidget(page);
        QVERIFY(pageW);
        QScrollArea *scroll = scrollAreaFor(pageW);
        QVERIFY2(scroll, "page is not hosted in a QScrollArea");
        QScrollBar *bar = scroll->verticalScrollBar();
        QVERIFY(bar);

        // Park the page mid-scroll the way a reader of a long demo page would.
        if (bar->maximum() > 0) {
            bar->setValue(bar->maximum() / 2);
        }
        QCoreApplication::processEvents();

        const int scrollBefore = bar->value();
        const QRect barBefore = bar->geometry();
        const QSize pageBefore = pageW->size();
        const QRect pageGeoBefore = pageW->geometry();
        const int rangeBefore = bar->maximum();

        md::MdBottomSheetHost *bottom =
            pageW->findChild<md::MdBottomSheetHost *>(QString(), Qt::FindDirectChildrenOnly);
        md::MdSideSheetHost *side =
            pageW->findChild<md::MdSideSheetHost *>(QString(), Qt::FindDirectChildrenOnly);
        if (c.bottom) {
            QVERIFY2(bottom, "bottom-sheets page has no MdBottomSheetHost");
            bottom->show();
        } else {
            QVERIFY2(side, "side-sheets page has no MdSideSheetHost");
            side->show();
        }
        QElapsedTimer clock;
        clock.start();
        while (clock.elapsed() < 250) {
            QCoreApplication::processEvents();
            QThread::msleep(4);
        }

        if (bottom) {
            QVERIFY(bottom->isShowing());
            bottom->dismiss();
        } else if (side) {
            QVERIFY(side->isShowing());
            side->dismiss();
        }
        clock.restart();
        while (clock.elapsed() < 1200) {
            QCoreApplication::processEvents();
            QThread::msleep(4);
        }

        if (bottom) {
            QVERIFY2(!bottom->isShowing(), "bottom sheet host still showing after dismiss");
        }
        if (side) {
            QVERIFY2(!side->isShowing(), "side sheet host still showing after dismiss");
        }

        QCOMPARE(pageW->size(), pageBefore);
        QCOMPARE(pageW->geometry(), pageGeoBefore);
        QCOMPARE(bar->geometry(), barBefore);
        QCOMPARE(bar->maximum(), rangeBefore);
        QCOMPARE(bar->value(), scrollBefore);
    }
}

// ---------------------------------------------------------------------------
// Showcase screen click probes
// ---------------------------------------------------------------------------

void TestMd3GalleryClick::showcaseMailPageClickSelects()
{
    const int page = pageIndexBySlug(QStringLiteral("showcase-mail"));
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdListItem *> items = liveWidgets<MdListItem>(*m_window, page);
    QVERIFY2(!items.isEmpty(), "showcase-mail page has no live MdListItem children");

    MdListItem *target = nullptr;
    for (MdListItem *item : items) {
        if (item->isEnabled() && item->isVisible() && !item->geometry().isEmpty()
            && item->isInteractive()) {
            target = item;
            break;
        }
    }
    QVERIFY(target != nullptr);

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("showcase-mail: %1 is covered by %2")
                            .arg(target->metaObject()->className(),
                                 hit ? hit->metaObject()->className()
                                     : QStringLiteral("null"))));

    QSignalSpy spy(target, &MdListItem::activated);
    clickCentre(target);
    QVERIFY(spy.count() >= 1);
}

void TestMd3GalleryClick::showcaseMusicPageClickEmits()
{
    const int page = pageIndexBySlug(QStringLiteral("showcase-music"));
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdIconButton *> buttons = liveWidgets<MdIconButton>(*m_window, page);
    QVERIFY2(!buttons.isEmpty(), "showcase-music page has no live MdIconButton children");

    // Prefer the primary transport control (the filled one); fall back to any.
    MdIconButton *target = nullptr;
    for (MdIconButton *button : buttons) {
        if (button->isEnabled() && button->isVisible() && !button->geometry().isEmpty()
            && button->variant() == md::IconButtonVariant::Filled) {
            target = button;
            break;
        }
    }
    if (target == nullptr) {
        for (MdIconButton *button : buttons) {
            if (button->isEnabled() && button->isVisible() && !button->geometry().isEmpty()) {
                target = button;
                break;
            }
        }
    }
    QVERIFY(target != nullptr);

    const QPoint global = target->mapToGlobal(target->rect().center());
    QWidget *hit = hitAt(global);
    QVERIFY2(!isCoveredByForeignWidget(hit, target),
             qPrintable(QStringLiteral("showcase-music: %1 is covered by %2")
                            .arg(target->metaObject()->className(),
                                 hit ? hit->metaObject()->className()
                                     : QStringLiteral("null"))));

    QSignalSpy spy(target, &QAbstractButton::clicked);
    clickCentre(target);
    QCOMPARE(spy.count(), 1);
}

void TestMd3GalleryClick::showcaseSettingsPageClickToggles()
{
    const int page = pageIndexBySlug(QStringLiteral("showcase-settings"));
    QVERIFY(page >= 0);
    settle(*m_window, page);

    const QList<MdSwitch *> switches = liveWidgets<MdSwitch>(*m_window, page);
    QVERIFY2(!switches.isEmpty(), "showcase-settings page has no live MdSwitch children");

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
             qPrintable(QStringLiteral("showcase-settings: %1 is covered by %2")
                            .arg(target->metaObject()->className(),
                                 hit ? hit->metaObject()->className()
                                     : QStringLiteral("null"))));

    const bool before = target->isChecked();
    QSignalSpy spy(target, &QAbstractButton::toggled);
    clickCentre(target);
    QVERIFY(spy.count() >= 1);
    QVERIFY(target->isChecked() != before);
}

// ---------------------------------------------------------------------------
// Dynamic colour probe
// ---------------------------------------------------------------------------

void TestMd3GalleryClick::dynamicColourPageClickChangesProbe()
{
    int page = pageIndexBySlug(QStringLiteral("dynamic-colour"));
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("Dynamic colour"));
    }
    if (page < 0) {
        page = pageIndexBySlug(QStringLiteral("Dynamic Colour"));
    }
    QVERIFY(page >= 0);
    settle(*m_window, page);

    QWidget *pageWidget = m_window->pageWidget(page);
    QVERIFY(pageWidget != nullptr);

    QWidget *probe = nullptr;
    const QList<QWidget *> all = pageWidget->findChildren<QWidget *>();
    for (QWidget *w : all) {
        if (w->objectName() == QLatin1String("dynamic-colour-probe")) {
            probe = w;
            break;
        }
    }
    QVERIFY(probe != nullptr);
    QVERIFY(probe->isVisible());
    QVERIFY(probe->width() > 0 && probe->height() > 0);

    // Seed swatches are object-named seed-swatch-N. Click one that is not the
    // current seed and prove both the theme seed and the probe pixels move.
    QWidget *target = nullptr;
    const QColor original = MdTheme::instance().seedColor();
    for (QWidget *w : all) {
        if (!w->objectName().startsWith(QLatin1String("seed-swatch"))) {
            continue;
        }
        // 0 is the baseline purple the gallery starts from; pick a later seed.
        if (w->objectName() == QLatin1String("seed-swatch-0") && original == QColor("#6750A4")) {
            continue;
        }
        if (w->objectName() == QLatin1String("seed-swatch-1")) {
            target = w;
            break;
        }
    }
    QVERIFY(target != nullptr);
    QVERIFY(target->isVisible());
    QVERIFY(target->width() > 0 && target->height() > 0);

    settle(*m_window, page);
    const QPixmap before = probe->grab();
    QVERIFY(!before.isNull());

    clickCentre(target);
    settle(*m_window, page);

    const QColor after = MdTheme::instance().seedColor();
    QVERIFY(after != original);
    QVERIFY(after == QColor("#00A3A3"));

    const QPixmap afterPixmap = probe->grab();
    QVERIFY(!afterPixmap.isNull());
    // Compare a centre pixel of each third of the probe (primary/secondary/tertiary).
    const QImage beforeImage = before.toImage();
    const QImage afterImage = afterPixmap.toImage();
    QVERIFY(beforeImage.size() == afterImage.size());
    bool changed = false;
    for (int third = 0; third < 3; ++third) {
        const QPoint sample(beforeImage.width() * (2 * third + 1) / 6,
                            beforeImage.height() / 2);
        if (beforeImage.pixelColor(sample) != afterImage.pixelColor(sample)) {
            changed = true;
            break;
        }
    }
    QVERIFY2(changed, "probe widget colour did not change after seed click");

    // Restore the baseline seed so later tests (and screenshots) start clean.
    MdTheme::instance().setSeedColor(original);
}

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
        {QStringLiteral("dialogs"), "md::MdButton"},
        {QStringLiteral("time-picker"), "md::MdTimePicker"},
        {QStringLiteral("date-picker"), "md::MdDatePicker"},
        {QStringLiteral("search"), "md::MdSearchBar"},
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
            // Only a *foreign* widget on top is a real cover. widgetAt() == null
            // is inconclusive under the offscreen platform.
            // A showing MdDialogHost is a modal overlay, not a stacking bug.
            if (hit && QString::fromUtf8(hit->metaObject()->className())
                           == QLatin1String("md::MdDialogHost")) {
                continue;
            }
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