// TestMd3Switch — the `md.comp.switch.*` port.
//
// Numbers transcribed from material-web tokens/versions/latest/sass —
// _md-comp-switch.scss — behaviour cross-checked against androidx Compose
// Material3 Switch.kt (its `ThumbNode.measure` ported verbatim).
//
// The assertions worth keeping are the ones a token table cannot express:
//
//   * **the thumb travels and resizes on the fast spatial spring** — 16 px at
//     the track's left inset unchecked, 24 px at the far bound checked, and
//     **while pressed both targets land instantly** (Compose's `SnapSpec`):
//     28 px snapped 2 px inward;
//   * **no colour animation** — the track/handle/icon colours resolve by
//     state and land this frame;
//   * **the disabled selected handle keeps full strength** — the export's
//     `disabled.selected.handle.opacity` is 1; the track fades at its own
//     0.12, the icon and the unselected handle at 0.38;
//   * **the ripple rides the thumb** — an unselected press ripples
//     `on-surface` at the thumb, a selected press `primary`;
//   * **the with-icon handle is 24 px unchecked** — Compose's `hasContent`
//     branch.

#include "core/MdFocusRing.h"
#include "core/MdRipple.h"
#include "core/MdSwitchTokens.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdSwitchStyle.h"
#include "widgets/MdSwitch.h"

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

void paintOnce(QWidget &widget)
{
    pixelColorAt(widget, QPointF(0, 0));
}

/// Pump the event loop until the widget's animations have landed.
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

class TestMd3Switch : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    // --- token tables ---------------------------------------------------------
    void metricTable();
    void unselectedTable();
    void selectedTable();

    // --- geometry ------------------------------------------------------------------
    void geometryIsFiftyTwoOverFortyEight();
    void uncheckedThumbSitsAtTheLeftInset();
    void checkedThumbTravelsToTheFarBound();
    void withIconHandleIsTwentyFourUnchecked();

    // --- animation ---------------------------------------------------------------
    void thumbSpringsOnCheck();
    void pressSnapsAndReleaseSpringsBack();

    // --- the paint ----------------------------------------------------------------
    void selectedPaintsInstantly();
    void disabledFadesByRow();
    void pressRipplesAtTheThumb();
    void focusRingOnlyOnKeyboardFocus();

    // --- semantics ---------------------------------------------------------------
    void togglesIndependently();
    void wholeRectIsTheTouchTarget();

    // --- the style ------------------------------------------------------------------
    void styleIsInstalled();
};

void TestMd3Switch::cleanup()
{
    // Each test builds its own widgets on the stack; no shared mutable state.
}

// ---------------------------------------------------------------------------
// Token tables
// ---------------------------------------------------------------------------

void TestMd3Switch::metricTable()
{
    const MdSwitchTokens tokens = MdSwitchTokens::resolve();
    QCOMPARE(tokens.trackWidth, 52.0);
    QCOMPARE(tokens.trackHeight, 32.0);
    QCOMPARE(tokens.trackOutlineWidth, 2.0);
    QCOMPARE(tokens.unselectedHandleSize, 16.0);
    QCOMPARE(tokens.selectedHandleSize, 24.0);
    QCOMPARE(tokens.pressedHandleSize, 28.0);
    QCOMPARE(tokens.withIconHandleSize, 24.0);
    QCOMPARE(tokens.iconSize, 16.0);
    QCOMPARE(tokens.stateLayerSize, 40.0);
    QCOMPARE(tokens.disabledTrackOpacity, 0.12);
    QCOMPARE(tokens.disabledUnselectedHandleOpacity, 0.38);
    QCOMPARE(tokens.disabledSelectedHandleOpacity, 1.0);
    QCOMPARE(tokens.disabledIconOpacity, 0.38);
    QCOMPARE(tokens.handleElevation, ElevationLevel::Level1);
    QCOMPARE(tokens.disabledHandleElevation, ElevationLevel::Level0);
    QCOMPARE(tokens.handleShadowColor, ColorRole::Shadow);
    QCOMPARE(tokens.focusIndicatorColor, ColorRole::Secondary);
    QCOMPARE(tokens.focusIndicatorOuterOffset, 2.0);
    QCOMPARE(tokens.focusIndicatorThickness, 3.0);
    QCOMPARE(tokens.hoverStateLayerOpacity, 0.08);
    QCOMPARE(tokens.focusStateLayerOpacity, 0.12);
    QCOMPARE(tokens.pressedStateLayerOpacity, 0.12);
    // Compose's behaviour constants.
    QCOMPARE(tokens.minimumInteractiveSize, 48.0);
    QCOMPARE(tokens.rippleRadius, 20.0);
}

void TestMd3Switch::unselectedTable()
{
    const MdSwitchTokens tokens = MdSwitchTokens::resolve();
    const int enabled = int(MdNavigationItemState::Enabled);
    const int hovered = int(MdNavigationItemState::Hovered);
    const int focused = int(MdNavigationItemState::Focused);
    const int pressed = int(MdNavigationItemState::Pressed);
    const int disabled = int(MdNavigationItemState::Disabled);
    const int s = int(MdSwitchSelection::Unselected);

    // The track rests on `surface-container-highest` with an `outline` border;
    // disabled both fade to `on-surface` at the track's own 0.12.
    for (int i : {enabled, hovered, focused, pressed}) {
        QCOMPARE(tokens.track[s][i].role, ColorRole::SurfaceContainerHighest);
        QCOMPARE(tokens.trackOutline[s][i].role, ColorRole::Outline);
    }
    QCOMPARE(tokens.track[s][disabled].role, ColorRole::OnSurface);
    QCOMPARE(tokens.track[s][disabled].opacity, 0.12);
    QCOMPARE(tokens.trackOutline[s][disabled].role, ColorRole::OnSurface);
    QCOMPARE(tokens.trackOutline[s][disabled].opacity, 0.12);

    // The handle: `outline` resting, `on-surface-variant` under interaction,
    // `on-surface` at 0.38 disabled. The icon rides the track's colour.
    QCOMPARE(tokens.handle[s][enabled].role, ColorRole::Outline);
    for (int i : {hovered, focused, pressed}) {
        QCOMPARE(tokens.handle[s][i].role, ColorRole::OnSurfaceVariant);
        QCOMPARE(tokens.stateLayer[s][i].role, ColorRole::OnSurface);
    }
    QCOMPARE(tokens.handle[s][disabled].role, ColorRole::OnSurface);
    QCOMPARE(tokens.handle[s][disabled].opacity, 0.38);
    QCOMPARE(tokens.icon[s][enabled].role, ColorRole::SurfaceContainerHighest);
    QVERIFY(!tokens.stateLayer[s][enabled].isPresent());
    QVERIFY(!tokens.stateLayer[s][disabled].isPresent());
}

void TestMd3Switch::selectedTable()
{
    const MdSwitchTokens tokens = MdSwitchTokens::resolve();
    const int enabled = int(MdNavigationItemState::Enabled);
    const int hovered = int(MdNavigationItemState::Hovered);
    const int focused = int(MdNavigationItemState::Focused);
    const int pressed = int(MdNavigationItemState::Pressed);
    const int disabled = int(MdNavigationItemState::Disabled);
    const int s = int(MdSwitchSelection::Selected);

    // The track `primary`, no outline rows (the checked border resolves
    // transparent — Compose's default `checkedBorderColor`).
    for (int i : {enabled, hovered, focused, pressed}) {
        QCOMPARE(tokens.track[s][i].role, ColorRole::Primary);
    }
    QVERIFY(!tokens.trackOutline[s][enabled].isPresent());

    // The handle `on-primary` resting, `primary-container` under interaction,
    // **`surface` at full strength disabled**.
    QCOMPARE(tokens.handle[s][enabled].role, ColorRole::OnPrimary);
    for (int i : {hovered, focused, pressed}) {
        QCOMPARE(tokens.handle[s][i].role, ColorRole::PrimaryContainer);
        QCOMPARE(tokens.stateLayer[s][i].role, ColorRole::Primary);
    }
    QCOMPARE(tokens.handle[s][disabled].role, ColorRole::Surface);
    QCOMPARE(tokens.handle[s][disabled].opacity, 1.0);
    QCOMPARE(tokens.icon[s][enabled].role, ColorRole::Primary);
    QCOMPARE(tokens.icon[s][disabled].role, ColorRole::OnSurface);
    QCOMPARE(tokens.icon[s][disabled].opacity, 0.38);
    QCOMPARE(tokens.track[s][disabled].role, ColorRole::OnSurface);
    QCOMPARE(tokens.track[s][disabled].opacity, 0.12);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

void TestMd3Switch::geometryIsFiftyTwoOverFortyEight()
{
    MdSwitch sw;
    sw.resize(sw.sizeHint());
    QCOMPARE(sw.sizeHint(), QSize(52, 48));
    QCOMPARE(sw.trackRect().size(), QSizeF(52.0, 32.0));
    QCOMPARE(sw.trackRect().topLeft(), QPointF(0.0, 8.0));
    QCOMPARE(MdSwitchStyle::focusRingSpec(sw.switchTokens()).shape, ShapeCorner::Full);
}

void TestMd3Switch::uncheckedThumbSitsAtTheLeftInset()
{
    MdSwitch sw;
    sw.resize(sw.sizeHint());
    QCOMPARE(sw.animatedThumbDiameter(), 16.0);
    // `thumbPaddingStart = (32 - 16) / 2 = 8`.
    QCOMPARE(sw.animatedThumbOffset(), 8.0);
    QCOMPARE(sw.thumbRect().size(), QSizeF(16.0, 16.0));
}

void TestMd3Switch::checkedThumbTravelsToTheFarBound()
{
    MdSwitch sw;
    sw.setChecked(true); // the spring is running; settle before reading
    sw.resize(sw.sizeHint());
    settleAnimations(sw);
    // `maxBound = (52 - 24) - 4 = 24`.
    QCOMPARE(sw.animatedThumbDiameter(), 24.0);
    QCOMPARE(sw.animatedThumbOffset(), 24.0);
    QCOMPARE(sw.thumbRect().right(), 48.0);
}

void TestMd3Switch::withIconHandleIsTwentyFourUnchecked()
{
    MdSwitch sw;
    sw.setIconName(QStringLiteral("check")); // the spring is running
    sw.resize(sw.sizeHint());
    settleAnimations(sw);
    // Compose's `hasContent` branch: a 24 px thumb even unchecked, at the
    // 24 px thumb's own inset `(32 - 24) / 2 = 4`.
    QCOMPARE(sw.animatedThumbDiameter(), 24.0);
    QCOMPARE(sw.animatedThumbOffset(), 4.0);
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

void TestMd3Switch::thumbSpringsOnCheck()
{
    MdSwitch sw;
    sw.resize(sw.sizeHint());
    QCOMPARE(sw.animatedThumbOffset(), 8.0);

    sw.setChecked(true);
    QVERIFY(sw.thumbAnimationRunning());
    settleAnimations(sw);
    QCOMPARE(sw.animatedThumbDiameter(), 24.0);
    QCOMPARE(sw.animatedThumbOffset(), 24.0);

    // Unchecking runs the same spring back.
    sw.setChecked(false);
    QVERIFY(sw.thumbAnimationRunning());
    settleAnimations(sw);
    QCOMPARE(sw.animatedThumbDiameter(), 16.0);
    QCOMPARE(sw.animatedThumbOffset(), 8.0);
}

void TestMd3Switch::pressSnapsAndReleaseSpringsBack()
{
    MdSwitch sw;
    sw.setChecked(true);
    sw.resize(sw.sizeHint());
    sw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&sw));
    settleAnimations(sw);
    QCOMPARE(sw.animatedThumbOffset(), 24.0);

    // A press lands **instantly** — 28 px snapped 2 px inward from the far
    // bound (Compose's `SnapSpec` while pressed).
    const QPointF centre(26.0, 24.0);
    QTest::mousePress(&sw, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    QVERIFY(sw.isDown());
    // The snap lands on the first timer tick; wait for it instead of guessing
    // a wall-clock budget (the full suite's load can starve one 8 ms tick).
    QElapsedTimer snapClock;
    snapClock.start();
    while (sw.animatedThumbDiameter() != 28.0 && snapClock.elapsed() < 400) {
        QTest::qWait(8);
    }
    QCOMPARE(sw.animatedThumbDiameter(), 28.0);
    QCOMPARE(sw.animatedThumbOffset(), 22.0);

    // Releasing springs back to the (still checked) resting target — and the
    // click has toggled the switch off.
    QTest::mouseRelease(&sw, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    QVERIFY(!sw.isChecked());
    QVERIFY(sw.thumbAnimationRunning());
    settleAnimations(sw);
    QCOMPARE(sw.animatedThumbDiameter(), 16.0);
    QCOMPARE(sw.animatedThumbOffset(), 8.0);
}

// ---------------------------------------------------------------------------
// The paint
// ---------------------------------------------------------------------------

void TestMd3Switch::selectedPaintsInstantly()
{
    MdSwitch sw;
    sw.setChecked(true);
    sw.resize(sw.sizeHint());
    settleAnimations(sw); // the thumb's travel springs; the colours do not

    // No colour animation: once the thumb lands, the checked track reads
    // `primary` with no further fade (sample left of the thumb).
    const QColor track = pixelColorAt(sw, QPointF(6.0, 24.0));
    const QColor primary = MdTheme::instance().color(ColorRole::Primary);
    QVERIFY2(closeEnough(track, primary, 2),
             qPrintable(QStringLiteral("track %1 vs %2")
                            .arg(track.name(QColor::HexArgb))
                            .arg(primary.name(QColor::HexArgb))));
    // The handle is `on-primary`.
    const QColor handle = pixelColorAt(sw, QPointF(36.0, 24.0));
    const QColor onPrimary = MdTheme::instance().color(ColorRole::OnPrimary);
    QVERIFY2(closeEnough(handle, onPrimary, 2),
             qPrintable(QStringLiteral("handle %1 vs %2")
                            .arg(handle.name(QColor::HexArgb))
                            .arg(onPrimary.name(QColor::HexArgb))));
}

void TestMd3Switch::disabledFadesByRow()
{
    MdSwitch sw;
    sw.setChecked(true);
    sw.resize(sw.sizeHint());
    settleAnimations(sw);
    sw.setEnabled(false);

    // The track fades at its own 0.12; the handle keeps full strength on its
    // `surface` row.
    const QColor onSurface = MdTheme::instance().color(ColorRole::OnSurface);
    QColor expectedTrack = onSurface;
    expectedTrack.setAlphaF(onSurface.alphaF() * 0.12);
    const QColor track = pixelColorAt(sw, QPointF(6.0, 24.0));
    QVERIFY2(closeEnough(track, expectedTrack, 6),
             qPrintable(QStringLiteral("track %1 vs %2")
                            .arg(track.name(QColor::HexArgb))
                            .arg(expectedTrack.name(QColor::HexArgb))));
    const QColor surface = MdTheme::instance().color(ColorRole::Surface);
    const QColor handle = pixelColorAt(sw, QPointF(36.0, 24.0));
    QVERIFY2(closeEnough(handle, surface, 2),
             qPrintable(QStringLiteral("handle %1 vs %2")
                            .arg(handle.name(QColor::HexArgb))
                            .arg(surface.name(QColor::HexArgb))));
}

void TestMd3Switch::pressRipplesAtTheThumb()
{
    MdSwitch sw;
    sw.resize(sw.sizeHint());
    sw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&sw));

    // An unselected press ripples `on-surface` — at the thumb's centre.
    const QPointF thumbCentre(16.0, 24.0);
    QTest::mousePress(&sw, Qt::LeftButton, Qt::NoModifier, thumbCentre.toPoint());
    QVERIFY(sw.rippleController()->isActive());
    paintOnce(sw);
    QCOMPARE(sw.rippleController()->contentColor(),
             MdTheme::instance().color(ColorRole::OnSurface));
    QTest::mouseRelease(&sw, Qt::LeftButton, Qt::NoModifier, thumbCentre.toPoint());
    QVERIFY(sw.isChecked());

    // A selected press ripples `primary` — at the thumb's new centre.
    settleAnimations(sw);
    const QPointF checkedThumbCentre(36.0, 24.0);
    QTest::mousePress(&sw, Qt::LeftButton, Qt::NoModifier, checkedThumbCentre.toPoint());
    paintOnce(sw);
    QCOMPARE(sw.rippleController()->contentColor(),
             MdTheme::instance().color(ColorRole::Primary));
    QTest::mouseRelease(&sw, Qt::LeftButton, Qt::NoModifier, checkedThumbCentre.toPoint());
}

void TestMd3Switch::focusRingOnlyOnKeyboardFocus()
{
    MdSwitch sw;
    sw.resize(sw.sizeHint());
    sw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&sw));

    sw.clearFocus();
    sw.setFocus(Qt::MouseFocusReason);
    QVERIFY(sw.hasFocus());
    QVERIFY(!sw.hasKeyboardFocus());

    sw.clearFocus();
    sw.setFocus(Qt::TabFocusReason);
    QVERIFY(sw.hasKeyboardFocus());
    QVERIFY(sw.focusRingController() != nullptr);
}

// ---------------------------------------------------------------------------
// Semantics
// ---------------------------------------------------------------------------

void TestMd3Switch::togglesIndependently()
{
    QWidget group;
    auto *first = new MdSwitch(&group);
    auto *second = new MdSwitch(&group);
    group.show();
    QVERIFY(QTest::qWaitForWindowExposed(&group));

    first->setChecked(true);
    QVERIFY(first->isChecked());
    QVERIFY(!second->isChecked());
    second->setChecked(true);
    QVERIFY(second->isChecked());
    // Unlike the radio, a switch stays checked beside its sibling.
    QVERIFY(first->isChecked());
}

void TestMd3Switch::wholeRectIsTheTouchTarget()
{
    MdSwitch sw;
    sw.resize(sw.sizeHint());
    sw.show();
    QVERIFY(QTest::qWaitForWindowExposed(&sw));

    // A click in the widget's far corner — outside the 52×32 track — still
    // toggles: the touch target is the whole rect.
    QTest::mouseClick(&sw, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
    QVERIFY(sw.isChecked());
    QTest::mouseClick(&sw, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
    QVERIFY(!sw.isChecked());
}

// ---------------------------------------------------------------------------
// The style
// ---------------------------------------------------------------------------

void TestMd3Switch::styleIsInstalled()
{
    MdSwitch sw;
    QVERIFY(MdSwitchStyle::isInstalled());
}

QTEST_MAIN(TestMd3Switch)
#include "TestMd3Switch.moc"
