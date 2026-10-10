// TestMd3SearchBar — the Search family: MdSearchBar + tokens + style.
//
// Pins the export's metric rows (the 56 px container, the 24 px icons at the
// 16 px side spaces, the 30 px avatar / 48 px target), the three surfaces'
// shapes (bar corner-full, docked corner-extra-large, full-screen
// corner-none), the state-layer rows and the search-requested signal.

#include "core/MdSearchTokens.h"
#include "core/MdTheme.h"
#include "styles/MdSearchBarStyle.h"
#include "widgets/MdSearchBar.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
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
};

void TestMd3SearchBar::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    QVERIFY(MdSearchBarStyle::shared() != nullptr);
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

QTEST_MAIN(TestMd3SearchBar)
#include "TestMd3SearchBar.moc"
