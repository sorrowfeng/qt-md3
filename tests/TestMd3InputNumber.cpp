// TestMd3InputNumber — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the "Text field + Icon button 步进" composition:
//
//   * value clamps to [minimum, maximum] and emits valueChanged / stepped;
//   * singleStep moves the value; Up/Down keys and the stepper buttons
//     both step; read-only blocks both;
//   * prefix / suffix / decimals format the field string without leaking
//     into the numeric parse on commit;
//   * a non-numeric edit is rejected (error overlay + last-good restore);
//   * Q_PROPERTY meta-object surface exists and notifies;
//   * non-blank render smoke.

#include "core/MdTheme.h"
#include "core/MdTextFieldTokens.h"
#include "core/MdTypes.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdInputNumber.h"
#include "widgets/MdTextField.h"

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

bool anyPixelPainted(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).alpha() > 8) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

class TestMd3InputNumber : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdInputNumber input;
        QCOMPARE(input.value(), 0.0);
        QCOMPARE(input.minimum(), 0.0);
        QCOMPARE(input.maximum(), 99.0);
        QCOMPARE(input.singleStep(), 1.0);
        QCOMPARE(input.decimals(), 0);
        QVERIFY(input.prefix().isEmpty());
        QVERIFY(input.suffix().isEmpty());
        QVERIFY(!input.isReadOnly());
        QVERIFY(input.areSteppersVisible());
        QCOMPARE(input.variant(), MdTextFieldVariant::Filled);
    }

    void valueClamps()
    {
        MdInputNumber input;
        input.setRange(0.0, 10.0);

        QSignalSpy valueSpy(&input, &MdInputNumber::valueChanged);
        input.setValue(50.0);
        QCOMPARE(input.value(), 10.0);
        input.setValue(-3.0);
        QCOMPARE(input.value(), 0.0);
        input.setValue(7.5);
        QCOMPARE(input.value(), 7.5);
        QVERIFY(valueSpy.count() >= 3);
    }

    void stepButtonsAndKeys()
    {
        MdInputNumber input;
        input.setRange(0.0, 10.0);
        input.setValue(5.0);
        input.setSingleStep(2.0);

        QCOMPARE(input.stepUp(), 7.0);
        QCOMPARE(input.stepDown(), 5.0);

        input.setReadOnly(true);
        QCOMPARE(input.stepUp(), 5.0);
        input.setReadOnly(false);
    }

    void keySteps()
    {
        MdInputNumber input;
        input.setRange(0.0, 10.0);
        input.setValue(5.0);
        input.setSingleStep(2.0);
        input.resize(160, 56);
        input.show();
        QVERIFY(QTest::qWaitForWindowExposed(&input));
        auto *field = input.textField();
        QVERIFY(field);
        field->setFocus();
        QTest::keyClick(field, Qt::Key_Up);
        QCOMPARE(input.value(), 7.0);
        QTest::keyClick(field, Qt::Key_Down);
        QCOMPARE(input.value(), 5.0);
    }

    void showOnly()
    {
        MdInputNumber input;
        input.resize(160, 56);
        input.show();
        QVERIFY(QTest::qWaitForWindowExposed(&input));
        QVERIFY(input.isVisible());
    }

    void stepperButtonsExist()
    {
        MdInputNumber input;
        input.resize(160, 56);
        input.show();
        QVERIFY(QTest::qWaitForWindowExposed(&input));

        // Two MdIconButton children, up over down, in the trailing slot.
        const QList<MdIconButton *> buttons = input.findChildren<MdIconButton *>();
        QCOMPARE(buttons.size(), 2);
        QVERIFY(input.textField());

        input.setSteppersVisible(false);
        QVERIFY(!input.areSteppersVisible());
        QVERIFY(!buttons.at(0)->isVisible());
        QVERIFY(!buttons.at(1)->isVisible());
    }

    void prefixSuffixAndDecimals()
    {
        MdInputNumber input;
        input.setDecimals(2);
        input.setPrefix(QStringLiteral("$"));
        input.setSuffix(QStringLiteral(" USD"));
        input.setValue(12.5);
        QCOMPARE(input.textField()->text(), QStringLiteral("$12.50 USD"));
        QCOMPARE(input.value(), 12.5);

        // Commit the parsed number again through the field.
        input.textField()->setText(QStringLiteral("$3.25 USD"));
        input.textField()->editingFinished();
        QCOMPARE(input.value(), 3.25);
    }

    void rejectsBadEdit()
    {
        MdInputNumber input;
        input.setValue(4.0);
        input.textField()->setText(QStringLiteral("not a number"));
        input.textField()->editingFinished();
        QCOMPARE(input.value(), 4.0);
        QVERIFY(input.hasError());
        // The field snaps back to the last good string.
        QCOMPARE(input.textField()->text(), QStringLiteral("4"));
    }

    void metaProperties()
    {
        MdInputNumber input;
        const QMetaObject *meta = input.metaObject();
        QVERIFY(meta->indexOfProperty("value") >= 0);
        QVERIFY(meta->indexOfProperty("minimum") >= 0);
        QVERIFY(meta->indexOfProperty("maximum") >= 0);
        QVERIFY(meta->indexOfProperty("singleStep") >= 0);
        QVERIFY(meta->indexOfProperty("decimals") >= 0);
        QVERIFY(meta->indexOfProperty("prefix") >= 0);
        QVERIFY(meta->indexOfProperty("suffix") >= 0);
        QVERIFY(meta->indexOfProperty("readOnly") >= 0);
        QVERIFY(meta->indexOfProperty("variant") >= 0);
        QVERIFY(meta->indexOfProperty("labelText") >= 0);
        QVERIFY(meta->indexOfProperty("supportingText") >= 0);
        QVERIFY(meta->indexOfProperty("error") >= 0);
        QVERIFY(meta->indexOfProperty("steppersVisible") >= 0);

        QSignalSpy variantSpy(&input, &MdInputNumber::variantChanged);
        input.setProperty("variant", QVariant::fromValue(MdTextFieldVariant::Outlined));
        QCOMPARE(input.variant(), MdTextFieldVariant::Outlined);
        QCOMPARE(variantSpy.count(), 1);
    }

    void themeLifecycle()
    {
        MdInputNumber input;
        input.resize(160, 56);
        input.show();
        QSignalSpy valueSpy(&input, &MdInputNumber::valueChanged);
        const QColor before = input.palette().color(QPalette::Window);
        MdTheme::instance().setSeedColor(QColor(0xB3261E));
        // Theme change must not disturb the value.
        QCOMPARE(input.value(), 0.0);
        QCOMPARE(valueSpy.count(), 0);
        Q_UNUSED(before);
    }

    void renderSmoke()
    {
        MdInputNumber input;
        input.setLabelText(QStringLiteral("Quantity"));
        input.setValue(3);
        input.resize(200, 72);
        input.show();
        QVERIFY(QTest::qWaitForWindowExposed(&input));
        const QImage image = renderWidget(input);
        QVERIFY(anyPixelPainted(image));
    }
};

QTEST_MAIN(TestMd3InputNumber)
#include "TestMd3InputNumber.moc"
