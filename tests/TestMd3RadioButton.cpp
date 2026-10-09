// TestMd3RadioButton — the `md.comp.radio-button.*` port.
//
// Numbers transcribed from material-web tokens/versions/latest/sass —
// _md-comp-radio-button.scss — behaviour cross-checked against androidx
// Compose Material3 RadioButton.kt and its RadioButtonTokens.kt.
//
// The assertions worth keeping are the ones a token table cannot express:
//
//   * the **pressed state-layer special cases** — an unselected press ripples
//     `primary` (the colour the button is about to earn) while a selected
//     press ripples `on-surface`; hover and focus read each side's own colour;
//   * the **dot scales on the fast spatial spring** — a selection grows the
//     dot from 0 to 12 px (drawn at 5 px radius after the stroke inset), a
//     deselection shrinks it back, no snap delays anywhere;
//   * **the disabled transition snaps** — no dot or colour animation into
//     disabled ("there should be no animations between enabled / disabled");
//   * **group exclusivity is native** — checking one of two siblings unchecks
//     the other, and a click on an already-checked button keeps it checked;
//   * **one colour paints stroke and dot** — a pixel probe through the stroke
//     and the dot centre reads the same resolved row;
//   * the **focus ring is keyboard-only** and circular.

#include "core/MdFocusRing.h"
#include "core/MdRadioButtonTokens.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdRadioButtonStyle.h"
#include "widgets/MdRadioButton.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

QColor pixelColorAt(QWidget &widget, const QPointF &position)
{
    QImage image(int(widget.width()), int(widget.height()), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32).pixelColor(position.toPoint());
}

/// One synchronous paint — what makes the ripple's colour observable right
/// after a press.
void paintOnce(QWidget &widget)
{
    pixelColorAt(widget, QPointF(0, 0));
}

/// Pump the event loop until the widget's animations have landed (Qt timers
/// are wall-clock based, which is why the short sleeps advance them).
void settleAnimations(QWidget &widget, int ms = 800)
{
    Q_UNUSED(widget);
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < ms) {
        QTest::qWait(8);
    }
}

bool closeEnough(const QColor &a, const QColor &b, int tolerance)
{
    return qAbs(a.alpha() - b.alpha()) <= tolerance && qAbs(a.red() - b.red()) <= tolerance
           && qAbs(a.green() - b.green()) <= tolerance && qAbs(a.blue() - b.blue()) <= tolerance;
}

} // namespace

class TestMd3RadioButton : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    // --- token tables ---------------------------------------------------------
    void metricTable();
    void unselectedTable();
    void selectedTable();

    // --- geometry -----------------------------------------------------------------
    void geometryIsFortyEightOverTwenty();

    // --- the paint ---------------------------------------------------------------
    void unselectedPaintsStrokeOnly();
    void selectedPaintsStrokeAndDot();
    void disabledFadesToThirtyEightPercent();
    void pressRipplesInTheSpecialColour();
    void hoverPaintsTheStateLayer();

    // --- animation ---------------------------------------------------------------
    void dotSpringsInAndOut();
    void colourFadesOnSelection();
    void disableSnaps();

    // --- group behaviour --------------------------------------------------------------
    void exclusivityIsNative();
    void clickingCheckedKeepsChecked();

    // --- focus ------------------------------------------------------------------------------
    void focusRingOnlyOnKeyboardFocus();

    // --- the style ------------------------------------------------------------------------------
    void styleIsInstalled();
};

void TestMd3RadioButton::cleanup()
{
    // Each test builds its own widgets on the stack; no shared mutable state.
}

// ---------------------------------------------------------------------------
// Token tables
// ---------------------------------------------------------------------------

void TestMd3RadioButton::metricTable()
{
    const MdRadioButtonTokens tokens = MdRadioButtonTokens::resolve();
    QCOMPARE(tokens.iconSize, 20.0);
    QCOMPARE(tokens.stateLayerSize, 40.0);
    QCOMPARE(tokens.disabledIconOpacity, 0.38);
    QCOMPARE(tokens.focusIndicatorColor, ColorRole::Secondary);
    QCOMPARE(tokens.focusIndicatorOuterOffset, 2.0);
    QCOMPARE(tokens.focusIndicatorThickness, 3.0);
    QCOMPARE(tokens.hoverStateLayerOpacity, 0.08);
    QCOMPARE(tokens.focusStateLayerOpacity, 0.12);
    QCOMPARE(tokens.pressedStateLayerOpacity, 0.12);
    // Compose's behaviour constants.
    QCOMPARE(tokens.strokeWidth, 2.0);
    QCOMPARE(tokens.dotSize, 12.0);
    QCOMPARE(tokens.padding, 2.0);
    QCOMPARE(tokens.minimumInteractiveSize, 48.0);
}

void TestMd3RadioButton::unselectedTable()
{
    const MdRadioButtonTokens tokens = MdRadioButtonTokens::resolve();
    const int enabled = int(MdNavigationItemState::Enabled);
    const int hovered = int(MdNavigationItemState::Hovered);
    const int focused = int(MdNavigationItemState::Focused);
    const int pressed = int(MdNavigationItemState::Pressed);
    const int disabled = int(MdNavigationItemState::Disabled);
    const int unselected = int(MdRadioButtonSelection::Unselected);

    // The icon rests on `on-surface-variant` and lifts to `on-surface` under
    // every interaction; disabled reads `on-surface` at 0.38.
    QCOMPARE(tokens.icon[unselected][enabled].role, ColorRole::OnSurfaceVariant);
    for (int i : {hovered, focused, pressed}) {
        QCOMPARE(tokens.icon[unselected][i].role, ColorRole::OnSurface);
        QCOMPARE(tokens.icon[unselected][i].opacity, 1.0);
    }
    QCOMPARE(tokens.icon[unselected][disabled].role, ColorRole::OnSurface);
    QCOMPARE(tokens.icon[unselected][disabled].opacity, 0.38);

    // The state layer: `on-surface` under hover/focus, the `primary` pressed
    // special case, and no enabled/dragged/disabled rows.
    QCOMPARE(tokens.stateLayer[unselected][hovered].role, ColorRole::OnSurface);
    QCOMPARE(tokens.stateLayer[unselected][focused].role, ColorRole::OnSurface);
    QCOMPARE(tokens.stateLayer[unselected][pressed].role, ColorRole::Primary);
    QVERIFY(!tokens.stateLayer[unselected][enabled].isPresent());
    QVERIFY(!tokens.stateLayer[unselected][disabled].isPresent());
}

void TestMd3RadioButton::selectedTable()
{
    const MdRadioButtonTokens tokens = MdRadioButtonTokens::resolve();
    const int enabled = int(MdNavigationItemState::Enabled);
    const int hovered = int(MdNavigationItemState::Hovered);
    const int focused = int(MdNavigationItemState::Focused);
    const int pressed = int(MdNavigationItemState::Pressed);
    const int disabled = int(MdNavigationItemState::Disabled);
    const int selected = int(MdRadioButtonSelection::Selected);

    // The icon is `primary` in every enabled interaction.
    for (int i : {enabled, hovered, focused, pressed}) {
        QCOMPARE(tokens.icon[selected][i].role, ColorRole::Primary);
    }
    QCOMPARE(tokens.icon[selected][disabled].role, ColorRole::OnSurface);
    QCOMPARE(tokens.icon[selected][disabled].opacity, 0.38);

    // The state layer: `primary` under hover/focus, the `on-surface` pressed
    // special case, and no enabled row.
    QCOMPARE(tokens.stateLayer[selected][hovered].role, ColorRole::Primary);
    QCOMPARE(tokens.stateLayer[selected][focused].role, ColorRole::Primary);
    QCOMPARE(tokens.stateLayer[selected][pressed].role, ColorRole::OnSurface);
    QVERIFY(!tokens.stateLayer[selected][enabled].isPresent());
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

void TestMd3RadioButton::geometryIsFortyEightOverTwenty()
{
    MdRadioButton button;
    button.resize(button.sizeHint());
    QCOMPARE(button.sizeHint(), QSize(48, 48));
    QCOMPARE(button.iconRect().size(), QSizeF(20.0, 20.0));
    QCOMPARE(button.iconRect().center(), QPointF(24.0, 24.0));
    QCOMPARE(button.stateLayerRect().size(), QSizeF(40.0, 40.0));
    QCOMPARE(MdRadioButtonStyle::focusRingSpec(button.radioButtonTokens()).shape,
             ShapeCorner::Full);
}

// ---------------------------------------------------------------------------
// The paint
// ---------------------------------------------------------------------------

void TestMd3RadioButton::unselectedPaintsStrokeOnly()
{
    MdRadioButton button;
    button.resize(button.sizeHint());

    // The stroke's top pixel: one `on-surface-variant` ring of 2 px.
    const QColor stroke = pixelColorAt(button, QPointF(24.0, 15.0));
    const QColor expectedStroke = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
    QVERIFY2(closeEnough(stroke, expectedStroke, 2),
             qPrintable(QStringLiteral("stroke %1 vs %2")
                            .arg(stroke.name(QColor::HexArgb))
                            .arg(expectedStroke.name(QColor::HexArgb))));
    // The centre is transparent (no dot yet).
    QCOMPARE(pixelColorAt(button, QPointF(24.0, 24.0)).alpha(), 0);
    // Outside the icon canvas, transparent.
    QCOMPARE(pixelColorAt(button, QPointF(24.0, 4.0)).alpha(), 0);
}

void TestMd3RadioButton::selectedPaintsStrokeAndDot()
{
    MdRadioButton button;
    button.setChecked(true);
    button.resize(button.sizeHint());
    settleAnimations(button);

    // One colour paints stroke and dot — both read the `primary` row.
    const QColor stroke = pixelColorAt(button, QPointF(24.0, 15.0));
    const QColor dot = pixelColorAt(button, QPointF(24.0, 24.0));
    const QColor primary = MdTheme::instance().color(ColorRole::Primary);
    QVERIFY2(closeEnough(stroke, primary, 2),
             qPrintable(QStringLiteral("stroke %1 vs %2")
                            .arg(stroke.name(QColor::HexArgb))
                            .arg(primary.name(QColor::HexArgb))));
    QVERIFY2(closeEnough(dot, primary, 2),
             qPrintable(QStringLiteral("dot %1 vs %2")
                            .arg(dot.name(QColor::HexArgb))
                            .arg(primary.name(QColor::HexArgb))));
}

void TestMd3RadioButton::disabledFadesToThirtyEightPercent()
{
    MdRadioButton button;
    button.setChecked(true);
    button.setEnabled(false);
    button.resize(button.sizeHint());

    const QColor onSurface = MdTheme::instance().color(ColorRole::OnSurface);
    QColor expected = onSurface;
    expected.setAlphaF(onSurface.alphaF() * 0.38);
    const QColor dot = pixelColorAt(button, QPointF(24.0, 24.0));
    // The 0.38 fill rides the premultiplied render target; widen past the
    // round-trip (the same standing as the chips' 0.12 probe).
    QVERIFY2(closeEnough(dot, expected, 6),
             qPrintable(QStringLiteral("dot %1 vs expected %2")
                            .arg(dot.name(QColor::HexArgb))
                            .arg(expected.name(QColor::HexArgb))));
}

void TestMd3RadioButton::pressRipplesInTheSpecialColour()
{
    MdRadioButton button;
    button.resize(button.sizeHint());
    button.show();
    QVERIFY(QTest::qWaitForWindowExposed(&button));

    // An unselected press ripples `primary` — the colour the button is about
    // to earn.
    const QPointF centre(24.0, 24.0);
    QTest::mousePress(&button, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    QVERIFY(button.rippleController()->isActive());
    paintOnce(button);
    QCOMPARE(button.rippleController()->contentColor(),
             MdTheme::instance().color(ColorRole::Primary));
    QTest::mouseRelease(&button, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    QVERIFY(button.isChecked());

    // A selected press ripples the other side of the special case:
    // `on-surface`.
    settleAnimations(button);
    QTest::mousePress(&button, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    paintOnce(button);
    QCOMPARE(button.rippleController()->contentColor(),
             MdTheme::instance().color(ColorRole::OnSurface));
    QTest::mouseRelease(&button, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
}

void TestMd3RadioButton::hoverPaintsTheStateLayer()
{
    MdRadioButton button;
    button.resize(button.sizeHint());

    // The hover layer paints only through the real event path (Qt5 offscreen
    // does not synthesize enter on mouseMove).
    QEnterEvent enter(QPointF(24, 24), QPointF(24, 24), QPointF(24, 24));
    QApplication::sendEvent(&button, &enter);
    QVERIFY(button.isHovered());
    const QColor layer = pixelColorAt(button, QPointF(24.0, 6.0));
    // The 40 px circle at y=6 (2 px inside its top edge, clear of the icon
    // stroke): the hover overlay over the parent — nonzero alpha.
    QVERIFY(layer.alpha() > 0);
    QCOMPARE(button.radioButtonTokens()
                 .stateLayerFor(MdRadioButtonSelection::Unselected,
                                MdNavigationItemState::Hovered)
                 .role,
             ColorRole::OnSurface);
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

void TestMd3RadioButton::dotSpringsInAndOut()
{
    // Two siblings under one parent: an autoExclusive checked radio cannot be
    // unchecked — `QAbstractButton::setChecked(false)` ignores it (matching
    // Compose's `selectable`, which never unchecks on click) — so the shrink
    // runs through the group exclusivity, the one real deselection path.
    QWidget group;
    auto *button = new MdRadioButton(&group);
    auto *other = new MdRadioButton(&group);
    group.show();
    QVERIFY(QTest::qWaitForWindowExposed(&group));
    QCOMPARE(button->animatedDotDiameter(), 0.0);

    button->setChecked(true);
    QVERIFY(button->dotAnimationRunning());
    settleAnimations(*button);
    QCOMPARE(button->animatedDotDiameter(), 12.0);

    // Checking the sibling unchecks this one: the same spring back to 0 — no
    // snap delay anywhere.
    other->setChecked(true);
    QVERIFY(!button->isChecked());
    QVERIFY(button->dotAnimationRunning());
    settleAnimations(*button);
    QCOMPARE(button->animatedDotDiameter(), 0.0);
}

void TestMd3RadioButton::colourFadesOnSelection()
{
    MdRadioButton button;
    button.resize(button.sizeHint());
    QCOMPARE(button.colourProgress(), 0.0);

    button.setChecked(true);
    QVERIFY(button.colourAnimationRunning());
    settleAnimations(button);
    QCOMPARE(button.colourProgress(), 1.0);
}

void TestMd3RadioButton::disableSnaps()
{
    MdRadioButton button;
    button.setChecked(true);
    button.resize(button.sizeHint());
    settleAnimations(button);
    QCOMPARE(button.animatedDotDiameter(), 12.0);

    // Into disabled both values land instantly — the timers never run.
    button.setEnabled(false);
    QVERIFY(!button.dotAnimationRunning());
    QVERIFY(!button.colourAnimationRunning());
    QCOMPARE(button.animatedDotDiameter(), 12.0);
    QCOMPARE(button.colourProgress(), 1.0);
}

// ---------------------------------------------------------------------------
// Group behaviour
// ---------------------------------------------------------------------------

void TestMd3RadioButton::exclusivityIsNative()
{
    QWidget group;
    auto *first = new MdRadioButton(&group);
    auto *second = new MdRadioButton(&group);
    group.show();
    QVERIFY(QTest::qWaitForWindowExposed(&group));

    first->setChecked(true);
    QVERIFY(first->isChecked());
    QVERIFY(!second->isChecked());
    second->setChecked(true);
    QVERIFY(second->isChecked());
    QVERIFY(!first->isChecked());
}

void TestMd3RadioButton::clickingCheckedKeepsChecked()
{
    MdRadioButton button;
    button.resize(button.sizeHint());
    button.show();
    QVERIFY(QTest::qWaitForWindowExposed(&button));

    const QPointF centre(24.0, 24.0);
    QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    QVERIFY(button.isChecked());
    // A second click on a checked radio keeps it checked.
    QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    QVERIFY(button.isChecked());
}

// ---------------------------------------------------------------------------
// Focus
// ---------------------------------------------------------------------------

void TestMd3RadioButton::focusRingOnlyOnKeyboardFocus()
{
    MdRadioButton button;
    button.resize(button.sizeHint());
    button.show();
    QVERIFY(QTest::qWaitForWindowExposed(&button));

    button.clearFocus();
    button.setFocus(Qt::MouseFocusReason);
    QVERIFY(button.hasFocus());
    QVERIFY(!button.hasKeyboardFocus());

    button.clearFocus();
    button.setFocus(Qt::TabFocusReason);
    QVERIFY(button.hasKeyboardFocus());
    QVERIFY(button.focusRingController() != nullptr);
}

// ---------------------------------------------------------------------------
// The style
// ---------------------------------------------------------------------------

void TestMd3RadioButton::styleIsInstalled()
{
    MdRadioButton button;
    QVERIFY(MdRadioButtonStyle::isInstalled());
}

QTEST_MAIN(TestMd3RadioButton)
#include "TestMd3RadioButton.moc"
