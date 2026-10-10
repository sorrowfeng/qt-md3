// TestMd3TextField — the Text fields family: MdTextField + tokens + style.
//
// Pins the export's metric rows (the 56 px container, the 24 px icons, the
// indicator heights 1/1/2/1), the filled / outlined variants' container
// contract, the colour tables' state rows and the error overlay, the label's
// float and the disabled opacities.

#include "core/MdTextFieldTokens.h"
#include "core/MdTheme.h"
#include "styles/MdTextFieldStyle.h"
#include "widgets/MdTextField.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

using namespace md;

namespace {

QImage render(const MdTextField &field)
{
    QImage image(field.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const_cast<MdTextField &>(field).render(&painter);
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

class TestMd3TextField : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void tokenTableMatchesTheExport();
    void outlinedVariantDiffers();
    void colourTablesFollowTheExport();
    void geometryFollowsTheTokens();
    void labelFloats();
    void errorOverlayApplies();
    void disabledOpacities();
    void renderSmoke();
    void metaProperties();
};

void TestMd3TextField::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    QVERIFY(MdTextFieldStyle::shared() != nullptr);
}

void TestMd3TextField::cleanup()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

void TestMd3TextField::tokenTableMatchesTheExport()
{
    const MdTextFieldTokens tokens = MdTextFieldTokens::resolve(MdTextFieldVariant::Filled);

    // md.comp.filled-text-field.* at 34.0.21.
    QCOMPARE(tokens.containerHeight, 56.0);
    QCOMPARE(tokens.iconSize, 24.0);
    QCOMPARE(tokens.indicatorHeight, 1.0);
    QCOMPARE(tokens.hoverIndicatorHeight, 1.0);
    QCOMPARE(tokens.focusIndicatorHeight, 2.0);
    QCOMPARE(tokens.containerRadius, 4.0);
    QCOMPARE(tokens.containerRadiusTopOnly, true);
    QCOMPARE(tokens.disabledContainerOpacity, 0.04);
    QCOMPARE(tokens.disabledContentOpacity, 0.38);
}

void TestMd3TextField::outlinedVariantDiffers()
{
    const MdTextFieldTokens filled = MdTextFieldTokens::resolve(MdTextFieldVariant::Filled);
    const MdTextFieldTokens outlined = MdTextFieldTokens::resolve(MdTextFieldVariant::Outlined);

    QCOMPARE(filled.outlined, false);
    QCOMPARE(outlined.outlined, true);
    QCOMPARE(filled.containerColor, ColorRole::SurfaceContainerHighest);
    // The outlined set's container row is absent — the paint treats Count as
    // no fill and draws the 1 px outline instead.
    QCOMPARE(outlined.containerColor, ColorRole::Count);
    QCOMPARE(outlined.containerRadiusTopOnly, false);
}

void TestMd3TextField::colourTablesFollowTheExport()
{
    const MdTextFieldTokens tokens = MdTextFieldTokens::resolve(MdTextFieldVariant::Filled);

    QCOMPARE(tokens.indicator[int(MdTextFieldState::Enabled)].role, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.indicator[int(MdTextFieldState::Hovered)].role, ColorRole::OnSurface);
    QCOMPARE(tokens.indicator[int(MdTextFieldState::Focused)].role, ColorRole::Primary);
    QCOMPARE(tokens.label[int(MdTextFieldState::Focused)].role, ColorRole::Primary);
    QCOMPARE(tokens.label[int(MdTextFieldState::Enabled)].role, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.errorColor, ColorRole::Error);
}

void TestMd3TextField::geometryFollowsTheTokens()
{
    MdTextField field;
    field.setLabelText(QStringLiteral("Label"));
    QCOMPARE(field.sizeHint().height(), 56);

    field.resize(280, 56);
    const QRectF container = field.containerRect();
    QCOMPARE(container.height(), 56.0);
}

void TestMd3TextField::labelFloats()
{
    MdTextField field;
    field.setLabelText(QStringLiteral("Label"));
    QCOMPARE(field.labelFloat(), 0.0);

    // Typing popsulates the field and floats the label.
    field.setText(QStringLiteral("hello"));
    QVERIFY(field.isPopulated());
    QTRY_VERIFY_WITH_TIMEOUT(field.labelFloat() > 0.5, 500);
}

void TestMd3TextField::errorOverlayApplies()
{
    MdTextField field;
    field.setLabelText(QStringLiteral("Label"));
    QCOMPARE(field.hasError(), false);
    field.setError(true);
    QCOMPARE(field.hasError(), true);
}

void TestMd3TextField::disabledOpacities()
{
    const MdTextFieldTokens tokens = MdTextFieldTokens::resolve(MdTextFieldVariant::Filled);
    const int d = int(MdTextFieldState::Disabled);

    QCOMPARE(tokens.indicator[d].opacity, 0.38);
    QCOMPARE(tokens.label[d].opacity, 0.38);
    QCOMPARE(tokens.disabledContainerOpacity, 0.04);
}

void TestMd3TextField::renderSmoke()
{
    MdTextField field;
    field.setLabelText(QStringLiteral("Email"));
    field.setSupportingText(QStringLiteral("Supporting"));
    field.resize(280, 76);
    const QImage image = render(field);
    QVERIFY(hasInk(image));
}

void TestMd3TextField::metaProperties()
{
    MdTextField field;
    const QMetaObject *meta = field.metaObject();
    QVERIFY(meta->indexOfProperty("labelText") >= 0);
    QVERIFY(meta->indexOfProperty("supportingText") >= 0);
    QVERIFY(meta->indexOfProperty("placeholderText") >= 0);
    QVERIFY(meta->indexOfProperty("error") >= 0);
    QVERIFY(meta->indexOfProperty("variant") >= 0);
    QVERIFY(meta->indexOfProperty("populated") >= 0);

    QSignalSpy spy(&field, &MdTextField::labelTextChanged);
    field.setProperty("labelText", QStringLiteral("Name"));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestMd3TextField)
#include "TestMd3TextField.moc"
