// TestMd3Navigation — the Navigation family, bar first: MdNavigationBar and
// its MdNavigationBarItem children (rail and drawer follow in this file).
//
// Numbers transcribed from material-web tokens/versions/latest/sass —
// _md-comp-navigation-bar.scss, -nav-bar.scss, -nav-bar-item-vertical.scss,
// -nav-bar-item-horizontal.scss — cross-checked against androidx Compose
// Material3 NavigationBar.kt (the baseline behaviour) and ShortNavigationBar.kt
// + NavigationItem.kt (the flexible behaviour).
//
// This is the family where **one component has two unrelated token families in
// one export version** (both 34.0.21), and the tests keep them apart:
//
//   |          | family                      | height | indicator | active label |
//   | -------- | --------------------------- | ------ | --------- | ------------ |
//   | Baseline | `md.comp.navigation-bar.*`  | 80     | 64 x 32   | `on-surface` |
//   | Flexible | `md.comp.nav-bar.*` + items | 64     | 56 x 32   | `secondary`  |
//
// The assertions worth keeping are the ones a token table cannot express:
//
//   * the flexible family's own numbers are **not** the struct's defaults (the
//     defaults are the baseline's, and a resolve that inherited them would
//     silently turn the short family into the tall one);
//   * `item.between-space` is 8 — Compose's hard-coded `spacedBy(8.dp)`, not
//     the export's published `0px`, because a gap between items is behaviour;
//   * `Centered` insets the run by a *fraction of the bar* that shrinks as
//     items are added, and the items still share the remaining band equally;
//   * the item **fills the slot it is given** and centres its content inside —
//     the opposite of the button rule, and the reason `placeChildren` uses
//     `resizedGeometryOn`;
//   * the pill animates its **width only**, the label rides the same progress,
//     and a `alwaysShowLabel = false` item centres its icon while at rest;
//   * the `Start` position's pill wraps its content — 16 + icon + 4 + label +
//     16 wide, 40 tall — and its label is always visible, because it rides
//     inside the pill;
//   * the selected label is the *prominent* cut of the same size.

#include "core/MdNavigationBarTokens.h"
#include "core/MdNavigationRailTokens.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdChildBox.h"
#include "styles/MdNavigationBarItemStyle.h"
#include "styles/MdNavigationBarStyle.h"
#include "styles/MdNavigationRailStyle.h"
#include "widgets/MdNavigationBar.h"
#include "widgets/MdNavigationBarItem.h"
#include "widgets/MdNavigationRail.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

#include <cmath>

using namespace md;

namespace {

QColor pixelColorAt(QWidget &widget, const QPointF &position)
{
    QImage image(int(widget.width()), int(widget.height()), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    // DrawChildren only: a top-level widget would otherwise be erased with its
    // opaque window background first.
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32).pixelColor(position.toPoint());
}

/// Adds `count` heap items labelled A, B, C, ... The bar takes ownership, so
/// the items must never be stack objects (a QWidget deletes its children).
void addItems(MdNavigationBar &bar, int count)
{
    for (int i = 0; i < count; ++i) {
        auto *item = new MdNavigationBarItem(QString(QChar('A' + i)), QStringLiteral("home"));
        bar.addItem(item);
    }
}

/// The same for a rail, whose items are the same shared widget.
void addRailItems(MdNavigationRail &rail, int count)
{
    for (int i = 0; i < count; ++i) {
        auto *item = new MdNavigationBarItem(QString(QChar('A' + i)), QStringLiteral("home"));
        rail.addItem(item);
    }
}

/// A stand-in with a fixed hint, for the rail's header slot.
class FixedProbe : public QWidget
{
public:
    explicit FixedProbe(const QSize &hint, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_hint(hint)
    {
        resize(hint);
    }

    QSize sizeHint() const override { return m_hint; }

private:
    QSize m_hint;
};

/// Runs the pill's spring to `target`. The spring is driven by an 8 ms timer,
/// so the answer arrives through the event loop rather than synchronously.
bool settleIndicator(MdNavigationBarItem &item, qreal target)
{
    for (int i = 0; i < 400; ++i) {
        if (qAbs(item.indicatorProgress() - target) <= 1e-9) {
            return true;
        }
        QTest::qWait(10);
    }
    return qAbs(item.indicatorProgress() - target) <= 1e-9;
}

/// Runs the rail's width spring to `target` — same event-loop rule.
bool settleWidth(MdNavigationRail &rail, qreal target)
{
    for (int i = 0; i < 400; ++i) {
        if (qAbs(rail.currentWidth() - target) <= 1e-9) {
            return true;
        }
        QTest::qWait(10);
    }
    return qAbs(rail.currentWidth() - target) <= 1e-9;
}

/// A filter that counts the watched widget's resize/move events — used to pin
/// "painting must not move widgets".
class GeometrySpy : public QObject
{
public:
    using QObject::QObject;
    int count = 0;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
            ++count;
        }
        return QObject::eventFilter(watched, event);
    }
};

} // namespace

class TestMd3Navigation : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // --- tokens -------------------------------------------------------------
    void baselineTokenTable();
    void flexibleTokenTable();
    void flexibleDefaultsAreItsOwn();
    void disabledPairIsDerived();
    void tokenOverridesReachTheFamilies();

    // --- bar ----------------------------------------------------------------
    void equalWeightDividesTheBand();
    void centeredShrinksTheBand();
    void betweenSpaceIsTheBehaviourRow();
    void selectionIsExclusiveAndMirrored();
    void itemsInheritTheVariant();
    void itemsFillTheirSlot();
    void resizeReflowsTheItems();
    void barSizeHintFollowsTheVariant();
    void barPaintsItsContainer();

    // --- item ---------------------------------------------------------------
    void topItemPillGeometry();
    void startItemPillGeometry();
    void indicatorAnimatesWidthOnly();
    void labelFollowsTheSelectionWhenNotAlwaysShown();
    void selectedLabelIsTheProminentCut();
    void disabledWinsOverSelected();

    // --- rail ---------------------------------------------------------------
    void railTokenTable();
    void railCollapsedGeometry();
    void railHeaderGeometry();
    void railExpandedIsRefusedOnTheBaseline();
    void railFlexibleExpands();
    void railWidthAnimates();
    void railModalRows();
    void railNarrowRowIsCarried();

    // --- cross-cutting ------------------------------------------------------
    void rtlIsNotMirrored();
};

void TestMd3Navigation::init()
{
}

void TestMd3Navigation::cleanup()
{
    // A failing QCOMPARE / QVERIFY returns out of its slot immediately, so
    // every global a slot can touch is reset here. The list family paid for
    // getting this wrong four times.
    MdComponentTokens &global = MdComponentTokens::global();
    global.remove(QStringLiteral("md.comp.navigation-bar.container.height"));
    global.remove(QStringLiteral("md.comp.navigation-bar.active-indicator.shape"));
    global.remove(QStringLiteral("md.comp.navigation-bar.active-indicator.width"));
    global.remove(QStringLiteral("md.comp.nav-bar.container.height"));
    global.remove(QStringLiteral("md.comp.nav-bar-item-vertical.active-indicator.width"));
    global.remove(QStringLiteral("md.comp.nav-bar-item-horizontal.active-indicator.height"));
    global.remove(QStringLiteral("md.comp.nav-rail-collapsed.narrow-container-width"));
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

void TestMd3Navigation::baselineTokenTable()
{
    const MdNavigationBarTokens tokens = MdNavigationBarTokens::resolve();
    const MdNavigationBarVariantTokens &v = tokens.forVariant(MdNavigationBarVariant::Baseline);

    // The tall family. Compose's `NavigationBar.kt` reads
    // `TallContainerHeight` (80) while its own token file declares
    // `ContainerHeight = 64` under a `// TODO` — the implementation body and
    // the export agree on 80, so 80 it is.
    QCOMPARE(v.containerHeight, 80.0);
    QCOMPARE(v.activeIndicatorWidth, 64.0);
    QCOMPARE(v.activeIndicatorHeight, 32.0);
    QCOMPARE(v.iconSize, 24.0);
    // The baseline publishes no item rows at all: no item padding, no
    // icon-label space of its own. The 8 px between items is the container's
    // arrangement, not the item's — see `betweenSpaceIsTheBehaviourRow`.
    QCOMPARE(v.containerBetweenSpace, 0.0);

    // The pill paddings are derived, and Compose spells the results out as
    // constants: `(56 - 24) / 2` would be 16, but this family's 64 px pill
    // gives 20 — the row Compose's baseline never reads and the export's own
    // arithmetic contradicts it on.
    QCOMPARE(v.indicatorHorizontalPadding(), 20.0);
    QCOMPARE(v.indicatorVerticalPadding(), 4.0);

    QVERIFY(v.containerShape == ShapeCorner::None);
    QVERIFY(v.indicatorShape == ShapeCorner::Full);
    // Published as level2 and carried; the paint does not draw it (the spec's
    // "Differences from M2: Elevation: No shadow"). See MdNavigationBarStyle.
    QVERIFY(v.containerElevation == ElevationLevel::Level2);
    QVERIFY(v.containerColor == ColorRole::SurfaceContainer);
    QVERIFY(v.containerSurfaceTintLayerColor == ColorRole::SurfaceTint);

    // The active label is `on-surface` — the row the flexible family changed
    // to `secondary`.
    QVERIFY(v.coloursFor(true).iconFor(MdNavigationItemState::Enabled).role
            == ColorRole::OnSecondaryContainer);
    QVERIFY(v.coloursFor(true).labelFor(MdNavigationItemState::Enabled).role
            == ColorRole::OnSurface);
    QVERIFY(v.coloursFor(true).indicator.role == ColorRole::SecondaryContainer);
    QVERIFY(v.coloursFor(false).iconFor(MdNavigationItemState::Enabled).role
            == ColorRole::OnSurfaceVariant);
    QVERIFY(v.coloursFor(false).indicator.isPresent() == false);

    // In a state the unselected icon and label both lift to `on-surface`.
    QVERIFY(v.coloursFor(false).iconFor(MdNavigationItemState::Hovered).role
            == ColorRole::OnSurface);
    QVERIFY(v.coloursFor(false).labelFor(MdNavigationItemState::Hovered).role
            == ColorRole::OnSurface);
    QVERIFY(v.coloursFor(false).stateLayerFor(MdNavigationItemState::Hovered).role
            == ColorRole::OnSurface);

    // The focus ring rows the baseline publishes and the flexible family
    // inherits.
    QVERIFY(v.focusIndicatorColor == ColorRole::Secondary);
    QCOMPARE(v.focusIndicatorOffset, 2.0);
    QCOMPARE(v.focusIndicatorThickness, 3.0);

    QCOMPARE(v.hoverStateLayerOpacity, 0.08);
    QCOMPARE(v.focusStateLayerOpacity, 0.12);
    QCOMPARE(v.pressedStateLayerOpacity, 0.12);
}

void TestMd3Navigation::flexibleTokenTable()
{
    const MdNavigationBarTokens tokens = MdNavigationBarTokens::resolve();
    const MdNavigationBarVariantTokens &v = tokens.forVariant(MdNavigationBarVariant::Flexible);

    // The short family that replaces the baseline one. Both families report
    // the same export version (34.0.21); they are two products, not two
    // snapshots of one.
    QCOMPARE(v.containerHeight, 64.0);
    QCOMPARE(v.activeIndicatorWidth, 56.0);
    QCOMPARE(v.activeIndicatorHeight, 32.0);
    QCOMPARE(v.iconSize, 24.0);
    // The item's own vertical padding, which the baseline family has no row
    // for: Compose's `TopIconItemVerticalPadding = ContainerBetweenSpace`.
    QCOMPARE(v.containerBetweenSpace, 6.0);
    QCOMPARE(v.indicatorHorizontalPadding(), 16.0);
    QCOMPARE(v.indicatorVerticalPadding(), 4.0);

    // The `Start` position's rows — the horizontal item. The baseline
    // publishes no horizontal rows at all, so these are flexible-only.
    QCOMPARE(v.horizontalIndicatorHeight, 40.0);
    QCOMPARE(v.horizontalIndicatorLeadingSpace, 16.0);
    QCOMPARE(v.horizontalIndicatorTrailingSpace, 16.0);
    QCOMPARE(v.horizontalIndicatorVerticalPadding(), 8.0);

    // The active label moved to `secondary`, and the state layers ride the
    // pill on *both* sides of the selection, because the pill is
    // `secondary-container` either way.
    QVERIFY(v.coloursFor(true).labelFor(MdNavigationItemState::Enabled).role
            == ColorRole::Secondary);
    QVERIFY(v.coloursFor(true).stateLayerFor(MdNavigationItemState::Pressed).role
            == ColorRole::OnSecondaryContainer);
    QVERIFY(v.coloursFor(false).stateLayerFor(MdNavigationItemState::Pressed).role
            == ColorRole::OnSecondaryContainer);
    QVERIFY(v.coloursFor(true).indicator.role == ColorRole::SecondaryContainer);

    // The `Start` label sits inside the pill, so it takes the pill's own
    // content colour rather than the `Top` table's `secondary`.
    QVERIFY(v.labelColorStart == ColorRole::OnSecondaryContainer);

    // The flexible family publishes no focus rows; the ring is a system token
    // and the values are the baseline's.
    QVERIFY(v.focusIndicatorColor == ColorRole::Secondary);
    QCOMPARE(v.focusIndicatorOffset, 2.0);
    QCOMPARE(v.focusIndicatorThickness, 3.0);

    // Only the state layer moves with state in this family: no per-state icon
    // or label rows are published at all.
    QVERIFY(v.coloursFor(false).iconFor(MdNavigationItemState::Hovered).role
            == v.coloursFor(false).iconFor(MdNavigationItemState::Enabled).role);
}

void TestMd3Navigation::flexibleDefaultsAreItsOwn()
{
    // The resolve's flexible branch cannot inherit the struct's defaults,
    // because those defaults are the *baseline's* numbers — inheriting them
    // would silently turn the short family into the tall one. This is the
    // regression the two-families-in-one-export confusion produces, so it is
    // pinned on its own rather than as a side effect of the table above.
    const MdNavigationBarTokens tokens = MdNavigationBarTokens::resolve();
    const MdNavigationBarVariantTokens &baseline =
        tokens.forVariant(MdNavigationBarVariant::Baseline);
    const MdNavigationBarVariantTokens &flexible =
        tokens.forVariant(MdNavigationBarVariant::Flexible);

    QVERIFY(flexible.containerHeight != baseline.containerHeight);
    QVERIFY(flexible.activeIndicatorWidth != baseline.activeIndicatorWidth);
    QVERIFY(flexible.containerBetweenSpace != baseline.containerBetweenSpace);
    QCOMPARE(flexible.containerHeight, 64.0);
    QCOMPARE(flexible.activeIndicatorWidth, 56.0);
    QCOMPARE(flexible.containerBetweenSpace, 6.0);
}

void TestMd3Navigation::disabledPairIsDerived()
{
    // Neither family publishes a disabled row of its own. Compose writes the
    // rule in code — the unselected colour at the system's `DisabledAlpha`
    // 0.38 — and that is what the export's silence means.
    const MdNavigationBarTokens tokens = MdNavigationBarTokens::resolve();
    for (const MdNavigationBarVariant variant :
         {MdNavigationBarVariant::Baseline, MdNavigationBarVariant::Flexible}) {
        const MdNavigationBarVariantTokens &v = tokens.forVariant(variant);
        const MdNavigationColourSlot &selected =
            v.coloursFor(true).iconFor(MdNavigationItemState::Disabled);
        const MdNavigationColourSlot &unselected =
            v.coloursFor(false).iconFor(MdNavigationItemState::Disabled);
        QVERIFY(selected.role == ColorRole::OnSurfaceVariant);
        QVERIFY(unselected.role == ColorRole::OnSurfaceVariant);
        QCOMPARE(selected.opacity, 0.38);
        QCOMPARE(unselected.opacity, 0.38);
        QCOMPARE(v.coloursFor(true).labelFor(MdNavigationItemState::Disabled).opacity, 0.38);

        // And no state layer: a disabled item paints nothing interactive.
        QVERIFY(!v.coloursFor(true).stateLayerFor(MdNavigationItemState::Disabled).isPresent());
        QVERIFY(!v.coloursFor(false).stateLayerFor(MdNavigationItemState::Disabled).isPresent());
    }
}

void TestMd3Navigation::tokenOverridesReachTheFamilies()
{
    MdComponentTokens &global = MdComponentTokens::global();

    // Each family reads its own component namespace: `navigation-bar` for the
    // baseline, `nav-bar` (+ the item files) for the flexible one.
    global.setValue(QStringLiteral("md.comp.navigation-bar.container.height"),
                    QStringLiteral("96"));
    QCOMPARE(MdNavigationBarTokens::resolve()
                 .forVariant(MdNavigationBarVariant::Baseline)
                 .containerHeight,
             96.0);

    global.setValue(QStringLiteral("md.comp.nav-bar.container.height"), QStringLiteral("72"));
    QCOMPARE(MdNavigationBarTokens::resolve()
                 .forVariant(MdNavigationBarVariant::Flexible)
                 .containerHeight,
             72.0);

    // The flexible indicator's width is the *item-vertical* row, and the
    // pill's padding is derived from it rather than stored.
    global.setValue(QStringLiteral("md.comp.nav-bar-item-vertical.active-indicator.width"),
                    QStringLiteral("48"));
    const MdNavigationBarVariantTokens &flexible =
        MdNavigationBarTokens::resolve().forVariant(MdNavigationBarVariant::Flexible);
    QCOMPARE(flexible.activeIndicatorWidth, 48.0);
    QCOMPARE(flexible.indicatorHorizontalPadding(), 12.0);

    global.setValue(QStringLiteral("md.comp.nav-bar-item-horizontal.active-indicator.height"),
                    QStringLiteral("44"));
    QCOMPARE(MdNavigationBarTokens::resolve()
                 .forVariant(MdNavigationBarVariant::Flexible)
                 .horizontalIndicatorHeight,
             44.0);

    // A typo falls back to the published value instead of being coerced.
    global.setValue(QStringLiteral("md.comp.navigation-bar.active-indicator.shape"),
                    QStringLiteral("corner-banana"));
    QVERIFY(MdNavigationBarTokens::resolve()
                .forVariant(MdNavigationBarVariant::Baseline)
                .indicatorShape
            == ShapeCorner::Full);

    // The override reaches a live widget, not just the free function. A bar
    // resolves its tokens when it is built, so this half needs a fresh widget
    // with the override already in place — the order
    // `tokenOverridesReachTheLayout` established.
    MdNavigationBar bar;
    addItems(bar, 2);
    QCOMPARE(bar.tokens().forVariant(MdNavigationBarVariant::Baseline).containerHeight, 96.0);
    QCOMPARE(bar.sizeHint().height(), 96);
}

// ---------------------------------------------------------------------------
// Bar
// ---------------------------------------------------------------------------

void TestMd3Navigation::equalWeightDividesTheBand()
{
    MdNavigationBar bar;
    addItems(bar, 4);
    bar.resize(360, 80);

    const MdNavigationBarStyle::Layout layout = MdNavigationBarStyle::layoutFor(bar, bar.tokens());
    QCOMPARE(layout.container, QRectF(0, 0, 360, 80));

    // `Box(Modifier.weight(1f))` per item, with the baseline's hard-coded
    // `spacedBy(8.dp)` between them: four slots out of 360 - 3 * 8 = 336.
    QCOMPARE(layout.itemBoxes.size(), 4);
    QCOMPARE(layout.itemBoxes.at(0), QRectF(0, 0, 84, 80));
    QCOMPARE(layout.itemBoxes.at(1), QRectF(92, 0, 84, 80));
    QCOMPARE(layout.itemBoxes.at(2), QRectF(184, 0, 84, 80));
    QCOMPARE(layout.itemBoxes.at(3), QRectF(276, 0, 84, 80));

    // Every box is the full container height: the bar decides the row, and the
    // item centres its own content inside whatever it is given.
    QCOMPARE(layout.itemBoxes.at(2).height(), 80.0);
}

void TestMd3Navigation::centeredShrinksTheBand()
{
    MdNavigationBar bar;
    addItems(bar, 3);
    bar.resize(360, 80);
    bar.setArrangement(MdNavigationBarArrangement::Centered);

    // Compose's `((100 - 10 * (itemsCount + 3)) / 2) / 100`: 20 % a side at
    // three items. The run takes 216 of the 360, and the items still share it
    // equally — (216 - 2 * 8) / 3 = 66.67 — so the two arrangements differ only
    // in the band they divide, not in how they divide it.
    MdNavigationBarStyle::Layout layout = MdNavigationBarStyle::layoutFor(bar, bar.tokens());
    QVERIFY(qAbs(layout.itemBoxes.first().left() - 72.0) <= 0.01);
    QVERIFY(qAbs(layout.itemBoxes.last().right() - 288.0) <= 0.01);
    QVERIFY(qAbs(layout.itemBoxes.at(1).width() - (216.0 - 16.0) / 3.0) <= 0.01);

    // Five items: 10 % a side, 288 of 360.
    MdNavigationBar five;
    addItems(five, 5);
    five.resize(360, 80);
    five.setArrangement(MdNavigationBarArrangement::Centered);
    layout = MdNavigationBarStyle::layoutFor(five, five.tokens());
    QVERIFY(qAbs(layout.itemBoxes.first().left() - 36.0) <= 0.01);
    QVERIFY(qAbs(layout.itemBoxes.last().right() - 324.0) <= 0.01);

    // From seven items on there is no inset left — clamped, not negative.
    MdNavigationBar seven;
    addItems(seven, 7);
    seven.resize(360, 80);
    seven.setArrangement(MdNavigationBarArrangement::Centered);
    layout = MdNavigationBarStyle::layoutFor(seven, seven.tokens());
    QCOMPARE(layout.itemBoxes.first().left(), 0.0);
    QCOMPARE(layout.itemBoxes.last().right(), 360.0);
}

void TestMd3Navigation::betweenSpaceIsTheBehaviourRow()
{
    MdNavigationBar bar;
    addItems(bar, 2);
    bar.resize(360, 80);

    // The export publishes `item.between-space: 0px`; Compose's baseline
    // hard-codes `Arrangement.spacedBy(8.dp)` and its `Centered` arrangement
    // offsets by a percentage instead. A gap between items is behaviour, so
    // Compose's 8 wins and the export's 0 is recorded in docs/porting-todo.md.
    QCOMPARE(bar.tokens().itemBetweenSpace, 8.0);

    const MdNavigationBarStyle::Layout layout = MdNavigationBarStyle::layoutFor(bar, bar.tokens());
    QCOMPARE(layout.itemBoxes.at(0), QRectF(0, 0, 176, 80));
    QCOMPARE(layout.itemBoxes.at(1), QRectF(184, 0, 176, 80));
}

void TestMd3Navigation::selectionIsExclusiveAndMirrored()
{
    MdNavigationBar bar;
    addItems(bar, 3);

    // The first item to arrive is the one: a navigation bar shows a
    // destination as selected rather than publishing an unselected state.
    QCOMPARE(bar.currentIndex(), 0);
    QVERIFY(bar.itemAt(0)->isSelected());
    QVERIFY(!bar.itemAt(1)->isSelected());

    int changed = -2;
    connect(&bar, &MdNavigationBar::currentIndexChanged, this, [&changed](int index) {
        changed = index;
    });

    bar.setCurrentIndex(2);
    QCOMPARE(bar.currentIndex(), 2);
    QCOMPARE(changed, 2);
    QVERIFY(bar.itemAt(2)->isSelected());
    QVERIFY(!bar.itemAt(0)->isSelected());
    QVERIFY(!bar.itemAt(1)->isSelected());

    // An item reporting its own selection moves the bar — clicking a
    // destination goes through the same `selectedChanged` path.
    bar.itemAt(0)->setSelected(true);
    QCOMPARE(bar.currentIndex(), 0);
    QVERIFY(bar.itemAt(2)->isSelected() == false);

    // And a selection is a mirror, not an independent flag: setting the bar
    // unselects the rest through the guarded path.
    bar.setCurrentIndex(1);
    QVERIFY(!bar.itemAt(0)->isSelected());
    QVERIFY(bar.itemAt(1)->isSelected());
}

void TestMd3Navigation::itemsInheritTheVariant()
{
    MdNavigationBar bar;
    addItems(bar, 2);

    QVERIFY(bar.itemAt(0)->variant() == MdNavigationBarVariant::Baseline);
    QCOMPARE(bar.itemAt(0)->variantTokens().containerHeight, 80.0);

    // The family is pushed, not re-resolved: an item that resolved its own
    // table would keep the old numbers after `setVariant`.
    bar.setVariant(MdNavigationBarVariant::Flexible);
    QVERIFY(bar.itemAt(0)->variant() == MdNavigationBarVariant::Flexible);
    QVERIFY(bar.itemAt(1)->variant() == MdNavigationBarVariant::Flexible);
    QCOMPARE(bar.itemAt(0)->variantTokens().containerHeight, 64.0);
    QCOMPARE(bar.itemAt(0)->variantTokens().activeIndicatorWidth, 56.0);
    QCOMPARE(bar.itemAt(1)->variantTokens().containerHeight, 64.0);
}

void TestMd3Navigation::itemsFillTheirSlot()
{
    MdNavigationBar bar;
    addItems(bar, 4);
    // Qt defers a hidden widget's resize to show() — only `WA_PendingResizeEvent`
    // is set — so the relayout is exercised from a shown state, which is also
    // how a real bar lives.
    bar.show();
    bar.resize(360, 80);

    // The item is its own container *and* fills the slot it is given, so the
    // placement is `resizedGeometryOn` — the top-left rule the buttons follow
    // would park a 64 x 52 item in the corner of its 84 x 80 box.
    QCOMPARE(bar.itemAt(0)->geometry(), QRect(0, 0, 84, 80));
    QCOMPARE(bar.itemAt(3)->geometry(), QRect(276, 0, 84, 80));

    // The centring is then the item's own arithmetic: the pill is centred
    // horizontally, and the content band (pill top to label bottom) is
    // centred vertically — a sum that holds whatever the label's height
    // turns out to be on this platform.
    const auto boxes = bar.itemAt(0)->boxes();
    QCOMPARE(boxes.indicatorRipple.center().x(), 42.0);
    QCOMPARE(boxes.indicatorRipple.top() + boxes.label.bottom(), 80.0);
}

void TestMd3Navigation::resizeReflowsTheItems()
{
    MdNavigationBar bar;
    addItems(bar, 4);
    bar.show();
    bar.resize(360, 80);
    QCOMPARE(bar.itemAt(0)->geometry(), QRect(0, 0, 84, 80));

    // The bar is `Expanding` horizontally, so a parent resize is its most
    // common event; without the relayout the items would keep the old pitch.
    bar.resize(200, 80);
    const MdNavigationBarStyle::Layout layout = MdNavigationBarStyle::layoutFor(bar, bar.tokens());
    QCOMPARE(layout.itemBoxes.at(0), QRectF(0, 0, 44, 80));
    QCOMPARE(layout.itemBoxes.at(3), QRectF(156, 0, 44, 80));
    QCOMPARE(bar.itemAt(0)->geometry(), QRect(0, 0, 44, 80));
    QCOMPARE(bar.itemAt(3)->geometry(), QRect(156, 0, 44, 80));
}

void TestMd3Navigation::barSizeHintFollowsTheVariant()
{
    MdNavigationBar bar;
    addItems(bar, 2);

    // Two 64 px items and one 8 px gap, 80 px tall.
    QCOMPARE(bar.sizeHint(), QSize(136, 80));
    QCOMPARE(bar.minimumSizeHint(), QSize(0, 80));

    bar.setVariant(MdNavigationBarVariant::Flexible);
    // Two 56 px items and one 8 px gap, 64 px tall.
    QCOMPARE(bar.sizeHint(), QSize(120, 64));
    QCOMPARE(bar.minimumSizeHint(), QSize(0, 64));
}

void TestMd3Navigation::barPaintsItsContainer()
{
    MdNavigationBar bar;
    addItems(bar, 2);
    // Same deferred-resize rule as `itemsFillTheirSlot`: shown first, so the
    // sampled geometry is the 360 x 80 one.
    bar.show();
    bar.resize(360, 80);

    const QColor container = MdTheme::instance().color(ColorRole::SurfaceContainer);
    // The gap between the two items shows the container.
    QCOMPARE(pixelColorAt(bar, QPointF(180, 40)), container);
    // `container.shape` is corner-none, so even the corner is the container's
    // own.
    QCOMPARE(pixelColorAt(bar, QPointF(0, 0)), container);

    // The selected item's pill: `secondary-container`, drawn behind the icon.
    // The pill's spring runs on a timer, and this test has been synchronous so
    // far — settle it before sampling. The sample is taken on the *second*
    // item: Qt 6 delivers a synthetic enter to the widget under the cursor on
    // show (Qt 5's offscreen plugin does not), so the first item sits in the
    // Hovered state and its pill carries an 8 % state layer the sample must
    // not depend on.
    bar.setCurrentIndex(1);
    QVERIFY(settleIndicator(*bar.itemAt(1), 1.0));
    QCOMPARE(pixelColorAt(bar, QPointF(250, 30)),
             MdTheme::instance().color(ColorRole::SecondaryContainer));
}

// ---------------------------------------------------------------------------
// Item
// ---------------------------------------------------------------------------

void TestMd3Navigation::topItemPillGeometry()
{
    MdNavigationBarItem item(QStringLiteral("A"), QStringLiteral("home"));
    item.resize(80, 80);

    const auto boxes = item.boxes();
    QVERIFY(boxes.iconAboveLabel);

    // The pill is the icon plus the derived paddings: 24 + 2 * 20, 24 + 2 * 4.
    QCOMPARE(boxes.indicatorRipple.width(), 64.0);
    QCOMPARE(boxes.indicatorRipple.height(), 32.0);
    QCOMPARE(boxes.indicatorRipple.center().x(), 40.0);
    QCOMPARE(boxes.icon.width(), 24.0);
    QCOMPARE(boxes.icon.height(), 24.0);

    // Unselected: the pill is away, and the label shows because
    // `alwaysShowLabel` defaults to true.
    QCOMPARE(item.indicatorProgress(), 0.0);
    QCOMPARE(boxes.indicator.width(), 0.0);
    QVERIFY(boxes.labelVisible);

    // 4 px pill padding + 4 px icon-label space between the two.
    QCOMPARE(boxes.label.top() - boxes.icon.bottom(), 8.0);
}

void TestMd3Navigation::startItemPillGeometry()
{
    MdNavigationBarItem item(QStringLiteral("A"), QStringLiteral("home"));
    item.setIconPosition(MdNavigationItemIconPosition::Start);
    item.resize(120, 64);

    const auto boxes = item.boxes();
    QVERIFY(!boxes.iconAboveLabel);

    // The pill wraps its content, so its width *is* the content: 16 + icon +
    // 4 + label + 16, and the horizontal item's 40 px height.
    QCOMPARE(boxes.indicatorRipple.height(), 40.0);
    QCOMPARE(boxes.indicatorRipple.width(),
             16.0 + 24.0 + 4.0 + boxes.label.width() + 16.0);
    QCOMPARE(item.sizeHint().height(), 40);
    QCOMPARE(boxes.icon.top(), 8.0);
    QCOMPARE(boxes.label.left() - boxes.icon.right(), 4.0);

    // The label rides inside the pill, so it is shown whether or not the item
    // is selected — there is no "hide the label" mode in this position.
    QVERIFY(boxes.labelVisible);
    QCOMPARE(boxes.labelOpacity, 1.0);
}

void TestMd3Navigation::indicatorAnimatesWidthOnly()
{
    MdNavigationBarItem item(QStringLiteral("A"), QStringLiteral("home"));
    item.resize(80, 80);

    item.setSelected(true);
    QVERIFY(settleIndicator(item, 1.0));

    // Fully open, the pill is exactly its full-size rect — same width, same
    // origin, and the height never moved: the pill grows out of the icon in
    // one axis only.
    const auto boxes = item.boxes();
    QCOMPARE(boxes.indicator, boxes.indicatorRipple);
    QCOMPARE(boxes.indicator.height(), 32.0);

    item.setSelected(false);
    QVERIFY(settleIndicator(item, 0.0));
    QCOMPARE(item.boxes().indicator.width(), 0.0);
}

void TestMd3Navigation::labelFollowsTheSelectionWhenNotAlwaysShown()
{
    MdNavigationBarItem item(QStringLiteral("A"), QStringLiteral("home"));
    item.setAlwaysShowLabel(false);
    item.resize(80, 80);

    // At rest the icon sits in the middle of the item and there is no label.
    const auto rest = item.boxes();
    QVERIFY(!rest.labelVisible);
    QCOMPARE(rest.icon.center().y(), 40.0);

    item.setSelected(true);
    QVERIFY(settleIndicator(item, 1.0));

    // The icon rose to leave room for the label, the label faded in at full
    // strength, and the icon-label spacing survived the ride.
    const auto open = item.boxes();
    QVERIFY(open.labelVisible);
    QCOMPARE(open.labelOpacity, 1.0);
    QVERIFY(open.icon.center().y() < 40.0);
    QCOMPARE(open.label.top() - open.icon.bottom(), 8.0);
}

void TestMd3Navigation::selectedLabelIsTheProminentCut()
{
    // `label-medium-weight-prominent` is the emphasized cut of the same size:
    // 500 -> 700, no size change.
    MdNavigationBarItem item(QStringLiteral("A"), QStringLiteral("home"));
    item.resize(80, 80);
    const QFont resting = item.labelFont();
    item.setSelected(true);
    const QFont selected = item.labelFont();
    QVERIFY(selected.weight() > resting.weight());
    QCOMPARE(selected.pixelSize(), resting.pixelSize());
}

void TestMd3Navigation::disabledWinsOverSelected()
{
    MdNavigationBarItem item(QStringLiteral("A"), QStringLiteral("home"));
    item.resize(80, 80);
    item.setSelected(true);
    item.setEnabled(false);

    // Disabled first, matching Compose's `!enabled -> ... -> selected`
    // when-clauses: a disabled selected item paints the disabled colours, not
    // the selected ones.
    QVERIFY(MdNavigationBarItemStyle::stateFor(item) == MdNavigationItemState::Disabled);

    const MdNavigationBarVariantTokens &tokens = item.variantTokens();
    const MdNavigationColourSlot &icon =
        tokens.coloursFor(true).iconFor(MdNavigationItemState::Disabled);
    QVERIFY(icon.role == ColorRole::OnSurfaceVariant);
    QCOMPARE(icon.opacity, 0.38);
}

// ---------------------------------------------------------------------------
// Rail
// ---------------------------------------------------------------------------

void TestMd3Navigation::railTokenTable()
{
    const MdNavigationRailTokens tokens = MdNavigationRailTokens::resolve();

    // --- baseline: md.comp.navigation-rail.* --------------------------------
    const MdNavigationRailVariantTokens &base =
        tokens.forVariant(MdNavigationRailVariant::Baseline);
    QCOMPARE(base.containerWidth, 80.0);
    QCOMPARE(base.narrowContainerWidth, 80.0);
    // No expanded rows at all: the baseline rail does not expand.
    QCOMPARE(base.expandedWidthMinimum, 0.0);
    QCOMPARE(base.expandedWidthMaximum, 0.0);
    QVERIFY(!base.supportsExpanded());

    // The three spacing rows the baseline family does not publish and Compose's
    // `NavigationRail.kt` hard-codes.
    QCOMPARE(base.containerVerticalPadding, 4.0);
    QCOMPARE(base.itemVerticalSpace, 4.0);
    QCOMPARE(base.headerSpace, 8.0);

    QVERIFY(base.containerColor == ColorRole::Surface);
    QVERIFY(base.containerElevation == ElevationLevel::Level0);
    QVERIFY(base.containerShape == ShapeCorner::None);

    // The item rows, carried on the shared item's tokens. The no-label pill is
    // this family's own 56 x 56 square.
    QCOMPARE(base.item.activeIndicatorWidth, 56.0);
    QCOMPARE(base.item.activeIndicatorHeight, 32.0);
    QCOMPARE(base.item.noLabelIndicatorHeight, 56.0);
    QCOMPARE(base.item.containerBetweenSpace, 4.0);
    QVERIFY(base.item.labelTextType == TypeStyle::LabelMedium);
    // The colour tables agree with the baseline bar's row for row.
    QVERIFY(base.item.coloursFor(true).labelFor(MdNavigationItemState::Enabled).role
            == ColorRole::OnSurface);
    QVERIFY(base.item.coloursFor(false).iconFor(MdNavigationItemState::Enabled).role
            == ColorRole::OnSurfaceVariant);

    // --- flexible: nav-rail-collapsed / -expanded / nav-rail / items ---------
    const MdNavigationRailVariantTokens &flex =
        tokens.forVariant(MdNavigationRailVariant::Flexible);
    QCOMPARE(flex.containerWidth, 96.0);
    QCOMPARE(flex.narrowContainerWidth, 80.0);
    QCOMPARE(flex.expandedWidthMinimum, 220.0);
    QCOMPARE(flex.expandedWidthMaximum, 360.0);
    QVERIFY(flex.supportsExpanded());

    QCOMPARE(flex.containerTopSpace, 44.0);
    QCOMPARE(flex.itemVerticalSpace, 4.0);
    QCOMPARE(flex.expandedBetweenItemSpace, 0.0);
    // The wide rail's header gap is the *baseline item* family's
    // `header-space-minimum` — Compose reads across files.
    QCOMPARE(flex.headerSpace, 40.0);
    // Compose's hard-coded `WNRItemHorizontalPadding`; numerically the
    // expanded family's `vertical-trailing-space`, from a different source.
    QCOMPARE(flex.expandedItemPadding, 20.0);

    // The modal rows — the drawer's replacement.
    QVERIFY(flex.modalContainerColor == ColorRole::SurfaceContainer);
    QVERIFY(flex.modalContainerElevation == ElevationLevel::Level2);
    QVERIFY(flex.modalContainerShape == ShapeCorner::Large);

    // The item rows. Collapsed: the vertical item, label-medium. Expanded: the
    // horizontal item, 56 tall, icon-label 8 where the vertical item's is 4,
    // label-large.
    QCOMPARE(flex.item.activeIndicatorWidth, 56.0);
    QCOMPARE(flex.item.activeIndicatorHeight, 32.0);
    QCOMPARE(flex.item.containerBetweenSpace, 6.0);
    QCOMPARE(flex.item.indicatorIconLabelSpace, 4.0);
    QCOMPARE(flex.item.horizontalIndicatorHeight, 56.0);
    QCOMPARE(flex.item.horizontalIconLabelSpace, 8.0);
    QCOMPARE(flex.item.horizontalIndicatorLeadingSpace, 16.0);
    QCOMPARE(flex.item.horizontalIndicatorTrailingSpace, 16.0);
    QVERIFY(flex.item.horizontalLabelTextType == TypeStyle::LabelLarge);
    // The flexible rail's colour table agrees with the flexible bar's.
    QVERIFY(flex.item.coloursFor(true).labelFor(MdNavigationItemState::Enabled).role
            == ColorRole::Secondary);
    QVERIFY(flex.item.coloursFor(true).indicator.role == ColorRole::SecondaryContainer);
    QVERIFY(flex.item.coloursFor(false).stateLayerFor(MdNavigationItemState::Hovered).role
            == ColorRole::OnSecondaryContainer);
}

void TestMd3Navigation::railCollapsedGeometry()
{
    MdNavigationRail rail;
    addRailItems(rail, 2);
    rail.show();
    rail.resize(80, 600);

    // The baseline rail's item is the shared item with `alwaysShowLabel` off.
    // The item still *reserves* its label's height — that is what the shared
    // item measures, whatever the platform's metrics turn it into — so the
    // assertions are relationships, not absolute heights. Items start after
    // the hard-coded 4 px container padding and are 4 apart.
    const int itemHeight = rail.itemAt(0)->geometry().height();
    QVERIFY(itemHeight >= 32 + 8);
    QCOMPARE(rail.itemAt(0)->geometry(), QRect(0, 4, 80, itemHeight));
    QCOMPARE(rail.itemAt(1)->geometry().top(), 4 + itemHeight + 4);

    // The pill around the labelled item: 56 x 32 (the no-label square is a
    // different item, tested below), centred horizontally.
    const auto boxes = rail.itemAt(0)->boxes();
    QCOMPARE(boxes.indicatorRipple.width(), 56.0);
    QCOMPARE(boxes.indicatorRipple.height(), 32.0);
    QCOMPARE(boxes.indicatorRipple.center().x(), 40.0);

    // The label-less item is where the baseline family's
    // `no-label-active-indicator-height` shows: a 56 x 56 square pill, and an
    // item 56 + 2 * 4 tall.
    MdNavigationBarItem bare;
    bare.setIconName(QStringLiteral("home"));
    bare.setVariantTokens(
        MdNavigationRailTokens::resolve().forVariant(MdNavigationRailVariant::Baseline).item);
    bare.setAlwaysShowLabel(false);
    bare.resize(80, 64);
    const auto bareBoxes = bare.boxes();
    QCOMPARE(bareBoxes.indicatorRipple.width(), 56.0);
    QCOMPARE(bareBoxes.indicatorRipple.height(), 56.0);
    QCOMPARE(bare.sizeHint().height(), 64);
}

void TestMd3Navigation::railHeaderGeometry()
{
    MdNavigationRail rail;
    FixedProbe fab(QSize(56, 56));
    rail.setHeader(&fab);
    addRailItems(rail, 2);
    rail.show();
    rail.resize(80, 600);

    // The header sits at the content's top and the items follow after the
    // hard-coded 8 px: 4 + 56 + 8. Item heights keep their label reserved, so
    // the second item's position is asserted relative to the first.
    QCOMPARE(rail.itemAt(0)->geometry().top(), 68);
    QCOMPARE(rail.itemAt(1)->geometry().top(),
             68 + rail.itemAt(0)->geometry().height() + 4);
}

void TestMd3Navigation::railExpandedIsRefusedOnTheBaseline()
{
    MdNavigationRail rail;
    addRailItems(rail, 2);
    rail.show();
    rail.resize(80, 600);

    // The baseline family publishes no expanded rows; the state is refused
    // rather than approximated.
    rail.setExpanded(true);
    QVERIFY(!rail.isExpanded());
    QVERIFY(rail.itemAt(0)->iconPosition() == MdNavigationItemIconPosition::Top);
    QCOMPARE(rail.width(), 80);
}

void TestMd3Navigation::railFlexibleExpands()
{
    MdNavigationRail rail;
    addRailItems(rail, 3);
    rail.show();
    rail.resize(96, 600);
    QVERIFY(rail.variant() == MdNavigationRailVariant::Baseline);
    rail.setVariant(MdNavigationRailVariant::Flexible);
    QVERIFY(settleWidth(rail, 96.0));

    // Collapsed first: bare pills, 44 px from the top, 4 apart. The items
    // reserve their labels' heights, so the exact height is the platform's —
    // the pitch is the assertion.
    QVERIFY(rail.itemAt(0)->iconPosition() == MdNavigationItemIconPosition::Top);
    QVERIFY(!rail.itemAt(0)->alwaysShowLabel());
    // The spring has settled, so any geometry event from here on would be a
    // repaint reshuffling the widgets: `layoutFor` reads, it never measures.
    GeometrySpy spy;
    rail.itemAt(0)->installEventFilter(&spy);
    QTest::qWait(50);
    QCOMPARE(spy.count, 0);
    const int collapsedHeight = rail.itemAt(0)->geometry().height();
    QCOMPARE(rail.itemAt(0)->geometry().left(), 0);
    QCOMPARE(rail.itemAt(0)->geometry().width(), 96);
    QCOMPARE(rail.itemAt(1)->geometry().top(), 44 + collapsedHeight + 4);

    rail.setExpanded(true);

    // The item arrangement flips with the state — Compose hands its shared
    // item a different style set when `expanded` moves, no interpolation.
    QVERIFY(rail.itemAt(0)->iconPosition() == MdNavigationItemIconPosition::Start);
    QVERIFY(rail.itemAt(0)->alwaysShowLabel());

    // The width follows the content between the published bounds: a short
    // label's pill plus the 20 px trailing room is under 220, so the minimum
    // wins.
    QVERIFY(settleWidth(rail, 220.0));
    QCOMPARE(rail.width(), 220);

    // Expanded items hug the leading edge at their natural width, and the
    // expanded family's own `between-item-space` (0) replaces the collapsed 4.
    // (QRect::bottom() is inclusive, so the pitch is top-to-top-minus-height.)
    QVERIFY(rail.itemAt(0)->geometry().width() < 220);
    QCOMPARE(rail.itemAt(0)->geometry().left(), 0);
    QCOMPARE(rail.itemAt(1)->geometry().top(),
             rail.itemAt(0)->geometry().top() + rail.itemAt(0)->geometry().height());

    rail.setExpanded(false);
    QVERIFY(settleWidth(rail, 96.0));
    QCOMPARE(rail.width(), 96);
    QVERIFY(rail.itemAt(0)->iconPosition() == MdNavigationItemIconPosition::Top);
}

void TestMd3Navigation::railWidthAnimates()
{
    MdNavigationRail rail;
    addRailItems(rail, 2);
    rail.show();
    rail.resize(96, 600);
    rail.setVariant(MdNavigationRailVariant::Flexible);
    QVERIFY(settleWidth(rail, 96.0));

    // A spring, not a jump: a moment in, the width is strictly between the two
    // states (the first tick cannot have landed yet, and the spring is
    // damped — it has not passed 220 either).
    rail.setExpanded(true);
    QTest::qWait(30);
    QVERIFY(rail.currentWidth() > 96.0);
    QVERIFY(rail.currentWidth() < 220.0);
    QVERIFY(settleWidth(rail, 220.0));
    QCOMPARE(rail.currentWidth(), 220.0);

    // The modal rail arrives on the fast spring — the target is the same, only
    // the scheme differs.
    rail.setModal(true);
    rail.setExpanded(false);
    QVERIFY(settleWidth(rail, 96.0));
    QCOMPARE(rail.currentWidth(), 96.0);
}

void TestMd3Navigation::railModalRows()
{
    MdNavigationRail rail;
    addRailItems(rail, 2);
    rail.show();
    rail.resize(96, 600);
    rail.setVariant(MdNavigationRailVariant::Flexible);
    QVERIFY(settleWidth(rail, 96.0));

    const QColor surface = MdTheme::instance().color(ColorRole::Surface);
    QCOMPARE(pixelColorAt(rail, QPointF(48, 300)), surface);

    // The modal rows are the drawer's replacement: a `surface-container`
    // container.
    rail.setModal(true);
    QCOMPARE(pixelColorAt(rail, QPointF(48, 300)),
             MdTheme::instance().color(ColorRole::SurfaceContainer));
}

void TestMd3Navigation::railNarrowRowIsCarried()
{
    // `nav-rail-collapsed.narrow-container-width` (80) is published and read by
    // nothing in Compose — carried, like every row of its kind, so a theme
    // that sets it still resolves.
    MdComponentTokens::global().setValue(QStringLiteral("md.comp.nav-rail-collapsed.narrow-container-width"),
                                         QStringLiteral("88"));
    QCOMPARE(MdNavigationRailTokens::resolve()
                 .forVariant(MdNavigationRailVariant::Flexible)
                 .narrowContainerWidth,
             88.0);
}

// ---------------------------------------------------------------------------
// Cross-cutting
// ---------------------------------------------------------------------------

void TestMd3Navigation::rtlIsNotMirrored()
{
    // A library-wide gap, not a navigation defect: `MdTheme::isRightToLeft()`
    // reaches only the four button families and nothing here mirrors. The test
    // pins the *current* behaviour so a future RTL pass has to change it
    // deliberately. Recorded in docs/porting-todo.md.
    MdNavigationBar bar;
    addItems(bar, 3);
    bar.resize(360, 80);

    const QList<QRectF> before = MdNavigationBarStyle::layoutFor(bar, bar.tokens()).itemBoxes;
    bar.setLayoutDirection(Qt::RightToLeft);
    const QList<QRectF> after = MdNavigationBarStyle::layoutFor(bar, bar.tokens()).itemBoxes;

    QCOMPARE(after.size(), before.size());
    for (int i = 0; i < before.size(); ++i) {
        QCOMPARE(after.at(i), before.at(i));
    }
}

QTEST_MAIN(TestMd3Navigation)
#include "TestMd3Navigation.moc"
