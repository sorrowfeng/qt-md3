// TestMd3Select — Stage 2 Qt extension (Appendix D, P0), the exposed
// dropdown menu.
//
// MdSelect is the M3 composition of a text field and a menu: the field
// shows the current choice (read-only, with the text-field variant / label /
// error chrome) and the menu offers the options. This suite pins:
//
//   * the model contract — addItem / insertItem / removeItem / clear and the
//     currentIndex / currentText pair with NOTIFY signals;
//   * the field appearance properties (variant, label, supporting, error,
//     leading icon) forwarding to the embedded MdTextField;
//   * popup open/close through showPopup / hidePopup and the user path
//     (clicking the field);
//   * keyboard: Enter / Space / F4 / Alt+Down open, Escape closes, Up/Down
//     move the selection when closed;
//   * activated() fires only on the user path, not on setCurrentIndex.

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdMenu.h"
#include "widgets/MdSelect.h"
#include "widgets/MdTextField.h"

#include <QtGui/QImage>
#include <QtGui/QPalette>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>

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

class TestMd3Select : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdSelect select;
        QCOMPARE(select.count(), 0);
        QCOMPARE(select.currentIndex(), -1);
        QVERIFY(select.currentText().isEmpty());
        QCOMPARE(select.variant(), MdTextFieldVariant::Filled);
        QVERIFY(!select.hasError());
        QVERIFY(!select.isPopupVisible());
    }

    void addItemSelectsFirst()
    {
        MdSelect select;
        QSignalSpy indexSpy(&select, &MdSelect::currentIndexChanged);
        QSignalSpy textSpy(&select, &MdSelect::currentTextChanged);

        const int index = select.addItem(QStringLiteral("Apple"));
        QCOMPARE(index, 0);
        QCOMPARE(select.count(), 1);
        QCOMPARE(select.currentIndex(), 0);
        QCOMPARE(select.currentText(), QStringLiteral("Apple"));
        QCOMPARE(indexSpy.count(), 1);
        QCOMPARE(textSpy.count(), 1);
    }

    void addItemsAndModelOps()
    {
        MdSelect select;
        select.addItems({QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")});
        QCOMPARE(select.count(), 3);
        QCOMPARE(select.currentText(), QStringLiteral("A"));

        select.setCurrentIndex(2);
        QCOMPARE(select.currentText(), QStringLiteral("C"));

        select.insertItem(1, QStringLiteral("B2"));
        QCOMPARE(select.count(), 4);
        // The current selection shifted down with the insertion.
        QCOMPARE(select.currentIndex(), 3);
        QCOMPARE(select.currentText(), QStringLiteral("C"));

        select.removeItem(0);
        QCOMPARE(select.count(), 3);
        QCOMPARE(select.currentIndex(), 2);
        QCOMPARE(select.currentText(), QStringLiteral("C"));

        select.clear();
        QCOMPARE(select.count(), 0);
        QCOMPARE(select.currentIndex(), -1);
        QVERIFY(select.currentText().isEmpty());
    }

    void setCurrentIndexEmitsSignalsButNotActivated()
    {
        MdSelect select;
        select.addItems({QStringLiteral("X"), QStringLiteral("Y")});

        QSignalSpy activatedSpy(&select, &MdSelect::activated);
        QSignalSpy indexSpy(&select, &MdSelect::currentIndexChanged);

        select.setCurrentIndex(1);
        QCOMPARE(indexSpy.count(), 1);
        QCOMPARE(activatedSpy.count(), 0); // programmatic, not a user pick
    }

    void fieldAppearanceForwards()
    {
        MdSelect select;
        select.setVariant(MdTextFieldVariant::Outlined);
        QCOMPARE(select.variant(), MdTextFieldVariant::Outlined);

        select.setLabelText(QStringLiteral("Flavour"));
        QCOMPARE(select.labelText(), QStringLiteral("Flavour"));

        select.setSupportingText(QStringLiteral("Pick one"));
        QCOMPARE(select.supportingText(), QStringLiteral("Pick one"));

        select.setPlaceholderText(QStringLiteral("Choose…"));
        QCOMPARE(select.placeholderText(), QStringLiteral("Choose…"));

        select.setError(true);
        QVERIFY(select.hasError());

        select.setLeadingIconName(QStringLiteral("restaurant"));
        QCOMPARE(select.leadingIconName(), QStringLiteral("restaurant"));
    }

    void popupOpensAndCloses()
    {
        MdSelect select;
        select.addItems({QStringLiteral("One"), QStringLiteral("Two")});
        select.resize(240, 56);
        select.show();
        QVERIFY(QTest::qWaitForWindowExposed(&select));

        select.showPopup();
        QVERIFY(select.isPopupVisible());

        select.hidePopup();
        QVERIFY(!select.isPopupVisible());
    }

    void keyboardOpensAndMoves()
    {
        MdSelect select;
        select.addItems({QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")});
        select.resize(240, 56);
        select.show();
        QVERIFY(QTest::qWaitForWindowExposed(&select));
        select.setFocus(Qt::OtherFocusReason);

        // Enter opens.
        QTest::keyClick(&select, Qt::Key_Return);
        QVERIFY(select.isPopupVisible());

        // Escape closes.
        QTest::keyClick(&select, Qt::Key_Escape);
        QVERIFY(!select.isPopupVisible());

        // Alt+Down opens.
        QTest::keyClick(&select, Qt::Key_Down, Qt::AltModifier);
        QVERIFY(select.isPopupVisible());
        QTest::keyClick(&select, Qt::Key_Escape);

        // Up/Down move the selection when closed and emit activated.
        QSignalSpy activatedSpy(&select, &MdSelect::activated);
        const int start = select.currentIndex();
        QTest::keyClick(&select, Qt::Key_Down);
        QCOMPARE(select.currentIndex(), qMin(start + 1, select.count() - 1));
        QCOMPARE(activatedSpy.count(), 1);
    }

    void nonBlankRender()
    {
        MdSelect select;
        select.addItems({QStringLiteral("Tea"), QStringLiteral("Coffee")});
        select.setLabelText(QStringLiteral("Drink"));
        select.resize(280, 72);
        select.show();
        QVERIFY(QTest::qWaitForWindowExposed(&select));

        const QImage image = renderWidget(select);
        QVERIFY2(anyPixelPainted(image), "MdSelect must paint its text-field chrome");
    }

    void menuClearIsSafe()
    {
        // MdSelect rebuilds the menu on every model change; clear() must not
        // leave dangling rows behind.
        MdSelect select;
        select.addItems({QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3")});
        select.addItems({QStringLiteral("4")});
        QCOMPARE(select.count(), 4);
        select.removeItem(1);
        QCOMPARE(select.count(), 3);
        select.clear();
        QCOMPARE(select.count(), 0);
        select.addItem(QStringLiteral("fresh"));
        QCOMPARE(select.count(), 1);
        QCOMPARE(select.currentText(), QStringLiteral("fresh"));
    }

    void fieldShowsCurrentText()
    {
        // The read-only MdTextField must mirror currentText — the ComboBox
        // contract. It is private, so look it up as a child QLineEdit.
        MdSelect select;
        select.setLabelText(QStringLiteral("Theme"));
        select.addItems({QStringLiteral("Light"), QStringLiteral("Dark"), QStringLiteral("System")});
        select.setCurrentIndex(1);
        auto *field = select.findChild<QLineEdit *>();
        QVERIFY(field != nullptr);
        QCOMPARE(field->text(), QStringLiteral("Dark"));
        QCOMPARE(field->text(), select.currentText());
    }

    void fieldShowsCurrentTextAfterThemeSwitch()
    {
        MdSelect select;
        select.setLabelText(QStringLiteral("Theme"));
        select.addItems({QStringLiteral("Light"), QStringLiteral("Dark"), QStringLiteral("System")});
        select.setCurrentIndex(1);
        select.resize(280, 56);
        select.show();
        QVERIFY(QTest::qWaitForWindowExposed(&select));

        auto *field = select.findChild<QLineEdit *>();
        QVERIFY(field != nullptr);

        for (int i = 0; i < 6; ++i) {
            MdTheme::instance().setThemeMode(i % 2 == 0 ? ThemeMode::Dark : ThemeMode::Light);
            QCOMPARE(select.currentIndex(), 1);
            QCOMPARE(select.currentText(), QStringLiteral("Dark"));
            QCOMPARE(field->text(), QStringLiteral("Dark"));
            const QColor textColour = field->palette().color(QPalette::Text);
            const QColor surface = MdTheme::instance().color(ColorRole::Surface);
            const int distance = qAbs(textColour.red() - surface.red())
                + qAbs(textColour.green() - surface.green())
                + qAbs(textColour.blue() - surface.blue());
            QVERIFY2(distance > 40,
                     qPrintable(QStringLiteral("select field text colour %1 too close to surface %2")
                                    .arg(textColour.name())
                                    .arg(surface.name())));
        }
        MdTheme::instance().setThemeMode(ThemeMode::Light);
        QCOMPARE(field->text(), QStringLiteral("Dark"));
    }
};

QTEST_MAIN(TestMd3Select)
#include "TestMd3Select.moc"
