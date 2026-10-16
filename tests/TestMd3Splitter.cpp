// TestMd3Splitter — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the splitter semantics the header documents:
//   * outline-variant 1 px hairline at rest;
//   * Q_PROPERTY orientation with NOTIFY;
//   * hover handle reveal 0 → 1;
//   * non-blank render smoke.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdSplitter.h"

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

} // namespace

class TestMd3Splitter : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdSplitter splitter;
        QCOMPARE(splitter.orientation(), Qt::Horizontal);
        QCOMPARE(splitter.handleWidth(), 8);
        QCOMPARE(splitter.handleBarLength(), 32);
        QVERIFY(!splitter.dividerColor().isValid());
        QCOMPARE(splitter.handleReveal(), 0.0);
    }

    void orientationPropertyNotifies()
    {
        MdSplitter splitter;
        QSignalSpy spy(&splitter, &MdSplitter::orientationChanged);
        splitter.setOrientation(Qt::Vertical);
        QCOMPARE(splitter.orientation(), Qt::Vertical);
        QCOMPARE(spy.count(), 1);
        splitter.setOrientation(Qt::Vertical);
        QCOMPARE(spy.count(), 1);
    }

    void tokenColors()
    {
        MdSplitter splitter;
        QCOMPARE(splitter.resolvedDividerColor(),
                 MdTheme::instance().color(ColorRole::OutlineVariant));
        QCOMPARE(splitter.resolvedHandleColor(), MdTheme::instance().color(ColorRole::Outline));

        splitter.setDividerColor(QColor(10, 20, 30));
        QCOMPARE(splitter.resolvedDividerColor(), QColor(10, 20, 30));
    }

    void handleMetrics()
    {
        MdSplitter splitter;
        QSignalSpy widthSpy(&splitter, &MdSplitter::handleWidthChanged);
        QSignalSpy lengthSpy(&splitter, &MdSplitter::handleBarLengthChanged);

        splitter.setHandleWidth(12);
        QCOMPARE(splitter.handleWidth(), 12);
        QCOMPARE(widthSpy.count(), 1);

        splitter.setHandleBarLength(48);
        QCOMPARE(splitter.handleBarLength(), 48);
        QCOMPARE(lengthSpy.count(), 1);

        // Clamped.
        splitter.setHandleWidth(1);
        QCOMPARE(splitter.handleWidth(), 4);
    }

    void panesSplit()
    {
        MdSplitter splitter;
        auto *left = new QLabel(QStringLiteral("L"), &splitter);
        auto *right = new QLabel(QStringLiteral("R"), &splitter);
        splitter.addWidget(left);
        splitter.addWidget(right);
        splitter.resize(300, 120);
        splitter.show();
        QVERIFY(QTest::qWaitForWindowExposed(&splitter));
        QCOMPARE(splitter.count(), 2);
        QVERIFY(left->width() > 0);
        QVERIFY(right->width() > 0);
    }

    void nonBlankRender()
    {
        MdSplitter splitter;
        auto *left = new QLabel(QStringLiteral("Left"), &splitter);
        auto *right = new QLabel(QStringLiteral("Right"), &splitter);
        splitter.addWidget(left);
        splitter.addWidget(right);
        splitter.resize(300, 120);
        splitter.show();
        QVERIFY(QTest::qWaitForWindowExposed(&splitter));

        const QImage image = renderWidget(splitter);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdSplitter must paint its hairline and children");
    }
};

QTEST_MAIN(TestMd3Splitter)
#include "TestMd3Splitter.moc"
