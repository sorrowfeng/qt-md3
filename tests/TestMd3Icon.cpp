// MdIcon — the codepoint table, the Material Symbols font front end and the
// bundled classic-SVG baseline.
//
// The interesting failures here are silent ones: an icon name that resolves to
// the wrong glyph, or a "missing" icon that draws a tofu box instead of
// reporting itself missing. Both are covered below.

#include "TestMd3Common.h"

#include "core/MdIcon.h"
#include "core/MdResources.h"

#include <QtCore/QFile>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtTest/QtTest>

using namespace md;

class TestMd3Icon : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void resourcesAreLinkedIntoTheLibrary();
    void codepointTableLoadsFromResources();
    void codepointLookupIsExact();
    void unknownIconIsReportedNotGuessed();
    void customIconsCanBeRegistered();
    void fontFamiliesMatchTokenNames();
    void classicBaselineIsBundled();
    void classicPixmapIsTintedAndCached();
    void paintPrefersSymbolsAndFallsBackToSvg();
};

void TestMd3Icon::initTestCase()
{
    // Every slot below reads the table, so load it once and fail loudly here
    // rather than with a confusing cascade later.
    QVERIFY2(MdIcon::loadCodepoints(),
             "resources/icons/material-symbols-codepoints.txt is missing from the Qt resource bundle");
}

void TestMd3Icon::resourcesAreLinkedIntoTheLibrary()
{
    // Regression guard for the static-archive trap: the .qrc object lives in
    // libqt-md3.a and nothing referenced its initialiser, so the linker dropped
    // it and every `:/qt-md3/...` lookup failed for tests and applications
    // alike. MdResources::ensure() is the reference that keeps it.
    QVERIFY(MdResources::isInitialized());

    QFile codepoints(MdIcon::codepointsResourcePath());
    QVERIFY2(codepoints.open(QIODevice::ReadOnly | QIODevice::Text),
             qPrintable(QStringLiteral("cannot open %1").arg(codepoints.fileName())));
    QVERIFY(codepoints.size() > 10'000);
    codepoints.close();

    // The classic directory has to be enumerable through the same archive.
    QVERIFY(MdIcon::classicNames().size() >= 20);
}

void TestMd3Icon::codepointTableLoadsFromResources()
{
    QVERIFY(MdIcon::loadCodepoints());
    // google/material-design-icons publishes 4299 entries for this font.
    QCOMPARE(MdIcon::count(), 4299);
    QCOMPARE(MdIcon::names().size(), 4299);
    QVERIFY(MdIcon::names().contains(QStringLiteral("home")));
}

void TestMd3Icon::codepointLookupIsExact()
{
    // Values read straight out of the bundled, verbatim upstream table.
    QCOMPARE(MdIcon::codepoint(QStringLiteral("home")), 0xE9B2u);
    QCOMPARE(MdIcon::codepoint(QStringLiteral("settings")), 0xE8B8u);
    QCOMPARE(MdIcon::codepoint(QStringLiteral("search")), 0xEF7Au);
    QCOMPARE(MdIcon::codepoint(QStringLiteral("menu")), 0xE5D2u);
    QCOMPARE(MdIcon::codepoint(QStringLiteral("favorite")), 0xE87Eu);
    QCOMPARE(MdIcon::codepoint(QStringLiteral("dark_mode")), 0xE51Cu);

    // glyph() must be a single UTF-16 unit, i.e. inside the BMP.
    const QString glyph = MdIcon::glyph(QStringLiteral("home"));
    QCOMPARE(glyph.size(), 1);
    QCOMPARE(glyph.at(0).unicode(), ushort(0xE9B2));
}

void TestMd3Icon::unknownIconIsReportedNotGuessed()
{
    QVERIFY(!MdIcon::contains(QStringLiteral("definitely-not-an-icon")));
    QCOMPARE(MdIcon::codepoint(QStringLiteral("definitely-not-an-icon")), 0u);
    QVERIFY(MdIcon::glyph(QStringLiteral("definitely-not-an-icon")).isEmpty());
    QVERIFY(!MdIcon::contains(QString()));

    QImage image(24, 24, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    QVERIFY(!MdIcon::paint(&painter, QRectF(0, 0, 24, 24),
                           QStringLiteral("definitely-not-an-icon"), QColor(Qt::black),
                           MdIconSet::Classic));
    painter.end();
    QCOMPARE(mdtest::paintedPixelCount(image, qRgb(255, 255, 255)), 0);
}

void TestMd3Icon::customIconsCanBeRegistered()
{
    const QString name = QStringLiteral("qt-md3-test-icon");
    QVERIFY(!MdIcon::contains(name));
    MdIcon::registerIcon(name, 0xE999);
    QVERIFY(MdIcon::contains(name));
    QCOMPARE(MdIcon::codepoint(name), 0xE999u);
    QCOMPARE(MdIcon::glyph(name).at(0).unicode(), ushort(0xE999));

    // Registering nothing must not corrupt the table.
    const int before = MdIcon::count();
    MdIcon::registerIcon(QString(), 0);
    MdIcon::registerIcon(name, 0);
    QCOMPARE(MdIcon::count(), before);
}

void TestMd3Icon::fontFamiliesMatchTokenNames()
{
    // tokens/_md-comp-icon.scss: font = 'Material Symbols Outlined', size 24px.
    QCOMPARE(MdIcon::fontFamily(MdIconFamily::Outlined),
             QStringLiteral("Material Symbols Outlined"));
    QCOMPARE(MdIcon::fontFamily(MdIconFamily::Rounded),
             QStringLiteral("Material Symbols Rounded"));
    QCOMPARE(MdIcon::fontFamily(MdIconFamily::Sharp),
             QStringLiteral("Material Symbols Sharp"));
    QVERIFY(qFuzzyCompare(MdIcon::defaultSize(), 24.0));
    QVERIFY(qFuzzyCompare(MdIcon::preferredSize().width(), 24.0));

    // The four axes must be recorded on the font where the platform allows it.
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    QVERIFY(MdIcon::axesSupported());
    MdIconStyle style;
    style.fill = 1.0;
    style.weight = 500.0;
    style.grade = 25.0;
    style.opticalSize = 40.0;
    const QFont font = MdIcon::font(MdIconFamily::Outlined, style, 40.0);
    QCOMPARE(font.variableAxisValue(QFont::Tag("FILL")), 1.0f);
    QCOMPARE(font.variableAxisValue(QFont::Tag("wght")), 500.0f);
    QCOMPARE(font.variableAxisValue(QFont::Tag("GRAD")), 25.0f);
    QCOMPARE(font.variableAxisValue(QFont::Tag("opsz")), 40.0f);
#else
    QVERIFY(!MdIcon::axesSupported());
#endif

    QVERIFY(MdIconStyle().isDefault());
    MdIconStyle nonDefault;
    nonDefault.weight = 500.0;
    QVERIFY(!nonDefault.isDefault());
}

void TestMd3Icon::classicBaselineIsBundled()
{
    const QStringList names = MdIcon::classicNames();
    QVERIFY(names.size() >= 20);
    QVERIFY(names.contains(QStringLiteral("home")));
    QVERIFY(names.contains(QStringLiteral("settings")));
    QVERIFY(MdIcon::hasClassicIcon(QStringLiteral("home")));
    QVERIFY(!MdIcon::hasClassicIcon(QStringLiteral("nope")));
    // Every bundled SVG must also be a Material Symbols name, otherwise the
    // two back ends disagree about what an icon is called.
    for (const QString &name : names) {
        QVERIFY2(MdIcon::contains(name),
                 qPrintable(QStringLiteral("classic icon not in Symbols table: %1").arg(name)));
    }
}

void TestMd3Icon::classicPixmapIsTintedAndCached()
{
    const QPixmap red = MdIcon::classicPixmap(QStringLiteral("home"), 24, QColor(255, 0, 0));
    QVERIFY(!red.isNull());
    QCOMPARE(red.size(), QSize(24, 24));
    const QImage image = red.toImage();
    QVERIFY(image.pixelColor(12, 12).red() > 200);
    QCOMPARE(image.pixelColor(12, 12).green(), 0);
    QCOMPARE(image.pixelColor(12, 12).blue(), 0);

    // Same arguments return the cached instance.
    const QPixmap again = MdIcon::classicPixmap(QStringLiteral("home"), 24, QColor(255, 0, 0));
    QCOMPARE(again.cacheKey(), red.cacheKey());

    QVERIFY(MdIcon::classicPixmap(QStringLiteral("nope"), 24, Qt::black).isNull());
    QVERIFY(MdIcon::classicPixmap(QStringLiteral("home"), 0, Qt::black).isNull());
}

void TestMd3Icon::paintPrefersSymbolsAndFallsBackToSvg()
{
    // The classic set must always be able to render a bundled icon.
    QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    QVERIFY(MdIcon::paint(&painter, QRectF(0, 0, 32, 32), QStringLiteral("home"),
                          QColor(Qt::black), MdIconSet::Classic));
    painter.end();
    QVERIFY(mdtest::paintedPixelCount(image, qRgb(255, 255, 255)) > 20);

    // Auto picks whichever back end the platform can actually serve, and must
    // never end up with nothing.
    const MdIconSet resolved = MdIcon::resolveSet(MdIconSet::Auto);
    QVERIFY(resolved == MdIconSet::MaterialSymbols || resolved == MdIconSet::Classic);

    // Asking for Symbols without the font installed degrades to the SVG
    // baseline rather than drawing an empty box.
    if (!MdIcon::isFontAvailable(MdIconFamily::Outlined)) {
        QCOMPARE(MdIcon::resolveSet(MdIconSet::MaterialSymbols), MdIconSet::Classic);
    }

    // A degenerate target rect must be refused, not drawn at zero scale.
    QImage tiny(4, 4, QImage::Format_ARGB32_Premultiplied);
    tiny.fill(Qt::white);
    QPainter painter2(&tiny);
    QVERIFY(!MdIcon::paint(&painter2, QRectF(), QStringLiteral("home"), QColor(Qt::black),
                           MdIconSet::Classic));
    painter2.end();
}

QTEST_MAIN(TestMd3Icon)

#include "TestMd3Icon.moc"
