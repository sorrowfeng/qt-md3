// TestMd3TextArea — the multiline text field: MdTextArea + shared tokens.
//
// Pins the shared text-field token set serving both forms (the export
// publishes no `text-area` namespace — material-web renders `type="textarea"`
// through the same rows), the `rows` contract (material-web's default 2),
// the floating label and the error overlay.

#include "core/MdTextFieldTokens.h"
#include "core/MdTheme.h"
#include "styles/MdTextAreaStyle.h"
#include "widgets/MdTextArea.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

using namespace md;

namespace {

QImage render(const MdTextArea &area)
{
    QImage image(area.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const_cast<MdTextArea &>(area).render(&painter);
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

class TestMd3TextArea : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void sharedTokensServeBothForms();
    void rowsContract();
    void labelFloats();
    void errorOverlayApplies();
    void renderSmoke();
    void metaProperties();
};

void TestMd3TextArea::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    QVERIFY(MdTextAreaStyle::shared() != nullptr);
}

void TestMd3TextArea::cleanup()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

void TestMd3TextArea::sharedTokensServeBothForms()
{
    // The export publishes no text-area namespace; both forms resolve the
    // same shared set.
    const MdTextFieldTokens filled = MdTextFieldTokens::resolve(MdTextFieldVariant::Filled);
    QCOMPARE(filled.containerHeight, 56.0);
    QCOMPARE(filled.iconSize, 24.0);
    QCOMPARE(filled.focusIndicatorHeight, 2.0);
}

void TestMd3TextArea::rowsContract()
{
    MdTextArea area;
    QCOMPARE(area.rows(), 2); // material-web's default
    area.setRows(5);
    QCOMPARE(area.rows(), 5);
    // Rows clamp to 1 minimum.
    area.setRows(0);
    QCOMPARE(area.rows(), 1);

    // The body's height follows the rows.
    area.setRows(2);
    const QSize twoRows = area.sizeHint();
    area.setRows(6);
    const QSize sixRows = area.sizeHint();
    QVERIFY(sixRows.height() > twoRows.height());
}

void TestMd3TextArea::labelFloats()
{
    MdTextArea area;
    area.setLabelText(QStringLiteral("Label"));
    QCOMPARE(area.labelFloat(), 0.0);

    area.setPlainText(QStringLiteral("hello"));
    QVERIFY(area.isPopulated());
    QTRY_VERIFY_WITH_TIMEOUT(area.labelFloat() > 0.5, 500);
}

void TestMd3TextArea::errorOverlayApplies()
{
    MdTextArea area;
    QCOMPARE(area.hasError(), false);
    area.setError(true);
    QCOMPARE(area.hasError(), true);
}

void TestMd3TextArea::renderSmoke()
{
    MdTextArea area;
    area.setLabelText(QStringLiteral("Bio"));
    area.setSupportingText(QStringLiteral("Tell us about yourself"));
    area.setRows(3);
    area.resize(320, 160);
    const QImage image = render(area);
    QVERIFY(hasInk(image));
}

void TestMd3TextArea::metaProperties()
{
    MdTextArea area;
    const QMetaObject *meta = area.metaObject();
    QVERIFY(meta->indexOfProperty("labelText") >= 0);
    QVERIFY(meta->indexOfProperty("supportingText") >= 0);
    QVERIFY(meta->indexOfProperty("error") >= 0);
    QVERIFY(meta->indexOfProperty("variant") >= 0);
    QVERIFY(meta->indexOfProperty("rows") >= 0);
    QVERIFY(meta->indexOfProperty("populated") >= 0);

    QSignalSpy spy(&area, &MdTextArea::rowsChanged);
    area.setProperty("rows", 4);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestMd3TextArea)
#include "TestMd3TextArea.moc"
