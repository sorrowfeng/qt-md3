// MdStyleBase — the Pattern A paint hub.
//
// qt-md3 never uses a stylesheet. Instead one QProxyStyle is installed on the
// application and a meta-object registry routes each widget's paint event to
// the style that owns its class. These tests pin that registry, the theme
// broadcast, and the crisp-edge helper the components draw their outlines with.

#include "TestMd3Common.h"

#include "styles/MdStyleBase.h"

#include <QtCore/QEvent>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

using namespace md;

namespace {

/// The registry needs to be non-empty before the window is shown, and the
/// offscreen platform is what keeps this runnable in CI. Setting it from a
/// static initialiser means a plain `./TestMd3StyleBase` invocation works too,
/// not just the CTest one.
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

/// A bare widget with a meta-object, so installPaintFilter<ProbeWidget>() has
/// something to register.
class ProbeWidget : public QWidget
{
    Q_OBJECT
public:
    using QWidget::QWidget;
};

/// Pattern A style: paints the widget itself instead of going through a
/// stylesheet.
class ProbeStyle : public md::MdStyleBase
{
    Q_OBJECT
public:
    int paintCount = 0;
    int themeUpdateCount = 0;
    QSize lastSize;

    void drawWidget(QPainter *painter, QWidget *widget) override
    {
        ++paintCount;
        lastSize = widget->size();
        painter->fillRect(widget->rect(), QColor(255, 0, 0));
    }

    void onThemeUpdate() override { ++themeUpdateCount; }
};

class TestMd3StyleBase : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void paintFilterRegistryTracksInstallAndRemove();
    void paintFilterDrivesTheWidgetPaint();
    void themeUpdateFiresOnThemeChange();
    void crispRoundedRectSnapsAndPaints();
    void metaObjectLookupWalksSuperclasses();
};

/// A leaked registration would poison every later slot, so the registry is
/// cleared between tests rather than trusted.
void TestMd3StyleBase::cleanup()
{
    md::MdStyleBase::removePaintFilter<ProbeWidget>();
}

void TestMd3StyleBase::paintFilterRegistryTracksInstallAndRemove()
{
    QVERIFY(!md::MdStyleBase::hasPaintFilter(&ProbeWidget::staticMetaObject));

    ProbeStyle style;
    md::MdStyleBase::installPaintFilter<ProbeWidget>(&style);
    QVERIFY(md::MdStyleBase::hasPaintFilter(&ProbeWidget::staticMetaObject));

    md::MdStyleBase::removePaintFilter<ProbeWidget>();
    QVERIFY(!md::MdStyleBase::hasPaintFilter(&ProbeWidget::staticMetaObject));

    // Null arguments must be ignored rather than crash.
    md::MdStyleBase::installPaintFilter<ProbeWidget>(nullptr);
    QVERIFY(!md::MdStyleBase::hasPaintFilter(&ProbeWidget::staticMetaObject));
    md::MdStyleBase::removePaintFilter<ProbeWidget>();
}

void TestMd3StyleBase::paintFilterDrivesTheWidgetPaint()
{
    ProbeStyle style;
    md::MdStyleBase::installPaintFilter<ProbeWidget>(&style);

    ProbeWidget widget;
    widget.resize(64, 40);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QTRY_VERIFY_WITH_TIMEOUT(style.paintCount > 0, 5000);
    QCOMPARE(style.lastSize, QSize(64, 40));

    widget.hide();
}

void TestMd3StyleBase::themeUpdateFiresOnThemeChange()
{
    ProbeStyle style;
    md::MdTheme &theme = md::MdTheme::instance();
    const md::ThemeMode original = theme.themeMode();
    const int before = style.themeUpdateCount;

    theme.setThemeMode(original == md::ThemeMode::Dark ? md::ThemeMode::Light
                                                       : md::ThemeMode::Dark);
    QCOMPARE(style.themeUpdateCount, before + 1);

    theme.setThemeMode(original);
    QCOMPARE(style.themeUpdateCount, before + 2);

    // A no-op assignment must not broadcast, otherwise every component
    // repaints on every idle header refresh.
    const int after = style.themeUpdateCount;
    theme.setThemeMode(original);
    QCOMPARE(style.themeUpdateCount, after);
}

void TestMd3StyleBase::crispRoundedRectSnapsAndPaints()
{
    // Half-pixel edges are snapped so a 1 px outline lands on one device row.
    // 10.3..50.2 -> 10..50, 20.6..40.7 -> 21..41.
    const QPainterPath path =
        md::MdStyleBase::crispRoundedRectPath(QRectF(10.3, 20.6, 39.9, 20.1), 6.0);
    const QRectF bounds = path.boundingRect();
    QVERIFY(qFuzzyCompare(bounds.left(), 10.0));
    QVERIFY(qFuzzyCompare(bounds.top(), 21.0));
    QVERIFY(qFuzzyCompare(bounds.width(), 40.0));
    QVERIFY(qFuzzyCompare(bounds.height(), 20.0));

    QImage image(72, 52, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    md::MdStyleBase::drawCrispRoundedRect(&painter, QRectF(10.0, 10.0, 50.0, 30.0),
                                          QList<qreal>{8.0, 8.0, 8.0, 8.0}, QBrush(Qt::black),
                                          QPen(Qt::NoPen));
    painter.end();
    QVERIFY(mdtest::paintedPixelCount(image, qRgb(255, 255, 255)) > 500);
    // The corners are rounded away.
    QCOMPARE(image.pixel(10, 10), qRgb(255, 255, 255));
    QCOMPARE(image.pixel(59, 39), qRgb(255, 255, 255));

    // Null painter and empty rect are no-ops.
    md::MdStyleBase::drawCrispRoundedRect(nullptr, QRectF(0, 0, 10, 10), {}, QBrush(Qt::black));
    md::MdStyleBase::drawCrispRoundedRect(&painter, QRectF(), {}, QBrush(Qt::black));
}

void TestMd3StyleBase::metaObjectLookupWalksSuperclasses()
{
    // A filter registered on the base type must also catch subclasses, which is
    // what lets one style serve a whole component family.
    class DerivedProbe : public ProbeWidget
    {
    public:
        using ProbeWidget::ProbeWidget;
        // No Q_OBJECT: this deliberately relies on the inherited meta-object.
    };
    QVERIFY(DerivedProbe::staticMetaObject.superClass() != nullptr
            || qstrcmp(DerivedProbe::staticMetaObject.className(), "ProbeWidget") == 0);

    ProbeStyle style;
    md::MdStyleBase::installPaintFilter<ProbeWidget>(&style);

    DerivedProbe derived;
    derived.resize(30, 20);
    derived.show();
    QVERIFY(QTest::qWaitForWindowExposed(&derived));
    QTRY_VERIFY_WITH_TIMEOUT(style.paintCount > 0, 5000);
    QCOMPARE(style.lastSize, QSize(30, 20));

    derived.hide();
}

QTEST_MAIN(TestMd3StyleBase)

#include "TestMd3StyleBase.moc"
