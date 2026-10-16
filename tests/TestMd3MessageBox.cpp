// TestMd3MessageBox — Stage 2 Qt extension (Appendix D, P0).
//
// MdMessageBox is the QMessageBox replacement: Dialog tokens + Buttons +
// semantic colour. This suite pins:
//
//   * the dialog token anchor (extra-large corner, surface-container-high,
//     level 3) through the resolved MdDialogTokens the box paints with;
//   * the variant -> semantic colour role mapping (Information / Question ->
//     primary, Warning -> tertiary, Error -> error);
//   * title / text / informativeText Q_PROPERTYs with NOTIFY;
//   * standard buttons produce MdButton children (Filled for affirmative,
//     Text for dismissive) and exec() returns the standard-button value;
//   * Escape dismisses with result 0.

#include "core/MdDialogTokens.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdAvatar.h"
#include "widgets/MdButton.h"
#include "widgets/MdMessageBox.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtCore/QTimer>
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

class TestMd3MessageBox : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdMessageBox box;
        QCOMPARE(box.variant(), MdMessageBoxVariant::None);
        QCOMPARE(box.result(), 0);
        QCOMPARE(box.standardButtons(), MdMessageBoxStandardButton::Ok);
        QCOMPARE(box.buttons().size(), 1);
    }

    void dialogTokenAnchor()
    {
        const MdDialogTokens tokens = MdDialogTokens::resolve();
        QCOMPARE(tokens.containerColor, ColorRole::SurfaceContainerHigh);
        QCOMPARE(tokens.containerElevation, ElevationLevel::Level3);
        QCOMPARE(tokens.containerShape, ShapeCorner::ExtraLarge);
        QCOMPARE(tokens.headlineStyle, TypeStyle::HeadlineSmall);
        QCOMPARE(tokens.supportingTextStyle, TypeStyle::BodyMedium);
    }

    void semanticColors()
    {
        MdMessageBox box;
        box.setVariant(MdMessageBoxVariant::Error);
        QCOMPARE(box.semanticColor(), MdTheme::instance().color(ColorRole::Error));

        box.setVariant(MdMessageBoxVariant::Warning);
        QCOMPARE(box.semanticColor(), MdTheme::instance().color(ColorRole::Tertiary));

        box.setVariant(MdMessageBoxVariant::Information);
        QCOMPARE(box.semanticColor(), MdTheme::instance().color(ColorRole::Primary));

        box.setVariant(MdMessageBoxVariant::Question);
        QCOMPARE(box.semanticColor(), MdTheme::instance().color(ColorRole::Primary));
    }

    void contentProperties()
    {
        MdMessageBox box;
        QSignalSpy titleSpy(&box, &MdMessageBox::titleChanged);
        QSignalSpy textSpy(&box, &MdMessageBox::textChanged);
        QSignalSpy infoSpy(&box, &MdMessageBox::informativeTextChanged);
        QSignalSpy variantSpy(&box, &MdMessageBox::variantChanged);

        box.setTitle(QStringLiteral("Delete?"));
        QCOMPARE(box.title(), QStringLiteral("Delete?"));
        QCOMPARE(titleSpy.count(), 1);

        box.setText(QStringLiteral("This cannot be undone."));
        QCOMPARE(box.text(), QStringLiteral("This cannot be undone."));
        QCOMPARE(textSpy.count(), 1);

        box.setInformativeText(QStringLiteral("The file will be gone."));
        QCOMPARE(box.informativeText(), QStringLiteral("The file will be gone."));
        QCOMPARE(infoSpy.count(), 1);

        box.setVariant(MdMessageBoxVariant::Warning);
        QCOMPARE(box.variant(), MdMessageBoxVariant::Warning);
        QCOMPARE(variantSpy.count(), 1);
    }

    void standardButtonsBuildMdButtons()
    {
        MdMessageBox box;
        box.setStandardButtons(MdMessageBoxStandardButton::Yes | MdMessageBoxStandardButton::No
                               | MdMessageBoxStandardButton::Cancel);
        QCOMPARE(box.buttons().size(), 3);

        // Affirmative (Yes) is Filled; No / Cancel are Text.
        bool sawFilled = false;
        bool sawText = false;
        for (MdButton *button : box.buttons()) {
            if (button->variant() == ButtonVariant::Filled) {
                sawFilled = true;
            }
            if (button->variant() == ButtonVariant::Text) {
                sawText = true;
            }
        }
        QVERIFY(sawFilled);
        QVERIFY(sawText);
    }

    void customButtonRoles()
    {
        MdMessageBox box;
        box.clearButtons();
        MdButton *action = box.addButton(QStringLiteral("Later"), MdMessageBoxButtonRole::ActionRole);
        QVERIFY(action != nullptr);
        QCOMPARE(action->variant(), ButtonVariant::Text);
        QCOMPARE(box.standardButtonValue(action), 0);

        MdButton *accept = box.addButton(QStringLiteral("Continue"), MdMessageBoxButtonRole::AcceptRole);
        QCOMPARE(accept->variant(), ButtonVariant::Filled);
        QCOMPARE(box.standardButtonValue(accept), int(MdMessageBoxStandardButton::Ok));
    }

    void execReturnsButtonResult()
    {
        MdMessageBox box(MdMessageBoxVariant::Question, QStringLiteral("Quit?"),
                         QStringLiteral("Unsaved changes will be lost."));
        box.setStandardButtons(MdMessageBoxStandardButton::Yes | MdMessageBoxStandardButton::No);

        // Drive the affirmative button after the box is up.
        QTimer::singleShot(0, &box, [&box]() {
            for (MdButton *button : box.buttons()) {
                if (box.standardButtonValue(button) == int(MdMessageBoxStandardButton::Yes)) {
                    button->click();
                    return;
                }
            }
            QTest::keyClick(&box, Qt::Key_Escape);
        });
        QCOMPARE(box.exec(), int(MdMessageBoxStandardButton::Yes));
    }

    void escapeDismissesWithZero()
    {
        MdMessageBox box(MdMessageBoxVariant::Error, QStringLiteral("Nope"),
                         QStringLiteral("Something broke."));
        box.setStandardButtons(MdMessageBoxStandardButton::Ok);

        QTimer::singleShot(0, &box, [&box]() {
            QTest::keyClick(&box, Qt::Key_Escape);
        });
        QCOMPARE(box.exec(), 0);
    }

    void nonBlankRender()
    {
        MdMessageBox box(MdMessageBoxVariant::Warning, QStringLiteral("Battery low"),
                         QStringLiteral("10% remaining."));
        box.setStandardButtons(MdMessageBoxStandardButton::Ok);
        box.resize(400, 220);
        box.show();
        QVERIFY(QTest::qWaitForWindowExposed(&box));

        const QImage image = renderWidget(box);
        QVERIFY2(anyPixelPainted(image), "MdMessageBox must paint its dialog chrome");
    }
};

QTEST_MAIN(TestMd3MessageBox)
#include "TestMd3MessageBox.moc"
