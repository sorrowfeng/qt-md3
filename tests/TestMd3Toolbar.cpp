// TestMd3Toolbar — the Toolbars family: MdDockedToolbar + MdFloatingToolbar.
//
// Numbers transcribed from material-web
// tokens/versions/latest/sass/_md-comp-toolbar-docked.scss,
// -floating.scss, -floating-fab.scss, -standard.scss and -vibrant.scss (every
// row), cross-checked against androidx Compose Material3
// DockedToolbarTokens.kt / FloatingToolbarTokens.kt.
//
// The *behaviour* sources differ per variant and that is the point of the
// family: material-web ships no toolbars implementation at all, so the docked
// toolbar's layout comes from `FlexibleBottomAppBar` (AppBar.kt — the only
// place the variant exists as behaviour in Compose) and the floating toolbar's
// from `HorizontalFloatingToolbar` / `VerticalFloatingToolbar`
// (FloatingToolbar.kt).
//
// The assertions worth keeping are the geometry ones, because they are what a
// token table cannot express:
//
//   * a docked toolbar's row is `spacedBy(32, CenterHorizontally)` and its
//     *whole* height collapses (`heightOffsetLimit == -64`), unlike a two-row
//     top app bar;
//   * a floating toolbar's pill lengths with `expandedProgress` while its outer
//     bounds stay put, its leading and trailing slots exist only while
//     expanded, and its action button is 56 px expanded and *80* collapsed, so
//     the pair keeps a roughly constant footprint;
//   * children go on their **containers** in both, because `MdIconButton` is a
//     55 px widget around a 40 px container — see `MdChildBox`;
//   * the floating toolbar's slots are separated by the published
//     `container.between-space` (4), a row Compose declares and never reads —
//     so the pill is `8 + n * item + (n - 1) * 4 + 8` long. The gap reached the
//     layout only after a pixel audit of page 29 caught the pill measuring 136
//     px where the export's own arithmetic says 144.

#include "core/MdDockedToolbarTokens.h"
#include "core/MdFloatingToolbarTokens.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdChildBox.h"
#include "styles/MdDockedToolbarStyle.h"
#include "styles/MdFloatingToolbarStyle.h"
#include "widgets/MdDockedToolbar.h"
#include "widgets/MdFab.h"
#include "widgets/MdFloatingToolbar.h"
#include "widgets/MdIconButton.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

#include <cmath>

using namespace md;

namespace {

/// A slot stand-in with a known intrinsic size and no container concept of its
/// own, so the layout arithmetic can be tested without an icon button's
/// geometry leaking into it.
///
/// Deliberately *not* fixed-size: `MdChildBox::measure` resizes the widget to
/// its `sizeHint()`, and the toolbar's action button is the one child that then
/// has to be resized again to a size that is not its hint — a fixed-size probe
/// cannot demonstrate that.
class ProbeWidget : public QWidget
{
public:
    explicit ProbeWidget(const QSize &hint, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_hint(hint)
    {
        resize(hint);
    }

    QSize sizeHint() const override { return m_hint; }

private:
    QSize m_hint;
};

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

} // namespace

class TestMd3Toolbar : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    // --- tokens -------------------------------------------------------------
    void dockedTokenTable();
    void floatingTokenTable();
    void floatingFabTable();
    void colourSchemeTables();
    void disabledWinsOverSelected();
    void tokenOverridesReachTheLayout();

    // --- docked -------------------------------------------------------------
    void dockedGeometry();
    void dockedCollapsesItsWholeHeight();
    void dockedChildrenUseContainers();
    void dockedPaintsItsContainer();

    // --- floating -----------------------------------------------------------
    void floatingPillGeometry();
    void betweenSpaceSeparatesTheSlots();
    void verticalIsTheTranspose();
    void leadingAndTrailingExistOnlyWhenExpanded();
    void expandedProgressLengthsThePill();
    void floatingFabStripAndSizeSets();
    void floatingFabPosition();
    void floatingFabIsAskedToTakeTheTokenSize();
    void floatingChildrenUseContainers();
    void floatingPaintsItsScheme();

    // --- cross-cutting ------------------------------------------------------
    void rtlIsNotMirrored();
};

void TestMd3Toolbar::init()
{
}

void TestMd3Toolbar::cleanup()
{
    // A failing QCOMPARE / QVERIFY returns out of its slot immediately, so
    // every global a slot can touch is reset here. The list family paid for
    // getting this wrong four times.
    MdComponentTokens &global = MdComponentTokens::global();
    global.remove(QStringLiteral("md.comp.toolbar.docked.container.max-spacing"));
    global.remove(QStringLiteral("md.comp.toolbar.docked.content.top-space"));
    global.remove(QStringLiteral("md.comp.toolbar.floating.container.leading-space"));
    global.remove(QStringLiteral("md.comp.toolbar.floating.container.between-space"));
    global.remove(QStringLiteral("md.comp.toolbar.floating.container.shape"));
    global.remove(QStringLiteral("md.comp.toolbar.floating.container.height"));
    global.remove(QStringLiteral("md.comp.toolbar.floating.horizontal.container.height"));
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

void TestMd3Toolbar::dockedTokenTable()
{
    const MdDockedToolbarTokens tokens = MdDockedToolbarTokens::resolve();

    QCOMPARE(tokens.containerHeight, 64.0);
    QCOMPARE(tokens.containerLeadingSpace, 16.0);
    QCOMPARE(tokens.containerTrailingSpace, 16.0);
    QCOMPARE(tokens.containerMaxSpacing, 32.0);
    // Published and read by nothing in Compose: `FlexibleHorizontalArrangement`
    // uses `max`, never `min`. Carried so a theme that sets it still resolves.
    QCOMPARE(tokens.containerMinSpacing, 4.0);
    QVERIFY(tokens.containerShape == ShapeCorner::None);
    QVERIFY(tokens.containerColor == ColorRole::SurfaceContainer);
    // Not a token row: `BottomAppBarLayout` centres the content in a 64 px row,
    // so there is no vertical content padding to publish.
    QCOMPARE(tokens.contentTopSpace, 0.0);
    QCOMPARE(tokens.contentBottomSpace, 0.0);
}

void TestMd3Toolbar::floatingTokenTable()
{
    const MdFloatingToolbarTokens tokens = MdFloatingToolbarTokens::resolve();

    // The horizontal / vertical split. Three of the four rows are deprecated
    // ("Deprecating this for a vertical and horizontal variant") and are
    // carried regardless, because a 2024 theme still reads them.
    QCOMPARE(tokens.containerHeight, 64.0);
    QCOMPARE(tokens.horizontalContainerHeight, 64.0);
    QCOMPARE(tokens.verticalContainerWidth, 64.0);
    QCOMPARE(tokens.containerExternalPadding, 16.0);
    QCOMPARE(tokens.horizontalContainerExternalSpace, 16.0);
    // A vertical toolbar sits further in than a horizontal one.
    QCOMPARE(tokens.verticalContainerExternalSpace, 24.0);

    // 8 + a 48 px icon button + 8 = the 64 px container height.
    QCOMPARE(tokens.containerLeadingSpace, 8.0);
    QCOMPARE(tokens.containerTrailingSpace, 8.0);
    QCOMPARE(tokens.containerBetweenSpace, 4.0);

    // Published, and not used by Compose at all: its own defaults are
    // `ElevationTokens.Level0` with a "TODO read from token". Carried.
    QVERIFY(tokens.containerElevation == ElevationLevel::Level3);
    QVERIFY(tokens.containerShape == ShapeCorner::Full);
    QCOMPARE(tokens.scrollDistanceThreshold, 40.0);

    QCOMPARE(tokens.containerCrossExtent(MdToolbarOrientation::Horizontal), 64.0);
    QCOMPARE(tokens.containerCrossExtent(MdToolbarOrientation::Vertical), 64.0);
    QCOMPARE(tokens.externalSpaceFor(MdToolbarOrientation::Horizontal), 16.0);
    QCOMPARE(tokens.externalSpaceFor(MdToolbarOrientation::Vertical), 24.0);
}

void TestMd3Toolbar::floatingFabTable()
{
    const MdFloatingToolbarTokens tokens = MdFloatingToolbarTokens::resolve();
    const MdToolbarFabTokens &fab = tokens.fab;

    // Compose spells this 8 px `FloatingToolbarDefaults.ToolbarToFabGap` with a
    // "TODO Load this from the component tokens?" comment; the export has the
    // row, so the row wins.
    QCOMPARE(fab.betweenSpace, 8.0);

    // Expanded: the small FAB.
    QCOMPARE(fab.expandedSize, 56.0);
    QCOMPARE(fab.expandedIconSize, 24.0);
    QVERIFY(fab.expandedShape == ShapeCorner::Large);
    QVERIFY(fab.expandedElevation == ElevationLevel::Level1);

    // Collapsed: the *medium* FAB — the export's word for it is "medium".
    QCOMPARE(fab.collapsedSize, 80.0);
    QCOMPARE(fab.collapsedIconSize, 28.0);
    QVERIFY(fab.collapsedShape == ShapeCorner::LargeIncreased);
    QVERIFY(fab.collapsedElevation == ElevationLevel::Level2);

    QVERIFY(fab.standardContainerColor == ColorRole::SecondaryContainer);
    QVERIFY(fab.standardIconColor == ColorRole::OnSecondaryContainer);
    QVERIFY(fab.vibrantContainerColor == ColorRole::TertiaryContainer);
    QVERIFY(fab.vibrantIconColor == ColorRole::OnTertiaryContainer);

    // The button *grows* as the toolbar shrinks: 56 at progress 1, 80 at 0.
    QCOMPARE(fab.sizeFor(1.0), 56.0);
    QCOMPARE(fab.sizeFor(0.5), 68.0);
    QCOMPARE(fab.sizeFor(0.0), 80.0);
    QVERIFY(fab.shapeFor(1.0) == ShapeCorner::Large);
    QVERIFY(fab.shapeFor(0.4) == ShapeCorner::LargeIncreased);
}

void TestMd3Toolbar::colourSchemeTables()
{
    const MdFloatingToolbarTokens tokens = MdFloatingToolbarTokens::resolve();
    const MdToolbarScheme &standard = tokens.schemeFor(MdToolbarColorScheme::Standard);
    const MdToolbarScheme &vibrant = tokens.schemeFor(MdToolbarColorScheme::Vibrant);

    QVERIFY(standard.containerColor == ColorRole::SurfaceContainer);
    QVERIFY(vibrant.containerColor == ColorRole::PrimaryContainer);
    QVERIFY(standard.containerShape == ShapeCorner::Full);
    QVERIFY(vibrant.containerShape == ShapeCorner::Full);

    // Standard: unselected items are on-surface-variant, selected ones move to
    // on-secondary-container on a secondary-container pill.
    const MdToolbarItemColours &unselected = standard.item(false, MdToolbarItemState::Enabled);
    QVERIFY(unselected.icon.role == ColorRole::OnSurfaceVariant);
    QVERIFY(unselected.labelText.role == ColorRole::OnSurfaceVariant);
    QVERIFY(!unselected.paintsContainer());

    const MdToolbarItemColours &selected = standard.item(true, MdToolbarItemState::Enabled);
    QVERIFY(selected.paintsContainer());
    QVERIFY(selected.container.role == ColorRole::SecondaryContainer);
    QVERIFY(selected.icon.role == ColorRole::OnSecondaryContainer);

    // Vibrant is the *inverse*: the unselected items sit on a primary-container
    // pill, so they are on-primary-container, and selection inverts to a
    // surface-container chip.
    const MdToolbarItemColours &vibUnselected = vibrant.item(false, MdToolbarItemState::Enabled);
    QVERIFY(vibUnselected.icon.role == ColorRole::OnPrimaryContainer);
    const MdToolbarItemColours &vibSelected = vibrant.item(true, MdToolbarItemState::Enabled);
    QVERIFY(vibSelected.container.role == ColorRole::SurfaceContainer);
    QVERIFY(vibSelected.icon.role == ColorRole::OnSurface);
}

void TestMd3Toolbar::disabledWinsOverSelected()
{
    const MdToolbarScheme &standard =
        MdFloatingToolbarTokens::resolve().schemeFor(MdToolbarColorScheme::Standard);

    const MdToolbarItemColours &disabledUnselected =
        standard.item(false, MdToolbarItemState::Disabled);
    QVERIFY(disabledUnselected.icon.role == ColorRole::OnSurface);
    QVERIFY(disabledUnselected.labelText.role == ColorRole::OnSurface);
    QCOMPARE(disabledUnselected.icon.opacity, 0.38);
    QCOMPARE(disabledUnselected.labelText.opacity, 0.38);

    // The export publishes no selected-disabled row at all. Compose's
    // `!enabled -> disabled` precedence means a disabled selected item paints
    // the unselected disabled colours, which is what the port carries across.
    const MdToolbarItemColours &disabledSelected = standard.item(true, MdToolbarItemState::Disabled);
    QVERIFY(disabledSelected.icon.role == disabledUnselected.icon.role);
    QCOMPARE(disabledSelected.icon.opacity, disabledUnselected.icon.opacity);

    // The state-layer opacities: unselected publishes all three, selected
    // publishes none, and the port carries the unselected values over.
    const MdToolbarItemColours &hovered = standard.item(false, MdToolbarItemState::Hovered);
    QVERIFY(hovered.stateLayer == ColorRole::OnSurfaceVariant);
    QCOMPARE(hovered.stateLayerOpacityFor(MdToolbarItemState::Enabled), 0.0);
    QCOMPARE(hovered.stateLayerOpacityFor(MdToolbarItemState::Hovered), 0.08);
    QCOMPARE(hovered.stateLayerOpacityFor(MdToolbarItemState::Focused), 0.12);
    QCOMPARE(hovered.stateLayerOpacityFor(MdToolbarItemState::Pressed), 0.12);
}

void TestMd3Toolbar::tokenOverridesReachTheLayout()
{
    MdComponentTokens &global = MdComponentTokens::global();

    global.setValue(QStringLiteral("md.comp.toolbar.docked.container.max-spacing"),
                    QStringLiteral("8"));
    QCOMPARE(MdDockedToolbarTokens::resolve().containerMaxSpacing, 8.0);

    global.setValue(QStringLiteral("md.comp.toolbar.floating.container.leading-space"),
                    QStringLiteral("12px"));
    QCOMPARE(MdFloatingToolbarTokens::resolve().containerLeadingSpace, 12.0);

    // The cross extent comes from the *split* row, not the deprecated single
    // one: `container.height` is published ("Deprecating this for a vertical
    // and horizontal variant") and is what Compose still reads, but the export
    // has superseded it and a theme is expected to move to the pair.
    global.setValue(QStringLiteral("md.comp.toolbar.floating.container.height"),
                    QStringLiteral("72"));
    QCOMPARE(MdFloatingToolbarTokens::resolve().containerHeight, 72.0);
    QCOMPARE(MdFloatingToolbarTokens::resolve()
                 .containerCrossExtent(MdToolbarOrientation::Horizontal),
             64.0);
    global.setValue(QStringLiteral("md.comp.toolbar.floating.horizontal.container.height"),
                    QStringLiteral("72"));
    QCOMPARE(MdFloatingToolbarTokens::resolve()
                 .containerCrossExtent(MdToolbarOrientation::Horizontal),
             72.0);

    // A typo falls back to the published value instead of being coerced.
    global.setValue(QStringLiteral("md.comp.toolbar.floating.container.shape"),
                    QStringLiteral("corner-banana"));
    QVERIFY(MdFloatingToolbarTokens::resolve().containerShape == ShapeCorner::Full);

    // The override reaches a live widget, not just the free function.
    MdFloatingToolbar bar;
    QCOMPARE(bar.tokens().containerLeadingSpace, 12.0);
    QCOMPARE(bar.sizeHint(), QSize(12 + 8, 72));
}

// ---------------------------------------------------------------------------
// Docked
// ---------------------------------------------------------------------------

void TestMd3Toolbar::dockedGeometry()
{
    MdDockedToolbar bar;
    ProbeWidget first(QSize(48, 48));
    ProbeWidget second(QSize(48, 48));
    bar.addWidget(&first);
    bar.addWidget(&second);
    bar.resize(360, 64);

    const MdDockedToolbarTokens tokens = bar.tokens();
    const MdDockedToolbarStyle::Layout layout = MdDockedToolbarStyle::layoutFor(bar, tokens);

    QCOMPARE(layout.container, QRectF(0, 0, 360, 64));
    // The content band is inset by the published 16 px at each end and runs the
    // full height: Compose centres the row vertically rather than padding it.
    QCOMPARE(layout.content, QRectF(16, 0, 328, 64));

    // `spacedBy(32, CenterHorizontally)`: two 48 px probes and a 32 px gap make
    // 128, centred in 328.
    QCOMPARE(layout.childBoxes.size(), 2);
    QCOMPARE(layout.childBoxes.at(0), QRectF(116, 8, 48, 48));
    QCOMPARE(layout.childBoxes.at(1), QRectF(196, 8, 48, 48));
}

void TestMd3Toolbar::dockedCollapsesItsWholeHeight()
{
    MdDockedToolbar bar;
    ProbeWidget probe(QSize(48, 48));
    bar.addWidget(&probe);
    bar.resize(360, 64);

    QCOMPARE(bar.expandedHeight(), 64.0);
    // `heightOffsetLimit = -placeable.height`: the whole bar goes, not a row.
    QCOMPARE(bar.heightOffsetLimit(), -64.0);

    bar.setHeightOffset(-32.0);
    QCOMPARE(bar.collapsedFraction(), 0.5);
    QCOMPARE(bar.currentHeight(), 32.0);
    QCOMPARE(bar.height(), 32);

    const MdDockedToolbarStyle::Layout layout =
        MdDockedToolbarStyle::layoutFor(bar, bar.tokens());
    // The painted container follows the collapse...
    QCOMPARE(layout.container.height(), 32.0);
    // ...but the children were laid out against the expanded geometry, so the
    // content does not reflow as the bar shrinks — the widget's own shorter
    // rect clips it, exactly as Compose lays a `Surface` out shorter than the
    // `Row` inside it.
    QCOMPARE(layout.content, QRectF(16, 0, 328, 64));
    QCOMPARE(layout.childBoxes.at(0), QRectF(156, 8, 48, 48));

    bar.setHeightOffset(-64.0);
    QCOMPARE(bar.collapsedFraction(), 1.0);
    QCOMPARE(bar.currentHeight(), 0.0);
}

void TestMd3Toolbar::dockedChildrenUseContainers()
{
    MdDockedToolbar bar;
    auto *first = new MdIconButton;
    auto *second = new MdIconButton;
    bar.addWidget(first);
    bar.addWidget(second);
    bar.resize(400, 64);

    // An icon button is a 55 px widget around a 40 px container; the row must
    // advance by the container.
    const MdChildBox measured = MdChildBox::measure(first);
    QCOMPARE(first->sizeHint(), QSize(55, 55));
    QCOMPARE(measured.containerSize(), QSizeF(40, 40));

    const MdDockedToolbarStyle::Layout layout =
        MdDockedToolbarStyle::layoutFor(bar, bar.tokens());
    QCOMPARE(layout.childBoxes.size(), 2);
    // Band is (16, 0, 368, 64); two 40 px containers and a 32 px gap make 112.
    QCOMPARE(layout.childBoxes.at(0), QRectF(144, 12, 40, 40));
    QCOMPARE(layout.childBoxes.at(1), QRectF(216, 12, 40, 40));

    // The widgets are placed so that those *containers* land there: the pitch
    // is 40 + 32 = 72, not 55 + 32 = 87.
    QVERIFY(std::abs((second->pos().x() - first->pos().x()) - 72) <= 1);
}

void TestMd3Toolbar::dockedPaintsItsContainer()
{
    MdDockedToolbar bar;
    bar.resize(200, 64);

    const QColor container = MdTheme::instance().color(ColorRole::SurfaceContainer);
    QCOMPARE(pixelColorAt(bar, QPointF(100, 32)), container);
    // `container.shape` is corner-none, so even the corner is the container's
    // own — unlike a floating toolbar's pill.
    QCOMPARE(pixelColorAt(bar, QPointF(0, 0)), container);
}

// ---------------------------------------------------------------------------
// Floating
// ---------------------------------------------------------------------------

void TestMd3Toolbar::floatingPillGeometry()
{
    MdFloatingToolbar bar;
    ProbeWidget content(QSize(48, 48));
    bar.addContentWidget(&content);

    // 8 px padding a side around a 48 px slot; 64 px across.
    QCOMPARE(bar.sizeHint(), QSize(64, 64));

    const MdFloatingToolbarStyle::Layout layout =
        MdFloatingToolbarStyle::layoutFor(bar, bar.tokens());
    QCOMPARE(layout.container, QRectF(0, 0, 64, 64));
    QCOMPARE(layout.content, QRectF(8, 8, 48, 48));
    QCOMPARE(layout.contentChildBoxes.size(), 1);
    QCOMPARE(layout.contentChildBoxes.at(0), QRectF(8, 8, 48, 48));
    // `corner-full`: a pill, half the container's 64 px height.
    QCOMPARE(layout.radii.size(), 4);
    QCOMPARE(layout.radii.first(), 32.0);
}

void TestMd3Toolbar::betweenSpaceSeparatesTheSlots()
{
    // `container.between-space` is the one published row this port reads and
    // Compose does not: upstream's toolbar items are arranged by the caller's
    // `Row`, so `FloatingToolbarTokens.ContainerBetweenSpace` is referenced
    // nowhere in `FloatingToolbar.kt`. This widget *is* the arrangement, so the
    // row has to reach the layout — and has to stay reachable, which is what a
    // token test alone would not have caught (the row was resolved and asserted
    // for a whole round while the layout ignored it).
    MdFloatingToolbar bar;
    ProbeWidget first(QSize(48, 48));
    ProbeWidget second(QSize(48, 48));
    ProbeWidget third(QSize(48, 48));
    bar.addContentWidget(&first);
    bar.addContentWidget(&second);
    bar.addContentWidget(&third);

    // 8 + 48 + 4 + 48 + 4 + 48 + 8.
    QCOMPARE(bar.sizeHint(), QSize(168, 64));

    MdFloatingToolbarStyle::Layout layout = MdFloatingToolbarStyle::layoutFor(bar, bar.tokens());
    QCOMPARE(layout.contentChildBoxes.at(0), QRectF(8, 8, 48, 48));
    QCOMPARE(layout.contentChildBoxes.at(1), QRectF(60, 8, 48, 48));
    QCOMPARE(layout.contentChildBoxes.at(2), QRectF(112, 8, 48, 48));
    // The run is `n - 1` gaps long, never `n`: a trailing gap would push the
    // centred run half a gap off centre, which the pill's own edges would show.
    QCOMPARE(layout.container.right(), 168.0);
    QCOMPARE(layout.contentChildBoxes.constLast().right(), 160.0);

    // The override reaches the layout, so the row cannot quietly stop being
    // read again. A toolbar resolves its tokens when it is built, so this half
    // needs a fresh widget with the override already in place — the same order
    // `tokenOverridesReachTheLayout` uses.
    MdComponentTokens::global().setValue(
        QStringLiteral("md.comp.toolbar.floating.container.between-space"),
        QStringLiteral("12px"));
    {
        MdFloatingToolbar wider;
        ProbeWidget a(QSize(48, 48));
        ProbeWidget b(QSize(48, 48));
        ProbeWidget c(QSize(48, 48));
        wider.addContentWidget(&a);
        wider.addContentWidget(&b);
        wider.addContentWidget(&c);

        QCOMPARE(wider.tokens().containerBetweenSpace, 12.0);
        // 8 + 48 + 12 + 48 + 12 + 48 + 8.
        QCOMPARE(wider.sizeHint(), QSize(184, 64));
        const MdFloatingToolbarStyle::Layout wide =
            MdFloatingToolbarStyle::layoutFor(wider, wider.tokens());
        QCOMPARE(wide.contentChildBoxes.at(1), QRectF(68, 8, 48, 48));
        QCOMPARE(wide.contentChildBoxes.at(2), QRectF(128, 8, 48, 48));
        QCOMPARE(wide.container.right(), 184.0);
    }
}

void TestMd3Toolbar::verticalIsTheTranspose()
{
    // The container is declared before the probes on purpose: a QWidget
    // deletes its children, so a stack child must not outlive its parent.
    MdFloatingToolbar horizontal;
    ProbeWidget first(QSize(48, 48));
    ProbeWidget second(QSize(48, 48));
    horizontal.addContentWidget(&first);
    horizontal.addContentWidget(&second);
    // 8 + 48 + 4 + 48 + 8.
    QCOMPARE(horizontal.sizeHint(), QSize(116, 64));
    const MdFloatingToolbarStyle::Layout hLayout =
        MdFloatingToolbarStyle::layoutFor(horizontal, horizontal.tokens());
    QCOMPARE(hLayout.contentChildBoxes.at(0), QRectF(8, 8, 48, 48));
    QCOMPARE(hLayout.contentChildBoxes.at(1), QRectF(60, 8, 48, 48));

    MdFloatingToolbar vertical;
    vertical.setOrientation(MdToolbarOrientation::Vertical);
    ProbeWidget third(QSize(48, 48));
    ProbeWidget fourth(QSize(48, 48));
    vertical.addContentWidget(&third);
    vertical.addContentWidget(&fourth);
    // Exactly the transpose: the same numbers with the axes swapped.
    QCOMPARE(vertical.sizeHint(), QSize(64, 116));
    const MdFloatingToolbarStyle::Layout vLayout =
        MdFloatingToolbarStyle::layoutFor(vertical, vertical.tokens());
    QCOMPARE(vLayout.container, QRectF(0, 0, 64, 116));
    QCOMPARE(vLayout.contentChildBoxes.at(0), QRectF(8, 8, 48, 48));
    QCOMPARE(vLayout.contentChildBoxes.at(1), QRectF(8, 60, 48, 48));
}

void TestMd3Toolbar::leadingAndTrailingExistOnlyWhenExpanded()
{
    MdFloatingToolbar bar;
    ProbeWidget leading(QSize(48, 48));
    ProbeWidget content(QSize(48, 48));
    ProbeWidget trailing(QSize(48, 48));
    bar.setLeadingWidget(&leading);
    bar.addContentWidget(&content);
    bar.setTrailingWidget(&trailing);

    // Three slots, 8 px a side and a 4 px gap between neighbours.
    QCOMPARE(bar.sizeHint(), QSize(168, 64));

    const MdFloatingToolbarTokens tokens = bar.tokens();
    MdFloatingToolbarStyle::Layout layout = MdFloatingToolbarStyle::layoutFor(bar, tokens);
    QCOMPARE(layout.leadingChildBoxes.size(), 1);
    QCOMPARE(layout.leadingChildBoxes.at(0), QRectF(8, 8, 48, 48));
    QCOMPARE(layout.contentChildBoxes.at(0), QRectF(60, 8, 48, 48));
    QCOMPARE(layout.trailingChildBoxes.at(0), QRectF(112, 8, 48, 48));

    bar.setExpansionProgress(0.0);
    layout = MdFloatingToolbarStyle::layoutFor(bar, tokens);
    // `AnimatedVisibility(visible = expandedState)`: the extra actions are gone
    // from the layout, so the content re-centres in the band...
    QVERIFY(layout.leadingChildBoxes.isEmpty());
    QVERIFY(layout.trailingChildBoxes.isEmpty());
    QCOMPARE(layout.contentChildBoxes.at(0), QRectF(60, 8, 48, 48));
    // ...and the pill has no main-axis extent left.
    QCOMPARE(layout.container.width(), 0.0);
    QVERIFY(layout.container.isEmpty());
    // The widget's own bounds do not move: Compose sizes the outer Layout from
    // the toolbar's *maximum* intrinsic width and only shortens the inside.
    QCOMPARE(bar.size(), QSize(168, 64));
    QCOMPARE(bar.sizeHint(), QSize(168, 64));
}

void TestMd3Toolbar::expandedProgressLengthsThePill()
{
    MdFloatingToolbar bar;
    ProbeWidget content(QSize(48, 48));
    bar.addContentWidget(&content);
    const MdFloatingToolbarTokens tokens = bar.tokens();

    bar.setExpansionProgress(1.0);
    MdFloatingToolbarStyle::Layout layout = MdFloatingToolbarStyle::layoutFor(bar, tokens);
    QCOMPARE(layout.container, QRectF(0, 0, 64, 64));

    // `targetWidth = maxIntrinsicWidth * expandedProgress`.
    bar.setExpansionProgress(0.5);
    layout = MdFloatingToolbarStyle::layoutFor(bar, tokens);
    QCOMPARE(layout.container.width(), 32.0);
    QCOMPARE(layout.container.height(), 64.0);

    bar.setExpansionProgress(0.25);
    layout = MdFloatingToolbarStyle::layoutFor(bar, tokens);
    QCOMPARE(layout.container.width(), 16.0);
    // The pill keeps its trailing edge and sweeps its leading one, which is
    // Compose's `toolbarX = maxToolbarWidth - toolbarWidth`.
    QCOMPARE(layout.container.left(), 48.0);

    // The band is always the expanded one: the content never reflows.
    QCOMPARE(layout.content, QRectF(8, 8, 48, 48));
}

void TestMd3Toolbar::floatingFabStripAndSizeSets()
{
    MdFloatingToolbar bar;
    ProbeWidget content(QSize(48, 48));
    ProbeWidget fab(QSize(56, 56));
    bar.addContentWidget(&content);
    bar.setFab(&fab);
    const MdFloatingToolbarTokens tokens = bar.tokens();

    // Pill (8 + 48 + 8 = 64) plus the reserved strip (between-space 8 +
    // *expanded* size 56 = 64); the cross extent is lifted to the collapsed
    // FAB's 80 by `defaultMinSize(minHeight = FabSizeRange.endInclusive)`.
    QCOMPARE(bar.sizeHint(), QSize(128, 80));

    MdFloatingToolbarStyle::Layout layout = MdFloatingToolbarStyle::layoutFor(bar, tokens);
    // The pill is 64 px across and centred in the 80 px the FAB raises the
    // cross extent to (`toolbarTopOffset = (height - toolbarHeight) / 2`).
    QCOMPARE(layout.container, QRectF(0, 8, 64, 64));
    // The default position is End: the FAB takes the trailing edge and the pill
    // keeps its own trailing edge fixed.
    QCOMPARE(layout.fabBox, QRectF(72, 12, 56, 56));

    bar.setExpansionProgress(0.0);
    layout = MdFloatingToolbarStyle::layoutFor(bar, tokens);
    QCOMPARE(layout.container.width(), 0.0);
    QCOMPARE(layout.container.left(), 64.0);
    // Collapsed, the FAB is the 80 px medium one, and this is where it lands:
    // `width - 80` really is 16 px inside `width - 64`, so it overlaps the
    // pill's ruins. Compose's arithmetic, not an error.
    QCOMPARE(layout.fabBox, QRectF(48, 0, 80, 80));
    QVERIFY(layout.fabBox.left() < layout.container.right());

    // The transpose, still at `End` (which is `Bottom` for a vertical toolbar).
    MdFloatingToolbar vertical;
    vertical.setOrientation(MdToolbarOrientation::Vertical);
    ProbeWidget vContent(QSize(48, 48));
    ProbeWidget vFab(QSize(56, 56));
    vertical.addContentWidget(&vContent);
    vertical.setFab(&vFab);
    QCOMPARE(vertical.sizeHint(), QSize(80, 128));
    const MdFloatingToolbarStyle::Layout vLayout =
        MdFloatingToolbarStyle::layoutFor(vertical, vertical.tokens());
    QCOMPARE(vLayout.container, QRectF(8, 0, 64, 64));
    QCOMPARE(vLayout.fabBox, QRectF(12, 72, 56, 56));
}

void TestMd3Toolbar::floatingFabPosition()
{
    MdFloatingToolbar bar;
    ProbeWidget content(QSize(48, 48));
    ProbeWidget fab(QSize(56, 56));
    bar.addContentWidget(&content);
    bar.setFab(&fab);
    bar.setFabPosition(MdToolbarFabPosition::Start);

    const MdFloatingToolbarStyle::Layout layout =
        MdFloatingToolbarStyle::layoutFor(bar, bar.tokens());
    // Compose: `toolbarX = width - maxToolbarWidth` when the FAB is at the
    // start, i.e. the pill begins *after* the strip.
    QCOMPARE(layout.fabBox, QRectF(0, 12, 56, 56));
    QCOMPARE(layout.container, QRectF(64, 8, 64, 64));
    QCOMPARE(bar.sizeHint(), QSize(128, 80));
}

void TestMd3Toolbar::floatingFabIsAskedToTakeTheTokenSize()
{
    MdFloatingToolbar bar;
    ProbeWidget content(QSize(48, 48));
    ProbeWidget fab(QSize(56, 56));
    bar.addContentWidget(&content);
    bar.setFab(&fab);

    // A widget with no container concept of its own fills whatever box the
    // toolbar hands it — which is how the 56 / 80 transition is expressed.
    QCOMPARE(fab.size(), QSize(56, 56));
    bar.setExpansionProgress(1.0);
    QCOMPARE(fab.size(), QSize(56, 56));

    bar.setExpansionProgress(0.0);
    QCOMPARE(fab.size(), QSize(80, 80));

    // An `MdFab` does *not*: its container is a fixed pair of tokens
    // (`containerWidth x containerHeight`), so it stays 56 px centred in the
    // 80 px box rather than growing. That is the FAB family's gap, recorded in
    // docs/porting-todo.md — the toolbar's box is right, the child's metrics
    // are not there yet.
    MdFloatingToolbar bar2;
    ProbeWidget content2(QSize(48, 48));
    MdFab realFab;
    bar2.addContentWidget(&content2);
    bar2.setFab(&realFab);
    bar2.setExpansionProgress(0.0);

    const MdFloatingToolbarStyle::Layout layout =
        MdFloatingToolbarStyle::layoutFor(bar2, bar2.tokens());
    QCOMPARE(layout.fabBox.size(), QSizeF(80, 80));
    QCOMPARE(realFab.containerRect().size(), QSizeF(56, 56));
    // ...but it is still centred on the box it could not fill, not adrift.
    const QRectF placed = realFab.containerRect().translated(realFab.pos());
    QVERIFY(std::abs(placed.center().x() - layout.fabBox.center().x()) <= 1.0);
    QVERIFY(std::abs(placed.center().y() - layout.fabBox.center().y()) <= 1.0);
}

void TestMd3Toolbar::floatingChildrenUseContainers()
{
    MdFloatingToolbar bar;
    auto *first = new MdIconButton;
    auto *second = new MdIconButton;
    bar.addContentWidget(first);
    bar.addContentWidget(second);

    QCOMPARE(first->sizeHint(), QSize(55, 55));
    QCOMPARE(MdChildBox::measure(first).containerSize(), QSizeF(40, 40));

    // 8 + 40 + 4 + 40 + 8. Advancing by the widgets would have made this 130.
    QCOMPARE(bar.sizeHint(), QSize(100, 64));

    const MdFloatingToolbarStyle::Layout layout =
        MdFloatingToolbarStyle::layoutFor(bar, bar.tokens());
    QCOMPARE(layout.contentChildBoxes.at(0), QRectF(8, 12, 40, 40));
    QCOMPARE(layout.contentChildBoxes.at(1), QRectF(52, 12, 40, 40));
    QVERIFY(std::abs((second->pos().x() - first->pos().x()) - 44) <= 1);
}

void TestMd3Toolbar::floatingPaintsItsScheme()
{
    MdFloatingToolbar bar;
    ProbeWidget content(QSize(48, 48));
    bar.addContentWidget(&content);

    // Standard: `surface-container`.
    QCOMPARE(pixelColorAt(bar, QPointF(32, 32)),
             MdTheme::instance().color(ColorRole::SurfaceContainer));
    // `corner-full`, so the corner is outside the pill and stays transparent.
    QCOMPARE(pixelColorAt(bar, QPointF(0, 0)), QColor(0, 0, 0, 0));

    // Vibrant: `primary-container`.
    bar.setColorScheme(MdToolbarColorScheme::Vibrant);
    QCOMPARE(pixelColorAt(bar, QPointF(32, 32)),
             MdTheme::instance().color(ColorRole::PrimaryContainer));
    QCOMPARE(pixelColorAt(bar, QPointF(0, 0)), QColor(0, 0, 0, 0));
}

// ---------------------------------------------------------------------------
// Cross-cutting
// ---------------------------------------------------------------------------

void TestMd3Toolbar::rtlIsNotMirrored()
{
    // A library-wide gap, not a toolbar defect: `MdTheme::isRightToLeft()`
    // reaches only the four button families and nothing here mirrors. The test
    // pins the *current* behaviour so a future RTL pass has to change it
    // deliberately. Recorded in docs/porting-todo.md.
    MdFloatingToolbar bar;
    ProbeWidget first(QSize(48, 48));
    ProbeWidget second(QSize(48, 48));
    bar.addContentWidget(&first);
    bar.addContentWidget(&second);
    bar.setLayoutDirection(Qt::RightToLeft);

    const MdFloatingToolbarStyle::Layout layout =
        MdFloatingToolbarStyle::layoutFor(bar, bar.tokens());
    QCOMPARE(layout.contentChildBoxes.at(0), QRectF(8, 8, 48, 48));
    QCOMPARE(layout.contentChildBoxes.at(1), QRectF(60, 8, 48, 48));

    MdDockedToolbar docked;
    ProbeWidget probe(QSize(48, 48));
    docked.addWidget(&probe);
    docked.resize(360, 64);
    docked.setLayoutDirection(Qt::RightToLeft);

    const MdDockedToolbarStyle::Layout dockedLayout =
        MdDockedToolbarStyle::layoutFor(docked, docked.tokens());
    QCOMPARE(dockedLayout.childBoxes.at(0), QRectF(156, 8, 48, 48));
}

QTEST_MAIN(TestMd3Toolbar)
#include "TestMd3Toolbar.moc"
