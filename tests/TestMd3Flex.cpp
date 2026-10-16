// TestMd3Flex — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the flex semantics the header documents:
//   * direction / justify / align / gap / wrap properties with NOTIFY;
//   * the MD3 spacing-scale values;
//   * children are laid out on the main axis with the resolved gap;
//   * wrap packs a second line when the run overflows.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdFlex.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

using namespace md;

namespace {

QWidget *makeBox(const QSize &size, QWidget *parent)
{
    auto *box = new QWidget(parent);
    box->setFixedSize(size);
    return box;
}

} // namespace

class TestMd3Flex : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdFlex flex;
        QCOMPARE(flex.direction(), MdFlexDirection::Row);
        QCOMPARE(flex.justify(), MdFlexJustify::Start);
        QCOMPARE(flex.align(), MdFlexAlign::Start);
        QCOMPARE(flex.gap(), MdFlexSpacing::None);
        QVERIFY(!flex.isWrap());
    }

    void spacingScale()
    {
        QCOMPARE(MdFlex::spacingPx(MdFlexSpacing::None), 0.0);
        QCOMPARE(MdFlex::spacingPx(MdFlexSpacing::ExtraSmall), 4.0);
        QCOMPARE(MdFlex::spacingPx(MdFlexSpacing::Small), 8.0);
        QCOMPARE(MdFlex::spacingPx(MdFlexSpacing::Medium), 12.0);
        QCOMPARE(MdFlex::spacingPx(MdFlexSpacing::Large), 16.0);
        QCOMPARE(MdFlex::spacingPx(MdFlexSpacing::ExtraLarge), 24.0);
        QCOMPARE(MdFlex::spacingPx(MdFlexSpacing::Huge), 32.0);

        MdFlex flex;
        flex.setGap(MdFlexSpacing::Large);
        QCOMPARE(flex.gapPx(), 16.0);
    }

    void propertiesNotify()
    {
        MdFlex flex;
        QSignalSpy dirSpy(&flex, &MdFlex::directionChanged);
        QSignalSpy justSpy(&flex, &MdFlex::justifyChanged);
        QSignalSpy alignSpy(&flex, &MdFlex::alignChanged);
        QSignalSpy gapSpy(&flex, &MdFlex::gapChanged);
        QSignalSpy wrapSpy(&flex, &MdFlex::wrapChanged);

        flex.setDirection(MdFlexDirection::Column);
        QCOMPARE(dirSpy.count(), 1);

        flex.setJustify(MdFlexJustify::Center);
        QCOMPARE(justSpy.count(), 1);

        flex.setAlign(MdFlexAlign::Stretch);
        QCOMPARE(alignSpy.count(), 1);

        flex.setGap(MdFlexSpacing::Small);
        QCOMPARE(gapSpy.count(), 1);

        flex.setWrap(true);
        QCOMPARE(wrapSpy.count(), 1);
    }

    void rowPlacesChildrenWithGap()
    {
        MdFlex flex;
        flex.setGap(MdFlexSpacing::Large);
        flex.resize(300, 40);
        auto *a = makeBox(QSize(40, 20), &flex);
        auto *b = makeBox(QSize(40, 20), &flex);
        flex.show();
        QVERIFY(QTest::qWaitForWindowExposed(&flex));
        flex.ensurePolished();

        // Trigger a layout pass.
        flex.resize(300, 41);
        flex.resize(300, 40);
        QCoreApplication::processEvents();

        QCOMPARE(a->x(), 0);
        // Gap 16 between the two 40-px boxes.
        QCOMPARE(b->x(), 56);
    }

    void wrapPacksSecondLine()
    {
        MdFlex flex;
        flex.setGap(MdFlexSpacing::None);
        flex.setWrap(true);
        flex.setDirection(MdFlexDirection::Row);
        flex.resize(90, 80);
        // Three 40-px boxes on a 90-px main axis: two fit, the third wraps.
        auto *a = makeBox(QSize(40, 20), &flex);
        auto *b = makeBox(QSize(40, 20), &flex);
        auto *c = makeBox(QSize(40, 20), &flex);
        flex.show();
        QVERIFY(QTest::qWaitForWindowExposed(&flex));
        flex.resize(90, 81);
        flex.resize(90, 80);
        QCoreApplication::processEvents();

        QCOMPARE(a->x(), 0);
        QCOMPARE(b->x(), 40);
        QCOMPARE(c->x(), 0);
        QVERIFY(c->y() > 0);
    }

    void columnStacksOnMainAxis()
    {
        MdFlex flex;
        flex.setDirection(MdFlexDirection::Column);
        flex.setGap(MdFlexSpacing::Small);
        flex.resize(100, 120);
        auto *a = makeBox(QSize(40, 20), &flex);
        auto *b = makeBox(QSize(40, 20), &flex);
        flex.show();
        QVERIFY(QTest::qWaitForWindowExposed(&flex));
        flex.resize(100, 121);
        flex.resize(100, 120);
        QCoreApplication::processEvents();

        QCOMPARE(a->y(), 0);
        QCOMPARE(b->y(), 28); // 20 + gap 8
    }
};

QTEST_MAIN(TestMd3Flex)
#include "TestMd3Flex.moc"
