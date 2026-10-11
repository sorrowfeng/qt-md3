// TestMd3ExpressiveMenu — the Expressive menus family: MdExpressiveMenu +
// MdExpressiveMenuItem + MdExpressiveMenuTokens.
//
// Numbers transcribed from material-web tokens/versions/latest/sass:
//
//   _md-comp-menus.scss           shared Expressive geometry
//   _md-comp-menus-standard.scss  StandardMenu colours
//   _md-comp-menus-vibrant.scss   VibrantMenu colours
//
// The assertions worth keeping are the ones a token table cannot express:
//
//   * **shape = corner-large** for the container and the pressed item;
//   * **item height 44**, gap 2, group padding 2;
//   * **Standard vs Vibrant colour rows** — the surface base, the selected
//     container, the Vibrant icon interaction lift;
//   * **the per-position item shapes** (selected / first / last → medium,
//     middle → extra-small);
//   * **`:focus-visible` keyboard-only focus ring**.

#include "core/MdExpressiveMenuTokens.h"
#include "core/MdTheme.h"
#include "styles/MdExpressiveMenuStyle.h"
#include "widgets/MdExpressiveMenu.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

QImage render(QWidget &widget)
{
    QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    widget.render(&painter);
    return image;
}

bool hasInk(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

class TestMd3ExpressiveMenu : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void sharedGeometryMatchesTheExport();
    void standardColoursMatchTheExport();
    void vibrantColoursMatchTheExport();
    void itemShapesFollowPositionAndSelection();
    void itemGeometryUsesTheSharedSpacing();
    void menuLayoutAppliesGapAndGroupPadding();
    void selectedPaintsTheVariantContainer();
    void focusRingIsKeyboardOnly();
    void variantSwitchesTheTokenSet();
    void renderSmoke();
    void metaProperties();
};

void TestMd3ExpressiveMenu::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    QVERIFY(MdExpressiveMenuStyle::shared() != nullptr);
}

void TestMd3ExpressiveMenu::cleanup()
{
    // Each test builds its own widgets on the stack. Reset the theme so a
    // failing QCOMPARE early-return cannot leak a dark mode into the next slot.
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

void TestMd3ExpressiveMenu::sharedGeometryMatchesTheExport()
{
    const MdExpressiveMenuTokens t =
        MdExpressiveMenuTokens::resolve(MdExpressiveMenuVariant::Standard);

    // _md-comp-menus.scss at 34.0.21.
    QCOMPARE(t.containerShape, ShapeCorner::Large);
    QCOMPARE(t.activeContainerShape, ShapeCorner::Large);
    QCOMPARE(t.containerElevation, ElevationLevel::Level2);
    QCOMPARE(t.itemGap, 2.0);
    QCOMPARE(t.groupPadding, 2.0);
    QCOMPARE(t.itemHeight, 44.0);
    QCOMPARE(t.leadingIconSize, 20.0);
    QCOMPARE(t.trailingIconSize, 20.0);
    QCOMPARE(t.itemIconTextSpacing, 12.0);
    QCOMPARE(t.itemLeadingSpace, 16.0);
    QCOMPARE(t.itemTrailingSpace, 16.0);
    QCOMPARE(t.itemShape, ShapeCorner::ExtraSmall);
    QCOMPARE(t.itemSelectedShape, ShapeCorner::Medium);
    QCOMPARE(t.itemFirstChildShape, ShapeCorner::Medium);
    QCOMPARE(t.itemLastChildShape, ShapeCorner::Medium);
    QCOMPARE(t.innerCornerSize, ShapeCorner::ExtraSmall);
    QCOMPARE(t.labelTextType, TypeStyle::LabelLarge);
    QCOMPARE(t.supportingTextType, TypeStyle::BodySmall);
    QCOMPARE(t.focusIndicatorColor, ColorRole::Secondary);
    QCOMPARE(t.focusIndicatorInnerOffset, 3.0);
    QCOMPARE(t.focusIndicatorThickness, 3.0);
}

void TestMd3ExpressiveMenu::standardColoursMatchTheExport()
{
    const MdExpressiveMenuTokens t =
        MdExpressiveMenuTokens::resolve(MdExpressiveMenuVariant::Standard);
    const auto enabled = MdNavigationItemState::Enabled;
    const auto disabled = MdNavigationItemState::Disabled;

    // _md-comp-menus-standard.scss.
    QCOMPARE(t.containerColor, ColorRole::SurfaceContainerLow);
    QCOMPARE(t.sectionLabelColor, ColorRole::OnSurfaceVariant);

    // Unselected: on-surface label, on-surface-variant icons.
    QCOMPARE(t.labelFor(MdExpressiveMenuSelection::Unselected, enabled).role, ColorRole::OnSurface);
    QCOMPARE(t.iconFor(MdExpressiveMenuSelection::Unselected, enabled).role,
             ColorRole::OnSurfaceVariant);

    // Selected: tertiary-container fill with on-tertiary-container content.
    QCOMPARE(t.itemContainerFor(MdExpressiveMenuSelection::Selected, enabled).role,
             ColorRole::TertiaryContainer);
    QCOMPARE(t.labelFor(MdExpressiveMenuSelection::Selected, enabled).role,
             ColorRole::OnTertiaryContainer);

    // Disabled content fades to 0.38; Standard publishes selected disabled
    // container at 0.38 too.
    QCOMPARE(t.labelFor(MdExpressiveMenuSelection::Unselected, disabled).opacity, 0.38);
    QCOMPARE(t.iconFor(MdExpressiveMenuSelection::Unselected, disabled).opacity, 0.38);
    QCOMPARE(t.itemContainerFor(MdExpressiveMenuSelection::Selected, disabled).opacity, 0.38);

    // The state layer is on-surface unselected / on-tertiary-container selected.
    QCOMPARE(t.stateLayerFor(MdExpressiveMenuSelection::Unselected, MdNavigationItemState::Hovered)
                 .role,
             ColorRole::OnSurface);
    QCOMPARE(t.stateLayerFor(MdExpressiveMenuSelection::Selected, MdNavigationItemState::Hovered)
                 .role,
             ColorRole::OnTertiaryContainer);
}

void TestMd3ExpressiveMenu::vibrantColoursMatchTheExport()
{
    const MdExpressiveMenuTokens t =
        MdExpressiveMenuTokens::resolve(MdExpressiveMenuVariant::Vibrant);
    const auto enabled = MdNavigationItemState::Enabled;
    const auto hovered = MdNavigationItemState::Hovered;
    const auto disabled = MdNavigationItemState::Disabled;

    // _md-comp-menus-vibrant.scss.
    QCOMPARE(t.containerColor, ColorRole::TertiaryContainer);
    QCOMPARE(t.sectionLabelColor, ColorRole::OnTertiaryContainer);

    // Unselected content is on-tertiary-container; the icons lift to tertiary
    // under hover / focus / press — the row Standard does not publish.
    QCOMPARE(t.labelFor(MdExpressiveMenuSelection::Unselected, enabled).role,
             ColorRole::OnTertiaryContainer);
    QCOMPARE(t.iconFor(MdExpressiveMenuSelection::Unselected, enabled).role,
             ColorRole::OnTertiaryContainer);
    QCOMPARE(t.iconFor(MdExpressiveMenuSelection::Unselected, hovered).role, ColorRole::Tertiary);

    // Selected: tertiary fill with on-tertiary content.
    QCOMPARE(t.itemContainerFor(MdExpressiveMenuSelection::Selected, enabled).role,
             ColorRole::Tertiary);
    QCOMPARE(t.labelFor(MdExpressiveMenuSelection::Selected, enabled).role, ColorRole::OnTertiary);

    // Disabled content fades to 0.38. Vibrant publishes no selected disabled
    // container-opacity row — the container stays at 1.0.
    QCOMPARE(t.labelFor(MdExpressiveMenuSelection::Unselected, disabled).opacity, 0.38);
    QCOMPARE(t.itemContainerFor(MdExpressiveMenuSelection::Selected, disabled).opacity, 1.0);
}

void TestMd3ExpressiveMenu::itemShapesFollowPositionAndSelection()
{
    const MdExpressiveMenuTokens t =
        MdExpressiveMenuTokens::resolve(MdExpressiveMenuVariant::Standard);

    // 5 rows: first / middle / last, and the selected / pressed overrides.
    QCOMPARE(t.itemShapeFor(0, 5, false, false), ShapeCorner::Medium);
    QCOMPARE(t.itemShapeFor(1, 5, false, false), ShapeCorner::ExtraSmall);
    QCOMPARE(t.itemShapeFor(4, 5, false, false), ShapeCorner::Medium);
    QCOMPARE(t.itemShapeFor(2, 5, true, false), ShapeCorner::Medium);
    QCOMPARE(t.itemShapeFor(2, 5, false, true), ShapeCorner::Large);
}

void TestMd3ExpressiveMenu::itemGeometryUsesTheSharedSpacing()
{
    MdExpressiveMenuItem item(QStringLiteral("A fairly long menu label"));
    const QSize hint = item.sizeHint();
    QCOMPARE(hint.height(), 44);
    // Leading space 16 + trailing space 16 + the label, so at least 32 wide.
    QVERIFY(hint.width() >= 32);
}

void TestMd3ExpressiveMenu::menuLayoutAppliesGapAndGroupPadding()
{
    MdExpressiveMenu menu;
    menu.addItem(QStringLiteral("One"));
    menu.addItem(QStringLiteral("Two"));
    menu.addItem(QStringLiteral("Three"));

    const QList<MdExpressiveMenuItem *> items = menu.items();
    QCOMPARE(items.size(), 3);

    // group-padding 2 above the first row; gap 2 between rows.
    QCOMPARE(qreal(items.at(0)->y()), 2.0);
    QCOMPARE(qreal(items.at(1)->y()) - qreal(items.at(0)->y() + items.at(0)->height()), 2.0);
    QCOMPARE(qreal(items.at(2)->y()) - qreal(items.at(1)->y() + items.at(1)->height()), 2.0);

    // group-padding 2 below the last row.
    QCOMPARE(qreal(menu.height() - (items.at(2)->y() + items.at(2)->height())), 2.0);
}

void TestMd3ExpressiveMenu::selectedPaintsTheVariantContainer()
{
    MdExpressiveMenu menu;
    menu.setVariant(MdExpressiveMenuVariant::Standard);
    auto *item = menu.addItem(QStringLiteral("Selected"));
    item->setSelected(true);
    menu.resize(menu.sizeHint());
    menu.show();

    const QImage image = render(menu);
    QVERIFY(hasInk(image));
}

void TestMd3ExpressiveMenu::focusRingIsKeyboardOnly()
{
    MdExpressiveMenuItem item(QStringLiteral("Row"));
    item.resize(item.sizeHint());
    item.show();
    QVERIFY(QTest::qWaitForWindowExposed(&item));

    // A mouse focus reason paints nothing (`:focus-visible`).
    item.setFocus(Qt::MouseFocusReason);
    QVERIFY(!item.hasKeyboardFocus());

    // A keyboard reason paints the ring.
    item.clearFocus();
    item.setFocus(Qt::TabFocusReason);
    QVERIFY(item.hasKeyboardFocus());
}

void TestMd3ExpressiveMenu::variantSwitchesTheTokenSet()
{
    MdExpressiveMenu menu;
    QCOMPARE(menu.variant(), MdExpressiveMenuVariant::Standard);
    QCOMPARE(menu.menuTokens().containerColor, ColorRole::SurfaceContainerLow);

    QSignalSpy spy(&menu, &MdExpressiveMenu::variantChanged);
    menu.setVariant(MdExpressiveMenuVariant::Vibrant);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(menu.variant(), MdExpressiveMenuVariant::Vibrant);
    QCOMPARE(menu.menuTokens().containerColor, ColorRole::TertiaryContainer);

    // New items pick up the menu's variant.
    auto *item = menu.addItem(QStringLiteral("Row"));
    QCOMPARE(item->menuTokens().containerColor, ColorRole::TertiaryContainer);
}

void TestMd3ExpressiveMenu::renderSmoke()
{
    MdExpressiveMenu menu;
    menu.setVariant(MdExpressiveMenuVariant::Vibrant);
    menu.addItem(QStringLiteral("One"));
    menu.addItem(QStringLiteral("Two"));
    menu.resize(menu.sizeHint());
    menu.show();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    const QImage image = render(menu);
    QVERIFY(hasInk(image));
}

void TestMd3ExpressiveMenu::metaProperties()
{
    MdExpressiveMenu menu;
    const QMetaObject *meta = menu.metaObject();
    QVERIFY(meta->indexOfProperty("variant") >= 0);

    QSignalSpy spy(&menu, &MdExpressiveMenu::variantChanged);
    menu.setVariant(MdExpressiveMenuVariant::Vibrant);
    QCOMPARE(spy.count(), 1);

    MdExpressiveMenuItem item(QStringLiteral("Row"));
    QVERIFY(item.metaObject()->indexOfProperty("selected") >= 0);
    QVERIFY(item.metaObject()->indexOfProperty("supportingText") >= 0);
    QVERIFY(item.metaObject()->indexOfProperty("trailingSupportingText") >= 0);
}

QTEST_MAIN(TestMd3ExpressiveMenu)
#include "TestMd3ExpressiveMenu.moc"
