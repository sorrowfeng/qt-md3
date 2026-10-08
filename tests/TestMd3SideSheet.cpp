// TestMd3SideSheet — the `md.comp.sheet.side.*` family lock.
//
// Tokens first (the two configuration rows, the recorded-only rows, the
// MDC-Android port constants), then the pure anchor math (both edges),
// the layout (per-edge margins, per-kind radii, divider, padding), the
// state machine (defaults, the springs, the cancel flow, the veto), the
// horizontal drag settle, and the host's modal presentation.

#include "styles/MdSideSheetStyle.h"

#include "widgets/MdSideSheet.h"
#include "widgets/MdSideSheetHost.h"

#include "core/MdTheme.h"
#include "core/MdTokens.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtTest/QTest>
#include <QtWidgets/QApplication>

#include <cmath>

namespace md {
namespace {

bool closeTo(qreal actual, qreal expected, qreal epsilon = 0.01)
{
    return std::abs(actual - expected) <= epsilon;
}

/// A press-move-release drag sequence sent straight to the target (the
/// hidden sheet sits off its parent's edge, so delivered events are the
/// only reliable channel).
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

class TestMd3SideSheet : public QObject
{
    Q_OBJECT

private slots:
    // --- tokens -------------------------------------------------------------
    void containerRows();
    void tokenOverride();
    void portConstants();

    // --- anchor math (pure) ---------------------------------------------------
    void anchorMathRight();
    void anchorMathLeft();
    void anchorAvailability();

    // --- layout (pure) --------------------------------------------------------
    void layoutRightModal();
    void layoutLeftModal();
    void layoutStandardAndContent();

    // --- widget ----------------------------------------------------------------
    void stateDefaultsAndSnap();
    void expandHideFlow();
    void confirmVeto();
    void dragSettle();
    void managedGeometryFollowsParent();
    void renderSmoke();

    // --- host -------------------------------------------------------------------
    void hostShowAndEscape();
    void hostScrimClick();
};

// --- tokens -----------------------------------------------------------------

void TestMd3SideSheet::containerRows()
{
    const MdSideSheetTokens tokens;
    // Standard row: surface at level 0, corner-none.
    QCOMPARE(tokens.standardContainerColor, ColorRole::Surface);
    QCOMPARE(tokens.standardContainerElevation, ElevationLevel::Level0);
    QCOMPARE(tokens.standardContainerShape, ShapeCorner::None);
    // Modal row: surface-container-low at level 1, the large-start radius
    // carried as the Large shape (the style splits it per edge).
    QCOMPARE(tokens.modalContainerColor, ColorRole::SurfaceContainerLow);
    QCOMPARE(tokens.modalContainerElevation, ElevationLevel::Level1);
    QCOMPARE(tokens.modalContainerShape, ShapeCorner::Large);
    // The shared rows.
    QCOMPARE(tokens.containerWidth, 256.0);
    QCOMPARE(tokens.detachedShape, ShapeCorner::Large);
    QCOMPARE(tokens.headlineColor, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.dividerColor, ColorRole::Outline);
    QCOMPARE(tokens.actionLabelColor, ColorRole::Primary);
}

void TestMd3SideSheet::tokenOverride()
{
    MdComponentTokens overrides;
    overrides.setValue(QStringLiteral("md.comp.sheet.side.docked.container.width"),
                       QStringLiteral("320px"));
    overrides.setValue(QStringLiteral("md.comp.sheet.side.docked.modal.container.shape"),
                       QStringLiteral("extra-large"));
    const MdSideSheetTokens tokens = MdSideSheetTokens::resolve(&overrides);
    QCOMPARE(tokens.containerWidth, 320.0);
    QCOMPARE(tokens.modalContainerShape, ShapeCorner::ExtraLarge);
    // The unlisted rows stay at the export values.
    QCOMPARE(tokens.standardContainerShape, ShapeCorner::None);

    // An unparseable override is ignored, not coerced.
    MdComponentTokens bad;
    bad.setValue(QStringLiteral("md.comp.sheet.side.docked.container.width"),
                 QStringLiteral("wide"));
    bad.setValue(QStringLiteral("md.comp.sheet.side.docked.modal.container.shape"),
                 QStringLiteral("blob"));
    const MdSideSheetTokens fallback = MdSideSheetTokens::resolve(&bad);
    QCOMPARE(fallback.containerWidth, 256.0);
    QCOMPARE(fallback.modalContainerShape, ShapeCorner::Large);
}

void TestMd3SideSheet::portConstants()
{
    // The MDC-Android behaviour constants, transcribed.
    QCOMPARE(MdSideSheetTokens::kSignificantVelocityThreshold, 500.0);
    QCOMPARE(MdSideSheetTokens::kHideThreshold, 0.5);
    QCOMPARE(MdSideSheetTokens::kHideFriction, 0.1);
    // The detached margin (recorded with the un-ported presentation) and the
    // scrim / shadow / padding constants.
    QCOMPARE(MdSideSheetTokens::kDetachedMargin, 16.0);
    QCOMPARE(MdSideSheetTokens::kScrimOpacity, 0.32);
    QCOMPARE(MdSideSheetTokens::kShadowMargin, 4.0);
    QCOMPARE(MdSideSheetTokens::kContentPadding, 24.0);
}

// --- anchor math (pure) -------------------------------------------------------

void TestMd3SideSheet::anchorMathRight()
{
    // MDC's RightSheetDelegate: hidden = the parent's width; expanded =
    // max(0, parentWidth - sheetWidth - innerMargin).
    QCOMPARE(MdSideSheetStyle::hiddenOffsetX(MdSideSheetEdge::Right, 600.0, 256.0), 600.0);
    QVERIFY(closeTo(MdSideSheetStyle::expandedOffsetX(MdSideSheetEdge::Right, 600.0, 256.0, 0.0),
                    344.0));
    QVERIFY(closeTo(MdSideSheetStyle::expandedOffsetX(MdSideSheetEdge::Right, 600.0, 256.0, 16.0),
                    328.0));
    // A sheet wider than the parent clamps against the parent's left.
    QVERIFY(closeTo(MdSideSheetStyle::expandedOffsetX(MdSideSheetEdge::Right, 600.0, 700.0, 0.0),
                    0.0));
    // The travel the drag clamps into.
    QCOMPARE(MdSideSheetStyle::sheetTravel(600.0, 344.0), 256.0);
}

void TestMd3SideSheet::anchorMathLeft()
{
    // Mirrored: hidden = -sheetWidth; expanded = min(innerMargin,
    // max(0, parentWidth - sheetWidth)).
    QCOMPARE(MdSideSheetStyle::hiddenOffsetX(MdSideSheetEdge::Left, 600.0, 256.0), -256.0);
    QVERIFY(closeTo(MdSideSheetStyle::expandedOffsetX(MdSideSheetEdge::Left, 600.0, 256.0, 0.0),
                    0.0));
    QVERIFY(closeTo(MdSideSheetStyle::expandedOffsetX(MdSideSheetEdge::Left, 600.0, 256.0, 16.0),
                    16.0));
    QVERIFY(closeTo(MdSideSheetStyle::expandedOffsetX(MdSideSheetEdge::Left, 600.0, 700.0, 16.0),
                    0.0));
}

void TestMd3SideSheet::anchorAvailability()
{
    // Hidden always exists; Expanded needs a non-zero sheet width.
    QVERIFY(MdSideSheetStyle::hasAnchor(MdSideSheetState::Hidden, 256.0));
    QVERIFY(MdSideSheetStyle::hasAnchor(MdSideSheetState::Expanded, 256.0));
    QVERIFY(!MdSideSheetStyle::hasAnchor(MdSideSheetState::Expanded, 0.0));
    QVERIFY(std::isnan(MdSideSheetStyle::anchorOffsetX(MdSideSheetEdge::Right,
                                                       MdSideSheetState::Expanded, 600.0, 0.0,
                                                       0.0)));
}

// --- layout (pure) --------------------------------------------------------------

void TestMd3SideSheet::layoutRightModal()
{
    const MdSideSheetTokens tokens;
    const auto layout = MdSideSheetStyle::layoutFor(MdSideSheetEdge::Right,
                                                    MdSideSheetKind::Modal, 260.0, 400.0, false,
                                                    tokens);
    // The widget rect carries the shadow margin on the top/bottom/left; the
    // right (docked) edge is flush.
    QCOMPARE(layout.container, QRectF(4.0, 4.0, 256.0, 392.0));
    // The modal radius sits on the START pair — for a right sheet the left
    // pair (TL / BL).
    QCOMPARE(layout.radii.size(), 4);
    QVERIFY(layout.radii[0] > 0.0);
    QCOMPARE(layout.radii[1], 0.0);
    QCOMPARE(layout.radii[2], 0.0);
    QVERIFY(layout.radii[3] > 0.0);
    // No divider requested.
    QVERIFY(!layout.dividerRect.isValid());
    // The content area: the spec's 24 px padding.
    const QRectF content(28.0, 28.0, 208.0, 344.0);
    QCOMPARE(layout.contentRect, content);
}

void TestMd3SideSheet::layoutLeftModal()
{
    const MdSideSheetTokens tokens;
    const auto layout = MdSideSheetStyle::layoutFor(MdSideSheetEdge::Left,
                                                    MdSideSheetKind::Modal, 260.0, 400.0, true,
                                                    tokens);
    // Mirrored margins: the left (docked) edge flush.
    QCOMPARE(layout.container, QRectF(0.0, 4.0, 256.0, 392.0));
    // The start pair faces the content — for a left sheet the right pair.
    QCOMPARE(layout.radii[0], 0.0);
    QVERIFY(layout.radii[1] > 0.0);
    QVERIFY(layout.radii[2] > 0.0);
    QCOMPARE(layout.radii[3], 0.0);
    // The divider runs the full height along the content-facing (right)
    // edge.
    QVERIFY(layout.dividerRect.isValid());
    QCOMPARE(layout.dividerRect.left(), layout.container.right() - 1.0);
    QCOMPARE(layout.dividerRect.height(), layout.container.height());
}

void TestMd3SideSheet::layoutStandardAndContent()
{
    const MdSideSheetTokens tokens;
    // The standard sheet: corner-none on every corner, whatever the edge.
    const auto standard = MdSideSheetStyle::layoutFor(MdSideSheetEdge::Right,
                                                      MdSideSheetKind::Standard, 260.0, 400.0,
                                                      false, tokens);
    const QList<qreal> squareRadii{0.0, 0.0, 0.0, 0.0};
    QCOMPARE(standard.radii, squareRadii);
}

// --- widget -----------------------------------------------------------------------

void TestMd3SideSheet::stateDefaultsAndSnap()
{
    // Modal starts hidden (MDC's behavior default), standard expanded (a
    // docked sheet sits in the layout from the start).
    QWidget parent;
    parent.resize(600, 400);
    MdSideSheet modal(MdSideSheetKind::Modal, MdSideSheetEdge::Right, &parent);
    QCOMPARE(modal.state(), MdSideSheetState::Hidden);
    QVERIFY(!modal.isVisible());
    MdSideSheet standard(MdSideSheetKind::Standard, MdSideSheetEdge::Left, &parent);
    QCOMPARE(standard.state(), MdSideSheetState::Expanded);
    QCOMPARE(standard.edge(), MdSideSheetEdge::Left);
    QVERIFY(!standard.hasDivider());

    // setState snaps without animating; the sheet's own geometry is the
    // caller's business while unmanaged.
    standard.resize(260, 400);
    standard.setState(MdSideSheetState::Hidden);
    QCOMPARE(standard.state(), MdSideSheetState::Hidden);
    QCOMPARE(standard.targetState(), MdSideSheetState::Hidden);
    standard.setState(MdSideSheetState::Expanded);
    QVERIFY(!standard.isAnimating());
}

void TestMd3SideSheet::expandHideFlow()
{
    QWidget parent;
    parent.resize(600, 400);
    MdSideSheet sheet(MdSideSheetKind::Modal, MdSideSheetEdge::Right, &parent);
    sheet.setGeometryManaged(true);
    sheet.resize(260, 400);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    int stateChanges = 0;
    int dismissals = 0;
    connect(&sheet, &MdSideSheet::stateChanged, [&] { ++stateChanges; });
    connect(&sheet, &MdSideSheet::dismissed, [&] { ++dismissals; });

    sheet.expandSheet();
    QVERIFY(sheet.isAnimating());
    QTest::qWait(900);
    QCOMPARE(sheet.state(), MdSideSheetState::Expanded);
    QVERIFY(sheet.isVisible());
    // Docked against the right edge: the container's x at parentWidth -
    // sheetWidth, the widget one shadow margin left of it.
    QCOMPARE(sheet.currentOffset(), 344.0);
    QCOMPARE(sheet.x(), 340);
    QCOMPARE(sheet.y(), 0);
    QCOMPARE(sheet.height(), 400);

    sheet.hideSheet();
    QTest::qWait(900);
    QCOMPARE(sheet.state(), MdSideSheetState::Hidden);
    QVERIFY(!sheet.isVisible());
    QCOMPARE(dismissals, 1);
    // The settled offsets only emit stateChanged once per stable arrival.
    QCOMPARE(stateChanges, 2);
}

void TestMd3SideSheet::confirmVeto()
{
    QWidget parent;
    parent.resize(600, 400);
    MdSideSheet sheet(MdSideSheetKind::Modal, MdSideSheetEdge::Right, &parent);
    sheet.setGeometryManaged(true);
    sheet.resize(260, 400);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    sheet.expandSheet();
    QTest::qWait(900);
    QCOMPARE(sheet.state(), MdSideSheetState::Expanded);

    sheet.setConfirmValueChange([](MdSideSheetState state) { return state != MdSideSheetState::Hidden; });
    sheet.hideSheet();
    QTest::qWait(400);
    // Vetoed: no animation, no settle, still expanded and visible.
    QCOMPARE(sheet.state(), MdSideSheetState::Expanded);
    QVERIFY(sheet.isVisible());
    QVERIFY(!sheet.isAnimating());
}

void TestMd3SideSheet::dragSettle()
{
    QWidget parent;
    parent.resize(600, 400);
    MdSideSheet sheet(MdSideSheetKind::Modal, MdSideSheetEdge::Right, &parent);
    sheet.setGeometryManaged(true);
    sheet.resize(260, 400);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    // From hidden, a 200 px leftward drag (past the 472 midpoint of the
    // 344..600 travel) settles expanded.
    dragSequence(&sheet, QPointF(30.0, 100.0), QPointF(30.0 - 200.0, 100.0));
    QTest::qWait(900);
    QCOMPARE(sheet.state(), MdSideSheetState::Expanded);

    // From expanded, a 150 px rightward drag (to 494, closer to the hidden
    // 600 than the expanded 344) settles hidden — the cancel flow.
    int dismissals = 0;
    connect(&sheet, &MdSideSheet::dismissed, [&] { ++dismissals; });
    dragSequence(&sheet, QPointF(30.0, 100.0), QPointF(30.0 + 150.0, 100.0));
    QTest::qWait(900);
    QCOMPARE(sheet.state(), MdSideSheetState::Hidden);
    QVERIFY(!sheet.isVisible());
    QCOMPARE(dismissals, 1);

    // A small drag returns to the settled state's anchor (below the
    // midpoint).
    sheet.expandSheet();
    QTest::qWait(900);
    QCOMPARE(sheet.state(), MdSideSheetState::Expanded);
    dragSequence(&sheet, QPointF(30.0, 100.0), QPointF(30.0 - 20.0, 100.0));
    QTest::qWait(900);
    QCOMPARE(sheet.state(), MdSideSheetState::Expanded);
}

void TestMd3SideSheet::managedGeometryFollowsParent()
{
    QWidget parent;
    parent.resize(600, 400);
    MdSideSheet sheet(MdSideSheetKind::Modal, MdSideSheetEdge::Right, &parent);
    sheet.setGeometryManaged(true);
    sheet.resize(260, 400);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    sheet.expandSheet();
    QTest::qWait(900);
    QCOMPARE(sheet.x(), 340);

    // The anchor math re-runs against the parent's new width.
    parent.resize(700, 400);
    QTest::qWait(50);
    QCOMPARE(sheet.x(), 440);
    QCOMPARE(sheet.y(), 0);
    QCOMPARE(sheet.height(), 400);
}

void TestMd3SideSheet::renderSmoke()
{
    QWidget parent;
    parent.resize(600, 400);
    MdSideSheet standard(MdSideSheetKind::Standard, MdSideSheetEdge::Right, &parent);
    standard.setGeometryManaged(true);
    standard.setDividerVisible(true);
    standard.resize(260, 400);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    const QImage image = standard.grab().toImage();
    QVERIFY(!image.isNull());
    QVERIFY(image.width() > 0);
    QVERIFY(MdSideSheetStyle::isInstalled());
}

// --- host ------------------------------------------------------------------------

void TestMd3SideSheet::hostShowAndEscape()
{
    QWidget parent;
    parent.resize(600, 400);
    MdSideSheetHost host(&parent);
    host.resize(600, 400);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    int dismissals = 0;
    connect(&host, &MdSideSheetHost::dismissed, [&] { ++dismissals; });

    host.show();
    QVERIFY(host.isShowing());
    QVERIFY(host.sheet()->isVisible());
    QTest::qWait(900);
    // Fully entered: the scrim at full strength, the sheet docked
    // full-height at the token's width.
    QCOMPARE(host.currentScrimAlpha(), 1.0);
    QCOMPARE(host.sheet()->width(), 260);
    QCOMPARE(host.sheet()->height(), 400);
    QCOMPARE(host.sheet()->x(), 340);
    QCOMPARE(host.sheet()->y(), 0);

    // Escape cancels the dialog: the sheet hides, the scrim fades out, the
    // idle host hides.
    QTest::keyClick(host.sheet(), Qt::Key_Escape);
    QTest::qWait(900);
    QCOMPARE(dismissals, 1);
    QVERIFY(!host.isShowing());
    QCOMPARE(host.currentScrimAlpha(), 0.0);
    QVERIFY(!host.isVisible());
}

void TestMd3SideSheet::hostScrimClick()
{
    QWidget parent;
    parent.resize(600, 400);
    MdSideSheetHost host(&parent);
    host.resize(600, 400);
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    int dismissals = 0;
    connect(&host, &MdSideSheetHost::dismissed, [&] { ++dismissals; });

    host.show();
    QTest::qWait(900);
    QVERIFY(host.sheet()->isVisible());

    // A click on the scrim (outside the sheet) cancels too.
    QTest::mousePress(&host, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
    QTest::qWait(900);
    QCOMPARE(dismissals, 1);
    QVERIFY(!host.isShowing());
}

} // namespace
} // namespace md

QTEST_MAIN(md::TestMd3SideSheet)
#include "TestMd3SideSheet.moc"
