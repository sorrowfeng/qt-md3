// TestMd3Steps — Stage 2 Qt extension (Appendix D, P2).
//
// Pins the steps semantics the header documents:
//   * complete / current / upcoming state machine from currentStep;
//   * progress-indicator colour pair on the connector;
//   * orientation / labels / readOnly properties with NOTIFY;
//   * keyboard walk and stepSelected;
//   * non-blank render smoke.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdSteps.h"
#include "widgets/MdStepper.h"

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

} // namespace

class TestMd3Steps : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdSteps steps;
        QCOMPARE(steps.stepCount(), 3);
        QCOMPARE(steps.currentStep(), 0);
        QCOMPARE(steps.orientation(), MdStepsOrientation::Horizontal);
        QVERIFY(!steps.isReadOnly());
        QCOMPARE(steps.stateAt(0), MdSteps::StepState::Current);
        QCOMPARE(steps.stateAt(1), MdSteps::StepState::Upcoming);
    }

    void stateMachine()
    {
        MdSteps steps;
        steps.setStepCount(4);
        steps.setCurrentStep(2);
        QCOMPARE(steps.stateAt(0), MdSteps::StepState::Complete);
        QCOMPARE(steps.stateAt(1), MdSteps::StepState::Complete);
        QCOMPARE(steps.stateAt(2), MdSteps::StepState::Current);
        QCOMPARE(steps.stateAt(3), MdSteps::StepState::Upcoming);
    }

    void currentStepClamps()
    {
        MdSteps steps;
        steps.setStepCount(2);
        steps.setCurrentStep(99);
        QCOMPARE(steps.currentStep(), 1);
        steps.setCurrentStep(-5);
        QCOMPARE(steps.currentStep(), 0);
    }

    void propertiesNotify()
    {
        MdSteps steps;
        QSignalSpy countSpy(&steps, &MdSteps::stepCountChanged);
        QSignalSpy currentSpy(&steps, &MdSteps::currentStepChanged);
        QSignalSpy orientSpy(&steps, &MdSteps::orientationChanged);
        QSignalSpy labelsSpy(&steps, &MdSteps::labelsChanged);
        QSignalSpy readSpy(&steps, &MdSteps::readOnlyChanged);

        steps.setStepCount(4);
        QCOMPARE(countSpy.count(), 1);

        steps.setCurrentStep(1);
        QCOMPARE(currentSpy.count(), 1);

        steps.setOrientation(MdStepsOrientation::Vertical);
        QCOMPARE(orientSpy.count(), 1);

        steps.setLabels(QStringList() << QStringLiteral("A") << QStringLiteral("B"));
        QCOMPARE(labelsSpy.count(), 1);

        steps.setReadOnly(true);
        QCOMPARE(readSpy.count(), 1);
    }

    void coloursFollowProgressSemantics()
    {
        MdSteps steps;
        steps.setStepCount(3);
        steps.setCurrentStep(1);

        QCOMPARE(steps.resolvedDiscColor(MdSteps::StepState::Complete),
                 MdTheme::instance().color(ColorRole::Primary));
        QCOMPARE(steps.resolvedDiscColor(MdSteps::StepState::Current),
                 MdTheme::instance().color(ColorRole::PrimaryContainer));
        QCOMPARE(steps.resolvedDiscColor(MdSteps::StepState::Upcoming),
                 MdTheme::instance().color(ColorRole::SurfaceContainerHighest));

        QCOMPARE(steps.resolvedConnectorColor(true),
                 MdTheme::instance().color(ColorRole::Primary));
        QCOMPARE(steps.resolvedConnectorColor(false),
                 MdTheme::instance().color(ColorRole::OutlineVariant));
    }

    void keyboardWalk()
    {
        MdSteps steps;
        steps.setStepCount(4);
        steps.setCurrentStep(1);
        steps.resize(400, 120);
        steps.show();
        QVERIFY(QTest::qWaitForWindowExposed(&steps));

        QSignalSpy selectedSpy(&steps, &MdSteps::stepSelected);
        steps.setFocus();
        QTest::keyClick(&steps, Qt::Key_Right);
        QCOMPARE(steps.currentStep(), 2);
        QCOMPARE(selectedSpy.count(), 1);

        QTest::keyClick(&steps, Qt::Key_Left);
        QCOMPARE(steps.currentStep(), 1);

        QTest::keyClick(&steps, Qt::Key_End);
        QCOMPARE(steps.currentStep(), 3);

        QTest::keyClick(&steps, Qt::Key_Home);
        QCOMPARE(steps.currentStep(), 0);
    }

    void readOnlyBlocksWalk()
    {
        MdSteps steps;
        steps.setStepCount(3);
        steps.setReadOnly(true);
        steps.resize(300, 100);
        steps.show();
        QVERIFY(QTest::qWaitForWindowExposed(&steps));

        steps.setFocus();
        QTest::keyClick(&steps, Qt::Key_Right);
        QCOMPARE(steps.currentStep(), 0);
    }

    void stepperAlias()
    {
        // MdStepper is the appendix D alternate name for the same widget.
        MdStepper stepper;
        QCOMPARE(stepper.stepCount(), 3);
    }

    void nonBlankRender()
    {
        MdSteps steps;
        steps.setStepCount(4);
        steps.setCurrentStep(1);
        steps.setLabels(QStringList() << QStringLiteral("Cart") << QStringLiteral("Ship")
                                      << QStringLiteral("Pay") << QStringLiteral("Done"));
        steps.resize(480, 100);
        steps.show();
        QVERIFY(QTest::qWaitForWindowExposed(&steps));

        const QImage image = renderWidget(steps);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdSteps must paint discs, connectors and labels");
    }
};

QTEST_MAIN(TestMd3Steps)
#include "TestMd3Steps.moc"
