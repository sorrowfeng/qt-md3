// TestMd3Divider — the divider family against its token sources.
//
// Numbers transcribed from material-web
// tokens/versions/latest/sass/_md-comp-divider.scss (thickness 1px, colour
// outline-variant — the whole export), with the behaviour rows from androidx
// Compose material3 Divider.kt (HorizontalDivider / VerticalDivider, the
// thickness/color parameters) and the m3.material.io measurements (the 16dp
// inset margins) — the three sources the token header names.

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdDividerStyle.h"
#include "widgets/MdDivider.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

QColor pixelColorAt(MdDivider &divider, const QPointF &position)
{
    QImage image(int(divider.width()), int(divider.height()),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    // DrawChildren only: a top-level widget would otherwise be erased with
    // its opaque window background first, and the transparent inset pixels
    // the inset tests look for could never appear.
    divider.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32).pixelColor(position.toPoint());
}

} // namespace

class TestMd3Divider : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void theTokenTableMatchesTheExport();
    void aDividerIsNeverInteractive();
    void horizontalInsetGeometry();
    void verticalInsetGeometry();
    void rtlMirrorsStartAndEnd();
    void customThicknessAndColor();
    void sizeHintsAndPolicies();
    void overridesReachTheTokens();
    void renderSmoke();

private:
    MdDivider *m_horizontal = nullptr;
    MdDivider *m_vertical = nullptr;
};

void TestMd3Divider::initTestCase()
{
    MdTheme::instance();

    m_horizontal = new MdDivider;
    m_horizontal->resize(200, 1);
    m_horizontal->show();

    m_vertical = new MdDivider(Qt::Vertical);
    m_vertical->resize(1, 200);
    m_vertical->show();
}

void TestMd3Divider::cleanup()
{
    // Token overrides are global state; every test that sets one removes it.
    MdComponentTokens::global().remove(QStringLiteral("md.comp.divider.thickness"));
    MdComponentTokens::global().remove(QStringLiteral("md.comp.divider.inset"));
    m_horizontal->componentTokens().clear();
    m_vertical->componentTokens().clear();
    m_horizontal->setInsetMode(MdDivider::InsetMode::None);
    m_horizontal->setThickness(0.0);
    m_horizontal->setCustomColor(QColor());
    m_vertical->setThickness(0.0);
    // A failed compare aborts its slot mid-way, so every piece of mutable
    // state any slot touches is reset here — including the layout direction
    // the RTL test swaps.
    m_horizontal->setLayoutDirection(Qt::LeftToRight);
    m_horizontal->resize(200, 1);
}

void TestMd3Divider::theTokenTableMatchesTheExport()
{
    // The whole export is two rows: thickness 1px, colour outline-variant.
    const MdDividerTokens tokens = MdDividerTokens::resolve();
    QCOMPARE(tokens.thickness, 1.0);
    QCOMPARE(int(tokens.color), int(ColorRole::OutlineVariant));

    // The inset is a spec measurement, not an export row — but it is
    // tokenised here, at the measured 16dp.
    QCOMPARE(tokens.inset, 16.0);

    // And the state half of a component token table is absent: no hover, no
    // focus, no press, no disabled row is published at all.
}

void TestMd3Divider::aDividerIsNeverInteractive()
{
    // The export publishes no state rows, so the widget contracts for none
    // of them: no focus, and clicks pass through the hairline.
    QCOMPARE(int(m_horizontal->focusPolicy()), int(Qt::NoFocus));
    QVERIFY(m_horizontal->testAttribute(Qt::WA_TransparentForMouseEvents));
    QVERIFY(m_vertical->testAttribute(Qt::WA_TransparentForMouseEvents));
    QCOMPARE(m_horizontal->accessibleName(), QStringLiteral("divider"));
}

void TestMd3Divider::horizontalInsetGeometry()
{
    // --- full width: 100% of the widget ------------------------------------
    m_horizontal->setInsetMode(MdDivider::InsetMode::None);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_horizontal, m_horizontal->tokens());
        QCOMPARE(layout.line, QRectF(0.0, 0.0, 200.0, 1.0));
    }

    // --- the spec's "inset": start margin 16dp, end 0 ----------------------
    m_horizontal->setInsetMode(MdDivider::InsetMode::Start);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_horizontal, m_horizontal->tokens());
        QCOMPARE(layout.line, QRectF(16.0, 0.0, 200.0 - 16.0, 1.0));
    }

    // --- material-web's [inset-end]: the trailing edge only ----------------
    m_horizontal->setInsetMode(MdDivider::InsetMode::End);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_horizontal, m_horizontal->tokens());
        QCOMPARE(layout.line, QRectF(0.0, 0.0, 200.0 - 16.0, 1.0));
    }

    // --- the spec's "middle-inset" / material-web's [inset]: both edges ----
    m_horizontal->setInsetMode(MdDivider::InsetMode::Both);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_horizontal, m_horizontal->tokens());
        QCOMPARE(layout.line, QRectF(16.0, 0.0, 200.0 - 32.0, 1.0));
    }

    // The inset never paints a negative-length line, however narrow the
    // widget gets.
    m_horizontal->resize(20, 1);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_horizontal, m_horizontal->tokens());
        QCOMPARE(layout.line.width(), 0.0);
    }
    m_horizontal->resize(200, 1);
    m_horizontal->setInsetMode(MdDivider::InsetMode::None);
}

void TestMd3Divider::verticalInsetGeometry()
{
    // The vertical divider (Compose's VerticalDivider [compose]) swaps the
    // roles: the inset pads top/bottom, the line spans the full width.
    QCOMPARE(m_vertical->orientation(), Qt::Vertical);
    // A shown top-level window may be resized by the platform; pin the
    // geometry the geometry tests reason about.
    m_vertical->resize(1, 200);
    m_vertical->setInsetMode(MdDivider::InsetMode::Start);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_vertical, m_vertical->tokens());
        QCOMPARE(layout.line, QRectF(0.0, 16.0, 1.0, 200.0 - 16.0));
    }
    m_vertical->setInsetMode(MdDivider::InsetMode::Both);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_vertical, m_vertical->tokens());
        QCOMPARE(layout.line, QRectF(0.0, 16.0, 1.0, 200.0 - 32.0));
    }
    m_vertical->setInsetMode(MdDivider::InsetMode::None);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_vertical, m_vertical->tokens());
        QCOMPARE(layout.line, QRectF(0.0, 0.0, 1.0, 200.0));
    }
}

void TestMd3Divider::rtlMirrorsStartAndEnd()
{
    // The inset pads *inline* edges — material-web spells the attribute
    // padding-inline-* — so an RTL layout swaps the physical sides.
    m_horizontal->setInsetMode(MdDivider::InsetMode::Start);
    m_horizontal->setLayoutDirection(Qt::RightToLeft);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_horizontal, m_horizontal->tokens());
        QCOMPARE(layout.line, QRectF(0.0, 0.0, 200.0 - 16.0, 1.0));
    }

    m_horizontal->setInsetMode(MdDivider::InsetMode::End);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_horizontal, m_horizontal->tokens());
        QCOMPARE(layout.line, QRectF(16.0, 0.0, 200.0 - 16.0, 1.0));
    }

    m_horizontal->setLayoutDirection(Qt::LeftToRight);
    m_horizontal->setInsetMode(MdDivider::InsetMode::None);
}

void TestMd3Divider::customThicknessAndColor()
{
    // Compose exposes thickness and color as parameters [compose]; the
    // widget mirrors that with 0 / invalid meaning "the token".
    QCOMPARE(m_horizontal->thickness(), 1.0);
    QCOMPARE(m_horizontal->effectiveColor(),
             MdTheme::instance().color(ColorRole::OutlineVariant));

    m_horizontal->setThickness(4.0);
    QCOMPARE(m_horizontal->thickness(), 4.0);
    QCOMPARE(m_horizontal->sizeHint(), QSize(4, 4));
    m_horizontal->resize(200, 4);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_horizontal, m_horizontal->tokens());
        QCOMPARE(layout.line.height(), 4.0);
    }

    const QColor custom(0x11, 0x22, 0x33);
    m_horizontal->setCustomColor(custom);
    QCOMPARE(m_horizontal->effectiveColor(), custom);

    // Resetting to the token values.
    m_horizontal->setThickness(0.0);
    m_horizontal->setCustomColor(QColor());
    QCOMPARE(m_horizontal->thickness(), 1.0);
    QCOMPARE(m_horizontal->effectiveColor(),
             MdTheme::instance().color(ColorRole::OutlineVariant));
    m_horizontal->resize(200, 1);
}

void TestMd3Divider::sizeHintsAndPolicies()
{
    // Compose: fillMaxWidth().height(thickness) horizontal, fillMaxHeight()
    // .width(thickness) vertical [compose] — the long axis expands, the
    // cross axis is fixed to the hairline.
    QCOMPARE(m_horizontal->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
    QCOMPARE(m_horizontal->sizePolicy().verticalPolicy(), QSizePolicy::Fixed);
    QCOMPARE(m_horizontal->sizeHint(), QSize(1, 1));
    QCOMPARE(m_horizontal->minimumSizeHint(), QSize(1, 1));

    QCOMPARE(m_vertical->sizePolicy().horizontalPolicy(), QSizePolicy::Fixed);
    QCOMPARE(m_vertical->sizePolicy().verticalPolicy(), QSizePolicy::Expanding);
    QCOMPARE(m_vertical->sizeHint(), QSize(1, 1));

    m_horizontal->setThickness(4.0);
    QCOMPARE(m_horizontal->sizeHint(), QSize(4, 4));
    m_horizontal->setThickness(0.0);
}

void TestMd3Divider::overridesReachTheTokens()
{
    // --- instance-level ---------------------------------------------------
    m_horizontal->componentTokens().setValue(QStringLiteral("md.comp.divider.thickness"),
                                             QStringLiteral("3px"));
    QCOMPARE(m_horizontal->thickness(), 3.0);

    m_horizontal->componentTokens().setValue(QStringLiteral("md.comp.divider.inset"),
                                             QStringLiteral("24px"));
    m_horizontal->setInsetMode(MdDivider::InsetMode::Start);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(*m_horizontal, m_horizontal->tokens());
        QCOMPARE(layout.line.left(), 24.0);
    }

    // --- application-wide -------------------------------------------------
    MdComponentTokens::global().setValue(QStringLiteral("md.comp.divider.inset"),
                                         QStringLiteral("40px"));
    MdDivider fresh;
    fresh.resize(200, 1);
    fresh.setInsetMode(MdDivider::InsetMode::Both);
    {
        const MdDividerStyle::Layout layout =
            MdDividerStyle::layoutFor(fresh, fresh.tokens());
        QCOMPARE(layout.line, QRectF(40.0, 0.0, 200.0 - 80.0, 1.0));
    }

    // A malformed length is ignored, not coerced.
    MdComponentTokens::global().setValue(QStringLiteral("md.comp.divider.thickness"),
                                         QStringLiteral("thick"));
    MdDivider typo;
    QCOMPARE(typo.thickness(), 1.0);
}

void TestMd3Divider::renderSmoke()
{
    // The hairline renders exactly one outline-variant row and nothing else.
    const QColor line = MdTheme::instance().color(ColorRole::OutlineVariant);

    m_horizontal->setInsetMode(MdDivider::InsetMode::None);
    m_horizontal->resize(200, 1);
    QCOMPARE(pixelColorAt(*m_horizontal, QPointF(100, 0)), line);
    QCOMPARE(pixelColorAt(*m_horizontal, QPointF(0, 0)), line);

    // The inset removes the leading pixels — they go transparent.
    m_horizontal->setInsetMode(MdDivider::InsetMode::Start);
    QCOMPARE(pixelColorAt(*m_horizontal, QPointF(0, 0)).alpha(), 0);
    QCOMPARE(pixelColorAt(*m_horizontal, QPointF(199, 0)), line);
    m_horizontal->setInsetMode(MdDivider::InsetMode::None);

    // A thicker line is a crisp band of the custom colour — every pixel in
    // the band, not an antialiased blur.
    const QColor custom(0x11, 0x22, 0x33);
    m_horizontal->setThickness(4.0);
    m_horizontal->setCustomColor(custom);
    m_horizontal->resize(200, 4);
    QCOMPARE(pixelColorAt(*m_horizontal, QPointF(100, 0)), custom);
    QCOMPARE(pixelColorAt(*m_horizontal, QPointF(100, 3)), custom);

    // The vertical divider paints its column.
    m_vertical->resize(1, 200);
    QCOMPARE(pixelColorAt(*m_vertical, QPointF(0, 100)), line);
}

QTEST_MAIN(TestMd3Divider)
#include "TestMd3Divider.moc"
