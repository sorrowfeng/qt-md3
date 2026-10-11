// TestMd3ScrollBarStyle — the overlay scrollbar chrome.
//
// MdScrollBarStyle is Pattern B (QProxyStyle + drawComplexControl), so there
// is no widget under test: the suite pins the geometry the style publishes
// (overlay handle, no arrow buttons, comfortable hit extent) and smoke-checks
// that a real QScrollBar painted through the style is not blank.

#include "TestMd3Common.h"

#include "styles/MdScrollBarStyle.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStyleOptionSlider>

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

} // namespace

class TestMd3ScrollBarStyle : public QObject
{
    Q_OBJECT

private slots:
    void sharedInstanceIsStable();
    void overlayMetrics();
    void handleIsAnInsetPill();
    void handleTracksSliderPosition();
    void noArrowButtons();
    void paintsThroughQScrollBar();
    void paintsDifferentEmphasisPerState();
};

void TestMd3ScrollBarStyle::sharedInstanceIsStable()
{
    QCOMPARE(MdScrollBarStyle::shared(), MdScrollBarStyle::shared());
}

void TestMd3ScrollBarStyle::overlayMetrics()
{
    MdScrollBarStyle style;
    QCOMPARE(style.pixelMetric(QStyle::PM_ScrollBarExtent), MdScrollBarStyle::hitExtent());
    QCOMPARE(style.pixelMetric(QStyle::PM_ScrollBarSliderMin),
             MdScrollBarStyle::minimumHandleLength());
    // Visible overlay width stays thin; the hit area is wider on purpose.
    QVERIFY(MdScrollBarStyle::overlayWidth() < MdScrollBarStyle::hitExtent());
    QCOMPARE(MdScrollBarStyle::overlayWidth(), 8);
    QCOMPARE(MdScrollBarStyle::hitExtent(), 16);
}

void TestMd3ScrollBarStyle::handleIsAnInsetPill()
{
    const QRect bar(0, 0, 16, 200);
    const QRect handle = MdScrollBarStyle::handleRectFor(
        bar, Qt::Vertical, 0, 100, 20, 0, false);

    QCOMPARE(handle.width(), MdScrollBarStyle::overlayWidth());
    QVERIFY(handle.height() >= MdScrollBarStyle::minimumHandleLength());
    // Centred in the 16 dp hit extent.
    QCOMPARE(handle.left(), (bar.width() - handle.width()) / 2);
    // At the leading end when the value is at the minimum.
    QCOMPARE(handle.top(), bar.top());

    const QRect bottom = MdScrollBarStyle::handleRectFor(
        bar, Qt::Vertical, 0, 100, 20, 100, false);
    QCOMPARE(bottom.bottom(), bar.bottom());

    const QRect horizontal = MdScrollBarStyle::handleRectFor(
        QRect(0, 0, 300, 16), Qt::Horizontal, 0, 100, 20, 50, false);
    QCOMPARE(horizontal.height(), MdScrollBarStyle::overlayWidth());
    QVERIFY(horizontal.width() >= MdScrollBarStyle::minimumHandleLength());
}

void TestMd3ScrollBarStyle::handleTracksSliderPosition()
{
    const QRect bar(0, 0, 16, 200);
    const QRect top = MdScrollBarStyle::handleRectFor(bar, Qt::Vertical, 0, 100, 25, 0, false);
    const QRect mid = MdScrollBarStyle::handleRectFor(bar, Qt::Vertical, 0, 100, 25, 50, false);
    const QRect end = MdScrollBarStyle::handleRectFor(bar, Qt::Vertical, 0, 100, 25, 100, false);

    QVERIFY(top.top() < mid.top());
    QVERIFY(mid.top() < end.top());
    QCOMPARE(end.bottom(), bar.bottom());

    // upsideDown flips the travel direction.
    const QRect flipped = MdScrollBarStyle::handleRectFor(bar, Qt::Vertical, 0, 100, 25, 0, true);
    QCOMPARE(flipped.bottom(), bar.bottom());
}

void TestMd3ScrollBarStyle::noArrowButtons()
{
    MdScrollBarStyle style;
    QStyleOptionSlider option;
    option.rect = QRect(0, 0, 16, 200);
    option.orientation = Qt::Vertical;
    option.minimum = 0;
    option.maximum = 100;
    option.pageStep = 20;
    option.sliderPosition = 0;
    option.sliderValue = 0;

    QVERIFY(style.subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarAddLine,
                                 nullptr)
                .isEmpty());
    QVERIFY(style.subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSubLine,
                                 nullptr)
                .isEmpty());

    const QRect slider =
        style.subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider, nullptr);
    QCOMPARE(slider.width(), MdScrollBarStyle::overlayWidth());
}

void TestMd3ScrollBarStyle::paintsThroughQScrollBar()
{
    QScrollBar bar(Qt::Vertical);
    bar.setRange(0, 100);
    bar.setPageStep(20);
    bar.setValue(0);
    bar.resize(16, 200);
    bar.setStyle(MdScrollBarStyle::shared());

    QImage image(bar.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        // QScrollBar::initStyleOption is protected; the style only reads the
        // slider fields drawComplexControl needs, so fill them in directly.
        QStyleOptionSlider option;
        option.rect = bar.rect();
        option.state = QStyle::State_Enabled;
        option.orientation = Qt::Vertical;
        option.minimum = bar.minimum();
        option.maximum = bar.maximum();
        option.pageStep = bar.pageStep();
        option.singleStep = bar.singleStep();
        option.sliderPosition = bar.sliderPosition();
        option.sliderValue = bar.value();
        option.upsideDown = false;
        option.activeSubControls = QStyle::SC_ScrollBarSlider;
        bar.style()->drawComplexControl(QStyle::CC_ScrollBar, &option, &painter, &bar);
    }

    // Idle handle is on-surface @ 0.38 — translucent but clearly not blank.
    const int painted = mdtest::paintedPixelCount(image, qRgba(0, 0, 0, 0));
    QVERIFY2(painted > 0, "MdScrollBarStyle painted nothing on a QScrollBar");
}

void TestMd3ScrollBarStyle::paintsDifferentEmphasisPerState()
{
    QVERIFY(MdScrollBarStyle::idleAlpha() < MdScrollBarStyle::hoverAlpha());
    QVERIFY(MdScrollBarStyle::hoverAlpha() < MdScrollBarStyle::pressedAlpha());
    QVERIFY(MdScrollBarStyle::disabledAlpha() < MdScrollBarStyle::idleAlpha());
    // Idle matches the M3 medium-emphasis content opacity.
    QCOMPARE(MdScrollBarStyle::idleAlpha(), 0.38);
}

QTEST_MAIN(TestMd3ScrollBarStyle)

#include "TestMd3ScrollBarStyle.moc"
