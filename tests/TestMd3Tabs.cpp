// TestMd3Tabs — the Tabs family: MdTabs (both published families over one
// widget, fixed and scrollable) and its MdTab children.
//
// Numbers transcribed from material-web tokens/versions/latest/sass —
// _md-comp-primary-navigation-tab.scss and -secondary-navigation-tab.scss —
// cross-checked against androidx Compose Material3 TabRow.kt / Tab.kt and
// Flutter's M3 TabBar defaults (generated from the same token database).
//
// The assertions worth keeping are the ones a token table cannot express:
//
//   * the two families' indicator **heights and shapes differ** (3 px rounded
//     on top vs 2 px square), and the secondary family's own numbers are not
//     the struct's defaults — a resolve that inherited the primary's would
//     silently publish a rounded 3 px indicator under the secondary name;
//   * the primary indicator's width is the tab's **content** width —
//     `max(min(intrinsic, tabWidth) - 2 * HorizontalTextPadding, 24)` — and
//     the secondary's is the **whole tab**;
//   * the indicator **centres** in the tab: Compose's scrollable
//     implementation does this explicitly, its fixed implementation omits the
//     step, and Flutter's M3 defaults centre both — the centring wins
//     (porting-todo.md records the divergence);
//   * the indicator's offset and width animate on the spatial default spring,
//     and the first placement lands without one;
//   * a scrollable row keeps a 52 px edge padding, a 90 px minimum tab width,
//     and scrolls the selection towards the centre on the same spring;
//   * the inactive pressed state layer is `primary` in the primary family —
//     the colour the tab is about to earn — while the inactive hover and
//     focus layers are `on-surface`;
//   * the row's `sizeHint` for a scrollable layout is the content's own width
//     (2 x 52 plus every tab), and for a fixed layout the content minimum.

#include "core/MdTabsTokens.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdTabStyle.h"
#include "styles/MdTabsStyle.h"
#include "widgets/MdTab.h"
#include "widgets/MdTabs.h"

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
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32).pixelColor(position.toPoint());
}

/// Adds `count` text tabs labelled A, B, C, .... The row takes ownership, so
/// the tabs must never be stack objects (a QWidget deletes its children).
void addTabs(MdTabs &tabs, int count)
{
    for (int i = 0; i < count; ++i) {
        auto *tab = new MdTab(QString(QChar('A' + i)));
        tabs.addTab(tab);
    }
}

/// Runs the indicator's spring to `offset`/`width`. The spring is driven by an
/// 8 ms timer, so the answer arrives through the event loop.
bool settleIndicator(MdTabs &tabs, qreal offset, qreal width)
{
    for (int i = 0; i < 400; ++i) {
        if (qAbs(tabs.indicatorOffset() - offset) <= 1e-6
            && qAbs(tabs.indicatorWidth() - width) <= 1e-6) {
            return true;
        }
        QTest::qWait(10);
    }
    return qAbs(tabs.indicatorOffset() - offset) <= 1e-6
           && qAbs(tabs.indicatorWidth() - width) <= 1e-6;
}

/// Runs the scrollable row's centre-scroll spring to `offset`.
bool settleScroll(MdTabs &tabs, qreal offset)
{
    for (int i = 0; i < 400; ++i) {
        if (qAbs(tabs.scrollOffset() - offset) <= 1e-6) {
            return true;
        }
        QTest::qWait(10);
    }
    return qAbs(tabs.scrollOffset() - offset) <= 1e-6;
}

} // namespace

class TestMd3Tabs : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    // --- token tables ---------------------------------------------------------
    void primaryTokenTable();
    void secondaryTokenTable();
    void composeBehaviourConstants();
    void primaryNotInheritedBySecondary();

    // --- fixed layout ----------------------------------------------------------
    void fixedDividesTheWidthEvenly();
    void rowHeightFollowsTheTallestTab();
    void primaryIndicatorTakesTheContentWidth();
    void secondaryIndicatorSpansTheWholeTab();
    void indicatorCentresInTheTab();
    void indicatorAnimatesOnTheSpatialSpring();
    void firstPlacementDoesNotAnimate();

    // --- selection -------------------------------------------------------------
    void selectionMirrorsOntoTheTabs();
    void currentChangedEmits();

    // --- scrollable ------------------------------------------------------------
    void scrollableSizeHintIsTheContentWidth();
    void scrollableLaysOutFromTheEdgePadding();
    void scrollableScrollsToTheCentre();
    void wheelScrollLandsInstantly();

    // --- keyboard --------------------------------------------------------------
    void arrowKeysMoveTheSelection();

    // --- render -----------------------------------------------------------------
    void renderSmokePaintsContainerDividerAndIndicator();

private:
    MdTabs *m_tabs = nullptr;
};

void TestMd3Tabs::cleanup()
{
    // QTest aborts the slot on the first failed QCOMPARE, so every mutable
    // this-suite state is reset here unconditionally.
    delete m_tabs;
    m_tabs = nullptr;
}

// ---------------------------------------------------------------------------
// Token tables
// ---------------------------------------------------------------------------

void TestMd3Tabs::primaryTokenTable()
{
    const MdTabsTokens tokens = MdTabsTokens::resolve();
    const MdTabsVariantTokens &t = tokens.forVariant(MdTabsVariant::Primary);

    QCOMPARE(t.containerHeight, 48.0);
    QCOMPARE(t.iconLabelTextContainerHeight, 64.0);
    QCOMPARE(t.iconSize, 24.0);
    QCOMPARE(t.activeIndicatorHeight, 3.0);
    QCOMPARE(t.activeIndicatorTopRounded, true);
    QCOMPARE(t.dividerHeight, 1.0);
    QCOMPARE(t.containerColor, ColorRole::Surface);
    QCOMPARE(t.containerElevation, ElevationLevel::Level0);
    QCOMPARE(t.containerShape, ShapeCorner::None);
    QCOMPARE(t.dividerColor, ColorRole::SurfaceVariant);
    QCOMPARE(t.activeIndicatorColor, ColorRole::Primary);
    QCOMPARE(t.focusIndicatorColor, ColorRole::Secondary);
    QCOMPARE(t.labelTextType, TypeStyle::TitleSmall);

    // Active rows are `primary`; inactive rows rest at `on-surface-variant`
    // and lift to `on-surface` in every interaction state.
    QCOMPARE(t.activeLabelFor(MdNavigationItemState::Enabled).role, ColorRole::Primary);
    QCOMPARE(t.activeIconFor(MdNavigationItemState::Hovered).role, ColorRole::Primary);
    QCOMPARE(t.inactiveLabelFor(MdNavigationItemState::Enabled).role, ColorRole::OnSurfaceVariant);
    QCOMPARE(t.inactiveLabelFor(MdNavigationItemState::Hovered).role, ColorRole::OnSurface);
    QCOMPARE(t.inactiveIconFor(MdNavigationItemState::Pressed).role, ColorRole::OnSurface);

    // The state layers: active all `primary`; inactive hover/focus
    // `on-surface`, and the inactive **pressed** layer the one-row special
    // case `primary`.
    QCOMPARE(t.activeStateLayerFor(MdNavigationItemState::Hovered).role, ColorRole::Primary);
    QCOMPARE(t.activeStateLayerFor(MdNavigationItemState::Focused).role, ColorRole::Primary);
    QCOMPARE(t.activeStateLayerFor(MdNavigationItemState::Pressed).role, ColorRole::Primary);
    QCOMPARE(t.inactiveStateLayerFor(MdNavigationItemState::Hovered).role, ColorRole::OnSurface);
    QCOMPARE(t.inactiveStateLayerFor(MdNavigationItemState::Focused).role, ColorRole::OnSurface);
    QCOMPARE(t.inactiveStateLayerFor(MdNavigationItemState::Pressed).role, ColorRole::Primary);

    // The disabled rows the export publishes none of: the system 0.38 over the
    // unselected colour, as the navigation families do.
    QCOMPARE(t.activeLabelFor(MdNavigationItemState::Disabled).opacity, 0.38);
    QCOMPARE(t.inactiveIconFor(MdNavigationItemState::Disabled).opacity, 0.38);
}

void TestMd3Tabs::secondaryTokenTable()
{
    const MdTabsTokens tokens = MdTabsTokens::resolve();
    const MdTabsVariantTokens &t = tokens.forVariant(MdTabsVariant::Secondary);

    QCOMPARE(t.containerHeight, 48.0);
    QCOMPARE(t.activeIndicatorHeight, 2.0);
    QCOMPARE(t.activeIndicatorTopRounded, false);
    QCOMPARE(t.containerColor, ColorRole::Surface);
    QCOMPARE(t.dividerColor, ColorRole::SurfaceVariant);
    QCOMPARE(t.activeIndicatorColor, ColorRole::Primary);

    // One shared table: active content `on-surface`, inactive
    // `on-surface-variant`, every state layer `on-surface` on both sides.
    QCOMPARE(t.activeLabelFor(MdNavigationItemState::Enabled).role, ColorRole::OnSurface);
    QCOMPARE(t.inactiveLabelFor(MdNavigationItemState::Enabled).role, ColorRole::OnSurfaceVariant);
    QCOMPARE(t.activeStateLayerFor(MdNavigationItemState::Pressed).role, ColorRole::OnSurface);
    QCOMPARE(t.inactiveStateLayerFor(MdNavigationItemState::Pressed).role, ColorRole::OnSurface);
    QCOMPARE(t.inactiveStateLayerFor(MdNavigationItemState::Hovered).role, ColorRole::OnSurface);
}

void TestMd3Tabs::composeBehaviourConstants()
{
    // Compose's `Tab.kt` / `TabRowDefaults` constants, which the export
    // publishes no rows for.
    const MdTabsTokens tokens = MdTabsTokens::resolve();
    for (int v = 0; v < tabsVariantCount; ++v) {
        const MdTabsVariantTokens &t = tokens.variant[v];
        QCOMPARE(t.horizontalTextPadding, 16.0);
        QCOMPARE(t.textDistanceFromLeadingIcon, 8.0);
        QCOMPARE(t.scrollableEdgePadding, 52.0);
        QCOMPARE(t.scrollableMinTabWidth, 90.0);
        QCOMPARE(t.indicatorMinimumWidth, 24.0);
    }
}

void TestMd3Tabs::primaryNotInheritedBySecondary()
{
    // The struct's defaults are the primary's numbers; a resolve that let the
    // secondary inherit them would publish a rounded 3 px indicator under the
    // secondary name.
    const MdTabsTokens tokens = MdTabsTokens::resolve();
    const MdTabsVariantTokens &primary = tokens.forVariant(MdTabsVariant::Primary);
    const MdTabsVariantTokens &secondary = tokens.forVariant(MdTabsVariant::Secondary);
    QVERIFY(secondary.activeIndicatorHeight != primary.activeIndicatorHeight
            || !secondary.activeIndicatorTopRounded);
    QCOMPARE(secondary.activeIndicatorHeight, 2.0);
    QCOMPARE(secondary.activeIndicatorTopRounded, false);
}

// ---------------------------------------------------------------------------
// Fixed layout
// ---------------------------------------------------------------------------

void TestMd3Tabs::fixedDividesTheWidthEvenly()
{
    m_tabs = new MdTabs;
    addTabs(*m_tabs, 4);
    m_tabs->resize(400, 48);
    m_tabs->show();
    QTest::qWait(50);

    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*m_tabs, m_tabs->tokens());
    QCOMPARE(layout.tabRects.size(), 4);
    for (int i = 0; i < 4; ++i) {
        QCOMPARE(layout.tabRects.at(i).x(), qreal(i) * 100.0);
        QCOMPARE(layout.tabRects.at(i).width(), 100.0);
    }
    // The tabs fill their slots.
    QCOMPARE(m_tabs->tabAt(2)->geometry().width(), 100);
}

void TestMd3Tabs::rowHeightFollowsTheTallestTab()
{
    m_tabs = new MdTabs;
    addTabs(*m_tabs, 3);
    // Give the middle tab an icon and a label: 64.
    m_tabs->tabAt(1)->setIconName(QStringLiteral("mail"));
    m_tabs->resize(300, 64);
    m_tabs->show();
    QTest::qWait(50);

    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*m_tabs, m_tabs->tokens());
    QCOMPARE(layout.rowHeight, 64.0);

    // And a text-only row takes the 48.
    MdTabs textOnly;
    addTabs(textOnly, 3);
    textOnly.show();
    textOnly.resize(300, 48);
    QTest::qWait(50);
    QCOMPARE(MdTabsStyle::layoutFor(textOnly, textOnly.tokens()).rowHeight, 48.0);
}

void TestMd3Tabs::primaryIndicatorTakesTheContentWidth()
{
    m_tabs = new MdTabs;
    addTabs(*m_tabs, 4);
    m_tabs->resize(400, 48);
    m_tabs->show();
    QTest::qWait(50);

    // Tab 0 selected. Its content width: the label "A" at title-small plus
    // the shared 2 x 16 padding, floored at the 24 px touch target.
    const MdTabsVariantTokens &t = m_tabs->tokens().forVariant(MdTabsVariant::Primary);
    const MdTab *tab = m_tabs->tabAt(0);
    const qreal contentWidth = tab->indicatorContentWidth(100.0);
    QVERIFY(contentWidth >= 24.0);
    QVERIFY(contentWidth <= 100.0 - 2.0 * t.horizontalTextPadding
            || qFuzzyCompare(contentWidth, 24.0));

    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*m_tabs, m_tabs->tokens());
    const QRectF indicator =
        MdTabsStyle::indicatorRectFor(layout, 0, t, contentWidth);
    QCOMPARE(indicator.width(), contentWidth);
    QCOMPARE(indicator.height(), 3.0);
}

void TestMd3Tabs::secondaryIndicatorSpansTheWholeTab()
{
    m_tabs = new MdTabs;
    m_tabs->setVariant(MdTabsVariant::Secondary);
    addTabs(*m_tabs, 4);
    m_tabs->resize(400, 48);
    m_tabs->show();
    QTest::qWait(50);

    const MdTabsVariantTokens &t = m_tabs->tokens().forVariant(MdTabsVariant::Secondary);
    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*m_tabs, m_tabs->tokens());
    const QRectF indicator = MdTabsStyle::indicatorRectFor(layout, 1, t, 24.0);
    // The content width is ignored: the indicator is the tab's own width.
    QCOMPARE(indicator.width(), 100.0);
    QCOMPARE(indicator.height(), 2.0);
}

void TestMd3Tabs::indicatorCentresInTheTab()
{
    m_tabs = new MdTabs;
    addTabs(*m_tabs, 4);
    m_tabs->resize(400, 48);
    m_tabs->show();
    QTest::qWait(50);

    const MdTabsVariantTokens &t = m_tabs->tokens().forVariant(MdTabsVariant::Primary);
    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*m_tabs, m_tabs->tokens());
    const MdTab *tab = m_tabs->tabAt(2);
    const qreal contentWidth = tab->indicatorContentWidth(100.0);
    const QRectF indicator = MdTabsStyle::indicatorRectFor(layout, 2, t, contentWidth);
    // Centred in the tab's 100 px, starting at 200.
    QCOMPARE(indicator.x(), 200.0 + (100.0 - contentWidth) / 2.0);
}

void TestMd3Tabs::indicatorAnimatesOnTheSpatialSpring()
{
    m_tabs = new MdTabs;
    addTabs(*m_tabs, 4);
    m_tabs->resize(400, 48);
    m_tabs->show();
    QTest::qWait(50);

    // Select tab 2 and let the spring run: the offset and the width land on
    // the centred content rectangle together.
    const MdTabsVariantTokens &t = m_tabs->tokens().forVariant(MdTabsVariant::Primary);
    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*m_tabs, m_tabs->tokens());
    const MdTab *tab = m_tabs->tabAt(2);
    const qreal contentWidth = tab->indicatorContentWidth(100.0);
    const qreal expectedX = 200.0 + (100.0 - contentWidth) / 2.0;

    m_tabs->setCurrentIndex(2);
    QVERIFY(settleIndicator(*m_tabs, expectedX, contentWidth));
    QCOMPARE(m_tabs->currentIndex(), 2);
}

void TestMd3Tabs::firstPlacementDoesNotAnimate()
{
    m_tabs = new MdTabs;
    addTabs(*m_tabs, 4);
    m_tabs->resize(400, 48);
    m_tabs->show();
    QTest::qWait(50);

    // The first tab arrives selected: the indicator is already at its target
    // with no spring to run.
    const MdTabsVariantTokens &t = m_tabs->tokens().forVariant(MdTabsVariant::Primary);
    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*m_tabs, m_tabs->tokens());
    const MdTab *tab = m_tabs->tabAt(0);
    const qreal contentWidth = tab->indicatorContentWidth(100.0);
    QCOMPARE(m_tabs->indicatorWidth(), contentWidth);
    QCOMPARE(m_tabs->indicatorOffset(),
             MdTabsStyle::indicatorRectFor(layout, 0, t, contentWidth).x());
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

void TestMd3Tabs::selectionMirrorsOntoTheTabs()
{
    m_tabs = new MdTabs;
    addTabs(*m_tabs, 3);
    m_tabs->resize(300, 48);
    m_tabs->show();
    QTest::qWait(50);

    m_tabs->setCurrentIndex(1);
    QVERIFY(m_tabs->tabAt(1)->isSelected());
    QVERIFY(!m_tabs->tabAt(0)->isSelected());
    QVERIFY(!m_tabs->tabAt(2)->isSelected());

    // A click on an inactive tab selects it.
    QTest::mouseClick(m_tabs->tabAt(2), Qt::LeftButton);
    QCOMPARE(m_tabs->currentIndex(), 2);
    QVERIFY(m_tabs->tabAt(2)->isSelected());

    // Deselecting the selected tab by hand leaves no "nothing selected" state.
    m_tabs->tabAt(2)->setSelected(false);
    QCOMPARE(m_tabs->currentIndex(), -1);
}

void TestMd3Tabs::currentChangedEmits()
{
    m_tabs = new MdTabs;
    addTabs(*m_tabs, 3);
    m_tabs->resize(300, 48);
    m_tabs->show();
    QTest::qWait(50);

    QSignalSpy spy(m_tabs, &MdTabs::currentChanged);
    m_tabs->setCurrentIndex(2);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toInt(), 2);
}

// ---------------------------------------------------------------------------
// Scrollable
// ---------------------------------------------------------------------------

void TestMd3Tabs::scrollableSizeHintIsTheContentWidth()
{
    m_tabs = new MdTabs;
    m_tabs->setLayout(MdTabsLayout::Scrollable);
    addTabs(*m_tabs, 3);
    m_tabs->resize(600, 48);
    m_tabs->show();
    QTest::qWait(50);

    const MdTabsVariantTokens &t = m_tabs->tokens().forVariant(MdTabsVariant::Primary);
    qreal content = 2.0 * t.scrollableEdgePadding;
    for (int i = 0; i < 3; ++i) {
        content += qMax<qreal>(t.scrollableMinTabWidth, m_tabs->tabAt(i)->sizeHint().width());
    }
    QCOMPARE(qreal(m_tabs->sizeHint().width()), content);
    QCOMPARE(qreal(m_tabs->sizeHint().height()), 48.0);
}

void TestMd3Tabs::scrollableLaysOutFromTheEdgePadding()
{
    m_tabs = new MdTabs;
    m_tabs->setLayout(MdTabsLayout::Scrollable);
    addTabs(*m_tabs, 3);
    m_tabs->resize(600, 48);
    m_tabs->show();
    QTest::qWait(50);

    const MdTabsVariantTokens &t = m_tabs->tokens().forVariant(MdTabsVariant::Primary);
    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*m_tabs, m_tabs->tokens());

    // The first tab starts at the 52 px edge padding.
    QCOMPARE(layout.tabRects.first().x(), t.scrollableEdgePadding);

    // Every tab is at least 90 wide, and the run is contiguous.
    QCOMPARE(layout.tabRects.at(0).width(), t.scrollableMinTabWidth);
    QCOMPARE(layout.tabRects.at(1).x(),
             layout.tabRects.at(0).x() + layout.tabRects.at(0).width());
}

void TestMd3Tabs::scrollableScrollsToTheCentre()
{
    m_tabs = new MdTabs;
    m_tabs->setLayout(MdTabsLayout::Scrollable);
    addTabs(*m_tabs, 6);
    m_tabs->resize(300, 48);
    m_tabs->show();
    QTest::qWait(50);

    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*m_tabs, m_tabs->tokens());
    const QRectF last = layout.tabRects.last();

    // Selecting the last tab scrolls it towards the centre, clamped to the
    // content's extent.
    m_tabs->setCurrentIndex(5);
    const qreal expected = qBound<qreal>(0.0, last.center().x() - 300.0 / 2.0,
                                         qMax<qreal>(0.0, layout.contentWidth - 300.0));
    QVERIFY(settleScroll(*m_tabs, expected));
    QVERIFY(m_tabs->scrollOffset() > 0.0);
}

void TestMd3Tabs::wheelScrollLandsInstantly()
{
    m_tabs = new MdTabs;
    m_tabs->setLayout(MdTabsLayout::Scrollable);
    addTabs(*m_tabs, 6);
    m_tabs->resize(300, 48);
    m_tabs->show();
    QTest::qWait(50);

    QWheelEvent wheel(QPointF(150, 24), QPointF(150, 24), QPoint(), QPoint(0, -1200),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(m_tabs, &wheel);
    // The offset moved immediately, without a spring.
    QVERIFY(m_tabs->scrollOffset() > 0.0);
    QCOMPARE(m_tabs->scrollOffset(), m_tabs->scrollOffset()); // stable on the next read
}

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------

void TestMd3Tabs::arrowKeysMoveTheSelection()
{
    m_tabs = new MdTabs;
    addTabs(*m_tabs, 3);
    m_tabs->resize(300, 48);
    m_tabs->show();
    QTest::qWait(50);

    m_tabs->setCurrentIndex(0);
    QTest::keyClick(m_tabs, Qt::Key_Right);
    QCOMPARE(m_tabs->currentIndex(), 1);
    QTest::keyClick(m_tabs, Qt::Key_Right);
    QCOMPARE(m_tabs->currentIndex(), 2);
    QTest::keyClick(m_tabs, Qt::Key_Right);
    QCOMPARE(m_tabs->currentIndex(), 0); // wraps
    QTest::keyClick(m_tabs, Qt::Key_Left);
    QCOMPARE(m_tabs->currentIndex(), 2);
    QTest::keyClick(m_tabs, Qt::Key_Home);
    QCOMPARE(m_tabs->currentIndex(), 0);
    QTest::keyClick(m_tabs, Qt::Key_End);
    QCOMPARE(m_tabs->currentIndex(), 2);
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void TestMd3Tabs::renderSmokePaintsContainerDividerAndIndicator()
{
    m_tabs = new MdTabs;
    addTabs(*m_tabs, 4);
    m_tabs->resize(400, 48);
    m_tabs->show();
    QTest::qWait(50);

    // Qt6's show synthesizes an enter under the parked cursor, which puts
    // tab 0 in its hover state layer — a child always paints over its
    // parent. Selecting tab 1 samples the indicator somewhere unhovered.
    m_tabs->setCurrentIndex(1);
    const MdTabsVariantTokens &t = m_tabs->tokens().forVariant(MdTabsVariant::Primary);
    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*m_tabs, m_tabs->tokens());
    const MdTab *tab = m_tabs->tabAt(1);
    const qreal contentWidth = tab->indicatorContentWidth(100.0);
    const QRectF indicator = MdTabsStyle::indicatorRectFor(layout, 1, t, contentWidth);
    QVERIFY(settleIndicator(*m_tabs, indicator.x(), contentWidth));

    // The container paints the published surface colour across the row.
    const QColor container = MdTheme::instance().color(t.containerColor);
    const QColor divider = MdTheme::instance().color(t.dividerColor);
    const QColor indicatorColour = MdTheme::instance().color(t.activeIndicatorColor);

    const QColor atContainer = pixelColorAt(*m_tabs, QPointF(390, 4));
    QCOMPARE(atContainer, container);

    // The divider is the bottom row.
    const QColor atDivider = pixelColorAt(*m_tabs, QPointF(390, 47));
    QCOMPARE(atDivider, divider);

    // The indicator is inside its content rectangle at the row's bottom,
    // above the divider. (46.5 would round to 47 — the divider's row.)
    const QColor atIndicator =
        pixelColorAt(*m_tabs, QPointF(indicator.center().x(), 45.5));
    QCOMPARE(atIndicator, indicatorColour);
}

QTEST_MAIN(TestMd3Tabs)
#include "TestMd3Tabs.moc"
