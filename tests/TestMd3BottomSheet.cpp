#include "TestMd3Common.h"

#include "core/MdBottomSheetTokens.h"
#include "core/MdShape.h"
#include "styles/MdBottomSheetStyle.h"
#include "widgets/MdBottomSheet.h"
#include "widgets/MdBottomSheetHost.h"

#include <QtGui/QMouseEvent>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

#include <cmath>

using namespace md;

namespace {

constexpr qreal kEpsilon = 1e-6;

bool closeTo(qreal a, qreal b, qreal epsilon = kEpsilon)
{
    return qAbs(a - b) <= epsilon;
}

/// Send a synthetic mouse sequence (press / move / release) in widget-local
/// coordinates. QTest::mouseMove is unreliable across platforms offscreen.
void dragSequence(QWidget *target, const QPointF &from, const QPointF &to)
{
    const QPoint fromPoint = from.toPoint();
    const QPoint toPoint = to.toPoint();
    QMouseEvent press(QEvent::MouseButtonPress, from, QPointF(target->mapToGlobal(fromPoint)),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(target, &press);
    QMouseEvent move(QEvent::MouseMove, to, QPointF(target->mapToGlobal(toPoint)), Qt::LeftButton,
                     Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(target, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, to, QPointF(target->mapToGlobal(toPoint)),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(target, &release);
}

void clickSequence(QWidget *target, const QPointF &at)
{
    dragSequence(target, at, at);
}

} // namespace

class TestMd3BottomSheet : public QObject
{
    Q_OBJECT

private slots:
    void tokenRows();
    void overrideParsing();
    void anchorMathModal();
    void anchorMathStandard();
    void anchorAvailability();
    void layoutGeometry();
    void layoutWithoutHandle();
    void stateTransitions();
    void confirmVeto();
    void handleToggle();
    void dragSettle();
    void staticPlacementIsUntouched();
    void hostGeometry();
    void hostScrimFade();
    void hostEscapeFlow();
    void hostScrimClick();
    void renderSmoke();
    void styleIsInstalled();
};

void TestMd3BottomSheet::tokenRows()
{
    const MdBottomSheetTokens tokens = MdBottomSheetTokens::resolve();

    // Container: surface-container-low, level1, corner-extra-large-top.
    QCOMPARE(tokens.containerColor, ColorRole::SurfaceContainerLow);
    QCOMPARE(tokens.containerElevation, ElevationLevel::Level1);
    QCOMPARE(tokens.containerShadowColor, ColorRole::Shadow);
    QCOMPARE(tokens.containerShape, ShapeCorner::ExtraLarge);
    // The published Hidden-state shape, recorded only.
    QCOMPARE(tokens.minimizedShape, ShapeCorner::None);

    // Drag handle: 32 x 4 in on-surface-variant; the deprecated 0.4 opacity
    // row is recorded, not applied.
    QCOMPARE(tokens.dragHandleWidth, 32.0);
    QCOMPARE(tokens.dragHandleHeight, 4.0);
    QCOMPARE(tokens.dragHandleColor, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.dragHandleOpacity, 0.4);

    // Focus indicator rows (recorded, not painted).
    QCOMPARE(tokens.focusIndicatorColor, ColorRole::Secondary);

    // Compose's private constants.
    QCOMPARE(MdBottomSheetTokens::kSheetPeekHeight, 56.0);
    QCOMPARE(MdBottomSheetTokens::kSheetMaxWidth, 640.0);
    QCOMPARE(MdBottomSheetTokens::kPositionalThreshold, 56.0);
    QCOMPARE(MdBottomSheetTokens::kVelocityThreshold, 125.0);
    QCOMPARE(MdBottomSheetTokens::kBoundaryDampeningZone, 125.0);
    QCOMPARE(MdBottomSheetTokens::kDragHandleVerticalPadding, 22.0);
    QCOMPARE(MdBottomSheetTokens::kScrimOpacity, 0.32);
    QCOMPARE(MdBottomSheetTokens::kShadowMargin, 4.0);
}

void TestMd3BottomSheet::overrideParsing()
{
    MdComponentTokens overrides;
    overrides.setValue(QStringLiteral("md.comp.sheet.bottom.docked.container.shape"),
                       QStringLiteral("medium"));
    overrides.setValue(QStringLiteral("md.comp.sheet.bottom.docked.minimized.container.shape"),
                       QStringLiteral("small"));
    overrides.setValue(QStringLiteral("md.comp.sheet.bottom.docked.drag-handle.width"),
                       QStringLiteral("48px"));
    overrides.setValue(QStringLiteral("md.comp.sheet.bottom.docked.drag-handle.height"),
                       QStringLiteral("6"));

    const MdBottomSheetTokens tokens = MdBottomSheetTokens::resolve(&overrides);
    QCOMPARE(tokens.containerShape, ShapeCorner::Medium);
    QCOMPARE(tokens.minimizedShape, ShapeCorner::Small);
    QCOMPARE(tokens.dragHandleWidth, 48.0);
    QCOMPARE(tokens.dragHandleHeight, 6.0);

    // An unparseable override is ignored, not coerced.
    MdComponentTokens bad;
    bad.setValue(QStringLiteral("md.comp.sheet.bottom.docked.container.shape"),
                 QStringLiteral("blob"));
    bad.setValue(QStringLiteral("md.comp.sheet.bottom.docked.drag-handle.width"),
                 QStringLiteral("wide"));
    const MdBottomSheetTokens fallback = MdBottomSheetTokens::resolve(&bad);
    QCOMPARE(fallback.containerShape, ShapeCorner::ExtraLarge);
    QCOMPARE(fallback.dragHandleWidth, 32.0);
}

void TestMd3BottomSheet::anchorMathModal()
{
    // A 600 px parent with a 240 px sheet: the deterministic partial rule
    // makes min(300, 120) = 120 visible.
    const qreal full = 600.0;
    const qreal sheet = 240.0;
    const qreal peek = MdBottomSheetTokens::kSheetPeekHeight;

    QCOMPARE(MdBottomSheetStyle::partialVisibleHeight(full, sheet), 120.0);
    QVERIFY(closeTo(MdBottomSheetStyle::anchorOffset(MdSheetKind::Modal,
                                                     MdSheetState::Hidden, full, sheet, peek,
                                                     false, false),
                    600.0));
    QVERIFY(closeTo(MdBottomSheetStyle::anchorOffset(MdSheetKind::Modal,
                                                     MdSheetState::PartiallyExpanded, full,
                                                     sheet, peek, false, false),
                    480.0));
    QVERIFY(closeTo(MdBottomSheetStyle::anchorOffset(MdSheetKind::Modal,
                                                     MdSheetState::Expanded, full, sheet, peek,
                                                     false, false),
                    360.0));

    // A sheet taller than half the parent: the content cap wins — half the
    // sheet (250), not half the screen.
    QCOMPARE(MdBottomSheetStyle::partialVisibleHeight(600.0, 500.0), 250.0);
    QVERIFY(closeTo(MdBottomSheetStyle::anchorOffset(MdSheetKind::Modal,
                                                     MdSheetState::PartiallyExpanded, 600.0,
                                                     500.0, peek, false, false),
                    350.0));
    // The expanded anchor never goes above the parent's top edge.
    QVERIFY(closeTo(MdBottomSheetStyle::anchorOffset(MdSheetKind::Modal,
                                                     MdSheetState::Expanded, 600.0, 700.0, peek,
                                                     false, false),
                    0.0));
}

void TestMd3BottomSheet::anchorMathStandard()
{
    // The standard sheet: partial rides the peek height, hidden is skipped
    // by default (skipHiddenState = true).
    const qreal full = 600.0;
    const qreal sheet = 240.0;
    const qreal peek = MdBottomSheetTokens::kSheetPeekHeight;

    QVERIFY(closeTo(MdBottomSheetStyle::anchorOffset(MdSheetKind::Standard,
                                                     MdSheetState::PartiallyExpanded, full,
                                                     sheet, peek, true, false),
                    544.0));
    QVERIFY(closeTo(MdBottomSheetStyle::anchorOffset(MdSheetKind::Standard,
                                                     MdSheetState::Expanded, full, sheet, peek,
                                                     true, false),
                    360.0));
    QVERIFY(std::isnan(MdBottomSheetStyle::anchorOffset(MdSheetKind::Standard,
                                                        MdSheetState::Hidden, full, sheet, peek,
                                                        true, false)));

    // With the hidden state enabled it anchors at the parent's bottom.
    QVERIFY(closeTo(MdBottomSheetStyle::anchorOffset(MdSheetKind::Standard,
                                                     MdSheetState::Hidden, full, sheet, peek,
                                                     false, false),
                    600.0));
}

void TestMd3BottomSheet::anchorAvailability()
{
    const qreal full = 600.0;
    const qreal sheet = 240.0;
    const qreal peek = MdBottomSheetTokens::kSheetPeekHeight;

    // Modal: hidden always, partial for any non-zero sheet, expanded for a
    // non-zero sheet.
    QVERIFY(MdBottomSheetStyle::hasAnchor(MdSheetKind::Modal, MdSheetState::Hidden, full, sheet,
                                          peek, false, false));
    QVERIFY(MdBottomSheetStyle::hasAnchor(MdSheetKind::Modal,
                                          MdSheetState::PartiallyExpanded, full, sheet, peek,
                                          false, false));
    QVERIFY(MdBottomSheetStyle::hasAnchor(MdSheetKind::Modal, MdSheetState::Expanded, full, sheet,
                                          peek, false, false));

    // skipPartiallyExpanded removes the partial anchor.
    QVERIFY(!MdBottomSheetStyle::hasAnchor(MdSheetKind::Modal, MdSheetState::PartiallyExpanded,
                                           full, sheet, peek, false, true));

    // A zero-height sheet has no partial or expanded anchor.
    QVERIFY(!MdBottomSheetStyle::hasAnchor(MdSheetKind::Modal, MdSheetState::PartiallyExpanded,
                                           full, 0.0, peek, false, false));
    QVERIFY(!MdBottomSheetStyle::hasAnchor(MdSheetKind::Modal, MdSheetState::Expanded, full, 0.0,
                                           peek, false, false));

    // Standard: hidden skipped by default, present when not skipped; partial
    // collapses when the peek equals the sheet height.
    QVERIFY(!MdBottomSheetStyle::hasAnchor(MdSheetKind::Standard, MdSheetState::Hidden, full,
                                           sheet, peek, true, false));
    QVERIFY(MdBottomSheetStyle::hasAnchor(MdSheetKind::Standard, MdSheetState::Hidden, full, sheet,
                                          peek, false, false));
    QVERIFY(!MdBottomSheetStyle::hasAnchor(MdSheetKind::Standard,
                                           MdSheetState::PartiallyExpanded, full, sheet, sheet,
                                           true, false));
}

void TestMd3BottomSheet::layoutGeometry()
{
    const MdBottomSheetTokens tokens = MdBottomSheetTokens::resolve();
    const MdBottomSheetStyle::Layout layout =
        MdBottomSheetStyle::layoutFor(320.0, 244.0, true, tokens);

    // The container: shadow margin on the top/left/right, flush bottom.
    QVERIFY(layout.container.isValid());
    QVERIFY(closeTo(layout.container.left(), 4.0));
    QVERIFY(closeTo(layout.container.top(), 4.0));
    QVERIFY(closeTo(layout.container.width(), 312.0));
    QVERIFY(closeTo(layout.container.bottom(), 244.0));

    // corner-extra-large-top: the extra-large radius on the top pair only.
    const qreal extraLarge = MdShape::radius(ShapeCorner::ExtraLarge);
    QCOMPARE(layout.radii.size(), 4);
    QVERIFY(closeTo(layout.radii.at(0), extraLarge));
    QVERIFY(closeTo(layout.radii.at(1), extraLarge));
    QVERIFY(closeTo(layout.radii.at(2), 0.0));
    QVERIFY(closeTo(layout.radii.at(3), 0.0));

    // The drag handle: 32 x 4, centred, 22 px below the container top.
    QVERIFY(layout.handleRect.isValid());
    QCOMPARE(layout.handleRect.size(), QSizeF(32.0, 4.0));
    QVERIFY(closeTo(layout.handleRect.center().x(), layout.container.center().x()));
    QVERIFY(closeTo(layout.handleRect.top(), 4.0 + 22.0));

    // The content area starts below the handle's touch padding.
    QVERIFY(layout.contentRect.isValid());
    QVERIFY(closeTo(layout.contentRect.top(),
                    layout.handleRect.top() + layout.handleRect.height() + 22.0));
    QVERIFY(closeTo(layout.contentRect.bottom(), layout.container.bottom()));
}

void TestMd3BottomSheet::layoutWithoutHandle()
{
    const MdBottomSheetTokens tokens = MdBottomSheetTokens::resolve();
    const MdBottomSheetStyle::Layout layout =
        MdBottomSheetStyle::layoutFor(320.0, 244.0, false, tokens);

    QVERIFY(!layout.handleRect.isValid());
    QVERIFY(!layout.contentRect.isValid());
    QVERIFY(layout.container.isValid());
}

void TestMd3BottomSheet::stateTransitions()
{
    QWidget parent;
    parent.resize(600, 600);
    MdBottomSheet sheet(MdSheetKind::Modal, &parent);
    sheet.setGeometryManaged(true);
    sheet.resize(320, 244);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    QCOMPARE(sheet.state(), MdSheetState::Hidden);
    QVERIFY(!sheet.isVisible());

    // showSheet: Hidden -> PartiallyExpanded (the deterministic 120 px).
    sheet.showSheet();
    QVERIFY(sheet.isAnimating());
    QTest::qWait(700);
    QVERIFY(!sheet.isAnimating());
    QCOMPARE(sheet.state(), MdSheetState::PartiallyExpanded);
    QVERIFY(sheet.isVisible());
    QVERIFY(closeTo(sheet.currentOffset(), 480.0, 1.0));
    QVERIFY(closeTo(sheet.y(), 480.0 - 4.0, 1.0));

    // expand: PartiallyExpanded -> Expanded.
    sheet.expand();
    QTest::qWait(700);
    QCOMPARE(sheet.state(), MdSheetState::Expanded);
    QVERIFY(closeTo(sheet.currentOffset(), 360.0, 1.0));

    // hideSheet: Expanded -> Hidden, dismissed emitted, widget hidden.
    int dismissals = 0;
    QObject::connect(&sheet, &MdBottomSheet::dismissed, [&dismissals] { ++dismissals; });
    sheet.hideSheet();
    QTest::qWait(700);
    QCOMPARE(sheet.state(), MdSheetState::Hidden);
    QVERIFY(!sheet.isVisible());
    QCOMPARE(dismissals, 1);
}

void TestMd3BottomSheet::confirmVeto()
{
    QWidget parent;
    parent.resize(600, 600);
    MdBottomSheet sheet(MdSheetKind::Modal, &parent);
    sheet.setGeometryManaged(true);
    sheet.resize(320, 244);
    sheet.setConfirmValueChange([](MdSheetState state) { return state != MdSheetState::Hidden; });
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    int dismissals = 0;
    QObject::connect(&sheet, &MdBottomSheet::dismissed, [&dismissals] { ++dismissals; });

    sheet.showSheet();
    QTest::qWait(700);
    QCOMPARE(sheet.state(), MdSheetState::PartiallyExpanded);

    // The veto refuses the hide: the sheet stays where it is.
    sheet.hideSheet();
    QTest::qWait(700);
    QVERIFY(sheet.isVisible());
    QCOMPARE(sheet.state(), MdSheetState::PartiallyExpanded);
    QCOMPARE(dismissals, 0);
}

void TestMd3BottomSheet::handleToggle()
{
    QWidget parent;
    parent.resize(600, 600);
    MdBottomSheet sheet(MdSheetKind::Modal, &parent);
    sheet.setGeometryManaged(true);
    sheet.resize(320, 244);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    sheet.showSheet();
    QTest::qWait(700);
    QCOMPARE(sheet.state(), MdSheetState::PartiallyExpanded);

    // Click the handle at partial: expands.
    const QRectF handle = sheet.currentLayout().handleRect;
    clickSequence(&sheet, handle.center());
    QTest::qWait(700);
    QCOMPARE(sheet.state(), MdSheetState::Expanded);

    // Click the handle at expanded (modal): the dismiss flow.
    int dismissals = 0;
    QObject::connect(&sheet, &MdBottomSheet::dismissed, [&dismissals] { ++dismissals; });
    clickSequence(&sheet, handle.center());
    QTest::qWait(700);
    QCOMPARE(dismissals, 1);
    QVERIFY(!sheet.isVisible());
}

void TestMd3BottomSheet::dragSettle()
{
    QWidget parent;
    parent.resize(600, 600);
    MdBottomSheet sheet(MdSheetKind::Modal, &parent);
    sheet.setGeometryManaged(true);
    sheet.resize(320, 244);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    sheet.expand();
    QTest::qWait(700);
    QVERIFY(closeTo(sheet.currentOffset(), 360.0, 1.0));

    // A 100 px downward drag from Expanded lands nearer the partial anchor
    // (480) than Hidden (600): it settles at partial.
    dragSequence(&sheet, QPointF(80.0, 100.0), QPointF(80.0, 200.0));
    QTest::qWait(700);
    QCOMPARE(sheet.state(), MdSheetState::PartiallyExpanded);
    QVERIFY(closeTo(sheet.currentOffset(), 480.0, 1.0));

    // A 200 px downward drag from Expanded passes the threshold closer to
    // Hidden: it settles at Hidden and dismisses.
    int dismissals = 0;
    QObject::connect(&sheet, &MdBottomSheet::dismissed, [&dismissals] { ++dismissals; });
    sheet.expand();
    QTest::qWait(700);
    dragSequence(&sheet, QPointF(80.0, 100.0), QPointF(80.0, 300.0));
    QTest::qWait(700);
    QCOMPARE(sheet.state(), MdSheetState::Hidden);
    QCOMPARE(dismissals, 1);

    // A small drag below the positional threshold returns to the settled
    // anchor.
    sheet.showSheet();
    QTest::qWait(700);
    dragSequence(&sheet, QPointF(80.0, 150.0), QPointF(80.0, 160.0));
    QTest::qWait(700);
    QCOMPARE(sheet.state(), MdSheetState::PartiallyExpanded);
    QVERIFY(closeTo(sheet.currentOffset(), 480.0, 1.0));
}

void TestMd3BottomSheet::staticPlacementIsUntouched()
{
    // A statically-placed sheet (the gallery snapshot idiom) never jumps to
    // its anchors: no geometry management, no movement on parent resize.
    QWidget parent;
    parent.resize(600, 600);
    MdBottomSheet sheet(MdSheetKind::Standard, &parent);
    sheet.resize(320, 244);
    QVERIFY(!sheet.geometryManaged());

    const QPoint original = sheet.pos();
    parent.resize(700, 500);
    QTest::qWait(50);
    QCOMPARE(sheet.pos(), original);
}

void TestMd3BottomSheet::hostGeometry()
{
    QWidget parent;
    parent.resize(800, 600);
    MdBottomSheetHost host(&parent);
    host.sheet()->resize(320, 244);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    host.show();
    QTest::qWait(700);
    QVERIFY(host.isShowing());

    // The sheet's width is clamped into the 640 cap (+ shadow margins) and
    // bottom-anchored: the widget's bottom edge lands at or below the host's
    // bottom (the partial state's container extends past it, clipped).
    const QRect sheetGeometry = host.sheet()->geometry();
    QVERIFY(sheetGeometry.width() <= int(MdBottomSheetTokens::kSheetMaxWidth + 8.0));
    QVERIFY(closeTo(sheetGeometry.center().x(), host.width() / 2.0, 1.5));
    QVERIFY(sheetGeometry.bottom() >= host.height() - 2);

    // Expanded: the container bottom sits exactly on the host's bottom.
    host.sheet()->expand();
    QTest::qWait(700);
    QCOMPARE(host.sheet()->state(), MdSheetState::Expanded);
    QVERIFY(closeTo(host.sheet()->geometry().bottom() + 1.0, qreal(host.height()), 1.5));
}

void TestMd3BottomSheet::hostScrimFade()
{
    QWidget parent;
    parent.resize(600, 600);
    MdBottomSheetHost host(&parent);
    host.sheet()->resize(320, 244);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    // The scrim starts transparent and settles fully dimmed.
    QCOMPARE(host.currentScrimAlpha(), 0.0);
    host.show();
    QVERIFY(host.isTransitioning());
    QTest::qWait(700);
    QCOMPARE(host.currentScrimAlpha(), 1.0);
    QVERIFY(!host.isTransitioning());

    // Showing while already showing is a no-op.
    host.show();
    QTest::qWait(100);
    QCOMPARE(host.currentScrimAlpha(), 1.0);

    // The leave phase runs the scrim back down to 0.
    host.dismiss();
    QTest::qWait(900);
    QVERIFY(!host.isShowing());
    QCOMPARE(host.currentScrimAlpha(), 0.0);
}

void TestMd3BottomSheet::hostEscapeFlow()
{
    QWidget parent;
    parent.resize(600, 600);
    MdBottomSheetHost host(&parent);
    host.sheet()->resize(320, 244);
    int dismissals = 0;
    QObject::connect(&host, &MdBottomSheetHost::dismissed, [&dismissals] { ++dismissals; });
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    host.show();
    QTest::qWait(700);

    // Escape at Expanded settles to partial (Compose's settleToDismiss) —
    // the sheet stays, no dismissal.
    host.sheet()->expand();
    QTest::qWait(700);
    QTest::keyClick(host.sheet(), Qt::Key_Escape);
    QTest::qWait(700);
    QCOMPARE(host.sheet()->state(), MdSheetState::PartiallyExpanded);
    QCOMPARE(dismissals, 0);
    QVERIFY(host.isShowing());

    // Escape at partial hides the sheet: the dismiss flow. The sheet emits
    // dismissed() when the slide-down SETTLES (Compose's
    // invokeOnCompletion), so wait for it.
    QTest::keyClick(host.sheet(), Qt::Key_Escape);
    QTest::qWait(900);
    QCOMPARE(dismissals, 1);
    QVERIFY(!host.isShowing());
}

void TestMd3BottomSheet::hostScrimClick()
{
    QWidget parent;
    parent.resize(600, 600);
    MdBottomSheetHost host(&parent);
    host.sheet()->resize(320, 244);
    int dismissals = 0;
    QObject::connect(&host, &MdBottomSheetHost::dismissed, [&dismissals] { ++dismissals; });
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    host.show();
    QTest::qWait(700);

    // A click on the scrim (outside the sheet) runs animateToDismiss —
    // always hide, even at partial.
    QTest::mousePress(&host, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
    QTest::qWait(900);
    QCOMPARE(dismissals, 1);
    QVERIFY(!host.isShowing());

    // A click inside the sheet never reaches the host.
    host.show();
    QTest::qWait(700);
    const QPoint inside = host.sheet()->geometry().center();
    QMouseEvent insidePress(QEvent::MouseButtonPress, QPointF(inside),
                            QPointF(host.mapToGlobal(inside)), Qt::LeftButton, Qt::LeftButton,
                            Qt::NoModifier);
    QApplication::sendEvent(&host, &insidePress);
    QCOMPARE(dismissals, 1);
    QVERIFY(host.isShowing());
}

void TestMd3BottomSheet::renderSmoke()
{
    // Static snapshots at several states/kinds must paint without crashing.
    MdBottomSheet standard(MdSheetKind::Standard);
    standard.setState(MdSheetState::PartiallyExpanded);
    standard.resize(320, 244);
    standard.show();
    QVERIFY(!standard.grab().toImage().isNull());

    standard.setState(MdSheetState::Expanded);
    QVERIFY(!standard.grab().toImage().isNull());

    MdBottomSheet bare(MdSheetKind::Modal);
    bare.setHandleVisible(false);
    bare.setState(MdSheetState::Expanded);
    bare.resize(320, 164);
    bare.show();
    QVERIFY(!bare.grab().toImage().isNull());

    // The host paints the scrim at several alphas.
    QWidget parent;
    parent.resize(600, 600);
    MdBottomSheetHost host(&parent);
    host.sheet()->resize(320, 244);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    host.show();
    QTest::qWait(50);
    QVERIFY(!host.grab().toImage().isNull());
    QTest::qWait(700);
    QVERIFY(!host.grab().toImage().isNull());
}

void TestMd3BottomSheet::styleIsInstalled()
{
    // Constructing a sheet installs the paint filter.
    MdBottomSheet sheet;
    QVERIFY(MdBottomSheetStyle::isInstalled());
}

QTEST_MAIN(TestMd3BottomSheet)

#include "TestMd3BottomSheet.moc"
