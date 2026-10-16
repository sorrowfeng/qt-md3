// TestMd3Collapse — Stage 2 Qt extension (Appendix D, P2).
//
// Pins the list expand/collapse semantics:
//   * title / supportingText / expanded / headerHeight properties with NOTIFY;
//   * the header is a list one-line row (56 px floor);
//   * expanding exposes the body widget, collapsing hides it;
//   * Space/Enter toggles; the body height animates on the FastSpatial spring.

#include "TestMd3Common.h"
#include "core/MdMotion.h"
#include "core/MdTheme.h"
#include "widgets/MdCollapse.h"

#include <QtGui/QImage>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>

using namespace md;

class TestMd3Collapse : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdCollapse collapse;
        QVERIFY(collapse.title().isEmpty());
        QVERIFY(collapse.supportingText().isEmpty());
        QCOMPARE(collapse.isExpanded(), false);
        QCOMPARE(collapse.headerHeight(), 56);
        QCOMPARE(collapse.bodyWidget(), nullptr);
    }

    void propertiesNotify()
    {
        MdCollapse collapse;
        QSignalSpy titleSpy(&collapse, &MdCollapse::titleChanged);
        QSignalSpy supportSpy(&collapse, &MdCollapse::supportingTextChanged);
        QSignalSpy expandedSpy(&collapse, &MdCollapse::expandedChanged);
        QSignalSpy heightSpy(&collapse, &MdCollapse::headerHeightChanged);

        collapse.setTitle(QStringLiteral("Section"));
        QCOMPARE(titleSpy.count(), 1);

        collapse.setSupportingText(QStringLiteral("3 items"));
        QCOMPARE(supportSpy.count(), 1);

        collapse.setExpanded(true);
        QCOMPARE(expandedSpy.count(), 1);
        QVERIFY(collapse.isExpanded());

        collapse.setHeaderHeight(64);
        QCOMPARE(heightSpy.count(), 1);
        QCOMPARE(collapse.headerHeight(), 64);
    }

    void bodyIsParentedAndShownWhenExpanded()
    {
        MdCollapse collapse;
        auto *body = new QLabel(QStringLiteral("body"), &collapse);
        collapse.setBodyWidget(body);
        QCOMPARE(collapse.bodyWidget(), body);
        QVERIFY(body->parent() == &collapse);

        collapse.resize(300, 200);
        collapse.show();
        QVERIFY(QTest::qWaitForWindowExposed(&collapse));

        // Collapsed: the body has no height and is hidden.
        QVERIFY(!body->isVisible() || body->height() == 0);

        collapse.setExpanded(true);
        // After the spring settles the body is visible at its full height.
        QTest::qWait(MdMotion::spring(MotionSpring::SpatialFast).settlingDurationMs() + 50);
        QVERIFY(body->isVisible());
        QVERIFY(body->height() > 0);
    }

    void toggleFromKeyboard()
    {
        MdCollapse collapse(QStringLiteral("Header"));
        collapse.setBodyWidget(new QLabel(QStringLiteral("body"), &collapse));
        collapse.resize(300, 120);
        collapse.show();
        QVERIFY(QTest::qWaitForWindowExposed(&collapse));
        collapse.setFocus();

        QVERIFY(!collapse.isExpanded());
        QTest::keyClick(&collapse, Qt::Key_Space);
        QVERIFY(collapse.isExpanded());
        QTest::keyClick(&collapse, Qt::Key_Return);
        QVERIFY(!collapse.isExpanded());
    }

    void heightForWidthTracksProgress()
    {
        auto *body = new QLabel(QStringLiteral("x"));
        body->setFixedHeight(80);
        MdCollapse collapse;
        collapse.setBodyWidget(body);
        QCOMPARE(collapse.heightForWidth(200), 56);

        collapse.setExpanded(true);
        // Mid-flight the height is between the floor and the open height.
        const int openHeight = collapse.heightForWidth(200);
        QVERIFY(openHeight >= 56);
        // After settle it is header + body.
        QTest::qWait(MdMotion::spring(MotionSpring::SpatialFast).settlingDurationMs() + 50);
        QCOMPARE(collapse.heightForWidth(200), 56 + 80);
    }

    void renderSmoke()
    {
        MdCollapse collapse(QStringLiteral("Settings"));
        collapse.resize(280, 56);
        QImage image(280, 56, QImage::Format_ARGB32);
        image.fill(Qt::white);
        collapse.render(&image);
        QVERIFY(mdtest::paintedPixelCount(image, qRgb(255, 255, 255)) > 0);
    }
};

QTEST_MAIN(TestMd3Collapse)
#include "TestMd3Collapse.moc"
