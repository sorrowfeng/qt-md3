// *****************************************************************************
// TestMd3Carousel — the carousel family's lock. Pins the token rows, the
// arrangement/keyline math (against hand-computed values from the Compose
// sources and the expectations of Compose's own MultiBrowseTest), the item
// interpolation, the scroll model and the widget's drag/snap behaviour.
// *****************************************************************************

#include "core/MdCarouselTokens.h"
#include "core/MdTheme.h"
#include "styles/MdCarouselStyle.h"
#include "widgets/MdCarousel.h"

#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtTest/QTest>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

#include <cmath>

using namespace md;
using Keyline = MdCarouselStyle::Keyline;

namespace {

bool closeTo(qreal actual, qreal expected, qreal epsilon = 0.01)
{
    return std::abs(actual - expected) <= epsilon;
}

} // namespace

class TestMd3Carousel : public QObject
{
    Q_OBJECT

private slots:
    void tokenDefaults();
    void tokenOverride();
    void arrangementFindsIdealFit();
    void arrangementResolves380Case();
    void multiBrowseMatchesCompose380Case();
    void multiBrowseTinyContainer();
    void multiBrowseTrimsSurplusKeylines();
    void itemMetricsAtRest();
    void maxScrollMatchesFormula();
    void scrollOffsetZeroReturnsDefault();
    void widgetLayoutAndSnap();
    void renderSmoke();
    void styleInstalled();
};

void TestMd3Carousel::tokenDefaults()
{
    const MdCarouselTokens tokens = MdCarouselTokens::resolve();
    QCOMPARE(tokens.containerColor, ColorRole::Surface);
    QCOMPARE(tokens.containerElevation, 0.0);
    QCOMPARE(tokens.hoverContainerElevation, 1.0);
    QCOMPARE(tokens.pressedContainerElevation, 0.0);
    QCOMPARE(tokens.containerShapeRadius, 28.0);
    QCOMPARE(tokens.outlineWidth, 1.0);
    QCOMPARE(tokens.disabledContainerOpacity, 0.38);
    QCOMPARE(tokens.disabledOutlineOpacity, 0.12);
    QCOMPARE(tokens.hoverStateLayerOpacity, 0.08);
    QCOMPARE(tokens.focusStateLayerOpacity, 0.12);
    QCOMPARE(tokens.pressedStateLayerOpacity, 0.12);
    // The specs page's "Padding between elements 8dp" wins over Compose's
    // 0.dp default — the divergence is recorded in docs/porting-todo.md.
    QCOMPARE(tokens.itemSpacing, 8.0);
    QCOMPARE(tokens.minSmallItemSize, 40.0);
    QCOMPARE(tokens.maxSmallItemSize, 56.0);
    QCOMPARE(tokens.anchorSize, 10.0);
    QCOMPARE(tokens.crossPadding, 8.0);
}

void TestMd3Carousel::tokenOverride()
{
    MdComponentTokens overrides;
    overrides.setValue("md.comp.carousel-item.container.shape.corner-radius", "20");
    overrides.setValue("md.comp.carousel-item.container.item-spacing", "12");
    const MdCarouselTokens tokens = MdCarouselTokens::resolve(&overrides);
    QCOMPARE(tokens.containerShapeRadius, 20.0);
    QCOMPARE(tokens.itemSpacing, 12.0);
    // The unlisted rows stay at the export values.
    QCOMPARE(tokens.minSmallItemSize, 40.0);
}

void TestMd3Carousel::arrangementFindsIdealFit()
{
    // When the space admits the ideal proportions exactly, the large item is
    // not resized and the cost is 0: 380 available, spacing 8, one of each
    // (the fit that Compose's own 380/186/8 case lands on).
    MdCarouselStyle::Arrangement arrangement;
    const bool found = MdCarouselStyle::findLowestCostArrangement(
        380.0, 8.0, 56.0, 40.0, 56.0, { 1 }, 121.0, { 1, 0 }, 186.0, { 1 }, &arrangement);
    QVERIFY(found);
    QVERIFY(closeTo(arrangement.largeSize, 186.0));
    QVERIFY(closeTo(arrangement.mediumSize, 122.0));
    QVERIFY(closeTo(arrangement.smallSize, 56.0));
    QCOMPARE(arrangement.largeCount, 1);
    QCOMPARE(arrangement.mediumCount, 1);
    QCOMPARE(arrangement.smallCount, 1);
    QVERIFY(arrangement.cost(186.0) <= 0.01);
}

void TestMd3Carousel::arrangementResolves380Case()
{
    // The `fit` math on a single permutation, hand-computed from
    // Arrangement.kt: small items take the delta first (clamped), the large
    // size is solved exactly, and the medium flex absorbs the rest.
    const MdCarouselStyle::Arrangement arrangement = MdCarouselStyle::fit(
        1, 380.0, 8.0, 1, 56.0, 40.0, 56.0, 1, 121.0, 1, 186.0);
    QVERIFY(closeTo(arrangement.smallSize, 56.0));
    QVERIFY(closeTo(arrangement.largeSize, 186.0));
    QVERIFY(closeTo(arrangement.mediumSize, 122.0));
    // The slot run fills the available space exactly.
    QVERIFY(closeTo(arrangement.largeSize + arrangement.mediumSize + arrangement.smallSize
                        + 2.0 * 8.0,
                    380.0));
}

void TestMd3Carousel::multiBrowseMatchesCompose380Case()
{
    // Compose's own test (MultiBrowseTest.adjustsForItemSpacing) pins this
    // case: 5 keylines, the large item unresized at the start, the last
    // visible item a 56 px small aligned with the end, and the unadjusted
    // offsets -101 / 93 / 287 / 481 / 675.
    const QList<Keyline> keylines = MdCarouselStyle::multiBrowseKeylineList(
        380.0, 186.0, 8.0, 10, 40.0, 56.0, 10.0);
    QCOMPARE(keylines.size(), 5);

    QVERIFY(keylines[0].isAnchor);
    QVERIFY(keylines[4].isAnchor);
    QVERIFY(keylines[1].isFocal);
    QVERIFY(!keylines[2].isFocal);

    QVERIFY(closeTo(keylines[1].size, 186.0));
    QVERIFY(closeTo(keylines[1].offset, 93.0));

    QVERIFY(closeTo(keylines[3].size, 56.0));
    QVERIFY(closeTo(keylines[3].offset, 380.0 - 28.0));

    const qreal expectedUnadjusted[] = { -101.0, 93.0, 287.0, 481.0, 675.0 };
    for (int i = 0; i < 5; ++i) {
        QVERIFY2(closeTo(keylines[i].unadjustedOffset, expectedUnadjusted[i]),
                 qPrintable(QStringLiteral("unadjusted[%1] = %2").arg(i).arg(
                     keylines[i].unadjustedOffset)));
    }
}

void TestMd3Carousel::multiBrowseTinyContainer()
{
    // Compose's test (resizesItemLargerThanContainerToFit1Small): a 100 px
    // container with a 200 px preferred size resolves to
    // [xSmall-Large-Small-xSmall], with the small at the 40 px minimum.
    const QList<Keyline> keylines = MdCarouselStyle::multiBrowseKeylineList(
        100.0, 200.0, 0.0, 10, 40.0, 56.0, 10.0);
    QCOMPARE(keylines.size(), 4);
    QVERIFY(keylines[0].unadjustedOffset < 0.0);
    QVERIFY(keylines[keylines.size() - 1].unadjustedOffset > 100.0);
    QVERIFY(keylines[1].isFocal);
    QVERIFY(closeTo(keylines[2].size, 40.0));
    // The large item had to shrink below the preferred size.
    QVERIFY(keylines[1].size < 200.0);
}

void TestMd3Carousel::multiBrowseTrimsSurplusKeylines()
{
    // Compose's test (withLessItemsThanKeylines): 512 px container, 200 px
    // preferred, only 3 items. The full arrangement wants six keylines; the
    // trim drops the small and re-fits, leaving
    // [xSmall-Large-Large-Medium-xSmall].
    const QList<Keyline> keylines = MdCarouselStyle::multiBrowseKeylineList(
        512.0, 200.0, 0.0, 3, 40.0, 56.0, 10.0);
    QCOMPARE(keylines.size(), 5);
    QVERIFY(keylines[1].isFocal);
    QVERIFY(keylines[2].isFocal);
    QVERIFY(keylines[3].size < keylines[2].size);
}

void TestMd3Carousel::itemMetricsAtRest()
{
    // At scroll offset 0 the first item's centre sits on the first focal
    // keyline: full size, flush with the container's start edge.
    const QList<Keyline> keylines = MdCarouselStyle::multiBrowseKeylineList(
        380.0, 186.0, 8.0, 10, 40.0, 56.0, 10.0);
    const MdCarouselStyle::ItemMetrics first = MdCarouselStyle::itemMetrics(keylines, 93.0);
    QVERIFY(closeTo(first.size, 186.0));
    QVERIFY(closeTo(first.translation, 0.0));

    // The second item's centre sits on the medium keyline: it renders at the
    // medium width, pulled left of its unclamped slot.
    const MdCarouselStyle::ItemMetrics second = MdCarouselStyle::itemMetrics(keylines, 287.0);
    QVERIFY(closeTo(second.size, 122.0));
    QVERIFY(closeTo(second.translation, 255.0 - 287.0));
}

void TestMd3Carousel::maxScrollMatchesFormula()
{
    // Carousel.kt's calculateMaxScrollOffset: the end-to-end extent minus the
    // container.
    QVERIFY(closeTo(MdCarouselStyle::maxScrollOffset(10, 186.0, 8.0, 380.0), 1552.0));
    // Fewer items than the container shows: nothing to scroll.
    QCOMPARE(MdCarouselStyle::maxScrollOffset(2, 186.0, 8.0, 380.0), 0.0);
}

void TestMd3Carousel::scrollOffsetZeroReturnsDefault()
{
    const QList<Keyline> defaultKeylines = MdCarouselStyle::multiBrowseKeylineList(
        380.0, 186.0, 8.0, 10, 40.0, 56.0, 10.0);
    const QList<QList<Keyline>> startSteps =
        MdCarouselStyle::startKeylineSteps(defaultKeylines, 380.0, 8.0);
    const QList<QList<Keyline>> endSteps =
        MdCarouselStyle::endKeylineSteps(defaultKeylines, 380.0, 8.0);
    const qreal maxScroll = MdCarouselStyle::maxScrollOffset(10, 186.0, 8.0, 380.0);

    // Offset 0 is inside the default range: the default keylines come back
    // untouched.
    const QList<Keyline> atRest = MdCarouselStyle::keylineListForScrollOffset(
        defaultKeylines, startSteps, endSteps, 380.0, 8.0, 0.0, maxScroll);
    QCOMPARE(atRest.size(), defaultKeylines.size());
    for (int i = 0; i < atRest.size(); ++i) {
        QVERIFY(atRest[i] == defaultKeylines[i]);
    }

    // The scroll extremes interpolate between steps: the returned lists keep
    // the same slot count and every slot stays within the anchor-small
    // envelope.
    const QList<Keyline> atEnd = MdCarouselStyle::keylineListForScrollOffset(
        defaultKeylines, startSteps, endSteps, 380.0, 8.0, maxScroll, maxScroll);
    QCOMPARE(atEnd.size(), defaultKeylines.size());
    for (const Keyline &k : atEnd) {
        QVERIFY(k.size >= 0.0);
        QVERIFY(k.size <= 186.0 + 0.01);
    }
}

void TestMd3Carousel::widgetLayoutAndSnap()
{
    // A 600 px carousel with three items: the arrangement resolves to one
    // 552 px large and a 40 px small (the medium row cannot fit and loses
    // the cost comparison — 552 + 8 + 40 fills the container exactly).
    QWidget parent;
    parent.resize(600, 300);
    MdCarousel carousel(&parent);
    carousel.resize(600, 300);
    QLabel first(QStringLiteral("A"));
    QLabel second(QStringLiteral("B"));
    QLabel third(QStringLiteral("C"));
    carousel.addItem(&first);
    carousel.addItem(&second);
    carousel.addItem(&third);
    carousel.scrollTo(0.0);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    QTest::qWait(50);

    QCOMPARE(carousel.itemCount(), 3);
    QVERIFY(closeTo(carousel.largeItemSize(), 552.0, 0.5));
    QVERIFY(closeTo(carousel.maxScrollOffset(), 552.0 * 3 + 8.0 * 2 - 600.0, 0.5));

    // The first item sits flush with the container's start, full size.
    QWidget *wrapper0 = carousel.currentKeylines().isEmpty() ? nullptr : first.parentWidget();
    QVERIFY(wrapper0);
    QVERIFY(wrapper0->x() <= 1);
    QVERIFY(closeTo(qreal(wrapper0->width()), 552.0, 0.5));

    // The second item rides the small keyline: its centre sits 20 px inside
    // the container's trailing edge (the 40 px small bleeds past the
    // right anchor exactly like Compose's cutoff rows).
    QWidget *wrapper1 = second.parentWidget();
    QVERIFY(wrapper1);
    QVERIFY(closeTo(qreal(wrapper1->x()), 304.0, 1.0));

    // Programmatic scroll to the maximum: the first item slides past the
    // leading anchor and out, the last item becomes the visible one.
    QWidget *wrapper2 = third.parentWidget();
    QVERIFY(wrapper2);
    carousel.scrollTo(carousel.maxScrollOffset());
    QTest::qWait(50);
    // At the max scroll offset the keyline list is the end-step
    // interpolation (the slots have reshaped), so pin the properties, not
    // the numbers: the first item is out of view, the last is the visible one.
    QVERIFY(wrapper0->x() < 0.0);
    QVERIFY(wrapper1->x() < wrapper2->x());
    QVERIFY(wrapper2->x() >= 0.0 && wrapper2->x() < 100.0);

    // A drag past half a slot snaps forward to the next item boundary.
    // Back to the rest position first — the scroll above maxed out.
    carousel.scrollTo(0.0);
    QTest::qWait(50);
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(300.0, 150.0), QPointF(300.0, 150.0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&carousel, &press);
    QMouseEvent move(QEvent::MouseMove, QPointF(10.0, 150.0), QPointF(10.0, 150.0),
                     Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&carousel, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(10.0, 150.0), QPointF(10.0, 150.0),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&carousel, &release);
    QTest::qWait(900);
    QVERIFY(!carousel.isAnimating());
    QVERIFY(closeTo(carousel.scrollOffset(), 560.0, 1.0));
}

void TestMd3Carousel::renderSmoke()
{
    QWidget parent;
    parent.resize(600, 300);
    MdCarousel carousel(&parent);
    carousel.resize(600, 300);
    QLabel first(QStringLiteral("A"));
    QLabel second(QStringLiteral("B"));
    carousel.addItem(&first);
    carousel.addItem(&second);
    carousel.scrollTo(0.0);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    QTest::qWait(50);

    const QImage image = carousel.grab().toImage();
    QVERIFY(!image.isNull());
    QVERIFY(image.pixelColor(30, 150).alpha() > 0);
}

void TestMd3Carousel::styleInstalled()
{
    QVERIFY(MdCarouselStyle::isInstalled());
}

QTEST_MAIN(TestMd3Carousel)
#include "TestMd3Carousel.moc"
