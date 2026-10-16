// TestMd3StatusBar — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the status-bar semantics the header documents:
//   * surface-container background, on-surface / on-surface-variant text;
//   * left / right message properties with NOTIFY;
//   * optional size grip geometry and drag signal;
//   * non-blank render smoke.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdStatusBar.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

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

} // namespace

class TestMd3StatusBar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdStatusBar bar;
        QVERIFY(bar.leftMessage().isEmpty());
        QVERIFY(bar.rightMessage().isEmpty());
        QVERIFY(!bar.isSizeGripVisible());
        QCOMPARE(bar.barHeight(), 0);
        QVERIFY(bar.sizeHint().height() > 0);
        QVERIFY(bar.sizeGripRect().isEmpty());
    }

    void messagesAndSignals()
    {
        MdStatusBar bar;
        QSignalSpy leftSpy(&bar, &MdStatusBar::leftMessageChanged);
        QSignalSpy rightSpy(&bar, &MdStatusBar::rightMessageChanged);

        bar.setLeftMessage(QStringLiteral("Ready"));
        QCOMPARE(bar.leftMessage(), QStringLiteral("Ready"));
        QCOMPARE(leftSpy.count(), 1);

        bar.setRightMessage(QStringLiteral("Ln 1, Col 1"));
        QCOMPARE(bar.rightMessage(), QStringLiteral("Ln 1, Col 1"));
        QCOMPARE(rightSpy.count(), 1);

        bar.setLeftMessage(QStringLiteral("Ready"));
        QCOMPARE(leftSpy.count(), 1);
    }

    void tokenColors()
    {
        MdStatusBar bar;
        QCOMPARE(bar.resolvedBackgroundColor(),
                 MdTheme::instance().color(ColorRole::SurfaceContainer));
        QCOMPARE(bar.resolvedLeftColor(), MdTheme::instance().color(ColorRole::OnSurface));
        QCOMPARE(bar.resolvedRightColor(),
                 MdTheme::instance().color(ColorRole::OnSurfaceVariant));

        bar.setBackgroundColor(QColor(1, 2, 3));
        QCOMPARE(bar.resolvedBackgroundColor(), QColor(1, 2, 3));
    }

    void sizeGrip()
    {
        MdStatusBar bar;
        bar.resize(200, 32);
        bar.setSizeGripVisible(true);
        QVERIFY(bar.isSizeGripVisible());
        const QRect grip = bar.sizeGripRect();
        QVERIFY(!grip.isEmpty());
        QCOMPARE(grip.right(), bar.width() - 1);
        QCOMPARE(grip.height(), bar.height());

        QSignalSpy spy(&bar, &MdStatusBar::sizeGripVisibleChanged);
        bar.setSizeGripVisible(false);
        QCOMPARE(spy.count(), 1);
        QVERIFY(bar.sizeGripRect().isEmpty());
    }

    void barHeightOverride()
    {
        MdStatusBar bar;
        QSignalSpy spy(&bar, &MdStatusBar::barHeightChanged);
        bar.setBarHeight(40);
        QCOMPARE(bar.barHeight(), 40);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(bar.sizeHint().height(), 40);
    }

    void nonBlankRender()
    {
        MdStatusBar bar;
        bar.setLeftMessage(QStringLiteral("Ready"));
        bar.setRightMessage(QStringLiteral("42 items"));
        bar.setSizeGripVisible(true);
        bar.resize(320, 32);
        bar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&bar));

        const QImage image = renderWidget(bar);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        // Fill with a sentinel first: the bar paints an opaque background.
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdStatusBar must paint its surface and messages");
    }
};

QTEST_MAIN(TestMd3StatusBar)
#include "TestMd3StatusBar.moc"
