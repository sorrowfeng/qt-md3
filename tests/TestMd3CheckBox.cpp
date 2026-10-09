// TestMd3CheckBox — `md.comp.checkbox.*` at export version 34.0.21.
//
// Numbers transcribed from material-web tokens/versions/latest/sass
// _md-comp-checkbox.scss, behaviour cross-checked against androidx Compose
// Material3 Checkbox.kt (the styling-fix path) and
// _md-sys-state-focus-indicator.scss (outer offset 2, thickness 3).
//
// The assertions worth keeping are the ones a token table cannot express:
//
//   * the **state-layer special cases** — an unselected press ripples
//     `primary` and a selected press `on-surface`, the colour the box is
//     about to earn / already has;
//   * **indeterminate is a selected state** for the colour tables but owns a
//     gravitation of its own — `Off → Indeterminate` snaps it (the dash draws
//     in from nothing), `On ↔ Indeterminate` springs the check-to-dash morph,
//     and anything → `Off` holds for the 100 ms snap delay and then vanishes;
//   * the **drawBox/drawCheck port** — a checked box is one filled round rect
//     (fill == border), an unchecked one a stroked border over nothing, and
//     the check path runs through Compose's fractions of the box size;
//   * **disabled snaps** — the colours land on the disabled rows without an
//     animation;
//   * **the error variant** overrides the enabled rows only and falls back to
//     the base tables when disabled.
//
// Platform note (as in the other suites): assertions here are token-table and
// colour-table ones, plus pixel samples taken through QWidget::render, which
// does not depend on the font machinery.

#include "core/MdCheckBoxTokens.h"
#include "core/MdFocusRing.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdCheckBoxStyle.h"
#include "widgets/MdCheckBox.h"

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

/// Runs the check animation to its targets. The springs are driven by an 8 ms
/// timer, so the answer arrives through the event loop.
bool settleCheck(MdCheckBox &box, qreal fraction, qreal gravitation)
{
    for (int i = 0; i < 600; ++i) {
        if (qAbs(box.checkFraction() - fraction) <= 1e-6
            && qAbs(box.crossGravitation() - gravitation) <= 1e-6) {
            return true;
        }
        QTest::qWait(10);
    }
    return qAbs(box.checkFraction() - fraction) <= 1e-6
           && qAbs(box.crossGravitation() - gravitation) <= 1e-6;
}

bool settleColour(MdCheckBox &box, qreal progress)
{
    for (int i = 0; i < 600; ++i) {
        if (qAbs(box.colourProgress() - progress) <= 1e-6) {
            return true;
        }
        QTest::qWait(10);
    }
    return qAbs(box.colourProgress() - progress) <= 1e-6;
}

/// One synchronous paint through the same QImage path the pixel samples take —
/// this is what makes the ripple's colour observable right after a press,
/// without waiting for the windowing system's repaint.
void paintOnce(QWidget &widget)
{
    pixelColorAt(widget, QPointF(0, 0));
}

/// The premultiplied roundtrip can shift a channel by one; colours compare
/// with that tolerance.
bool closeEnough(const QColor &a, const QColor &b, int tolerance = 2)
{
    return qAbs(a.red() - b.red()) <= tolerance && qAbs(a.green() - b.green()) <= tolerance
           && qAbs(a.blue() - b.blue()) <= tolerance && qAbs(a.alpha() - b.alpha()) <= tolerance;
}

/// The centre of the checkbox's 18 px box, in widget coordinates.
QPointF boxCentre(const MdCheckBox &box)
{
    return box.boxRect().center();
}

/// A point inside the box but safely clear of the check stroke — the check's
/// second leg passes within a stroke width of the box centre, so pixel tests
/// that assert the *fill* must sample above the path's topmost point (the
/// right end at 0.3 of the box height).
QPointF offCheckPoint(const MdCheckBox &box)
{
    const QRectF boxRect = box.boxRect();
    return QPointF(boxRect.center().x(), boxRect.top() + 0.15 * boxRect.height());
}

} // namespace

class TestMd3CheckBox : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    // --- token tables ---------------------------------------------------------
    void metricTokenTable();
    void unselectedTable();
    void selectedTable();
    void errorTable();
    void composeBehaviourConstants();

    // --- geometry ---------------------------------------------------------------
    void sizeHintIsTheTouchTarget();

    // --- the cycle -----------------------------------------------------------------
    void clickCyclesTwoState();
    void tristateCyclesThroughThree();

    // --- the check animation --------------------------------------------------------
    void checkFractionSpringsIn();
    void undoHoldsThenSnaps();
    void indeterminateSnapsTheGravitation();
    void checkToDashMorphSprings();

    // --- the colour fade -------------------------------------------------------------
    void colourFadesOnSelection();
    void disabledSnapsTheColours();

    // --- the paint ---------------------------------------------------------------------
    void checkedPaintsTheFilledBox();
    void uncheckedPaintsOnlyTheBorder();
    void indeterminatePaintsTheDash();
    void disabledResolvesTheDisabledRows();
    void errorVariantPaintsError();
    void ripplePressesInThePressedColour();
    void focusRingOnlyOnKeyboardFocus();

    // --- the style ----------------------------------------------------------------------
    void styleIsInstalled();
};

void TestMd3CheckBox::cleanup()
{
    // The QCOMPARE-failure early-return leaves widgets in arbitrary states;
    // there is no shared mutable state to reset beyond what each test builds
    // on the stack, so this stays empty on purpose.
}

// ---------------------------------------------------------------------------
// Token tables
// ---------------------------------------------------------------------------

void TestMd3CheckBox::metricTokenTable()
{
    const MdCheckBoxTokens tokens = MdCheckBoxTokens::resolve();
    QCOMPARE(tokens.containerSize, 18.0);
    QCOMPARE(tokens.containerShapeRadius, 2.0);
    QCOMPARE(tokens.iconSize, 18.0);
    // Compose's CheckboxDefaults.StrokeWidth — the outline and the check share it.
    QCOMPARE(tokens.strokeWidth, 2.0);
    QCOMPARE(tokens.stateLayerSize, 40.0);

    // The focus indicator: secondary, the system *outer* offset and thickness.
    QCOMPARE(tokens.focusIndicatorColor, ColorRole::Secondary);
    QCOMPARE(tokens.focusIndicatorOuterOffset, 2.0);
    QCOMPARE(tokens.focusIndicatorThickness, 3.0);

    // The system state-layer opacities.
    QCOMPARE(tokens.hoverStateLayerOpacity, 0.08);
    QCOMPARE(tokens.focusStateLayerOpacity, 0.12);
    QCOMPARE(tokens.pressedStateLayerOpacity, 0.12);

    // The disabled opacities fold the whole box (selected) or the outline
    // (unselected) to 38 %.
    QCOMPARE(tokens.selectedDisabledContainerOpacity, 0.38);
    QCOMPARE(tokens.unselectedDisabledContainerOpacity, 0.38);
}

void TestMd3CheckBox::unselectedTable()
{
    const MdCheckBoxTokens tokens = MdCheckBoxTokens::resolve();
    const int unselected = int(MdCheckBoxSelection::Unselected);
    const int enabled = int(MdNavigationItemState::Enabled);
    const int hovered = int(MdNavigationItemState::Hovered);
    const int focused = int(MdNavigationItemState::Focused);
    const int pressed = int(MdNavigationItemState::Pressed);
    const int disabled = int(MdNavigationItemState::Disabled);

    // No fill row exists — the unchecked box is transparent.
    for (int i = 0; i < navItemStateCount; ++i) {
        QVERIFY(!tokens.box[unselected][i].isPresent());
        QVERIFY(!tokens.stateLayer[unselected][i].isPresent() || i == hovered || i == focused
                || i == pressed);
    }

    // The outline rests on on-surface-variant and lifts to on-surface.
    QCOMPARE(tokens.outline[unselected][enabled].role, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.outline[unselected][hovered].role, ColorRole::OnSurface);
    QCOMPARE(tokens.outline[unselected][focused].role, ColorRole::OnSurface);
    QCOMPARE(tokens.outline[unselected][pressed].role, ColorRole::OnSurface);
    QCOMPARE(tokens.outline[unselected][disabled].role, ColorRole::OnSurface);
    QCOMPARE(tokens.outline[unselected][disabled].opacity, 0.38);

    // The pressed state layer's special case: the colour the box is about to
    // earn, not the one it has.
    QCOMPARE(tokens.stateLayer[unselected][hovered].role, ColorRole::OnSurface);
    QCOMPARE(tokens.stateLayer[unselected][focused].role, ColorRole::OnSurface);
    QCOMPARE(tokens.stateLayer[unselected][pressed].role, ColorRole::Primary);

    // The deprecated icon rows are carried (unselected icon color on-surface).
    QCOMPARE(tokens.checkmark[unselected][enabled].role, ColorRole::OnSurface);
}

void TestMd3CheckBox::selectedTable()
{
    const MdCheckBoxTokens tokens = MdCheckBoxTokens::resolve();
    const int selected = int(MdCheckBoxSelection::Selected);
    const int enabled = int(MdNavigationItemState::Enabled);
    const int hovered = int(MdNavigationItemState::Hovered);
    const int focused = int(MdNavigationItemState::Focused);
    const int pressed = int(MdNavigationItemState::Pressed);
    const int disabled = int(MdNavigationItemState::Disabled);

    // The fill and the border resolve to the same primary (Compose's drawBox
    // collapses the pair); no separate selected outline colour row exists.
    for (int i : {enabled, hovered, focused, pressed}) {
        QCOMPARE(tokens.box[selected][i].role, ColorRole::Primary);
        QVERIFY(!tokens.outline[selected][i].isPresent());
        QCOMPARE(tokens.checkmark[selected][i].role, ColorRole::OnPrimary);
    }

    // Disabled: the whole box on-surface at 38 %, the check surface.
    QCOMPARE(tokens.box[selected][disabled].role, ColorRole::OnSurface);
    QCOMPARE(tokens.box[selected][disabled].opacity, 0.38);
    QCOMPARE(tokens.checkmark[selected][disabled].role, ColorRole::Surface);

    // The pressed state layer's own special case: on-surface, not primary.
    QCOMPARE(tokens.stateLayer[selected][hovered].role, ColorRole::Primary);
    QCOMPARE(tokens.stateLayer[selected][focused].role, ColorRole::Primary);
    QCOMPARE(tokens.stateLayer[selected][pressed].role, ColorRole::OnSurface);
    QVERIFY(!tokens.stateLayer[selected][disabled].isPresent());
}

void TestMd3CheckBox::errorTable()
{
    const MdCheckBoxTokens tokens = MdCheckBoxTokens::resolve();
    const int unselected = int(MdCheckBoxSelection::Unselected);
    const int selected = int(MdCheckBoxSelection::Selected);
    const int enabled = int(MdNavigationItemState::Enabled);
    const int hovered = int(MdNavigationItemState::Hovered);
    const int focused = int(MdNavigationItemState::Focused);
    const int pressed = int(MdNavigationItemState::Pressed);
    const int disabled = int(MdNavigationItemState::Disabled);

    for (int i : {enabled, hovered, focused, pressed}) {
        QCOMPARE(tokens.errorBox[selected][i].role, ColorRole::Error);
        QCOMPARE(tokens.errorCheckmark[selected][i].role, ColorRole::OnError);
        QCOMPARE(tokens.errorOutline[unselected][i].role, ColorRole::Error);
        QCOMPARE(tokens.errorStateLayer[i].role, ColorRole::Error);
    }

    // The error variant publishes no disabled rows: the accessors fall back to
    // the base tables.
    QVERIFY(!tokens.errorBox[selected][disabled].isPresent());
    const MdNavigationColourSlot &disabledBox =
        tokens.boxFor(true, MdCheckBoxSelection::Selected, MdNavigationItemState::Disabled);
    QCOMPARE(disabledBox.role, ColorRole::OnSurface);
    QCOMPARE(disabledBox.opacity, 0.38);
    const MdNavigationColourSlot &errorBox =
        tokens.boxFor(true, MdCheckBoxSelection::Selected, MdNavigationItemState::Enabled);
    QCOMPARE(errorBox.role, ColorRole::Error);
    const MdNavigationColourSlot &errorOutline =
        tokens.outlineFor(true, MdCheckBoxSelection::Unselected, MdNavigationItemState::Enabled);
    QCOMPARE(errorOutline.role, ColorRole::Error);
    const MdNavigationColourSlot &errorStateLayer =
        tokens.stateLayerFor(true, MdCheckBoxSelection::Unselected, MdNavigationItemState::Pressed);
    QCOMPARE(errorStateLayer.role, ColorRole::Error);
}

void TestMd3CheckBox::composeBehaviourConstants()
{
    const MdCheckBoxTokens tokens = MdCheckBoxTokens::resolve();
    // The snap delay for an undo.
    QCOMPARE(tokens.snapAnimationDelayMs, 100.0);
    // The 48 px touch target the 18 px canvas centres in.
    QCOMPARE(tokens.minimumInteractiveSize, 48.0);
    // The check path (the styling fix's fractions).
    QCOMPARE(tokens.checkLeftX, 0.25);
    QCOMPARE(tokens.checkLeftY, 0.5);
    QCOMPARE(tokens.checkCrossX, 0.4);
    QCOMPARE(tokens.checkCrossY, 0.65);
    QCOMPARE(tokens.checkRightX, 0.75);
    QCOMPARE(tokens.checkRightY, 0.3);
    // Compose's focusRingShape, carried for the record.
    QCOMPARE(tokens.focusRingRadiusPercent, 25.0);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

void TestMd3CheckBox::sizeHintIsTheTouchTarget()
{
    MdCheckBox box;
    box.resize(box.sizeHint());
    QCOMPARE(box.sizeHint(), QSize(48, 48));

    // The 18 px box and the 40 px state layer both centre in it.
    QCOMPARE(box.boxRect(), QRectF(15.0, 15.0, 18.0, 18.0));
    QCOMPARE(box.stateLayerRect(), QRectF(4.0, 4.0, 40.0, 40.0));
}

// ---------------------------------------------------------------------------
// The cycle
// ---------------------------------------------------------------------------

void TestMd3CheckBox::clickCyclesTwoState()
{
    MdCheckBox box;
    QCOMPARE(box.checkState(), Qt::Unchecked);
    box.click();
    QCOMPARE(box.checkState(), Qt::Checked);
    box.click();
    QCOMPARE(box.checkState(), Qt::Unchecked);
}

void TestMd3CheckBox::tristateCyclesThroughThree()
{
    MdCheckBox box;
    box.setTristate(true);
    // Compose's toggleValue() order: Off → On → Indeterminate → Off.
    box.click();
    QCOMPARE(box.checkState(), Qt::Checked);
    box.click();
    QCOMPARE(box.checkState(), Qt::PartiallyChecked);
    box.click();
    QCOMPARE(box.checkState(), Qt::Unchecked);
}

// ---------------------------------------------------------------------------
// The check animation
// ---------------------------------------------------------------------------

void TestMd3CheckBox::checkFractionSpringsIn()
{
    MdCheckBox box;
    QCOMPARE(box.checkFraction(), 0.0);
    box.setCheckState(Qt::Checked);
    QVERIFY(settleCheck(box, 1.0, 0.0));
    box.setCheckState(Qt::Unchecked);
    QVERIFY(settleCheck(box, 0.0, 0.0));
}

void TestMd3CheckBox::undoHoldsThenSnaps()
{
    MdCheckBox box;
    box.setCheckState(Qt::Checked);
    QVERIFY(settleCheck(box, 1.0, 0.0));

    // Compose's `snap(delayMillis = 100)`: the visual holds for the delay,
    // then lands on the target instantly.
    box.setCheckState(Qt::Unchecked);
    QCOMPARE(box.checkFraction(), 1.0);
    QTest::qWait(50);
    QCOMPARE(box.checkFraction(), 1.0);
    QTest::qWait(150);
    QCOMPARE(box.checkFraction(), 0.0);
    QCOMPARE(box.crossGravitation(), 0.0);
}

void TestMd3CheckBox::indeterminateSnapsTheGravitation()
{
    MdCheckBox box;
    // Off → Indeterminate: the gravitation snaps (Compose's `snap()`), the
    // dash draws in on the spring — it does not grow out of a checkmark.
    box.setCheckState(Qt::PartiallyChecked);
    QCOMPARE(box.crossGravitation(), 1.0);
    QVERIFY(settleCheck(box, 1.0, 1.0));
}

void TestMd3CheckBox::checkToDashMorphSprings()
{
    MdCheckBox box;
    box.setCheckState(Qt::Checked);
    QVERIFY(settleCheck(box, 1.0, 0.0));

    // On → Indeterminate: the fraction stays at 1 and the gravitation springs
    // — the checkmark morphs into the dash.
    box.setCheckState(Qt::PartiallyChecked);
    QCOMPARE(box.checkFraction(), 1.0);
    QVERIFY(settleCheck(box, 1.0, 1.0));

    // Indeterminate → On: the morph runs back on the spring.
    box.setCheckState(Qt::Checked);
    QVERIFY(settleCheck(box, 1.0, 0.0));
}

// ---------------------------------------------------------------------------
// The colour fade
// ---------------------------------------------------------------------------

void TestMd3CheckBox::colourFadesOnSelection()
{
    MdCheckBox box;
    QCOMPARE(box.colourProgress(), 0.0);
    box.setCheckState(Qt::Checked);
    QVERIFY(settleColour(box, 1.0));
    box.setCheckState(Qt::Unchecked);
    QVERIFY(settleColour(box, 0.0));

    // Indeterminate is a selected state for the colours too.
    box.setCheckState(Qt::PartiallyChecked);
    QVERIFY(settleColour(box, 1.0));
}

void TestMd3CheckBox::disabledSnapsTheColours()
{
    MdCheckBox box;
    box.setCheckState(Qt::Checked);
    QVERIFY(settleColour(box, 1.0));

    // Compose: "if not enabled 'snap' to the disabled state, as there should
    // be no animations between enabled / disabled".
    box.setEnabled(false);
    QCOMPARE(box.colourAnimationRunning(), false);
    QCOMPARE(box.colourProgress(), 1.0);

    box.setEnabled(true);
    box.setCheckState(Qt::Unchecked);
    QVERIFY(settleColour(box, 0.0));
    box.setEnabled(false);
    QCOMPARE(box.colourAnimationRunning(), false);
    QCOMPARE(box.colourProgress(), 0.0);
}

// ---------------------------------------------------------------------------
// The paint
// ---------------------------------------------------------------------------

void TestMd3CheckBox::checkedPaintsTheFilledBox()
{
    MdCheckBox box;
    box.resize(box.sizeHint());
    box.setCheckState(Qt::Checked);
    QVERIFY(settleColour(box, 1.0));

    const QColor primary = MdTheme::instance().color(ColorRole::Primary);
    // A point inside the fill and clear of the check stroke: the fill itself.
    const QColor fill = pixelColorAt(box, offCheckPoint(box));
    QCOMPARE(fill, primary);
}

void TestMd3CheckBox::uncheckedPaintsOnlyTheBorder()
{
    MdCheckBox box;
    box.resize(box.sizeHint());

    const QColor onSurfaceVariant = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
    // The centre: no fill, no check — transparent over nothing.
    const QColor centre = pixelColorAt(box, boxCentre(box));
    QCOMPARE(centre.alpha(), 0);

    // The top edge of the box: the 2 px outline.
    const QRectF boxRect = box.boxRect();
    const QColor edge = pixelColorAt(box, QPointF(boxRect.center().x(), boxRect.top() + 1.0));
    QCOMPARE(edge, onSurfaceVariant);
}

void TestMd3CheckBox::indeterminatePaintsTheDash()
{
    MdCheckBox box;
    box.resize(box.sizeHint());
    box.setCheckState(Qt::PartiallyChecked);
    QVERIFY(settleCheck(box, 1.0, 1.0));
    QVERIFY(settleColour(box, 1.0));

    const QColor onPrimary = MdTheme::instance().color(ColorRole::OnPrimary);
    // The dash runs along the centre line: its midpoint is the box centre.
    const QColor centre = pixelColorAt(box, boxCentre(box));
    QCOMPARE(centre, onPrimary);
}

void TestMd3CheckBox::disabledResolvesTheDisabledRows()
{
    MdCheckBox box;
    box.resize(box.sizeHint());
    box.setCheckState(Qt::Checked);
    QVERIFY(settleColour(box, 1.0));
    box.setEnabled(false);

    // The whole box reads on-surface at the 0.38 container opacity — sampled
    // clear of the check stroke, which stays `surface` at full strength.
    const QColor onSurface = MdTheme::instance().color(ColorRole::OnSurface);
    QColor expected = onSurface;
    expected.setAlphaF(onSurface.alphaF() * 0.38);
    const QColor fill = pixelColorAt(box, offCheckPoint(box));
    QVERIFY2(closeEnough(fill, expected), qPrintable(QStringLiteral("fill %1 vs expected %2")
                                                        .arg(fill.name(QColor::HexArgb))
                                                        .arg(expected.name(QColor::HexArgb))));
}

void TestMd3CheckBox::errorVariantPaintsError()
{
    MdCheckBox box;
    box.resize(box.sizeHint());
    box.setCheckState(Qt::Checked);
    QVERIFY(settleColour(box, 1.0));
    box.setError(true);

    const QColor error = MdTheme::instance().color(ColorRole::Error);
    // The fill, sampled clear of the on-error check stroke.
    const QColor fill = pixelColorAt(box, offCheckPoint(box));
    QCOMPARE(fill, error);

    // The unchecked error outline reads error, not on-surface-variant.
    MdCheckBox unchecked;
    unchecked.resize(unchecked.sizeHint());
    unchecked.setError(true);
    const QColor edge =
        pixelColorAt(unchecked, QPointF(unchecked.boxRect().center().x(), unchecked.boxRect().top() + 1.0));
    QCOMPARE(edge, error);
}

void TestMd3CheckBox::ripplePressesInThePressedColour()
{
    MdCheckBox box;
    box.resize(box.sizeHint());
    // Mouse delivery needs a mapped window on the offscreen platform.
    box.show();
    QVERIFY(QTest::qWaitForWindowExposed(&box));

    // An unselected press ripples primary — the colour the box is about to
    // earn. (Compose's default colours ripple transparent here; the export's
    // state-layer rows win — see the tokens header.)
    QTest::mousePress(&box, Qt::LeftButton, Qt::NoModifier, box.boxRect().center().toPoint());
    QVERIFY(box.rippleController() != nullptr);
    QVERIFY(box.rippleController()->isActive());
    paintOnce(box);
    QCOMPARE(box.rippleController()->contentColor(),
             MdTheme::instance().color(ColorRole::Primary));
    QTest::mouseRelease(&box, Qt::LeftButton, Qt::NoModifier, box.boxRect().center().toPoint());
    QCOMPARE(box.checkState(), Qt::Checked);

    // A selected press ripples on-surface.
    QTest::mousePress(&box, Qt::LeftButton, Qt::NoModifier, box.boxRect().center().toPoint());
    paintOnce(box);
    QCOMPARE(box.rippleController()->contentColor(),
             MdTheme::instance().color(ColorRole::OnSurface));
    QTest::mouseRelease(&box, Qt::LeftButton, Qt::NoModifier, box.boxRect().center().toPoint());
}

void TestMd3CheckBox::focusRingOnlyOnKeyboardFocus()
{
    MdCheckBox box;
    box.resize(box.sizeHint());
    box.show();
    QVERIFY(QTest::qWaitForWindowExposed(&box));

    // A mouse focus paints nothing — `:focus-visible`. focusInEvent does not
    // re-fire while the widget already holds focus, so each step starts from
    // no focus at all.
    box.clearFocus();
    box.setFocus(Qt::MouseFocusReason);
    QVERIFY(box.hasFocus());
    QVERIFY(!box.hasKeyboardFocus());

    // A keyboard focus does.
    box.clearFocus();
    box.setFocus(Qt::TabFocusReason);
    QVERIFY(box.hasFocus());
    QVERIFY(box.hasKeyboardFocus());
    QVERIFY(box.focusRingController() != nullptr);
}

void TestMd3CheckBox::styleIsInstalled()
{
    // Constructing one checkbox installs the shared style.
    MdCheckBox box;
    QVERIFY(MdCheckBoxStyle::isInstalled());
}

QTEST_MAIN(TestMd3CheckBox)
#include "TestMd3CheckBox.moc"
