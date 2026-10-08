// MdButtonGroup — MD3 Expressive button groups.
//
// Four kinds of check, and they are deliberately different in character:
//
//   1. the token table, field by field, against the published
//      `md.comp.button-group.*` sets. A group publishes no colour at all, so
//      spacing and shape are the *whole* of what it contributes and a wrong
//      `between-space` is the entire component being wrong.
//   2. the geometry, measured on the real widgets. A button group is an
//      invisible container, so the only thing it can get wrong that anybody
//      sees is where the buttons ended up and what corner each of them shows.
//   3. the selection model — four modes that differ from each other by exactly
//      one behaviour each, which is precisely the kind of difference that
//      silently collapses into "they all work like single-select".
//   4. a render smoke check, because a group that lays its items out off-screen
//      still passes 1, 2 and 3.

#include "TestMd3Common.h"

#include "core/MdButtonTokens.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTokens.h"
#include "styles/MdButtonGroupStyle.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdButton.h"
#include "widgets/MdButtonGroup.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

struct OffscreenDefault
{
    OffscreenDefault()
    {
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
        }
    }
};

// NOLINTNEXTLINE(cert-err58-cpp) — qputenv cannot throw.
const OffscreenDefault g_offscreenDefault;

bool near(qreal a, qreal b, qreal tolerance = 0.001)
{
    return qAbs(a - b) <= tolerance;
}

/// The item's painted container, in *group* coordinates.
///
/// MdButton::containerRect() is widget-local and the group positions each item
/// itself, so the item's own offset has to be added back before two neighbours
/// can be compared. Every geometric claim below is about the containers, not
/// about the widget rects — the widget rects deliberately overlap, because each
/// one carries the transparent margin the outward focus indicator paints into.
QRectF containerIn(const MdButton *item)
{
    return item->containerRect().translated(item->pos());
}

/// An icon-only item, which measures to exactly
/// `leading-space + icon-size + trailing-space` — no font metrics involved, so
/// the expected geometry is arithmetic rather than a copied constant.
const char *const kIcon = "add";

qreal iconOnlyExtent(ButtonSize size)
{
    const MdButtonTokens tokens =
        MdButtonTokens::resolve(ButtonVariant::Filled, size, ButtonShape::Round);
    return tokens.leadingSpace + tokens.iconSize + tokens.trailingSpace;
}

MdButtonGroup *makeGroup(QWidget *parent,
                         ButtonGroupVariant variant,
                         ButtonSize size,
                         int count)
{
    auto *group = new MdButtonGroup(parent);
    group->setVariant(variant);
    group->setGroupSize(size);
    group->setSelectionMode(ButtonGroupSelection::None);
    for (int i = 0; i < count; ++i) {
        group->addItem(QString(), QString::fromUtf8(kIcon));
    }
    group->resize(group->sizeHint());
    return group;
}

/// Non-background pixel count. The reference is the widget's own top-left
/// corner, which is inside the first item's transparent focus margin.
int paintedPixels(const QImage &image)
{
    return mdtest::paintedPixelCount(image, image.pixel(0, 0));
}

QImage renderGroup(MdButtonGroup &group)
{
    group.show();
    // Deliberately not asserted: the offscreen platform plugin can report a
    // window as unexposed and still paint it correctly, and the check below is
    // about pixels rather than about the platform's opinion.
    (void)QTest::qWaitForWindowExposed(&group);
    const QPixmap pixmap = group.grab();
    group.hide();
    return pixmap.toImage();
}

} // namespace

class TestMd3ButtonGroup : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    void tokenTableMatchesThePublishedSets();
    void connectedPublishesNoWidthMultiplierAndNoSpring();
    void theExportWinsWhereItDisagreesWithTheSpecsPage();
    void componentOverridesReachTheTokenTable();
    void propertiesRoundTripThroughTheMetaObject();
    void signalsFireOnlyOnChange();
    void itemBookkeepingKeepsSelectionIndicesHonest();

    void noSelectionModeLeavesTheItemsUncheckable();
    void singleSelectReplacesAndCanBeCleared();
    void requiredSelectionCannotBeCleared();
    void multipleTogglesInependently();
    void clickingReportsTheItemAndTheSelection();
    void arrowKeysWalkTheGroupAndSelectInExclusiveModes();
    void arrowKeysDoNotRewriteTheSelectionInAToolbar();

    void betweenSpaceIsKeptBetweenContainers();
    void aVerticalGroupStacksTheSameWay();
    void connectedKeepsTheTwoPixelSpacingAtEverySize();
    void theOuterCornerGoesToTheEndsAndTheInnerToTheMiddle();
    void selectingAStandardItemSwapsItsShape();
    void pressingGrowsTheItemAboutItsCentre();
    void connectedPressDoesNotMoveTheNeighbours();
    void connectedXSmallAndSmallKeepTheFortyEightTargetArea();

    void theGroupInstallsNoPaintFilter();
    void iconOnlyItemsGetAnAccessibleName();
    void disablingTheGroupDisablesTheItems();
    void everyFormRendersSomething();
    void lightAndDarkPaintDifferentItems();
    void rightToLeftMirrorsTheRow();
};

void TestMd3ButtonGroup::cleanup()
{
    MdComponentTokens::global().clear();
    MdTheme::instance().setDirection(Qt::LeftToRight);
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

// ---------------------------------------------------------------------------
// 1. the token table
// ---------------------------------------------------------------------------

void TestMd3ButtonGroup::tokenTableMatchesThePublishedSets()
{
    struct Row
    {
        ButtonSize size;
        qreal height;
        qreal standardBetweenSpace;
        qreal pressedMultiplier;
        ShapeCorner inner;
        ShapeCorner pressedInner;
        qreal minimumExtent;
    };

    // md.comp.button-group.{standard,connected}.<size>.*, export 34.0.21.
    const QVector<Row> rows = {
        {ButtonSize::XSmall, 32.0, 18.0, 0.15, ShapeCorner::Small, ShapeCorner::ExtraSmall, 48.0},
        {ButtonSize::Small, 40.0, 12.0, 0.15, ShapeCorner::Small, ShapeCorner::ExtraSmall, 48.0},
        {ButtonSize::Medium, 56.0, 8.0, 0.15, ShapeCorner::Small, ShapeCorner::ExtraSmall, 0.0},
        {ButtonSize::Large, 96.0, 8.0, 0.15, ShapeCorner::Large, ShapeCorner::Medium, 0.0},
        {ButtonSize::XLarge, 136.0, 8.0, 0.15, ShapeCorner::LargeIncreased, ShapeCorner::Large, 0.0},
    };

    for (const Row &row : rows) {
        const QByteArray where = md::buttonSizeName(row.size).toLatin1();

        const MdButtonGroupTokens standard = MdButtonGroupTokens::resolve(
            ButtonGroupVariant::Standard, row.size, ButtonGroupShape::Round);
        QVERIFY2(near(standard.containerHeight, row.height), where.constData());
        QVERIFY2(near(standard.betweenSpace, row.standardBetweenSpace), where.constData());
        QVERIFY2(near(standard.pressedWidthMultiplier, row.pressedMultiplier), where.constData());

        const MdButtonGroupTokens connected = MdButtonGroupTokens::resolve(
            ButtonGroupVariant::Connected, row.size, ButtonGroupShape::Round);
        QVERIFY2(near(connected.containerHeight, row.height), where.constData());
        // "Connected button groups have 2dp of padding at every size."
        QVERIFY2(near(connected.betweenSpace, 2.0), where.constData());
        QVERIFY2(connected.innerCorner == row.inner, where.constData());
        QVERIFY2(connected.pressedInnerCorner == row.pressedInner, where.constData());
        QVERIFY2(near(connected.minimumItemExtent, row.minimumExtent), where.constData());
        // The export publishes the round connected form's outer shape only.
        QVERIFY2(connected.outerCorner == ShapeCorner::Full, where.constData());
    }

    // `selected.inner-corner.corner-size` is the literal 50%, and it is a
    // fraction rather than a shape because 50% of a 136 px group is not 50% of a
    // 32 px one.
    const MdButtonGroupTokens connected = MdButtonGroupTokens::resolve(
        ButtonGroupVariant::Connected, ButtonSize::Medium, ButtonGroupShape::Round);
    QVERIFY(near(connected.selectedInnerCornerFraction, 0.5));
}

void TestMd3ButtonGroup::connectedPublishesNoWidthMultiplierAndNoSpring()
{
    // A connected item's press changes only its own shape, so there is nothing
    // for a width multiplier or a spring to drive. Resolving one would be
    // inventing a token the export does not have.
    for (int i = 0; i < int(ButtonSize::Count); ++i) {
        const MdButtonGroupTokens connected = MdButtonGroupTokens::resolve(
            ButtonGroupVariant::Connected, ButtonSize(i), ButtonGroupShape::Round);
        const QByteArray where = md::buttonSizeName(ButtonSize(i)).toLatin1();

        QVERIFY2(near(connected.pressedWidthMultiplier, 0.0), where.constData());
        QVERIFY2(near(connected.springStiffness, 0.0), where.constData());
        QVERIFY2(near(connected.springDampingRatio, 0.0), where.constData());
    }

    // The standard form's spring is spring-fast-spatial, which is the same pair
    // a button's own press morph uses.
    const MdButtonGroupTokens standard = MdButtonGroupTokens::resolve(
        ButtonGroupVariant::Standard, ButtonSize::Small, ButtonGroupShape::Round);
    QVERIFY(near(standard.springStiffness, 1400.0));
    QVERIFY(near(standard.springDampingRatio, 0.9));
}

void TestMd3ButtonGroup::theExportWinsWhereItDisagreesWithTheSpecsPage()
{
    // Two published sources disagree and the token export wins, per the project
    // rule that numeric values come from the tokens. This test exists so that
    // "fixing" the disagreement by quietly taking the specs page's number fails
    // loudly. See docs/porting-todo.md.
    const MdButtonGroupTokens xsmall = MdButtonGroupTokens::resolve(
        ButtonGroupVariant::Connected, ButtonSize::XSmall, ButtonGroupShape::Round);

    // The export says corner-small (8 px); the specs page's measurement list
    // says 4 px.
    QCOMPARE(xsmall.innerCorner, ShapeCorner::Small);
    QVERIFY(near(MdShape::radius(ShapeCorner::Small), 8.0));
    QVERIFY(!near(MdShape::radius(ShapeCorner::Small), 4.0));

    // The square connected form's outer corner appears only on the specs page —
    // the export publishes `container.shape: corner-full` and nothing else — so
    // it is built from that list and marked as such.
    const MdButtonGroupTokens square = MdButtonGroupTokens::resolve(
        ButtonGroupVariant::Connected, ButtonSize::Medium, ButtonGroupShape::Square);
    QCOMPARE(square.outerCorner, ShapeCorner::Small);
    QVERIFY(near(MdShape::radius(square.outerCorner), 8.0));
    QVERIFY(near(MdShape::radius(ShapeCorner::Full), -1.0));
}

void TestMd3ButtonGroup::componentOverridesReachTheTokenTable()
{
    MdComponentTokens local;
    // The size-qualified key wins over the generic one.
    local.setValue(QStringLiteral("md.comp.button-group.standard.large.container.height"),
                   QStringLiteral("104"));
    local.setValue(QStringLiteral("md.comp.button-group.standard.large.between-space"),
                   QStringLiteral("20px"));
    local.setValue(QStringLiteral("md.comp.button-group.connected.small.inner-corner.corner-size"),
                   QStringLiteral("corner-medium"));

    const MdButtonGroupTokens large = MdButtonGroupTokens::resolve(
        ButtonGroupVariant::Standard, ButtonSize::Large, ButtonGroupShape::Round, &local);
    QVERIFY(near(large.containerHeight, 104.0));
    QVERIFY(near(large.betweenSpace, 20.0));

    // A size the override does not name still reads the published value.
    const MdButtonGroupTokens small = MdButtonGroupTokens::resolve(
        ButtonGroupVariant::Standard, ButtonSize::Small, ButtonGroupShape::Round, &local);
    QVERIFY(near(small.containerHeight, 40.0));
    QVERIFY(near(small.betweenSpace, 12.0));

    const MdButtonGroupTokens connected = MdButtonGroupTokens::resolve(
        ButtonGroupVariant::Connected, ButtonSize::Small, ButtonGroupShape::Round, &local);
    QCOMPARE(connected.innerCorner, ShapeCorner::Medium);

    // A typo'd shape name falls back rather than silently becoming "none",
    // which would turn the whole connected group square-cornered.
    MdComponentTokens typo;
    typo.setValue(QStringLiteral("md.comp.button-group.connected.small.inner-corner.corner-size"),
                  QStringLiteral("rounded"));
    const MdButtonGroupTokens fallback = MdButtonGroupTokens::resolve(
        ButtonGroupVariant::Connected, ButtonSize::Small, ButtonGroupShape::Round, &typo);
    QCOMPARE(fallback.innerCorner, ShapeCorner::Small);

    // The application-wide store is honoured when no instance bag is given.
    MdComponentTokens::global().setValue(
        QStringLiteral("md.comp.button-group.standard.small.between-space"), QStringLiteral("24"));
    const MdButtonGroupTokens globalOnly = MdButtonGroupTokens::resolve(
        ButtonGroupVariant::Standard, ButtonSize::Small, ButtonGroupShape::Round);
    QVERIFY(near(globalOnly.betweenSpace, 24.0));
}

// ---------------------------------------------------------------------------
// 2. the widget contract
// ---------------------------------------------------------------------------

void TestMd3ButtonGroup::propertiesRoundTripThroughTheMetaObject()
{
    MdButtonGroup group;
    const QMetaObject *meta = group.metaObject();

    const auto writeProperty = [&](const char *name, const QVariant &value) {
        const int index = meta->indexOfProperty(name);
        return index >= 0 && meta->property(index).write(&group, value);
    };
    const auto readProperty = [&](const char *name) {
        const int index = meta->indexOfProperty(name);
        return index >= 0 ? meta->property(index).read(&group) : QVariant();
    };

    QVERIFY(writeProperty("variant", QVariant::fromValue(ButtonGroupVariant::Connected)));
    QCOMPARE(readProperty("variant").value<ButtonGroupVariant>(), ButtonGroupVariant::Connected);

    QVERIFY(writeProperty("groupSize", QVariant::fromValue(ButtonSize::XLarge)));
    QCOMPARE(readProperty("groupSize").value<ButtonSize>(), ButtonSize::XLarge);

    QVERIFY(writeProperty("orientation", QVariant::fromValue(ButtonGroupOrientation::Vertical)));
    QCOMPARE(readProperty("orientation").value<ButtonGroupOrientation>(),
             ButtonGroupOrientation::Vertical);

    QVERIFY(writeProperty("selectionMode", QVariant::fromValue(ButtonGroupSelection::Required)));
    QCOMPARE(readProperty("selectionMode").value<ButtonGroupSelection>(),
             ButtonGroupSelection::Required);

    QVERIFY(writeProperty("groupShape", QVariant::fromValue(ButtonShape::Square)));
    QCOMPARE(readProperty("groupShape").value<ButtonShape>(), ButtonShape::Square);

    QVERIFY(writeProperty("itemVariant", QVariant::fromValue(ButtonVariant::Outlined)));
    QCOMPARE(readProperty("itemVariant").value<ButtonVariant>(), ButtonVariant::Outlined);

    QVERIFY(writeProperty("selectedItemVariant", QVariant::fromValue(ButtonVariant::Tonal)));
    QCOMPARE(readProperty("selectedItemVariant").value<ButtonVariant>(), ButtonVariant::Tonal);

    group.addItem(QStringLiteral("A"));
    group.addItem(QStringLiteral("B"));
    QVERIFY(writeProperty("currentIndex", 1));
    QCOMPARE(readProperty("currentIndex").toInt(), 1);

    // Every property must advertise NOTIFY, per the brief's DoD item 3.
    for (const char *name : {"variant", "groupSize", "orientation", "selectionMode", "groupShape",
                             "itemVariant", "selectedItemVariant", "currentIndex"}) {
        const int index = meta->indexOfProperty(name);
        QVERIFY2(index >= 0, name);
        QVERIFY2(meta->property(index).hasNotifySignal(), name);
        QVERIFY2(meta->property(index).read(&group).isValid(), name);
    }

    // The group itself takes no focus: it has no pixels, so a focus indicator on
    // it would be a ring around nothing.
    QCOMPARE(group.focusPolicy(), Qt::NoFocus);
}

void TestMd3ButtonGroup::signalsFireOnlyOnChange()
{
    MdButtonGroup group;
    group.addItem(QStringLiteral("A"));

    QSignalSpy variantSpy(&group, &MdButtonGroup::variantChanged);
    QSignalSpy sizeSpy(&group, &MdButtonGroup::groupSizeChanged);
    QSignalSpy orientationSpy(&group, &MdButtonGroup::orientationChanged);
    QSignalSpy modeSpy(&group, &MdButtonGroup::selectionModeChanged);
    QSignalSpy shapeSpy(&group, &MdButtonGroup::groupShapeChanged);
    QSignalSpy itemSpy(&group, &MdButtonGroup::itemVariantChanged);
    QSignalSpy selectedItemSpy(&group, &MdButtonGroup::selectedItemVariantChanged);
    QSignalSpy indexSpy(&group, &MdButtonGroup::currentIndexChanged);

    group.setVariant(ButtonGroupVariant::Standard);
    group.setGroupSize(ButtonSize::Small);
    group.setOrientation(ButtonGroupOrientation::Horizontal);
    group.setSelectionMode(ButtonGroupSelection::Single);
    group.setGroupShape(ButtonShape::Round);
    group.setItemVariant(ButtonVariant::Filled);
    group.setSelectedItemVariant(ButtonVariant::Tonal);
    QCOMPARE(variantSpy.count(), 0);
    QCOMPARE(sizeSpy.count(), 0);
    QCOMPARE(orientationSpy.count(), 0);
    QCOMPARE(modeSpy.count(), 0);
    QCOMPARE(shapeSpy.count(), 0);
    QCOMPARE(itemSpy.count(), 0);
    QCOMPARE(selectedItemSpy.count(), 0);
    QCOMPARE(indexSpy.count(), 0);

    group.setVariant(ButtonGroupVariant::Connected);
    group.setGroupSize(ButtonSize::Large);
    group.setOrientation(ButtonGroupOrientation::Vertical);
    group.setSelectionMode(ButtonGroupSelection::Multiple);
    group.setGroupShape(ButtonShape::Square);
    group.setItemVariant(ButtonVariant::Outlined);
    group.setSelectedItemVariant(ButtonVariant::Elevated);
    QCOMPARE(variantSpy.count(), 1);
    QCOMPARE(sizeSpy.count(), 1);
    QCOMPARE(orientationSpy.count(), 1);
    QCOMPARE(modeSpy.count(), 1);
    QCOMPARE(shapeSpy.count(), 1);
    QCOMPARE(itemSpy.count(), 1);
    QCOMPARE(selectedItemSpy.count(), 1);

    QCOMPARE(variantSpy.first().at(0).value<ButtonGroupVariant>(), ButtonGroupVariant::Connected);
    QCOMPARE(sizeSpy.first().at(0).value<ButtonSize>(), ButtonSize::Large);
    QCOMPARE(modeSpy.first().at(0).value<ButtonGroupSelection>(), ButtonGroupSelection::Multiple);
    QCOMPARE(shapeSpy.first().at(0).value<ButtonShape>(), ButtonShape::Square);
}

void TestMd3ButtonGroup::itemBookkeepingKeepsSelectionIndicesHonest()
{
    MdButtonGroup group;
    group.setSelectionMode(ButtonGroupSelection::Multiple);

    MdButton *a = group.addItem(QStringLiteral("A"));
    MdButton *b = group.addItem(QStringLiteral("B"));
    MdButton *c = group.addItem(QStringLiteral("C"));
    QCOMPARE(group.count(), 3);
    QCOMPARE(group.itemAt(0), a);
    QCOMPARE(group.itemAt(1), b);
    QCOMPARE(group.itemAt(2), c);
    QCOMPARE(group.indexOf(b), 1);
    QCOMPARE(group.indexOf(nullptr), -1);

    // An item the group does not own is not found, even if it looks the same.
    MdButton stranger(QStringLiteral("D"));
    QCOMPARE(group.indexOf(&stranger), -1);

    group.setSelected(0, true);
    group.setSelected(2, true);
    QCOMPARE(group.selectedIndexes(), QList<int>({0, 2}));

    // Inserting ahead of the selection shifts every stored index.
    group.insertItem(1, QStringLiteral("Inserted"));
    QCOMPARE(group.count(), 4);
    QCOMPARE(group.selectedIndexes(), QList<int>({0, 3}));
    QCOMPARE(group.itemAt(1)->text(), QStringLiteral("Inserted"));
    QCOMPARE(group.currentIndex(), 3);

    // Removing an unselected item shifts the ones after it down.
    group.removeItem(1);
    QCOMPARE(group.count(), 3);
    QCOMPARE(group.selectedIndexes(), QList<int>({0, 2}));
    QCOMPARE(group.currentIndex(), 2);

    // Removing the current item moves the index to a survivor rather than
    // leaving it dangling.
    QSignalSpy indexSpy(&group, &MdButtonGroup::currentIndexChanged);
    group.removeItem(2);
    QCOMPARE(group.count(), 2);
    QCOMPARE(group.selectedIndexes(), QList<int>({0}));
    QCOMPARE(group.currentIndex(), 0);
    QCOMPARE(indexSpy.count(), 1);

    // addButton() takes parenting and refuses a duplicate.
    auto *extra = new MdButton(QStringLiteral("Extra"));
    group.addButton(extra);
    QCOMPARE(group.count(), 3);
    QCOMPARE(extra->parentWidget(), &group);
    group.addButton(extra);
    QCOMPARE(group.count(), 3);

    group.clear();
    QCOMPARE(group.count(), 0);
    QCOMPARE(group.selectedIndexes(), QList<int>());
    QCOMPARE(group.currentIndex(), -1);
}

// ---------------------------------------------------------------------------
// 3. the selection model
// ---------------------------------------------------------------------------

void TestMd3ButtonGroup::noSelectionModeLeavesTheItemsUncheckable()
{
    MdButtonGroup group;
    group.setSelectionMode(ButtonGroupSelection::None);
    MdButton *a = group.addItem(QStringLiteral("A"));
    MdButton *b = group.addItem(QStringLiteral("B"));

    // A group with no selection model is a row of plain actions. A toggle state
    // on them would be a state nothing owns.
    QVERIFY(!a->isCheckable());
    QVERIFY(!b->isCheckable());
    QVERIFY(group.selectedIndexes().isEmpty());
    QCOMPARE(group.currentIndex(), -1);

    QSignalSpy selectedSpy(&group, &MdButtonGroup::itemSelected);
    group.setSelected(0, true);
    group.setCurrentIndex(0);
    group.clearSelection();
    QCOMPARE(selectedSpy.count(), 0);
    QCOMPARE(group.currentIndex(), -1);
}

void TestMd3ButtonGroup::singleSelectReplacesAndCanBeCleared()
{
    MdButtonGroup group;
    group.setSelectionMode(ButtonGroupSelection::Single);
    MdButton *a = group.addItem(QStringLiteral("A"));
    MdButton *b = group.addItem(QStringLiteral("B"));

    QVERIFY(a->isCheckable());
    QVERIFY(!a->isChecked());

    group.setCurrentIndex(0);
    QCOMPARE(group.selectedIndexes(), QList<int>({0}));
    QVERIFY(a->isChecked());
    QVERIFY(!b->isChecked());
    QCOMPARE(group.currentIndex(), 0);

    // Only one, ever: selecting another replaces it rather than adding.
    group.setCurrentIndex(1);
    QCOMPARE(group.selectedIndexes(), QList<int>({1}));
    QVERIFY(!a->isChecked());
    QVERIFY(b->isChecked());

    // ...and it can be cleared, which is what separates it from "required".
    group.clearSelection();
    QVERIFY(group.selectedIndexes().isEmpty());
    QCOMPARE(group.currentIndex(), -1);
    QVERIFY(!b->isChecked());

    // An exclusive move from one item to the next is one change of index, not a
    // collapse to -1 and back.
    group.setCurrentIndex(0);
    QSignalSpy indexSpy(&group, &MdButtonGroup::currentIndexChanged);
    group.setCurrentIndex(1);
    QCOMPARE(indexSpy.count(), 1);
    QCOMPARE(indexSpy.first().at(0).toInt(), 1);
}

void TestMd3ButtonGroup::requiredSelectionCannotBeCleared()
{
    MdButtonGroup group;
    group.setSelectionMode(ButtonGroupSelection::Required);

    // "Selection required" means exactly one, so an empty group is the only
    // state in which nothing is selected — and it does not last.
    group.addItem(QStringLiteral("A"));
    group.addItem(QStringLiteral("B"));
    QCOMPARE(group.selectedIndexes(), QList<int>({0}));
    QCOMPARE(group.currentIndex(), 0);

    group.clearSelection();
    QCOMPARE(group.selectedIndexes(), QList<int>({0}));
    group.setSelected(0, false);
    QCOMPARE(group.selectedIndexes(), QList<int>({0}));
    group.setCurrentIndex(-1);
    QCOMPARE(group.selectedIndexes(), QList<int>({0}));
    QCOMPARE(group.currentIndex(), 0);

    // Selecting another still replaces.
    group.setCurrentIndex(1);
    QCOMPARE(group.selectedIndexes(), QList<int>({1}));
    QCOMPARE(group.currentIndex(), 1);

    // Flipping a multi-select group into required keeps exactly one.
    MdButtonGroup multi;
    multi.setSelectionMode(ButtonGroupSelection::Multiple);
    multi.addItem(QStringLiteral("A"));
    multi.addItem(QStringLiteral("B"));
    multi.setSelected(0, true);
    multi.setSelected(1, true);
    QCOMPARE(multi.selectedIndexes(), QList<int>({0, 1}));
    multi.setSelectionMode(ButtonGroupSelection::Required);
    QCOMPARE(multi.selectedIndexes(), QList<int>({1}));
}

void TestMd3ButtonGroup::multipleTogglesInependently()
{
    MdButtonGroup group;
    group.setSelectionMode(ButtonGroupSelection::Multiple);
    MdButton *a = group.addItem(QStringLiteral("A"));
    MdButton *b = group.addItem(QStringLiteral("B"));
    MdButton *c = group.addItem(QStringLiteral("C"));

    group.setSelected(0, true);
    group.setSelected(2, true);
    QCOMPARE(group.selectedIndexes(), QList<int>({0, 2}));
    // currentIndex() is the item selected most recently, which is what a caller
    // driving a tool bar usually wants.
    QCOMPARE(group.currentIndex(), 2);

    group.setSelected(1, true);
    QCOMPARE(group.selectedIndexes(), QList<int>({0, 1, 2}));

    group.setSelected(1, false);
    QCOMPARE(group.selectedIndexes(), QList<int>({0, 2}));
    QVERIFY(!b->isChecked());
    QVERIFY(a->isChecked());
    QVERIFY(c->isChecked());

    // Deselecting the current one falls back to a survivor rather than -1: the
    // group still has a selection, it just is not that one.
    group.setCurrentIndex(0);
    QCOMPARE(group.currentIndex(), 0);
    group.setSelected(0, false);
    QCOMPARE(group.currentIndex(), 2);

    group.clearSelection();
    QVERIFY(group.selectedIndexes().isEmpty());
    QCOMPARE(group.currentIndex(), -1);
}

void TestMd3ButtonGroup::clickingReportsTheItemAndTheSelection()
{
    MdButtonGroup group;
    group.setSelectionMode(ButtonGroupSelection::Single);
    MdButton *a = group.addItem(QStringLiteral("A"));
    MdButton *b = group.addItem(QStringLiteral("B"));
    group.resize(group.sizeHint());
    group.show();
    QVERIFY(QTest::qWaitForWindowExposed(&group));

    QSignalSpy clickSpy(&group, &MdButtonGroup::itemClicked);
    QSignalSpy selectedSpy(&group, &MdButtonGroup::itemSelected);
    QSignalSpy deselectedSpy(&group, &MdButtonGroup::itemDeselected);

    QTest::mouseClick(a, Qt::LeftButton);
    QCOMPARE(clickSpy.count(), 1);
    QCOMPARE(clickSpy.first().at(0).toInt(), 0);
    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(group.selectedIndexes(), QList<int>({0}));

    QTest::mouseClick(b, Qt::LeftButton);
    QCOMPARE(clickSpy.count(), 2);
    QCOMPARE(clickSpy.last().at(0).toInt(), 1);
    QCOMPARE(deselectedSpy.count(), 1);
    QCOMPARE(deselectedSpy.first().at(0).toInt(), 0);
    QCOMPARE(group.selectedIndexes(), QList<int>({1}));

    // Clicking the selected item clears it — the difference between
    // single-select and selection-required.
    QTest::mouseClick(b, Qt::LeftButton);
    QCOMPARE(group.selectedIndexes(), QList<int>());
    QCOMPARE(deselectedSpy.count(), 2);

    // And the same click in a required group is a no-op.
    group.setSelectionMode(ButtonGroupSelection::Required);
    QTest::mouseClick(b, Qt::LeftButton);
    QCOMPARE(group.selectedIndexes(), QList<int>({1}));
    QVERIFY(b->isChecked());

    group.hide();
}

void TestMd3ButtonGroup::arrowKeysWalkTheGroupAndSelectInExclusiveModes()
{
    MdButtonGroup group;
    group.setOrientation(ButtonGroupOrientation::Horizontal);
    group.setSelectionMode(ButtonGroupSelection::Single);
    for (int i = 0; i < 3; ++i) {
        group.addItem(QStringLiteral("Item"));
    }
    group.resize(group.sizeHint());
    group.show();
    QVERIFY(QTest::qWaitForWindowExposed(&group));

    group.itemAt(0)->setFocus();
    QCOMPARE(group.currentIndex(), 0);

    // Selection follows focus in an exclusive group: it is a radiogroup.
    QTest::keyClick(group.itemAt(0), Qt::Key_Right);
    QCOMPARE(group.currentIndex(), 1);
    QVERIFY(group.itemAt(1)->hasFocus());
    QCOMPARE(group.selectedIndexes(), QList<int>({1}));

    QTest::keyClick(group.itemAt(1), Qt::Key_Right);
    QCOMPARE(group.currentIndex(), 2);

    // ...and a radiogroup is a cycle, so the arrow keys do not dead-end.
    QTest::keyClick(group.itemAt(2), Qt::Key_Right);
    QCOMPARE(group.currentIndex(), 0);

    QTest::keyClick(group.itemAt(0), Qt::Key_Left);
    QCOMPARE(group.currentIndex(), 2);

    QTest::keyClick(group.itemAt(2), Qt::Key_Home);
    QCOMPARE(group.currentIndex(), 0);
    QTest::keyClick(group.itemAt(0), Qt::Key_End);
    QCOMPARE(group.currentIndex(), 2);

    // Up and Down are not this group's axis.
    QTest::keyClick(group.itemAt(2), Qt::Key_Down);
    QCOMPARE(group.currentIndex(), 2);

    group.hide();
}

void TestMd3ButtonGroup::arrowKeysDoNotRewriteTheSelectionInAToolbar()
{
    MdButtonGroup group;
    group.setSelectionMode(ButtonGroupSelection::None);
    group.addItem(QStringLiteral("Cut"));
    group.addItem(QStringLiteral("Copy"));
    group.addItem(QStringLiteral("Paste"));
    group.resize(group.sizeHint());
    group.show();
    QVERIFY(QTest::qWaitForWindowExposed(&group));

    group.itemAt(0)->setFocus();
    QTest::keyClick(group.itemAt(0), Qt::Key_Right);
    QVERIFY(group.itemAt(1)->hasFocus());
    QCOMPARE(group.currentIndex(), -1);
    QVERIFY(group.selectedIndexes().isEmpty());

    // A tool bar is not a cycle: at the end the key escapes rather than wrapping,
    // and focus stays put.
    group.itemAt(2)->setFocus();
    QTest::keyClick(group.itemAt(2), Qt::Key_Right);
    QVERIFY(group.itemAt(2)->hasFocus());

    group.hide();
}

// ---------------------------------------------------------------------------
// 4. geometry
// ---------------------------------------------------------------------------

void TestMd3ButtonGroup::betweenSpaceIsKeptBetweenContainers()
{
    MdButtonGroup group;
    group.setVariant(ButtonGroupVariant::Standard);
    group.setGroupSize(ButtonSize::Small);
    for (int i = 0; i < 3; ++i) {
        group.addItem(QString(), QString::fromUtf8(kIcon));
    }
    group.resize(group.sizeHint());

    // The group's own token, not the button's: `between-space` is what a group
    // is for.
    QVERIFY(near(group.betweenSpace(), 12.0));

    for (int i = 0; i < 3; ++i) {
        const QRectF container = containerIn(group.itemAt(i));
        QVERIFY2(near(container.width(), iconOnlyExtent(ButtonSize::Small), 1.0),
                 qPrintable(QString::number(i)));
        QVERIFY2(near(container.height(), 40.0, 1.0), qPrintable(QString::number(i)));
    }

    // The headline claim: two neighbouring *containers* are exactly
    // `between-space` apart, even though the widget rects overlap by
    // `2 * focusRingInset - between-space` to make room for the focus ring.
    for (int i = 0; i + 1 < 3; ++i) {
        const QRectF left = containerIn(group.itemAt(i));
        const QRectF right = containerIn(group.itemAt(i + 1));
        QVERIFY2(near(right.left() - left.right(), group.betweenSpace(), 1.0),
                 qPrintable(QString::number(i)));
    }

    // And the overlap really is there, because each item reserves the outward
    // focus indicator's margin inside its own rectangle.
    const QRectF first = group.itemAt(0)->geometry();
    const QRectF second = group.itemAt(1)->geometry();
    QVERIFY2(second.left() < first.right(), "item rects should overlap");

    // The reported size is the row plus one multiplier of slack plus both
    // margins, so the pressed item has somewhere to grow into.
    const qreal expected = 3.0 * iconOnlyExtent(ButtonSize::Small) + 2.0 * 12.0
                           + iconOnlyExtent(ButtonSize::Small) * 0.15 + 2.0 * 7.5;
    QVERIFY(near(group.sizeHint().width(), expected, 1.5));
    QVERIFY(near(group.sizeHint().height(), 40.0 + 2.0 * 7.5, 1.5));
}

void TestMd3ButtonGroup::aVerticalGroupStacksTheSameWay()
{
    MdButtonGroup group;
    group.setOrientation(ButtonGroupOrientation::Vertical);
    group.setGroupSize(ButtonSize::Small);
    for (int i = 0; i < 3; ++i) {
        group.addItem(QString(), QString::fromUtf8(kIcon));
    }
    group.resize(group.sizeHint());

    const qreal cross = iconOnlyExtent(ButtonSize::Small);
    for (int i = 0; i < 3; ++i) {
        const QRectF container = containerIn(group.itemAt(i));
        QVERIFY2(near(container.height(), 40.0, 1.0), qPrintable(QString::number(i)));
        QVERIFY2(near(container.width(), cross, 1.0), qPrintable(QString::number(i)));
    }
    for (int i = 0; i + 1 < 3; ++i) {
        const QRectF top = containerIn(group.itemAt(i));
        const QRectF bottom = containerIn(group.itemAt(i + 1));
        QVERIFY2(near(bottom.top() - top.bottom(), group.betweenSpace(), 1.0),
                 qPrintable(QString::number(i)));
    }

    // The same tokens, rotated: the widget is taller than it is wide.
    QVERIFY(group.sizeHint().height() > group.sizeHint().width());
}

void TestMd3ButtonGroup::connectedKeepsTheTwoPixelSpacingAtEverySize()
{
    for (int i = 0; i < int(ButtonSize::Count); ++i) {
        const auto size = ButtonSize(i);
        MdButtonGroup group;
        group.setVariant(ButtonGroupVariant::Connected);
        group.setGroupSize(size);
        for (int n = 0; n < 2; ++n) {
            group.addItem(QString(), QString::fromUtf8(kIcon));
        }
        group.resize(group.sizeHint());

        const QByteArray where = md::buttonSizeName(size).toLatin1();
        QVERIFY2(near(group.betweenSpace(), 2.0), where.constData());

        const QRectF left = containerIn(group.itemAt(0));
        const QRectF right = containerIn(group.itemAt(1));
        QVERIFY2(near(right.left() - left.right(), 2.0, 1.0), where.constData());
        QVERIFY2(near(left.height(), group.tokens().containerHeight, 1.0), where.constData());
    }
}

void TestMd3ButtonGroup::theOuterCornerGoesToTheEndsAndTheInnerToTheMiddle()
{
    // A round connected group of three 40 px items. The ends take the group's
    // `container.shape` (corner-full, so a 20 px pill end) and the middle takes
    // `inner-corner.corner-size`, which the export publishes as corner-small.
    MdButtonGroup group;
    group.setVariant(ButtonGroupVariant::Connected);
    group.setGroupSize(ButtonSize::Small);
    for (int i = 0; i < 3; ++i) {
        group.addItem(QString(), QString::fromUtf8(kIcon));
    }
    group.resize(group.sizeHint());

    const QList<qreal> first = group.itemAt(0)->currentCornerRadii();
    const QList<qreal> middle = group.itemAt(1)->currentCornerRadii();
    const QList<qreal> last = group.itemAt(2)->currentCornerRadii();
    QCOMPARE(first.size(), 4);
    QCOMPARE(middle.size(), 4);
    QCOMPARE(last.size(), 4);

    // TL, TR, BR, BL. The leading end is round on its leading side only.
    QVERIFY(near(first.at(0), 20.0, 1.0));
    QVERIFY(near(first.at(3), 20.0, 1.0));
    QVERIFY(near(first.at(1), 8.0, 1.0));
    QVERIFY(near(first.at(2), 8.0, 1.0));

    QVERIFY(near(middle.at(0), 8.0, 1.0));
    QVERIFY(near(middle.at(3), 8.0, 1.0));

    QVERIFY(near(last.at(1), 20.0, 1.0));
    QVERIFY(near(last.at(2), 20.0, 1.0));

    // The standard form overrides nothing: each item keeps its own shape tokens,
    // which is why the list comes back empty.
    MdButtonGroup standard;
    standard.setVariant(ButtonGroupVariant::Standard);
    standard.addItem(QStringLiteral("A"));
    standard.addItem(QStringLiteral("B"));
    standard.resize(standard.sizeHint());
    QVERIFY(standard.itemAt(0)->restingCornerRadii().isEmpty());
    QVERIFY(standard.itemAt(1)->restingCornerRadii().isEmpty());
}

void TestMd3ButtonGroup::selectingAStandardItemSwapsItsShape()
{
    // "When a toggle button is selected in a standard button group, its shape
    // should change between square and round."
    MdButtonGroup group;
    group.setVariant(ButtonGroupVariant::Standard);
    group.setGroupShape(ButtonShape::Round);
    group.setSelectionMode(ButtonGroupSelection::Single);
    MdButton *a = group.addItem(QStringLiteral("A"));
    MdButton *b = group.addItem(QStringLiteral("B"));

    QCOMPARE(a->buttonShape(), ButtonShape::Round);
    QCOMPARE(b->buttonShape(), ButtonShape::Round);

    group.setCurrentIndex(0);
    QCOMPARE(a->buttonShape(), ButtonShape::Square);
    QCOMPARE(b->buttonShape(), ButtonShape::Round);

    group.setCurrentIndex(1);
    QCOMPARE(a->buttonShape(), ButtonShape::Round);
    QCOMPARE(b->buttonShape(), ButtonShape::Square);

    // ...and the group's shape is a caller decision, so the other pairing works
    // the same way round.
    group.setGroupShape(ButtonShape::Square);
    QCOMPARE(a->buttonShape(), ButtonShape::Square);
    QCOMPARE(b->buttonShape(), ButtonShape::Round);

    // The colour style swaps with the selection too, and the group owns both
    // ends of that pairing.
    group.setItemVariant(ButtonVariant::Filled);
    group.setSelectedItemVariant(ButtonVariant::Tonal);
    group.setCurrentIndex(0);
    QCOMPARE(a->variant(), ButtonVariant::Tonal);
    QCOMPARE(b->variant(), ButtonVariant::Filled);

    // Switching a connected group back to the standard form must drop the
    // per-corner override the connected form left behind, or the item would keep
    // corners from a form that is no longer in force.
    group.setVariant(ButtonGroupVariant::Connected);
    QVERIFY(!a->restingCornerRadii().isEmpty());
    group.setVariant(ButtonGroupVariant::Standard);
    QVERIFY(a->restingCornerRadii().isEmpty());
}

void TestMd3ButtonGroup::pressingGrowsTheItemAboutItsCentre()
{
    MdButtonGroup group;
    group.setVariant(ButtonGroupVariant::Standard);
    group.setGroupSize(ButtonSize::Small);
    group.setSelectionMode(ButtonGroupSelection::None);
    for (int i = 0; i < 3; ++i) {
        group.addItem(QString(), QString::fromUtf8(kIcon));
    }
    group.resize(group.sizeHint());
    group.show();
    QVERIFY(QTest::qWaitForWindowExposed(&group));

    const qreal base = iconOnlyExtent(ButtonSize::Small);
    const QRectF before0 = containerIn(group.itemAt(0));
    const QRectF before1 = containerIn(group.itemAt(1));
    const QRectF before2 = containerIn(group.itemAt(2));
    const QSize groupSize = group.size();

    MdButton *pressed = group.itemAt(1);
    QTest::mousePress(pressed, Qt::LeftButton, Qt::NoModifier, pressed->rect().center());

    // +15%, on spring-fast-spatial, which settles well inside half a second.
    QTRY_VERIFY_WITH_TIMEOUT(near(containerIn(pressed).width(), base * 1.15, 1.0), 2000);

    const QRectF after0 = containerIn(group.itemAt(0));
    const QRectF after1 = containerIn(group.itemAt(1));
    const QRectF after2 = containerIn(group.itemAt(2));

    // It grows about its own centre, so the press reads as a swell rather than
    // as the item sliding sideways.
    QVERIFY(near(after1.center().x(), before1.center().x(), 1.5));

    // The items either side are pushed away by half the growth, which is the
    // spec's "changes the width of itself and adjacent buttons".
    QVERIFY(after0.right() < before0.right() - 1.0);
    QVERIFY(after2.left() > before2.left() + 1.0);
    QVERIFY(near(before0.right() - after0.right(), after2.left() - before2.left(), 1.5));

    // The neighbours keep their own width and the gaps stay exact: the growth is
    // taken out of the reserve, not out of the spacing.
    QVERIFY(near(after0.width(), base, 1.0));
    QVERIFY(near(after2.width(), base, 1.0));
    QVERIFY(near(after1.left() - after0.right(), 12.0, 1.0));
    QVERIFY(near(after2.left() - after1.right(), 12.0, 1.0));

    // ...and the group itself does not resize while its item is pressed —
    // otherwise the whole row would jump the moment the press started.
    QCOMPARE(group.size(), groupSize);

    QTest::mouseRelease(pressed, Qt::LeftButton, Qt::NoModifier, pressed->rect().center());
    QTRY_VERIFY_WITH_TIMEOUT(near(containerIn(pressed).width(), base, 1.0), 2000);
    QVERIFY(near(containerIn(group.itemAt(1)).center().x(), before1.center().x(), 1.5));

    group.hide();
}

void TestMd3ButtonGroup::connectedPressDoesNotMoveTheNeighbours()
{
    MdButtonGroup group;
    group.setVariant(ButtonGroupVariant::Connected);
    group.setGroupSize(ButtonSize::Medium);
    group.setSelectionMode(ButtonGroupSelection::None);
    for (int i = 0; i < 3; ++i) {
        group.addItem(QString(), QString::fromUtf8(kIcon));
    }
    group.resize(group.sizeHint());
    group.show();
    QVERIFY(QTest::qWaitForWindowExposed(&group));

    const QRectF before0 = containerIn(group.itemAt(0));
    const QRectF before2 = containerIn(group.itemAt(2));
    const QList<qreal> restingMiddle = group.itemAt(1)->currentCornerRadii();

    MdButton *pressed = group.itemAt(1);
    QTest::mousePress(pressed, Qt::LeftButton, Qt::NoModifier, pressed->rect().center());

    // The pressed corner is a real, immediate change of shape — corner-extra-
    // small on a 56 px container is 4 px, against corner-small's 8.
    QTRY_VERIFY_WITH_TIMEOUT(
        near(group.itemAt(1)->currentCornerRadii().value(0), 4.0, 1.0), 2000);
    QVERIFY(!near(restingMiddle.value(0), 4.0, 1.0));

    // ...and nothing else moves. "Connected button groups don't add any
    // interaction between buttons on selection or activation — they only affect
    // the shape of the button being selected or activated."
    const QRectF after0 = containerIn(group.itemAt(0));
    const QRectF after2 = containerIn(group.itemAt(2));
    QVERIFY(near(after0.left(), before0.left(), 0.51));
    QVERIFY(near(after0.width(), before0.width(), 0.51));
    QVERIFY(near(after2.left(), before2.left(), 0.51));
    QVERIFY(near(after2.width(), before2.width(), 0.51));
    QVERIFY(near(containerIn(pressed).width(), before2.width(), 0.51));

    QTest::mouseRelease(pressed, Qt::LeftButton, Qt::NoModifier, pressed->rect().center());
    QTRY_VERIFY_WITH_TIMEOUT(
        near(group.itemAt(1)->currentCornerRadii().value(0), 8.0, 1.0), 2000);

    group.hide();
}

void TestMd3ButtonGroup::connectedXSmallAndSmallKeepTheFortyEightTargetArea()
{
    // "Extra small and small connected button groups have 48 dp target areas and
    // a minimum width of 48 dp." Everything larger publishes no minimum.
    for (int i = 0; i < int(ButtonSize::Count); ++i) {
        const auto size = ButtonSize(i);
        MdButtonGroup group;
        group.setVariant(ButtonGroupVariant::Connected);
        group.setGroupSize(size);
        // An icon-only item at xsmall measures 12 + 20 + 12 = 44, below the
        // minimum, so the clamp is doing real work at that size.
        group.addItem(QString(), QString::fromUtf8(kIcon));
        group.resize(group.sizeHint());

        const MdButtonGroupTokens tokens = group.tokens();
        const qreal natural = qMax(iconOnlyExtent(size), tokens.minimumItemExtent);
        const QByteArray where = md::buttonSizeName(size).toLatin1();
        QVERIFY2(near(containerIn(group.itemAt(0)).width(), natural, 1.0), where.constData());

        if (size == ButtonSize::XSmall || size == ButtonSize::Small) {
            QVERIFY2(near(tokens.minimumItemExtent, 48.0), where.constData());
            QVERIFY2(containerIn(group.itemAt(0)).width() >= 47.0, where.constData());
        } else {
            QVERIFY2(near(tokens.minimumItemExtent, 0.0), where.constData());
        }
    }
}

// ---------------------------------------------------------------------------
// 5. render smoke checks
// ---------------------------------------------------------------------------

void TestMd3ButtonGroup::theGroupInstallsNoPaintFilter()
{
    // A button group is an invisible container with no colour properties, so the
    // style deliberately paints nothing and registers no filter. Installing one
    // would only make the absence of paint look like a bug.
    MdButtonGroup group;
    MdButtonGroupStyle::shared();

    QVERIFY(!MdStyleBase::hasPaintFilter(&MdButtonGroup::staticMetaObject));
    // The items, on the other hand, are painted — by MdButton's own style.
    QVERIFY(MdStyleBase::hasPaintFilter(&MdButton::staticMetaObject));
}

void TestMd3ButtonGroup::iconOnlyItemsGetAnAccessibleName()
{
    MdButtonGroup group;

    // An icon-only control with no accessible name is unusable with a screen
    // reader, and a button group exists mostly to hold icon-only items.
    MdButton *derived = group.addItem(QString(), QStringLiteral("format_bold"));
    QCOMPARE(derived->accessibleName(), QStringLiteral("Format bold"));

    // A caller's own words win.
    MdButton *named = group.addItem(QString(), QStringLiteral("format_bold"),
                                    QStringLiteral("Bold"));
    QCOMPARE(named->accessibleName(), QStringLiteral("Bold"));

    // A labelled item's own text is its name; nothing is invented on top of it.
    MdButton *labelled = group.addItem(QStringLiteral("Cut"));
    QVERIFY(labelled->accessibleName().isEmpty());
}

void TestMd3ButtonGroup::disablingTheGroupDisablesTheItems()
{
    MdButtonGroup group;
    group.addItem(QStringLiteral("A"));
    group.addItem(QStringLiteral("B"));

    group.setEnabled(false);
    QVERIFY(!group.itemAt(0)->isEnabled());
    QVERIFY(!group.itemAt(1)->isEnabled());

    group.setEnabled(true);
    QVERIFY(group.itemAt(0)->isEnabled());
}

void TestMd3ButtonGroup::everyFormRendersSomething()
{
    for (int v = 0; v < int(ButtonGroupVariant::Count); ++v) {
        for (int s = 0; s < int(ButtonSize::Count); ++s) {
            for (int o = 0; o < int(ButtonGroupOrientation::Count); ++o) {
                auto *group = makeGroup(nullptr, ButtonGroupVariant(v), ButtonSize(s), 3);
                group->setOrientation(ButtonGroupOrientation(o));

                const QImage image = renderGroup(*group);
                const QByteArray where =
                    md::buttonGroupVariantName(ButtonGroupVariant(v)).toLatin1() + '/'
                    + md::buttonSizeName(ButtonSize(s)).toLatin1() + '/'
                    + md::buttonGroupOrientationName(ButtonGroupOrientation(o)).toLatin1();

                // Three items, so "something was drawn" has to mean three of
                // them — a style that painted one and dropped two would still
                // trip the naive check.
                QVERIFY2(paintedPixels(image) > 100, where.constData());

                // A vertical group is taller than it is wide and the other way
                // round, which is the cheapest proof the orientation reached the
                // layout rather than just the property.
                const QSize hint = group->sizeHint();
                if (ButtonGroupOrientation(o) == ButtonGroupOrientation::Vertical) {
                    QVERIFY2(hint.height() > hint.width(), where.constData());
                } else {
                    QVERIFY2(hint.width() > hint.height(), where.constData());
                }

                delete group;
            }
        }
    }
}

void TestMd3ButtonGroup::lightAndDarkPaintDifferentItems()
{
    MdTheme &theme = MdTheme::instance();
    const ThemeMode original = theme.themeMode();

    MdButtonGroup group;
    group.setVariant(ButtonGroupVariant::Standard);
    group.setSelectionMode(ButtonGroupSelection::Single);
    for (int i = 0; i < 3; ++i) {
        group.addItem(QString(), QString::fromUtf8(kIcon));
    }
    group.setCurrentIndex(1);
    group.resize(group.sizeHint());
    group.show();
    QVERIFY(QTest::qWaitForWindowExposed(&group));

    theme.setThemeMode(ThemeMode::Light);
    const QImage light = group.grab().toImage();
    theme.setThemeMode(ThemeMode::Dark);
    const QImage dark = group.grab().toImage();

    QVERIFY(paintedPixels(light) > 100);
    QVERIFY(paintedPixels(dark) > 100);
    QVERIFY(light.pixel(light.width() / 2, light.height() / 2)
            != dark.pixel(dark.width() / 2, dark.height() / 2));

    theme.setThemeMode(original);
    group.hide();
}

void TestMd3ButtonGroup::rightToLeftMirrorsTheRow()
{
    MdTheme &theme = MdTheme::instance();
    const Qt::LayoutDirection originalDirection = theme.direction();

    MdButtonGroup group;
    group.setVariant(ButtonGroupVariant::Connected);
    group.setGroupSize(ButtonSize::Small);
    for (int i = 0; i < 3; ++i) {
        group.addItem(QString(), QString::fromUtf8(kIcon));
    }
    group.resize(group.sizeHint());

    group.itemAt(0)->setFocus();
    const QRectF ltrFirst = containerIn(group.itemAt(0));
    const QRectF ltrLast = containerIn(group.itemAt(2));
    QVERIFY(ltrFirst.left() < ltrLast.left());
    // Inline start: the leading end is round on the left.
    QVERIFY(near(group.itemAt(0)->currentCornerRadii().at(0), 20.0, 1.0));

    theme.setDirection(Qt::RightToLeft);
    group.resize(group.sizeHint());

    const QRectF rtlFirst = containerIn(group.itemAt(0));
    const QRectF rtlLast = containerIn(group.itemAt(2));
    // The same order, mirrored: item 0 is now on the right.
    QVERIFY(rtlFirst.left() > rtlLast.left());
    // ...and the round end followed the inline start, so item 0 is round on its
    // right rather than on its left.
    QVERIFY(near(group.itemAt(0)->currentCornerRadii().at(1), 20.0, 1.0));
    QVERIFY(near(group.itemAt(0)->currentCornerRadii().at(0), 8.0, 1.0));

    theme.setDirection(originalDirection);
    group.resize(group.sizeHint());
    QVERIFY(near(group.itemAt(0)->currentCornerRadii().at(0), 20.0, 1.0));
}

QTEST_MAIN(TestMd3ButtonGroup)

#include "TestMd3ButtonGroup.moc"
