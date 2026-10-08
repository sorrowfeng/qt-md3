#include "TestMd3Common.h"

#include "core/MdCardTokens.h"
#include "core/MdElevation.h"
#include "core/MdShape.h"
#include "styles/MdCardStyle.h"
#include "widgets/MdCard.h"

#include <QtGui/QEnterEvent>
#include <QtGui/QMouseEvent>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

constexpr qreal kEpsilon = 1e-6;

bool closeTo(qreal a, qreal b, qreal epsilon = kEpsilon)
{
    return qAbs(a - b) <= epsilon;
}

} // namespace

class TestMd3Card : public QObject
{
    Q_OBJECT

private slots:
    void filledTokenRows();
    void elevatedTokenRows();
    void outlinedTokenRows();
    void disabledFacts();
    void overrideParsing();
    void layoutGeometry();
    void statePriority();
    void pointerContract();
    void keyboardContract();
    void elevationAnimation();
    void disabledSnaps();
    void clickabilityMargins();
    void renderSmoke();
    void styleIsInstalled();
};

void TestMd3Card::filledTokenRows()
{
    const MdCardTokens tokens = MdCardTokens::resolve(MdCardVariant::Filled);

    // Enabled: surface-container-highest at level0, no outline row.
    const MdCardStateRow &enabled = tokens.family.state(MdCardState::Enabled);
    QCOMPARE(enabled.container, ColorRole::SurfaceContainerHighest);
    QCOMPARE(enabled.content, ColorRole::OnSurface);
    QCOMPARE(enabled.stateLayer, ColorRole::Count);
    QCOMPARE(enabled.elevation, ElevationLevel::Level0);
    QCOMPARE(enabled.outline, ColorRole::Count);

    // The state rows restate the enabled colour; the layer colour is
    // on-surface everywhere; only elevation moves. Press does not raise —
    // the "Pressed (ripple)" row repeats level0.
    QCOMPARE(tokens.family.state(MdCardState::Hovered).elevation, ElevationLevel::Level1);
    QCOMPARE(tokens.family.state(MdCardState::Focused).elevation, ElevationLevel::Level0);
    QCOMPARE(tokens.family.state(MdCardState::Pressed).elevation, ElevationLevel::Level0);
    QCOMPARE(tokens.family.state(MdCardState::Dragged).elevation, ElevationLevel::Level3);
    for (const MdCardState state : { MdCardState::Hovered, MdCardState::Focused,
                                     MdCardState::Pressed, MdCardState::Dragged }) {
        const MdCardStateRow &row = tokens.family.state(state);
        QCOMPARE(row.container, ColorRole::SurfaceContainerHighest);
        QCOMPARE(row.stateLayer, ColorRole::OnSurface);
    }

    // Shared metrics: corner-medium container.
    QCOMPARE(tokens.containerShape, ShapeCorner::Medium);
    QVERIFY(closeTo(MdSystemTokens::cornerRadius(tokens.containerShape), 12.0));
}

void TestMd3Card::elevatedTokenRows()
{
    const MdCardTokens tokens = MdCardTokens::resolve(MdCardVariant::Elevated);

    // Enabled: surface-container-low, one level up, hover two, dragged four.
    const MdCardStateRow &enabled = tokens.family.state(MdCardState::Enabled);
    QCOMPARE(enabled.container, ColorRole::SurfaceContainerLow);
    QCOMPARE(enabled.content, ColorRole::OnSurface);
    QCOMPARE(enabled.elevation, ElevationLevel::Level1);
    QCOMPARE(tokens.family.state(MdCardState::Hovered).elevation, ElevationLevel::Level2);
    QCOMPARE(tokens.family.state(MdCardState::Focused).elevation, ElevationLevel::Level1);
    QCOMPARE(tokens.family.state(MdCardState::Pressed).elevation, ElevationLevel::Level1);
    QCOMPARE(tokens.family.state(MdCardState::Dragged).elevation, ElevationLevel::Level4);
    // No outline in the elevated export either.
    QCOMPARE(enabled.outline, ColorRole::Count);
}

void TestMd3Card::outlinedTokenRows()
{
    const MdCardTokens tokens = MdCardTokens::resolve(MdCardVariant::Outlined);

    // Enabled: plain surface at level0 with the 1 px outline-variant stroke.
    const MdCardStateRow &enabled = tokens.family.state(MdCardState::Enabled);
    QCOMPARE(enabled.container, ColorRole::Surface);
    QCOMPARE(enabled.elevation, ElevationLevel::Level0);
    QCOMPARE(enabled.outline, ColorRole::OutlineVariant);
    QVERIFY(closeTo(tokens.outlineWidth, 1.0));

    // Keyboard focus turns the stroke on-surface; the other states keep
    // outline-variant; disabled publishes no container change but keeps the
    // outline role (faded by the paint code).
    QCOMPARE(tokens.family.state(MdCardState::Focused).outline, ColorRole::OnSurface);
    QCOMPARE(tokens.family.state(MdCardState::Hovered).outline, ColorRole::OutlineVariant);
    QCOMPARE(tokens.family.state(MdCardState::Pressed).outline, ColorRole::OutlineVariant);
    QCOMPARE(tokens.family.state(MdCardState::Dragged).outline, ColorRole::OutlineVariant);
    const MdCardStateRow &disabledRow = tokens.family.state(MdCardState::Disabled);
    QCOMPARE(disabledRow.container, ColorRole::Surface);
    QCOMPARE(disabledRow.outline, ColorRole::OutlineVariant);
    QCOMPARE(tokens.family.state(MdCardState::Hovered).elevation, ElevationLevel::Level1);
    QCOMPARE(tokens.family.state(MdCardState::Dragged).elevation, ElevationLevel::Level3);
}

void TestMd3Card::disabledFacts()
{
    // Compose's disabled arithmetic, as constants: container 0.38, content
    // 0.38, outline 0.12 (outlined only).
    QVERIFY(closeTo(kDisabledContainerOpacity, 0.38));
    QVERIFY(closeTo(kDisabledContentAlpha, 0.38));
    QVERIFY(closeTo(kDisabledOutlineOpacity, 0.12));

    // Filled disables to surface-variant; elevated to surface; outlined
    // keeps its own container.
    const MdCardTokens filled = MdCardTokens::resolve(MdCardVariant::Filled);
    QCOMPARE(filled.family.state(MdCardState::Disabled).container, ColorRole::SurfaceVariant);
    const MdCardTokens elevated = MdCardTokens::resolve(MdCardVariant::Elevated);
    QCOMPARE(elevated.family.state(MdCardState::Disabled).container, ColorRole::Surface);
    const MdCardTokens outlined = MdCardTokens::resolve(MdCardVariant::Outlined);
    QCOMPARE(outlined.family.state(MdCardState::Disabled).container, ColorRole::Surface);

    // The disabled rows publish no state layer — a disabled card paints no
    // interactive state.
    QCOMPARE(filled.family.state(MdCardState::Disabled).stateLayer, ColorRole::Count);
    QCOMPARE(elevated.family.state(MdCardState::Disabled).stateLayer, ColorRole::Count);
    QCOMPARE(outlined.family.state(MdCardState::Disabled).stateLayer, ColorRole::Count);
}

void TestMd3Card::overrideParsing()
{
    MdComponentTokens overrides;
    overrides.setValue("md.comp.filled-card.container.shape", "large");
    overrides.setValue("md.comp.outlined-card.outline.width", "2px");
    overrides.setValue("md.comp.filled-card.icon.size", "28px");

    const MdCardTokens tokens = MdCardTokens::resolve(MdCardVariant::Filled, &overrides);
    QVERIFY(closeTo(MdSystemTokens::cornerRadius(tokens.containerShape), 16.0));
    QVERIFY(closeTo(tokens.iconSize, 28.0));

    const MdCardTokens outlined = MdCardTokens::resolve(MdCardVariant::Outlined, &overrides);
    QVERIFY(closeTo(outlined.outlineWidth, 2.0));

    // Unrelated components stay untouched.
    const MdCardTokens clean = MdCardTokens::resolve(MdCardVariant::Elevated, &overrides);
    QCOMPARE(clean.containerShape, ShapeCorner::Medium);
}

void TestMd3Card::layoutGeometry()
{
    MdCard card;
    card.resize(200, 100);

    // Non-clickable: the container is the widget rect, zero margin.
    const MdCardTokens &tokens = card.tokens();
    const MdCardStyle::Layout plain = MdCardStyle::layoutFor(card, tokens);
    QCOMPARE(plain.container, QRectF(QRect(0, 0, 200, 100)));
    QVERIFY(closeTo(plain.radii.first(), 12.0, 1e-4));

    // Clickable: the container shrinks by the focus-indicator margin, whose
    // derivation is the shared ring one (offset + active/2 + width/2).
    card.setClickable(true);
    const qreal inset = MdCardStyle::focusRingInset(tokens);
    QVERIFY(inset > 0.0);
    const MdCardStyle::Layout clickable = MdCardStyle::layoutFor(card, tokens);
    QCOMPARE(clickable.container, QRectF(inset, inset, 200.0 - 2.0 * inset, 100.0 - 2.0 * inset));
    QCOMPARE(card.containerRect(), clickable.container);
}

void TestMd3Card::statePriority()
{
    MdCard card;
    card.resize(200, 100);

    // A non-clickable card paints the enabled row even when disabled —
    // Compose's non-clickable overload has no interaction source.
    card.setEnabled(false);
    QCOMPARE(MdCardStyle::stateFor(card), MdCardState::Enabled);
    card.setEnabled(true);

    card.setClickable(true);
    QCOMPARE(MdCardStyle::stateFor(card), MdCardState::Enabled);

    // Disabled wins everything.
    card.setEnabled(false);
    QCOMPARE(MdCardStyle::stateFor(card), MdCardState::Disabled);
    card.setEnabled(true);

    // dragged > pressed > hovered > focused (focus needs keyboard focus).
    card.setDragged(true);
    QCOMPARE(MdCardStyle::stateFor(card), MdCardState::Dragged);
    card.setDragged(false);

    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));
    QTest::mousePress(&card, Qt::LeftButton, {}, QPoint(50, 50));
    QVERIFY(card.isPressed());
    QCOMPARE(MdCardStyle::stateFor(card), MdCardState::Pressed);
    QTest::mouseRelease(&card, Qt::LeftButton, {}, QPoint(50, 50));

    QEnterEvent enter(QPointF(1, 1), QPointF(1, 1), QPointF(1, 1));
    QApplication::sendEvent(&card, &enter);
    QVERIFY(card.isHovered());
    QCOMPARE(MdCardStyle::stateFor(card), MdCardState::Hovered);

    // Pointer focus does not reach the focused row (:focus-visible).
    card.setFocus(Qt::MouseFocusReason);
    QVERIFY(!card.hasKeyboardFocus());
    QCOMPARE(MdCardStyle::stateFor(card), MdCardState::Hovered);
}

void TestMd3Card::pointerContract()
{
    int clicks = 0;
    MdCard card;
    card.resize(200, 100);
    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));

    // Non-clickable: no signal, no ripple, press is inert.
    QTest::mouseClick(&card, Qt::LeftButton, {}, QPoint(100, 50));
    QCOMPARE(clicks, 0);
    card.setClickable(true);
    connect(&card, &MdCard::clicked, [&clicks] { ++clicks; });

    QTest::mouseClick(&card, Qt::LeftButton, {}, QPoint(100, 50));
    QCOMPARE(clicks, 1);

    // Press inside, release outside: no click.
    QTest::mousePress(&card, Qt::LeftButton, {}, QPoint(100, 50));
    QVERIFY(card.isPressed());
    QTest::mouseRelease(&card, Qt::LeftButton, {}, QPoint(300, 150));
    QCOMPARE(clicks, 1);
    QVERIFY(!card.isPressed());
}

void TestMd3Card::keyboardContract()
{
    MdCard card;
    card.resize(200, 100);
    card.setClickable(true);
    card.show();
    QVERIFY(QTest::qWaitForWindowExposed(&card));

    int clicks = 0;
    connect(&card, &MdCard::clicked, [&clicks] { ++clicks; });

    // Posted show/activation events must land before the focus request, and
    // the offscreen window must be active for the focus request to stick.
    card.activateWindow();
    QTest::qWait(50);

    // Tab arrives as keyboard focus: ring + focused row. activateWindow
    // already granted focus (ActiveWindowFocusReason), and Qt suppresses a
    // repeated FocusIn — clear first so the reason is the one under test.
    card.clearFocus();
    QTest::qWait(30);
    card.setFocus(Qt::TabFocusReason);
    QVERIFY(card.hasKeyboardFocus());

    QTest::keyClick(&card, Qt::Key_Space);
    QTest::keyClick(&card, Qt::Key_Return);
    QCOMPARE(clicks, 2);
}

void TestMd3Card::elevationAnimation()
{
    // The elevated card's ladder: resting level1 (1 dp), hovered level2
    // (3 dp), dragged level4 (12 dp).
    MdCard card(MdCardVariant::Elevated);
    card.resize(200, 100);
    card.setClickable(true);
    QVERIFY(closeTo(card.currentShadowDp(), MdElevation::shadowDp(ElevationLevel::Level1)));

    QEnterEvent enter(QPointF(1, 1), QPointF(1, 1), QPointF(1, 1));
    QApplication::sendEvent(&card, &enter);
    QVERIFY(card.isElevationAnimating());
    QTest::qWait(320);
    QVERIFY(!card.isElevationAnimating());
    QVERIFY(closeTo(card.currentShadowDp(), MdElevation::shadowDp(ElevationLevel::Level2), 1e-3));

    card.setDragged(true);
    QTest::qWait(320);
    QVERIFY(closeTo(card.currentShadowDp(), MdElevation::shadowDp(ElevationLevel::Level4), 1e-3));
    card.setDragged(false);
    QTest::qWait(320);
    // Back to the hovered row — the pointer never left.
    QVERIFY(closeTo(card.currentShadowDp(), MdElevation::shadowDp(ElevationLevel::Level2), 1e-3));

    // A non-clickable card has no ladder to animate: the dragged row applies
    // instantly (its state table is the enabled row anyway).
    MdCard plain(MdCardVariant::Elevated);
    plain.resize(200, 100);
    QVERIFY(!plain.isElevationAnimating());
    QVERIFY(closeTo(plain.currentShadowDp(), MdElevation::shadowDp(ElevationLevel::Level1)));
}

void TestMd3Card::disabledSnaps()
{
    MdCard card(MdCardVariant::Elevated);
    card.resize(200, 100);
    card.setClickable(true);
    QTest::qWait(320);
    QVERIFY(closeTo(card.currentShadowDp(), MdElevation::shadowDp(ElevationLevel::Level1), 1e-3));

    // Compose snaps to the disabled elevation with no transition.
    card.setEnabled(false);
    QVERIFY(!card.isElevationAnimating());
    QVERIFY(closeTo(card.currentShadowDp(), MdElevation::shadowDp(ElevationLevel::Level1)));
}

void TestMd3Card::clickabilityMargins()
{
    MdCard card;
    card.resize(200, 100);
    QCOMPARE(card.contentsMargins().left(), 0);

    // The focus margin is reserved exactly when the ring can appear.
    card.setClickable(true);
    const int inset = int(std::ceil(MdCardStyle::focusRingInset(card.tokens())));
    QCOMPARE(card.contentsMargins().left(), inset);
    card.setClickable(false);
    QCOMPARE(card.contentsMargins().left(), 0);
}

void TestMd3Card::renderSmoke()
{
    // Rendering exercises the full paint path; assertions stay structural
    // because offscreen antialiasing makes pixel sampling flaky.
    MdCard filled(MdCardVariant::Filled);
    filled.resize(200, 100);
    filled.show();
    QVERIFY(QTest::qWaitForWindowExposed(&filled));

    MdCard outlined(MdCardVariant::Outlined);
    outlined.resize(200, 100);
    outlined.setClickable(true);
    outlined.show();
    QVERIFY(QTest::qWaitForWindowExposed(&outlined));

    QImage image(220, 120, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    filled.render(&painter);
    outlined.render(&painter);
    painter.end();
    QVERIFY(!image.isNull());
}

void TestMd3Card::styleIsInstalled()
{
    QVERIFY(MdCardStyle::isInstalled());
}

QTEST_MAIN(TestMd3Card)

#include "TestMd3Card.moc"
