// TestMd3Scaffold — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the page-skeleton composition:
//
//   * appBar / navigation / endPane / body / fab slots notify and are laid
//     out by the scaffold (no restyling — Stage 1 owns their paint);
//   * navPlacement and fabPlacement move those slots;
//   * bodyRect() subtracts the occupied edges;
//   * meta-property surface + non-blank render smoke.

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdFab.h"
#include "widgets/MdScaffold.h"
#include "widgets/MdTopAppBar.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>

using namespace md;

namespace {

QImage renderWidget(QWidget &widget)
{
    QImage image(int(widget.width()), int(widget.height()), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32);
}

bool anyPixelPainted(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).alpha() > 8) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

class TestMd3Scaffold : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdScaffold scaffold;
        QVERIFY(scaffold.appBar() == nullptr);
        QVERIFY(scaffold.navigation() == nullptr);
        QVERIFY(scaffold.endPane() == nullptr);
        QVERIFY(scaffold.body() == nullptr);
        QVERIFY(scaffold.fab() == nullptr);
        QCOMPARE(scaffold.navPlacement(), MdNavPlacement::Start);
        QCOMPARE(scaffold.fabPlacement(), MdFabPlacement::EndFloat);
        QCOMPARE(scaffold.fabMargin(), 16);
    }

    void slotsNotifyAndAdopt()
    {
        MdScaffold scaffold;
        QSignalSpy bodySpy(&scaffold, &MdScaffold::bodyChanged);
        QSignalSpy navSpy(&scaffold, &MdScaffold::navigationChanged);
        QSignalSpy fabSpy(&scaffold, &MdScaffold::fabChanged);

        auto *body = new QLabel(QStringLiteral("body"));
        auto *nav = new QLabel(QStringLiteral("nav"));
        auto *fab = new QLabel(QStringLiteral("fab"));

        scaffold.setBody(body);
        scaffold.setNavigation(nav, MdNavPlacement::Start);
        scaffold.setFab(fab);

        QCOMPARE(scaffold.body(), body);
        QCOMPARE(scaffold.navigation(), nav);
        QCOMPARE(scaffold.fab(), fab);
        QCOMPARE(bodySpy.count(), 1);
        QCOMPARE(navSpy.count(), 1);
        QCOMPARE(fabSpy.count(), 1);

        // The scaffold parents the slots.
        QCOMPARE(body->parent(), &scaffold);
        QCOMPARE(nav->parent(), &scaffold);
        QCOMPARE(fab->parent(), &scaffold);
    }

    void bodyRectSubtractsEdges()
    {
        MdScaffold scaffold;
        auto *appBar = new QLabel(QStringLiteral("bar"), &scaffold);
        appBar->setFixedHeight(64);
        auto *nav = new QLabel(QStringLiteral("rail"), &scaffold);
        nav->setFixedWidth(80);
        auto *endPane = new QLabel(QStringLiteral("end"), &scaffold);
        endPane->setFixedWidth(120);
        auto *body = new QLabel(QStringLiteral("body"), &scaffold);

        scaffold.setAppBar(appBar);
        scaffold.setNavigation(nav, MdNavPlacement::Start);
        scaffold.setEndPane(endPane);
        scaffold.setBody(body);
        scaffold.resize(800, 600);
        scaffold.show();
        QVERIFY(QTest::qWaitForWindowExposed(&scaffold));

        const QRect bodyRect = scaffold.bodyRect();
        QCOMPARE(bodyRect.x(), 80);
        QCOMPARE(bodyRect.y(), 64);
        QCOMPARE(bodyRect.width(), 800 - 80 - 120);
        QCOMPARE(bodyRect.height(), 600 - 64);
        QCOMPARE(body->geometry(), bodyRect);
    }

    void navPlacementMovesSlot()
    {
        MdScaffold scaffold;
        auto *nav = new QLabel(QStringLiteral("nav"), &scaffold);
        nav->setFixedWidth(80);
        nav->setFixedHeight(80);
        auto *body = new QLabel(QStringLiteral("body"), &scaffold);
        scaffold.setBody(body);
        scaffold.setNavigation(nav, MdNavPlacement::Bottom);
        scaffold.resize(400, 400);
        scaffold.show();
        QVERIFY(QTest::qWaitForWindowExposed(&scaffold));

        QCOMPARE(scaffold.navPlacement(), MdNavPlacement::Bottom);
        QCOMPARE(nav->y() + nav->height(), scaffold.height());

        QSignalSpy placeSpy(&scaffold, &MdScaffold::navPlacementChanged);
        scaffold.setNavPlacement(MdNavPlacement::End);
        QCOMPARE(placeSpy.count(), 1);
        QCOMPARE(nav->x() + nav->width(), scaffold.width());
    }

    void fabPlacement()
    {
        MdScaffold scaffold;
        auto *fab = new QLabel(QStringLiteral("fab"), &scaffold);
        fab->setFixedSize(56, 56);
        auto *body = new QLabel(QStringLiteral("body"), &scaffold);
        scaffold.setBody(body);
        scaffold.setFab(fab);
        scaffold.resize(400, 400);
        scaffold.show();
        QVERIFY(QTest::qWaitForWindowExposed(&scaffold));

        const QRect bodyRect = scaffold.bodyRect();
        // EndFloat: trailing + bottom inset by fabMargin.
        QCOMPARE(fab->x() + fab->width() + 16, bodyRect.right() + 1);
        QCOMPARE(fab->y() + fab->height() + 16, bodyRect.bottom() + 1);

        QSignalSpy placeSpy(&scaffold, &MdScaffold::fabPlacementChanged);
        scaffold.setFabPlacement(MdFabPlacement::StartFloat);
        QCOMPARE(placeSpy.count(), 1);
        QCOMPARE(fab->x(), bodyRect.left() + 16);
    }

    void metaProperties()
    {
        MdScaffold scaffold;
        const QMetaObject *meta = scaffold.metaObject();
        QVERIFY(meta->indexOfProperty("appBar") >= 0);
        QVERIFY(meta->indexOfProperty("navigation") >= 0);
        QVERIFY(meta->indexOfProperty("navPlacement") >= 0);
        QVERIFY(meta->indexOfProperty("endPane") >= 0);
        QVERIFY(meta->indexOfProperty("body") >= 0);
        QVERIFY(meta->indexOfProperty("fab") >= 0);
        QVERIFY(meta->indexOfProperty("fabPlacement") >= 0);
        QVERIFY(meta->indexOfProperty("fabMargin") >= 0);

        QSignalSpy marginSpy(&scaffold, &MdScaffold::fabMarginChanged);
        scaffold.setProperty("fabMargin", 24);
        QCOMPARE(scaffold.fabMargin(), 24);
        QCOMPARE(marginSpy.count(), 1);
    }

    void themeLifecycle()
    {
        MdScaffold scaffold;
        scaffold.resize(320, 240);
        scaffold.show();
        QSignalSpy bodySpy(&scaffold, &MdScaffold::bodyChanged);
        MdTheme::instance().setSeedColor(QColor(0x1B5E20));
        QCOMPARE(bodySpy.count(), 0);
    }

    void renderSmoke()
    {
        MdScaffold scaffold;
        auto *appBar = new QLabel(QStringLiteral("App"), &scaffold);
        appBar->setFixedHeight(64);
        auto *body = new QLabel(QStringLiteral("Body"), &scaffold);
        scaffold.setAppBar(appBar);
        scaffold.setBody(body);
        scaffold.resize(480, 320);
        scaffold.show();
        QVERIFY(QTest::qWaitForWindowExposed(&scaffold));
        const QImage image = renderWidget(scaffold);
        QVERIFY(anyPixelPainted(image));
    }
};

QTEST_MAIN(TestMd3Scaffold)
#include "TestMd3Scaffold.moc"
