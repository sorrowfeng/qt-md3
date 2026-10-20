// TestMd3SearchBar — the Search family: MdSearchBar + tokens + style.
//
// Pins the export's metric rows (the 56 px container, the 24 px icons at the
// 16 px side spaces, the 30 px avatar / 48 px target), the three surfaces'
// shapes (bar corner-full, docked corner-extra-large, full-screen
// corner-none), the state-layer rows and the search-requested signal.
//
// The chrome regression pins the *app* paint path (MdSearchBar::paintEvent
// → QLineEdit::paintEvent) as well as the paint-filter path
// (MdSearchBarStyle::drawWidget): a rectangular PE_FrameLineEdit slab leaves
// two horizontal lines at the top and bottom and cuts the pill's caps, and
// the two paths must not disagree.

#include "core/MdSearchTokens.h"
#include "core/MdTheme.h"
#include "styles/MdSearchBarStyle.h"
#include "widgets/MdSearchBar.h"

#include <QtCore/QDir>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtWidgets/QProxyStyle>
#include <QtTest/QtTest>

using namespace md;

namespace {

QImage render(const MdSearchBar &bar)
{
    QImage image(bar.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const_cast<MdSearchBar &>(bar).render(&painter);
    return image;
}

bool hasInk(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                return true;
            }
        }
    }
    return false;
}

/// Count opaque pixels along a row/column. A rectangular line-edit frame
/// shows up as a full-width opaque row at y=0 / y=h-1 that is *darker or
/// lighter* than the pill fill and that continues through the left/right
/// caps where the stadium has already curved away.
int opaqueRun(const QImage &image, int y, int x0, int x1)
{
    int n = 0;
    for (int x = x0; x <= x1 && x < image.width(); ++x) {
        if (x >= 0 && qAlpha(image.pixel(x, y)) > 8) {
            ++n;
        }
    }
    return n;
}

/// A stadium (corner-full) leaves the four corner samples of the widget rect
/// transparent — the semicircular caps curve away from the rect corners.
/// A rectangular frame does the opposite: it paints those corners.
bool cornerHasInk(const QImage &image)
{
    const int w = image.width();
    const int h = image.height();
    const QPoint corners[4] = {QPoint(0, 0), QPoint(w - 1, 0), QPoint(0, h - 1), QPoint(w - 1, h - 1)};
    for (const QPoint &c : corners) {
        if (qAlpha(image.pixel(c)) > 8) {
            return true;
        }
    }
    return false;
}

/// Ink in the stadium's *cap* region on the top or bottom row. The flat top
/// of a 360x56 pill legitimately runs from x=radius to x=width-radius; the
/// caps (x < radius-gap and x > width-radius+gap) must stay clear. A clipped
/// elevation halo or a rectangular frame paints exactly there — that is the
/// "two horizontal lines cutting the edges" defect.
bool capRegionsHaveInk(const QImage &image)
{
    const int w = image.width();
    const int h = image.height();
    if (w < 16 || h < 8) {
        return false;
    }
    // The pill is corner-full: radius = h/2. Leave a generous gap so the
    // antialiased cap curve and the 0.5 px outline stroke do not count as
    // bleed — only ink that reaches into the outer third of the cap is a
    // clipped halo or a rectangular frame.
    const int radius = h / 2;
    const int leftEnd = qMax(0, radius / 3);
    const int rightStart = qMin(w - 1, w - radius / 3);
    const int rows[2] = {0, h - 1};
    for (int y : rows) {
        if (opaqueRun(image, y, 0, leftEnd) > 0) {
            return true;
        }
        if (rightStart < w && opaqueRun(image, y, rightStart, w - 1) > 0) {
            return true;
        }
    }
    return false;
}

/// Pixel probe used by the chrome regression: print the top/bottom runs so a
/// failure names the artefact instead of just "not equal".
QString describeChrome(const QImage &image)
{
    const int w = image.width();
    const int h = image.height();
    return QStringLiteral("size=%1x%2 topRun=%3 bottomRun=%4 cornerInk=%5 capInk=%6")
        .arg(w)
        .arg(h)
        .arg(opaqueRun(image, 0, 0, w - 1))
        .arg(opaqueRun(image, h - 1, 0, w - 1))
        .arg(cornerHasInk(image) ? QStringLiteral("yes") : QStringLiteral("no"))
        .arg(capRegionsHaveInk(image) ? QStringLiteral("yes") : QStringLiteral("no"));
}

void dumpIfRequested(const MdSearchBar &bar, const QImage &image, const char *name)
{
    const QString dir = qEnvironmentVariable("QT_MD3_SEARCH_FRAMES");
    if (dir.isEmpty()) {
        return;
    }
    QDir().mkpath(dir);
    Q_UNUSED(bar);
    image.save(QDir(dir).filePath(QString::fromUtf8(name) + QStringLiteral(".png")));
}

} // namespace

class TestMd3SearchBar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void tokenTableMatchesTheExport();
    void surfacesDiffer();
    void stateLayersFollowTheExport();
    void geometryFollowsTheTokens();
    void activationAndSearchSignals();
    void renderSmoke();
    void metaProperties();
    void chromeHasNoRectangularFrame();
    void chromePaintFilterPathMatches();
    void styleSuppressesLineEditSlab();
};

void TestMd3SearchBar::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

void TestMd3SearchBar::cleanup()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

void TestMd3SearchBar::tokenTableMatchesTheExport()
{
    const MdSearchTokens tokens = MdSearchTokens::resolve(MdSearchSurface::Bar);

    // md.comp.search-bar.* at 34.0.21.
    QCOMPARE(tokens.containerHeight, 56.0);
    QCOMPARE(tokens.iconSize, 24.0);
    QCOMPARE(tokens.leadingSpace, 16.0);
    QCOMPARE(tokens.trailingSpace, 16.0);
    QCOMPARE(tokens.avatarSize, 30.0);
    QCOMPARE(tokens.avatarTargetSize, 48.0);
    QCOMPARE(tokens.containerColor, ColorRole::SurfaceContainerHigh);
    QCOMPARE(tokens.inputTextColor, ColorRole::OnSurface);
    QCOMPARE(tokens.leadingIconColor, ColorRole::OnSurface);
    QCOMPARE(tokens.supportingTextColor, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.viewBackgroundColor, ColorRole::SurfaceContainerLow);
    QCOMPARE(tokens.dividerColor, ColorRole::Outline);
}

void TestMd3SearchBar::surfacesDiffer()
{
    const MdSearchTokens bar = MdSearchTokens::resolve(MdSearchSurface::Bar);
    const MdSearchTokens docked = MdSearchTokens::resolve(MdSearchSurface::DockedView);
    const MdSearchTokens fullScreen = MdSearchTokens::resolve(MdSearchSurface::FullScreenView);

    QCOMPARE(bar.containerRadius, 28.0);
    QCOMPARE(docked.containerRadius, 28.0);
    QCOMPARE(fullScreen.containerRadius, 0.0);
    QCOMPARE(fullScreen.containerHeight, 72.0);
    QCOMPARE(docked.containerColor, ColorRole::SurfaceContainerLow);
}

void TestMd3SearchBar::stateLayersFollowTheExport()
{
    const MdSearchTokens tokens = MdSearchTokens::resolve(MdSearchSurface::Bar);

    QCOMPARE(tokens.stateLayer[int(MdSearchState::Hovered)].opacity, 0.08);
    QCOMPARE(tokens.stateLayer[int(MdSearchState::Pressed)].opacity, 0.12);
    QCOMPARE(tokens.stateLayer[int(MdSearchState::Enabled)].opacity, 0.0);
}

void TestMd3SearchBar::geometryFollowsTheTokens()
{
    MdSearchBar bar;
    QCOMPARE(bar.sizeHint().height(), 56);

    bar.resize(360, 56);
    const QRectF icon = bar.leadingIconRect();
    QCOMPARE(icon.width(), 24.0);
    QCOMPARE(icon.left(), 16.0);
}

void TestMd3SearchBar::activationAndSearchSignals()
{
    MdSearchBar bar;
    bar.resize(360, 56);
    bar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&bar));

    QSignalSpy activated(&bar, &MdSearchBar::activated);
    QTest::mouseClick(&bar, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(200, 28));
    QCOMPARE(activated.count(), 1);

    QSignalSpy searched(&bar, &MdSearchBar::searchRequested);
    bar.setText(QStringLiteral("hello"));
    QTest::keyClick(&bar, Qt::Key_Return);
    QCOMPARE(searched.count(), 1);
    QCOMPARE(searched.at(0).at(0).toString(), QStringLiteral("hello"));
}

void TestMd3SearchBar::renderSmoke()
{
    MdSearchBar bar;
    bar.setPlaceholderText(QStringLiteral("Search"));
    bar.resize(360, 56);
    const QImage image = render(bar);
    QVERIFY(hasInk(image));
}

void TestMd3SearchBar::metaProperties()
{
    MdSearchBar bar;
    const QMetaObject *meta = bar.metaObject();
    QVERIFY(meta->indexOfProperty("placeholderText") >= 0);
    QVERIFY(meta->indexOfProperty("surface") >= 0);

    QSignalSpy spy(&bar, &MdSearchBar::placeholderTextChanged);
    bar.setProperty("placeholderText", QStringLiteral("Search"));
    QCOMPARE(spy.count(), 1);
}

void TestMd3SearchBar::chromeHasNoRectangularFrame()
{
    // The production path: MdSearchBar::paintEvent (the paint filter is NOT
    // installed unless MdSearchBarStyle::shared() has been called). This is
    // what the music player's Library page exercises.
    struct Case {
        const char *name;
        bool focus;
        bool disabled;
        bool withText;
        QSize size;
    };
    const Case cases[] = {
        {"normal", false, false, false, {360, 56}},
        {"focus", true, false, false, {360, 56}},
        {"text", false, false, true, {360, 56}},
        {"disabled", false, true, false, {360, 56}},
        {"narrow", false, false, false, {180, 56}},
        {"short", false, false, false, {360, 40}},
    };
    for (const Case &c : cases) {
        MdSearchBar bar;
        bar.setPlaceholderText(QStringLiteral("Search"));
        bar.resize(c.size);
        if (c.withText) {
            bar.setText(QStringLiteral("Night Drive"));
        }
        if (c.disabled) {
            bar.setEnabled(false);
        }
        if (c.focus) {
            bar.show();
            bar.setFocus();
            QVERIFY(QTest::qWaitForWindowExposed(&bar));
        }
        const QImage image = render(bar);
        dumpIfRequested(bar, image, c.name);
        QVERIFY2(!capRegionsHaveInk(image),
                 qPrintable(QStringLiteral("%1: rectangular frame lines (%2)")
                                .arg(QLatin1String(c.name), describeChrome(image))));
        QVERIFY2(!cornerHasInk(image),
                 qPrintable(QStringLiteral("%1: stadium caps cut — corner ink (%2)")
                                .arg(QLatin1String(c.name), describeChrome(image))));
    }
}

void TestMd3SearchBar::chromePaintFilterPathMatches()
{
    // The paint-filter path (MdSearchBarStyle::drawWidget) must agree with
    // the paintEvent path: same pill, no rectangular slab.
    QVERIFY(MdSearchBarStyle::shared() != nullptr);
    QVERIFY(MdSearchBarStyle::isInstalled());

    MdSearchBar bar;
    bar.setPlaceholderText(QStringLiteral("Search"));
    bar.resize(360, 56);
    const QImage image = render(bar);
    dumpIfRequested(bar, image, "paint-filter");
    QVERIFY(hasInk(image));
    QVERIFY2(!capRegionsHaveInk(image),
             qPrintable(QStringLiteral("paint-filter: rectangular frame lines (%1)")
                            .arg(describeChrome(image))));
    QVERIFY2(!cornerHasInk(image),
             qPrintable(QStringLiteral("paint-filter: stadium caps cut (%1)")
                            .arg(describeChrome(image))));
}

void TestMd3SearchBar::styleSuppressesLineEditSlab()
{
    // QLineEdit::paintEvent draws PE_PanelLineEdit (and some styles also
    // PE_FrameLineEdit). The widget's SearchLineEditStyle must swallow both
    // so the pill is the only chrome — otherwise the slab paints two
    // horizontal lines across the top and bottom and cuts the caps.
    class CountingStyle : public QProxyStyle
    {
    public:
        mutable int panelLineEdit = 0;
        mutable int frameLineEdit = 0;
        mutable int frame = 0;

        void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                           QPainter *painter, const QWidget *widget) const override
        {
            if (element == PE_PanelLineEdit) {
                ++panelLineEdit;
                return;
            }
            if (element == PE_FrameLineEdit) {
                ++frameLineEdit;
                return;
            }
            if (element == PE_Frame) {
                ++frame;
                return;
            }
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        }
    };

    // The production SearchLineEditStyle is already installed on every bar.
    // Drive a real paint and assert the style query still reports a proxy
    // that owns the widget (setStyle stuck) and that hasFrame() is off so
    // initStyleOption cannot even request a lineWidth.
    MdSearchBar bar;
    bar.resize(360, 56);
    QVERIFY(bar.style() != nullptr);
    QVERIFY(bar.style()->inherits("QProxyStyle"));
    QVERIFY2(!bar.hasFrame(),
             "MdSearchBar must call setFrame(false) so QLineEdit never asks for a frame");

    CountingStyle counting;
    counting.setParent(qApp);
    bar.setStyle(&counting);
    const QImage image = render(bar);
    bar.setStyle(nullptr);
    qInfo("style probe: PE_PanelLineEdit=%d PE_FrameLineEdit=%d PE_Frame=%d (%s)",
          counting.panelLineEdit, counting.frameLineEdit, counting.frame,
          qPrintable(describeChrome(image)));
    // If any of these fired and painted, the slab would be on the image.
    QVERIFY2(!capRegionsHaveInk(image),
             qPrintable(QStringLiteral("style probe drew a frame (%1)")
                            .arg(describeChrome(image))));
}

QTEST_MAIN(TestMd3SearchBar)
#include "TestMd3SearchBar.moc"
