// MdFabMenu — MD3 FAB menus.
//
// The same three kinds of check the button families' suites make:
//
//   1. the token table, field by field, against the published
//      `md.comp.fab-menu.*` sets — the common spacing, the close button's
//      square and elevation ladder, the list item's derived width rows, the
//      three colour groups, and the disabled row that fills the export's
//      gap from the spec's state table.
//   2. the container contract — items, expand/collapse with the staggered
//      reveal, the anchor/close crossfade, and the item-activation signal.
//   3. render smoke checks across the colour groups, and the item-level
//      interaction rules (`:focus-visible`, the ripple).

#include "TestMd3Common.h"

#include "core/MdFabMenuTokens.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "styles/MdFabMenuStyle.h"
#include "widgets/MdFabMenu.h"
#include "widgets/MdFabMenuItem.h"
#include "widgets/MdFab.h"

#include <QtCore/QElapsedTimer>
#include <QtGui/QImage>
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

QImage renderWidget(QWidget &widget)
{
    widget.resize(widget.sizeHint());
    widget.show();
    QTest::qWaitForWindowExposed(&widget);
    const QPixmap pixmap = widget.grab();
    widget.hide();
    return pixmap.toImage();
}

int paintedPixels(const QImage &image)
{
    return mdtest::paintedPixelCount(image, image.pixel(0, 0));
}

void settleAnimation(MdFabMenu &menu)
{
    QElapsedTimer clock;
    clock.start();
    while (menu.isAnimating() && clock.elapsed() < 5000) {
        QTest::qWait(50);
    }
}

} // namespace

class TestMd3FabMenu : public QObject
{
    Q_OBJECT

private slots:
    // --- token table -------------------------------------------------------

    void closeButtonRowsMatchTheExport();
    void listItemRowsMatchTheExport();
    void colourGroupsMatchTheExport();
    void spacingAndFocusIndicatorMatchTheSources();
    void disabledRowFillsTheExportGapFromTheSpecTable();
    void overridesReachTheResolver();

    // --- container contract ---------------------------------------------------

    void itemsAndActivationSignalWork();
    void expandCollapseAnimatesAndCrossfades();
    void sizeHintIsAlwaysTheExpandedSize();

    // --- item interaction ------------------------------------------------------

    void itemPointerFocusShowsNoRingKeyboardFocusDoes();
    void everyColourGroupRendersInk();
};

void TestMd3FabMenu::closeButtonRowsMatchTheExport()
{
    // md.comp.fab-menu: close-button 56 × 56, icon 20, corner-full; the
    // raised-FAB elevation ladder (level3 resting, level4 hovered).
    const MdFabMenuTokens tokens = MdFabMenuTokens::resolve(FabMenuVariant::Primary);
    const MdFabMenuElementTokens &close = tokens.closeButton;

    QCOMPARE(close.containerHeight, 56.0);
    QCOMPARE(close.containerWidth, 56.0);
    QCOMPARE(close.iconSize, 20.0);
    QVERIFY(close.containerShape == ShapeCorner::Full);
    QVERIFY(close.enabledElevation == ElevationLevel::Level3);
    QVERIFY(close.hoveredElevation == ElevationLevel::Level4);
    QVERIFY(close.focusedElevation == ElevationLevel::Level3);
    QVERIFY(close.pressedElevation == ElevationLevel::Level3);
    QVERIFY(close.disabledElevation == ElevationLevel::Level0);
}

void TestMd3FabMenu::listItemRowsMatchTheExport()
{
    // md.comp.fab-menu: menu-item 56 tall, icon 24, icon-label space 8,
    // leading/trailing 24, corner-full, title-medium, level0 in every state.
    const MdFabMenuTokens tokens = MdFabMenuTokens::resolve(FabMenuVariant::Primary);
    const MdFabMenuElementTokens &item = tokens.listItem;

    QCOMPARE(item.containerHeight, 56.0);
    QCOMPARE(item.containerWidth, 0.0); // content-derived
    QCOMPARE(item.iconSize, 24.0);
    QCOMPARE(item.iconLabelSpace, 8.0);
    QCOMPARE(item.leadingSpace, 24.0);
    QCOMPARE(item.trailingSpace, 24.0);
    QVERIFY(item.containerShape == ShapeCorner::Full);
    QVERIFY(item.labelStyle == TypeStyle::TitleMedium);
    QVERIFY(item.enabledElevation == ElevationLevel::Level0);
    QVERIFY(item.hoveredElevation == ElevationLevel::Level0);
    QVERIFY(item.pressedElevation == ElevationLevel::Level0);
    QVERIFY(item.disabledElevation == ElevationLevel::Level0);
}

void TestMd3FabMenu::colourGroupsMatchTheExport()
{
    // md.comp.fab-menu.<group>[-container]: the close button takes the pure
    // colour, the list items the container colour; content and state layer
    // share the matching on-* role; the interactive rows are flat.
    struct Row
    {
        FabMenuVariant variant;
        ColorRole closeButtonContainer;
        ColorRole closeButtonContent;
        ColorRole itemContainer;
        ColorRole itemContent;
    };
    const Row rows[] = {
        {FabMenuVariant::Primary, ColorRole::Primary, ColorRole::OnPrimary,
         ColorRole::PrimaryContainer, ColorRole::OnPrimaryContainer},
        {FabMenuVariant::Secondary, ColorRole::Secondary, ColorRole::OnSecondary,
         ColorRole::SecondaryContainer, ColorRole::OnSecondaryContainer},
        {FabMenuVariant::Tertiary, ColorRole::Tertiary, ColorRole::OnTertiary,
         ColorRole::TertiaryContainer, ColorRole::OnTertiaryContainer},
    };

    for (const Row &row : rows) {
        const MdFabMenuTokens tokens = MdFabMenuTokens::resolve(row.variant);
        QVERIFY(tokens.closeButton.container == row.closeButtonContainer);
        QVERIFY(tokens.closeButton.content == row.closeButtonContent);
        QVERIFY(tokens.closeButton.stateLayer == row.closeButtonContent);
        QVERIFY(tokens.listItem.container == row.itemContainer);
        QVERIFY(tokens.listItem.content == row.itemContent);
        QVERIFY(tokens.listItem.stateLayer == row.itemContent);
    }
}

void TestMd3FabMenu::spacingAndFocusIndicatorMatchTheSources()
{
    // md.comp.fab-menu common rows: close-button.between-space 8,
    // menu-item.between-space 4. The export publishes no focus-indicator
    // rows for this family, so the shared md-sys-state-focus-indicator
    // fallback applies (secondary, 3 px, offset 2).
    const MdFabMenuTokens tokens = MdFabMenuTokens::resolve(FabMenuVariant::Primary);
    QCOMPARE(tokens.closeButtonBetweenSpace, 8.0);
    QCOMPARE(tokens.menuItemBetweenSpace, 4.0);
    QVERIFY(tokens.focusIndicator == ColorRole::Secondary);
    QCOMPARE(tokens.focusIndicatorThickness, 3.0);
    QCOMPARE(tokens.focusIndicatorOffset, 2.0);
    QCOMPARE(MdFabMenuStyle::focusRingInset(tokens), 7.5);
}

void TestMd3FabMenu::disabledRowFillsTheExportGapFromTheSpecTable()
{
    // The token export publishes no disabled rows; the spec's disabled state
    // table fills them: on-surface @12% container, on-surface @38% icon and
    // label, level0. Exposed as the shared opacities the style folds in.
    QCOMPARE(fabMenuDisabledContainerOpacity(), 0.12);
    QCOMPARE(fabMenuDisabledContentOpacity(), 0.38);
}

void TestMd3FabMenu::overridesReachTheResolver()
{
    MdComponentTokens overrides;
    overrides.setValue(QStringLiteral("md.comp.fab-menu.close-button.container.height"),
                       QStringLiteral("64"));
    overrides.setValue(QStringLiteral("md.comp.fab-menu.container.height"),
                       QStringLiteral("60"));
    const MdFabMenuTokens tokens = MdFabMenuTokens::resolve(FabMenuVariant::Primary, &overrides);
    // The element-qualified key wins over the generic one; the generic key
    // is the shared fallback and reaches both elements.
    QCOMPARE(tokens.closeButton.containerHeight, 64.0);
    QCOMPARE(tokens.listItem.containerHeight, 60.0);

    // A shape override accepts both spellings, like the other components.
    MdComponentTokens shapeOverrides;
    shapeOverrides.setValue(QStringLiteral("md.comp.fab-menu.close-button.container.shape"),
                            QStringLiteral("corner-large"));
    const MdFabMenuTokens shaped =
        MdFabMenuTokens::resolve(FabMenuVariant::Primary, &shapeOverrides);
    QVERIFY(shaped.closeButton.containerShape == ShapeCorner::Large);
    QVERIFY(shaped.listItem.containerShape == ShapeCorner::Full);
}

void TestMd3FabMenu::itemsAndActivationSignalWork()
{
    MdFabMenu menu(QStringLiteral("add"));

    QCOMPARE(menu.itemCount(), 0);
    menu.addItem(QStringLiteral("edit"), QStringLiteral("Edit"));
    menu.addItem(QStringLiteral("delete"), QStringLiteral("Delete"));
    QCOMPARE(menu.itemCount(), 2);
    QVERIFY(menu.itemAt(0) != nullptr);
    QCOMPARE(menu.itemAt(0)->text(), QStringLiteral("Edit"));
    QCOMPARE(menu.itemAt(1)->iconName(), QStringLiteral("delete"));
    QCOMPARE(menu.itemAt(2), nullptr);

    QSignalSpy activatedSpy(&menu, &MdFabMenu::itemActivated);
    MdFabMenuItem *first = menu.itemAt(0);
    first->show();
    QTest::mouseClick(first, Qt::LeftButton);
    QCOMPARE(activatedSpy.count(), 1);
    QCOMPARE(activatedSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(activatedSpy.at(0).at(1).toString(), QStringLiteral("Edit"));

    menu.clearItems();
    QCOMPARE(menu.itemCount(), 0);
}

void TestMd3FabMenu::expandCollapseAnimatesAndCrossfades()
{
    MdFabMenu menu(QStringLiteral("add"));
    menu.addItem(QStringLiteral("edit"), QStringLiteral("Edit"));
    menu.addItem(QStringLiteral("delete"), QStringLiteral("Delete"));
    menu.resize(menu.sizeHint());
    menu.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));

    // Collapsed: the anchor FAB shows, the close button does not.
    QVERIFY(menu.anchorFab()->isVisible());
    QVERIFY(!menu.isExpanded());

    // Expanding runs the staggered reveal; when it settles the anchor is
    // replaced in place by the close button and both items are shown.
    menu.setExpanded(true);
    QVERIFY(menu.isExpanded());
    settleAnimation(menu);
    QVERIFY(!menu.isAnimating());
    QVERIFY(!menu.anchorFab()->isVisible());
    QVERIFY(menu.itemAt(0)->isVisible());
    QVERIFY(menu.itemAt(1)->isVisible());
    QCOMPARE(menu.itemAt(0)->reveal(), 1.0);
    QCOMPARE(menu.itemAt(1)->reveal(), 1.0);

    // Collapsing runs the reverse and settles back to the anchor alone.
    menu.setExpanded(false);
    settleAnimation(menu);
    QVERIFY(!menu.isAnimating());
    QVERIFY(menu.anchorFab()->isVisible());
    QVERIFY(!menu.itemAt(0)->isVisible());

    // Tapping the anchor expands; tapping the close button collapses.
    QTest::mouseClick(menu.anchorFab(), Qt::LeftButton);
    settleAnimation(menu);
    QVERIFY(menu.isExpanded());
    QTest::mouseClick(menu.itemAt(0), Qt::LeftButton); // a click on an item expands nothing
    QTest::mouseClick(menu.closeButton(), Qt::LeftButton);
    settleAnimation(menu);
    QVERIFY(!menu.isExpanded());
    menu.hide();
}

void TestMd3FabMenu::sizeHintIsAlwaysTheExpandedSize()
{
    MdFabMenu menu(QStringLiteral("add"));
    menu.addItem(QStringLiteral("edit"), QStringLiteral("Edit"));

    const QSize expandedHint = menu.sizeHint();
    // top margin + close hint (56 + 2 margins) + gap + item height + bottom
    // margin.
    QCOMPARE(expandedHint.height(), int(7.5 + 71.0 + 8.0 + 56.0 + 7.5));

    // The hint does not depend on the expanded state, so a layout that
    // reserved room for the open menu keeps its geometry while animating.
    menu.setExpanded(true);
    settleAnimation(menu);
    QCOMPARE(menu.sizeHint(), expandedHint);
}

void TestMd3FabMenu::itemPointerFocusShowsNoRingKeyboardFocusDoes()
{
    MdFabMenu menu(QStringLiteral("add"));
    menu.addItem(QStringLiteral("edit"), QStringLiteral("Edit"));
    menu.resize(menu.sizeHint());
    menu.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));

    // Items are hidden while collapsed; expand first so the item can take
    // the click at all.
    menu.setExpanded(true);
    settleAnimation(menu);
    MdFabMenuItem *item = menu.itemAt(0);
    // `:focus-visible` semantics, pinned here so a regression back to
    // "any focus shows the ring" cannot slip through.
    QTest::mouseClick(item, Qt::LeftButton);
    QVERIFY(item->hasFocus());
    QVERIFY(!item->hasKeyboardFocus());

    // setFocus() on a widget that already holds focus does not re-deliver
    // focusInEvent, so drop it first.
    item->clearFocus();
    item->setFocus(Qt::TabFocusReason);
    QVERIFY(item->hasKeyboardFocus());
    menu.hide();
}

void TestMd3FabMenu::everyColourGroupRendersInk()
{
    // 3 colour groups × expanded with items — every grab must paint ink.
    for (const FabMenuVariant variant :
         {FabMenuVariant::Primary, FabMenuVariant::Secondary, FabMenuVariant::Tertiary}) {
        MdFabMenu menu(QStringLiteral("add"));
        menu.setVariant(variant);
        menu.addItem(QStringLiteral("edit"), QStringLiteral("Edit"));
        menu.addItem(QStringLiteral("delete"), QStringLiteral("Delete"));
        menu.setExpanded(true);
        settleAnimation(menu);
        const QImage image = renderWidget(menu);
        const int ink = paintedPixels(image);
        QVERIFY2(ink > image.width() * image.height() / 6,
                 qPrintable(QStringLiteral("%1: only %2 ink pixels of %3")
                                .arg(fabMenuVariantName(variant))
                                .arg(ink)
                                .arg(image.width() * image.height())));
    }
}

QTEST_MAIN(TestMd3FabMenu)
#include "TestMd3FabMenu.moc"
