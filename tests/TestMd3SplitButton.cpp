// TestMd3SplitButton — the split-button family against its token sources.
//
// Every expected number in this file is transcribed from
// material-components/material-web
// tokens/versions/latest/sass/_md-comp-split-button-<size>.scss, or from the
// spec page's colour rule ("Split buttons use the same color schemes as
// standard buttons"), which is what the colour-row borrow implements.

#include "core/MdButtonTokens.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdSplitButtonStyle.h"
#include "widgets/MdSplitButton.h"

#include <QtGui/QImage>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

/// Paint the control into an image and return the pixel at `position`.
QRgb pixelAt(MdSplitButton &button, const QPointF &position)
{
    QImage image(int(button.width()), int(button.height()), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    button.render(&painter);
    painter.end();
    return image.pixel(int(position.x()), int(position.y()));
}

} // namespace

class TestMd3SplitButton : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void sizeTableMatchesTheExport();
    void colourRowsComeFromTheButtonFamily();
    void heightMatchedButtonSetIsUsedForTypography();
    void defaultGeometryMatchesTheTokens();
    void theTwoHalvesAreSeparatedByThePublishedGap();
    void outerCornersAreFullPills();
    void theInnerCornerMorphsOnPress();
    void trailingSelectionRoundsTheFacingCorners();
    void ripplesArePerHalf();
    void pointerPressEmitsTheRightHalfClicked();
    void pointerFocusShowsNoRingKeyboardFocusDoes();
    void arrowKeysMoveTheFocusedZoneAndCrossAxisIsSwallowed();
    void spaceActivatesTheFocusedHalf();
    void disabledPaintsNoInteraction();

private:
    static QPointF centerOf(const QRectF &rect)
    {
        return rect.center();
    }
};

void TestMd3SplitButton::initTestCase()
{
    // The theme singleton has to exist before any token resolution.
    MdTheme::instance();
}

void TestMd3SplitButton::sizeTableMatchesTheExport()
{
    // One row per published size set, straight from the export.
    struct Row
    {
        SplitButtonSize size;
        qreal height;
        qreal leadingLeading;
        qreal leadingTrailing;
        qreal trailingIcon;
        qreal trailingLeading;
        qreal innerRest;
        qreal innerActive;
    };
    const Row rows[] = {
        {SplitButtonSize::XSmall, 32.0, 12.0, 10.0, 22.0, 13.0, 4.0, 8.0},
        {SplitButtonSize::Small, 40.0, 16.0, 12.0, 22.0, 13.0, 4.0, 12.0},
        {SplitButtonSize::Medium, 56.0, 24.0, 24.0, 26.0, 15.0, 4.0, 12.0},
        {SplitButtonSize::Large, 96.0, 48.0, 48.0, 38.0, 29.0, 8.0, 20.0},
        {SplitButtonSize::XLarge, 136.0, 64.0, 64.0, 50.0, 43.0, 12.0, 20.0},
    };
    for (const Row &row : rows) {
        const MdSplitButtonTokens tokens =
            MdSplitButtonTokens::resolve(ButtonVariant::Filled, row.size);
        QCOMPARE(tokens.metrics.containerHeight, row.height);
        QCOMPARE(tokens.metrics.betweenSpace, 2.0);
        QCOMPARE(tokens.metrics.leadingLeadingSpace, row.leadingLeading);
        QCOMPARE(tokens.metrics.leadingTrailingSpace, row.leadingTrailing);
        QCOMPARE(tokens.metrics.trailingIconSize, row.trailingIcon);
        QCOMPARE(tokens.metrics.trailingLeadingSpace, row.trailingLeading);
        QCOMPARE(tokens.metrics.trailingTrailingSpace, row.trailingLeading);
        QCOMPARE(tokens.metrics.innerCornerHovered, row.innerActive);
        QCOMPARE(tokens.metrics.innerCornerPressed, row.innerActive);
        // corner-full: the outer corner and the selected trailing corner are
        // both half the height at every size.
        QCOMPARE(tokens.metrics.outerCornerRadius(), row.height / 2.0);
        QCOMPARE(tokens.metrics.selectedInnerCornerRadius(), row.height / 2.0);
    }
}

void TestMd3SplitButton::colourRowsComeFromTheButtonFamily()
{
    // "Split buttons use the same color schemes as standard buttons": a
    // filled split button's label colour at rest is the filled button's
    // on-primary, and the disabled rows carry the button family's opacities.
    const MdSplitButtonTokens tokens =
        MdSplitButtonTokens::resolve(ButtonVariant::Filled, SplitButtonSize::Small);
    const MdButtonTokens button =
        MdButtonTokens::resolve(ButtonVariant::Filled, ButtonSize::Small, ButtonShape::Round);

    QCOMPARE(int(tokens.button.enabled.labelText.role), int(button.enabled.labelText.role));
    QCOMPARE(int(tokens.button.enabled.container.role), int(button.enabled.container.role));
    QCOMPARE(int(tokens.button.pressed.stateLayer), int(button.pressed.stateLayer));
    QCOMPARE(tokens.button.disabled.labelText.opacity, 0.38);
    QCOMPARE(tokens.button.disabledContainerOpacity, 0.10);

    // The outlined variant borrows the outline row too.
    const MdSplitButtonTokens outlined =
        MdSplitButtonTokens::resolve(ButtonVariant::Outlined, SplitButtonSize::Small);
    QVERIFY(outlined.button.enabled.paintsOutline());
}

void TestMd3SplitButton::heightMatchedButtonSetIsUsedForTypography()
{
    // The two Expressive size scales agree on every height (32/40/56/96/136),
    // so the borrowed button set is the identity mapping and its container
    // height always matches the split button's own.
    const SplitButtonSize sizes[] = {SplitButtonSize::XSmall, SplitButtonSize::Small,
                                     SplitButtonSize::Medium, SplitButtonSize::Large,
                                     SplitButtonSize::XLarge};
    for (const SplitButtonSize size : sizes) {
        const MdSplitButtonTokens tokens =
            MdSplitButtonTokens::resolve(ButtonVariant::Filled, size);
        QCOMPARE(tokens.button.containerHeight, tokens.metrics.containerHeight);
    }
}

void TestMd3SplitButton::defaultGeometryMatchesTheTokens()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());
    split.show();

    // Focus margin: offset 2 + activeWidth/2 + width/2 = 7.5 px per side.
    const MdSplitButtonTokens tokens = split.tokens();
    const qreal inset = MdSplitButtonStyle::focusRingInset(tokens.button);
    QCOMPARE(inset, 7.5);

    const QRectF container = split.containerRect();
    QCOMPARE(container.height(), 40.0);
    QCOMPARE(container.topLeft(), QPointF(inset, inset));

    // The trailing half is pure token arithmetic: 13 + 22 + 13.
    QCOMPARE(split.trailingRect().width(), 48.0);
}

void TestMd3SplitButton::theTwoHalvesAreSeparatedByThePublishedGap()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());

    const QRectF leading = split.leadingRect();
    const QRectF trailing = split.trailingRect();
    QCOMPARE(trailing.left() - leading.right(), 2.0);
    QCOMPARE(leading.height(), 40.0);
    QCOMPARE(trailing.height(), 40.0);
    QCOMPARE(leading.right() < trailing.left(), true);
}

void TestMd3SplitButton::outerCornersAreFullPills()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());

    const MdSplitButtonStyle::Layout layout =
        MdSplitButtonStyle::layoutFor(split, split.tokens());
    // Leading half: outer edge on the left (LTR), so TL and BL are the pill
    // corners; the facing TR and BR carry the animated inner radius.
    QCOMPARE(layout.leadingRadii[0], 20.0); // TL
    QCOMPARE(layout.leadingRadii[3], 20.0); // BL
    QCOMPARE(layout.leadingRadii[1], 4.0);  // TR, rest inner corner
    QCOMPARE(layout.leadingRadii[2], 4.0);  // BR
    // Trailing half mirrors it.
    QCOMPARE(layout.trailingRadii[1], 20.0); // TR
    QCOMPARE(layout.trailingRadii[2], 20.0); // BR
    QCOMPARE(layout.trailingRadii[0], 4.0);  // TL
}

void TestMd3SplitButton::theInnerCornerMorphsOnPress()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());
    split.show();
    QVERIFY(QTest::qWaitForWindowExposed(&split));

    const qreal pressed = split.tokens().metrics.innerCornerPressed;
    QTest::mousePress(&split, Qt::LeftButton, Qt::NoModifier,
                      centerOf(split.leadingRect()).toPoint());
    QCOMPARE(split.pressedZone(), MdSplitButton::Zone::Leading);

    // The radius animates on the spring; past the settling time it is the
    // published pressed corner.
    QTest::qWait(400);
    QCOMPARE(split.innerCornerRadius(MdSplitButton::Zone::Leading), pressed);
    // The other half stays at rest.
    QCOMPARE(split.innerCornerRadius(MdSplitButton::Zone::Trailing),
             split.tokens().metrics.innerCornerRest);

    QTest::mouseRelease(&split, Qt::LeftButton, Qt::NoModifier,
                        centerOf(split.leadingRect()).toPoint());
    // The pointer is still over the leading half after the release, so the
    // corner relaxes to the *hovered* row, not to rest.
    QTest::qWait(400);
    QCOMPARE(split.innerCornerRadius(MdSplitButton::Zone::Leading),
             split.tokens().metrics.innerCornerHovered);

    // Leave: the corner settles all the way back to rest.
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(&split, &leave);
    QTest::qWait(400);
    QCOMPARE(split.innerCornerRadius(MdSplitButton::Zone::Leading),
             split.tokens().metrics.innerCornerRest);
    split.hide();
}

void TestMd3SplitButton::trailingSelectionRoundsTheFacingCorners()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());
    split.show();
    QVERIFY(QTest::qWaitForWindowExposed(&split));

    // Park the pointer outside so no half is hovered — the tests before this
    // one left the virtual cursor over the leading half, and a freshly shown
    // widget inherits that Enter.
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(&split, &leave);
    QTest::qWait(400);

    QSignalSpy spy(&split, &MdSplitButton::trailingSelectedChanged);
    split.setTrailingSelected(true);
    QCOMPARE(spy.count(), 1);

    QTest::qWait(400);
    // The selected trailing inner corner is the literal 50% — half the height.
    QCOMPARE(split.innerCornerRadius(MdSplitButton::Zone::Trailing), 20.0);
    QCOMPARE(split.innerCornerRadius(MdSplitButton::Zone::Leading), 4.0);
    split.hide();
}

void TestMd3SplitButton::ripplesArePerHalf()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());
    split.show();
    QVERIFY(QTest::qWaitForWindowExposed(&split));

    QTest::mousePress(&split, Qt::LeftButton, Qt::NoModifier,
                      centerOf(split.leadingRect()).toPoint());
    QVERIFY(split.rippleController(MdSplitButton::Zone::Leading)->isActive());
    QVERIFY(!split.rippleController(MdSplitButton::Zone::Trailing)->isActive());
    QTest::mouseRelease(&split, Qt::LeftButton, Qt::NoModifier,
                        centerOf(split.leadingRect()).toPoint());
    split.hide();
}

void TestMd3SplitButton::pointerPressEmitsTheRightHalfClicked()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());
    split.show();
    QVERIFY(QTest::qWaitForWindowExposed(&split));

    QSignalSpy leading(&split, &MdSplitButton::leadingClicked);
    QSignalSpy trailing(&split, &MdSplitButton::trailingClicked);

    QTest::mouseClick(&split, Qt::LeftButton, Qt::NoModifier,
                      centerOf(split.leadingRect()).toPoint());
    QTest::qWait(250); // the minimum-press hold
    QCOMPARE(leading.count(), 1);
    QCOMPARE(trailing.count(), 0);

    QTest::mouseClick(&split, Qt::LeftButton, Qt::NoModifier,
                      centerOf(split.trailingRect()).toPoint());
    QTest::qWait(250);
    QCOMPARE(leading.count(), 1);
    QCOMPARE(trailing.count(), 1);
    split.hide();
}

void TestMd3SplitButton::pointerFocusShowsNoRingKeyboardFocusDoes()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());
    split.show();
    QVERIFY(QTest::qWaitForWindowExposed(&split));

    // Mouse focus: no keyboard focus, no ring.
    split.clearFocus();
    split.setFocus(Qt::MouseFocusReason);
    QVERIFY(split.hasFocus());
    QVERIFY(!split.hasKeyboardFocus());
    // The ring's stroke centre sits `offset` (2 px) outside the container
    // edge, i.e. at 9.5 px from the widget edge on a Small control — sample
    // there, at mid-height so corner rounding is out of the way.
    const QPointF ringPoint(9.5, 20.0);
    const QRgb withoutRing = pixelAt(split, ringPoint);

    // Keyboard focus: the ring paints outside the container.
    split.clearFocus();
    split.setFocus(Qt::TabFocusReason);
    QVERIFY(split.hasKeyboardFocus());
    QCOMPARE(split.focusedZone(), MdSplitButton::Zone::Leading);
    const QRgb withRing = pixelAt(split, ringPoint);
    QVERIFY(withoutRing != withRing);
    split.hide();
}

void TestMd3SplitButton::arrowKeysMoveTheFocusedZoneAndCrossAxisIsSwallowed()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());
    split.show();
    QVERIFY(QTest::qWaitForWindowExposed(&split));

    split.clearFocus();
    split.setFocus(Qt::TabFocusReason);
    QCOMPARE(split.focusedZone(), MdSplitButton::Zone::Leading);

    QTest::keyClick(&split, Qt::Key_Right);
    QCOMPARE(split.focusedZone(), MdSplitButton::Zone::Trailing);
    QTest::keyClick(&split, Qt::Key_Right);
    QCOMPARE(split.focusedZone(), MdSplitButton::Zone::Leading); // wraps

    QTest::keyClick(&split, Qt::Key_Left);
    QCOMPARE(split.focusedZone(), MdSplitButton::Zone::Trailing);

    // Cross-axis keys are consumed as no-ops — never a focus-widget change.
    QTest::keyClick(&split, Qt::Key_Down);
    QCOMPARE(split.focusedZone(), MdSplitButton::Zone::Trailing);
    split.hide();
}

void TestMd3SplitButton::spaceActivatesTheFocusedHalf()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());
    split.show();
    QVERIFY(QTest::qWaitForWindowExposed(&split));

    QSignalSpy leading(&split, &MdSplitButton::leadingClicked);
    QSignalSpy trailing(&split, &MdSplitButton::trailingClicked);

    split.clearFocus();
    split.setFocus(Qt::TabFocusReason);
    QCOMPARE(split.focusedZone(), MdSplitButton::Zone::Leading);
    QTest::keyClick(&split, Qt::Key_Space);
    QTest::qWait(250);
    QCOMPARE(leading.count(), 1);
    QCOMPARE(trailing.count(), 0);

    QTest::keyClick(&split, Qt::Key_Right);
    QTest::keyClick(&split, Qt::Key_Space);
    QTest::qWait(250);
    QCOMPARE(leading.count(), 1);
    QCOMPARE(trailing.count(), 1);
    split.hide();
}

void TestMd3SplitButton::disabledPaintsNoInteraction()
{
    MdSplitButton split(QStringLiteral("Split"));
    split.resize(split.sizeHint());
    split.show();
    QVERIFY(QTest::qWaitForWindowExposed(&split));

    QSignalSpy leading(&split, &MdSplitButton::leadingClicked);
    split.setEnabled(false);
    QTest::mouseClick(&split, Qt::LeftButton, Qt::NoModifier,
                      centerOf(split.leadingRect()).toPoint());
    QTest::qWait(100);
    QCOMPARE(leading.count(), 0);
    QVERIFY(!split.rippleController(MdSplitButton::Zone::Leading)->isActive());
    split.hide();
}

QTEST_MAIN(TestMd3SplitButton)
#include "TestMd3SplitButton.moc"
