#include "TestMd3Common.h"

#include "core/MdSnackbarTokens.h"
#include "core/MdTypeScale.h"
#include "styles/MdSnackbarStyle.h"
#include "widgets/MdSnackbar.h"
#include "widgets/MdSnackbarHost.h"

#include <QtGui/QFontMetricsF>

#include <utility>
#include <vector>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtTest/QtTest>
#include <QtWidgets/QWidget>

using namespace md;

namespace {

constexpr qreal kEpsilon = 1e-6;

bool closeTo(qreal a, qreal b, qreal epsilon = kEpsilon)
{
    return qAbs(a - b) <= epsilon;
}

QFont supportingFont(const MdSnackbarTokens &tokens)
{
    return MdTypeScale::font(tokens.supportingTextStyle);
}

QFont actionFont(const MdSnackbarTokens &tokens)
{
    return MdTypeScale::font(tokens.actionLabelStyle);
}

} // namespace

class TestMd3Snackbar : public QObject
{
    Q_OBJECT

private slots:
    void tokenDefaults();
    void tokenOverrides();
    void elementStateRows();
    void hostDurations();
    void oneRowSingleLineGeometry();
    void oneRowInteractiveGeometry();
    void oneRowWrappedGeometry();
    void newLineGeometry();
    void widgetSignals();
    void widgetKeyboard();
    void widgetRenderSmoke();
    void hostQueueAndTransition();
};

void TestMd3Snackbar::tokenDefaults()
{
    const MdSnackbarTokens tokens = MdSnackbarTokens::resolve();
    QCOMPARE(int(tokens.containerColor), int(ColorRole::InverseSurface));
    QCOMPARE(int(tokens.containerElevation), int(ElevationLevel::Level3));
    QCOMPARE(int(tokens.containerShadowColor), int(ColorRole::Shadow));
    QCOMPARE(int(tokens.containerShape), int(ShapeCorner::ExtraSmall));
    QCOMPARE(tokens.singleLineHeight, 48.0);
    QCOMPARE(tokens.twoLinesHeight, 68.0);
    QCOMPARE(int(tokens.supportingTextColor), int(ColorRole::InverseOnSurface));
    QCOMPARE(int(tokens.supportingTextStyle), int(TypeStyle::BodyMedium));
    QCOMPARE(tokens.iconSize, 24.0);
    QCOMPARE(int(tokens.iconColor), int(ColorRole::InverseOnSurface));
    QCOMPARE(int(tokens.actionLabelColor), int(ColorRole::InversePrimary));
    QCOMPARE(int(tokens.actionLabelStyle), int(TypeStyle::LabelLarge));

    // The Compose-port layout constants.
    QCOMPARE(MdSnackbarTokens::kContainerMaxWidth, 600.0);
    QCOMPARE(MdSnackbarTokens::kHorizontalSpacing, 16.0);
    QCOMPARE(MdSnackbarTokens::kTextEndExtraSpacing, 8.0);
    QCOMPARE(MdSnackbarTokens::kTextVerticalPadding, 14.0);
    QCOMPARE(MdSnackbarTokens::kHeightToFirstLine, 30.0);
    QCOMPARE(MdSnackbarTokens::kActionButtonBottomPadding, 4.0);
    QCOMPARE(MdSnackbarTokens::kHorizontalSpacingButtonSide, 8.0);
}

void TestMd3Snackbar::tokenOverrides()
{
    MdComponentTokens overrides;
    overrides.setValue(QStringLiteral("md.comp.snackbar.with-single-line.container.height"),
                       QStringLiteral("56px"));
    overrides.setValue(QStringLiteral("md.comp.snackbar.icon.size"), QStringLiteral("20px"));
    overrides.setValue(QStringLiteral("md.comp.snackbar.container.corner-size"),
                       QStringLiteral("medium"));

    const MdSnackbarTokens tokens = MdSnackbarTokens::resolve(&overrides);
    QCOMPARE(tokens.singleLineHeight, 56.0);
    QCOMPARE(tokens.iconSize, 20.0);
    QCOMPARE(int(tokens.containerShape), int(ShapeCorner::Medium)); // the 8 px corner
    // Unpublished rows keep their values.
    QCOMPARE(tokens.twoLinesHeight, 68.0);
}

void TestMd3Snackbar::elementStateRows()
{
    const MdSnackbarTokens tokens = MdSnackbarTokens::resolve();

    // The action keeps inverse-primary through every state; the icon keeps
    // inverse-on-surface; the state layers swap in at the md.sys.state
    // opacities (hover 0.08, focus/pressed 0.12).
    const std::vector<std::pair<MdSnackbarState, qreal>> expected = {
        { MdSnackbarState::Enabled, 0.0 },
        { MdSnackbarState::Hovered, 0.08 },
        { MdSnackbarState::Focused, 0.12 },
        { MdSnackbarState::Pressed, 0.12 },
    };
    for (const auto &entry : expected) {
        const MdSnackbarElementRow action = tokens.actionRow(entry.first);
        QCOMPARE(int(action.content), int(ColorRole::InversePrimary));
        QCOMPARE(int(action.stateLayer), int(ColorRole::InversePrimary));
        QVERIFY(closeTo(action.stateLayerOpacity, entry.second));

        const MdSnackbarElementRow icon = tokens.iconRow(entry.first);
        QCOMPARE(int(icon.content), int(ColorRole::InverseOnSurface));
        QCOMPARE(int(icon.stateLayer), int(ColorRole::InverseOnSurface));
        QVERIFY(closeTo(icon.stateLayerOpacity, entry.second));
    }
}

void TestMd3Snackbar::hostDurations()
{
    // SnackbarHost.kt toMillis.
    QCOMPARE(MdSnackbarTokens::durationMs(MdSnackbarDuration::Short, false), qint64(4000));
    QCOMPARE(MdSnackbarTokens::durationMs(MdSnackbarDuration::Short, true), qint64(4000));
    QCOMPARE(MdSnackbarTokens::durationMs(MdSnackbarDuration::Long, false), qint64(10000));
    QCOMPARE(MdSnackbarTokens::durationMs(MdSnackbarDuration::Indefinite, false), qint64(-1));
    // Auto: an action pins Indefinite (an actionable snackbar must not
    // self-dismiss), otherwise Short.
    QCOMPARE(MdSnackbarTokens::durationMs(MdSnackbarDuration::Auto, false), qint64(4000));
    QCOMPARE(MdSnackbarTokens::durationMs(MdSnackbarDuration::Auto, true), qint64(-1));
}

void TestMd3Snackbar::oneRowSingleLineGeometry()
{
    const MdSnackbarTokens tokens = MdSnackbarTokens::resolve();
    const QRectF bounds(0.0, 0.0, 300.0, 48.0);
    const MdSnackbarStyle::OneRowLayout layout = MdSnackbarStyle::oneRowLayout(
        bounds, QStringLiteral("Saved"), QString(), false, supportingFont(tokens),
        actionFont(tokens), tokens.iconSize, tokens);

    // A plain snackbar: height 48, the text inset 16, the extra 8 end spacing
    // carved out of its width budget.
    QCOMPARE(layout.oneLine, true);
    QCOMPARE(layout.containerHeight, 48.0);
    QVERIFY(closeTo(layout.textRect.left(), 16.0));
    QVERIFY(layout.textRect.width() <= 300.0 - 16.0 - 8.0);

    // No interactive elements, no rects.
    QVERIFY(!layout.actionRect.isValid());
    QVERIFY(!layout.dismissRect.isValid());
}

void TestMd3Snackbar::oneRowInteractiveGeometry()
{
    const MdSnackbarTokens tokens = MdSnackbarTokens::resolve();
    const QRectF bounds(0.0, 0.0, 360.0, 48.0);
    const MdSnackbarStyle::OneRowLayout layout = MdSnackbarStyle::oneRowLayout(
        bounds, QStringLiteral("Archived"), QStringLiteral("Undo"), true,
        supportingFont(tokens), actionFont(tokens), tokens.iconSize, tokens);

    // The dismiss chrome (40 px) sits flush right; the action label left of
    // it; the text keeps its 16 px start inset.
    QVERIFY(layout.dismissRect.isValid());
    QVERIFY(closeTo(layout.dismissHitRect.right(), 360.0));
    QCOMPARE(layout.dismissHitRect.width(), 40.0);
    QVERIFY(layout.actionRect.isValid());
    QVERIFY(layout.actionRect.right() <= layout.dismissHitRect.left());
    QVERIFY(layout.actionHitRect.contains(layout.actionRect));
    QVERIFY(closeTo(layout.textRect.left(), 16.0));

    // Everything is vertically centred in the 48 px container.
    QVERIFY(closeTo(layout.actionRect.center().y(), 24.0, 0.5));
    QVERIFY(closeTo(layout.dismissRect.center().y(), 24.0, 0.5));
}

void TestMd3Snackbar::oneRowWrappedGeometry()
{
    const MdSnackbarTokens tokens = MdSnackbarTokens::resolve();
    // A message too long for a narrow container wraps; the first line sits
    // 30 px from the top and the container grows to the 68 px minimum.
    QString longMessage;
    for (int i = 0; i < 12; ++i) {
        longMessage += QStringLiteral("word ");
    }
    const QRectF bounds(0.0, 0.0, 180.0, 68.0);
    const MdSnackbarStyle::OneRowLayout layout = MdSnackbarStyle::oneRowLayout(
        bounds, longMessage, QString(), false, supportingFont(tokens), actionFont(tokens),
        tokens.iconSize, tokens);

    QCOMPARE(layout.oneLine, false);
    QVERIFY(layout.containerHeight >= 68.0);
    const QFontMetricsF metrics(supportingFont(tokens));
    QVERIFY(closeTo(layout.textRect.top(), 30.0 - metrics.ascent(), 0.5));
}

void TestMd3Snackbar::newLineGeometry()
{
    const MdSnackbarTokens tokens = MdSnackbarTokens::resolve();
    const QRectF bounds(0.0, 0.0, 360.0, 200.0);
    const MdSnackbarStyle::NewLineLayout layout = MdSnackbarStyle::newLineLayout(
        bounds, QStringLiteral("File moved to the archive folder"),
        QStringLiteral("Move to inbox"), false, supportingFont(tokens), actionFont(tokens),
        tokens.iconSize, tokens);

    // The text box: start 16, top 14, end 16.
    QVERIFY(closeTo(layout.textRect.left(), 16.0));
    QVERIFY(closeTo(layout.textRect.top(), 14.0));
    QVERIFY(layout.textRect.right() <= 360.0 - 16.0);

    // The action row sits bottom-right without a dismiss icon, 4 px above the
    // container's bottom and 8 px in from the right edge.
    QVERIFY(layout.actionRect.isValid());
    QVERIFY(layout.actionHitRect.bottom() <= 200.0 - 4.0 + 0.5);
    QVERIFY(layout.actionHitRect.right() <= 360.0 - 8.0 + 0.5);

    // Natural height: 14 + text + 4 + row.
    const QFontMetricsF actionMetrics(actionFont(tokens));
    const qreal rowHeight = 24.0 + actionMetrics.height();
    QCOMPARE(layout.containerHeight, 14.0 + layout.textRect.height() + 4.0 + rowHeight);
}

void TestMd3Snackbar::widgetSignals()
{
    MdSnackbar snackbar(QStringLiteral("Photo deleted"));
    snackbar.setActionLabel(QStringLiteral("Undo"));
    snackbar.setHasDismissAction(true);
    snackbar.resize(snackbar.sizeHint());
    snackbar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&snackbar));

    int actionClicks = 0;
    int dismissClicks = 0;
    connect(&snackbar, &MdSnackbar::actionClicked, [&] { ++actionClicks; });
    connect(&snackbar, &MdSnackbar::dismissClicked, [&] { ++dismissClicks; });

    // A click inside the action's region (offset from the label centre by
    // less than the 12 px hit padding) activates the action.
    const QPointF actionCentre = snackbar.actionRect().center();
    QTest::mouseClick(&snackbar, Qt::LeftButton, Qt::NoModifier,
                      actionCentre.toPoint() + QPoint(10, 0));
    QCOMPARE(actionClicks, 1);
    QCOMPARE(dismissClicks, 0);

    const QPointF dismissCentre = snackbar.dismissRect().center();
    QTest::mouseClick(&snackbar, Qt::LeftButton, Qt::NoModifier, dismissCentre.toPoint());
    QCOMPARE(dismissClicks, 1);
    QCOMPARE(actionClicks, 1);

    // A click on the message activates nothing.
    QTest::mouseClick(&snackbar, Qt::LeftButton, Qt::NoModifier,
                      snackbar.containerRect().bottomLeft().toPoint() + QPoint(30, -4));
    QCOMPARE(actionClicks, 1);
    QCOMPARE(dismissClicks, 1);
}

void TestMd3Snackbar::widgetKeyboard()
{
    MdSnackbar snackbar(QStringLiteral("Photo deleted"));
    snackbar.setActionLabel(QStringLiteral("Undo"));
    snackbar.setHasDismissAction(true);
    snackbar.resize(snackbar.sizeHint());
    snackbar.show();
    QVERIFY(QTest::qWaitForWindowExposed(&snackbar));

    int actionClicks = 0;
    int dismissClicks = 0;
    connect(&snackbar, &MdSnackbar::actionClicked, [&] { ++actionClicks; });
    connect(&snackbar, &MdSnackbar::dismissClicked, [&] { ++dismissClicks; });

    snackbar.setFocus(Qt::TabFocusReason);
    QVERIFY(snackbar.actionHasKeyboardFocus());
    QVERIFY(!snackbar.dismissHasKeyboardFocus());

    QTest::keyClick(&snackbar, Qt::Key_Tab);
    QVERIFY(snackbar.dismissHasKeyboardFocus());
    QVERIFY(!snackbar.actionHasKeyboardFocus());

    QTest::keyClick(&snackbar, Qt::Key_Enter);
    QCOMPARE(dismissClicks, 1);
    QCOMPARE(actionClicks, 0);

    QTest::keyClick(&snackbar, Qt::Key_Left); // back to the action
    QVERIFY(snackbar.actionHasKeyboardFocus());
    QTest::keyClick(&snackbar, Qt::Key_Space);
    QCOMPARE(actionClicks, 1);

    // The action region's painted state follows the keyboard focus.
    QCOMPARE(int(snackbar.actionState()), int(MdSnackbarState::Focused));
}

void TestMd3Snackbar::widgetRenderSmoke()
{
    MdSnackbar snackbar(QStringLiteral("Photo deleted"));
    snackbar.setActionLabel(QStringLiteral("Undo"));
    snackbar.setHasDismissAction(true);
    snackbar.resize(snackbar.sizeHint());

    QImage image(snackbar.size(), QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    snackbar.render(&painter);
    painter.end();

    // The inverse-surface container painted — something must differ from the
    // white background (the shadow margin stays white).
    const int painted = mdtest::paintedPixelCount(image, 0xFFFFFFFF);
    QVERIFY(painted > snackbar.size().width() * snackbar.size().height() / 4);
}

void TestMd3Snackbar::hostQueueAndTransition()
{
    QWidget canvas;
    canvas.resize(480, 320);
    MdSnackbarHost host(&canvas);
    host.setGeometry(canvas.rect());
    canvas.show();
    QVERIFY(QTest::qWaitForWindowExposed(&canvas));

    int actionClicks = 0;
    int dismissals = 0;
    int cleared = 0;
    connect(&host, &MdSnackbarHost::actionClicked, [&] { ++actionClicks; });
    connect(&host, &MdSnackbarHost::dismissed, [&] { ++dismissals; });
    connect(&host, &MdSnackbarHost::allCleared, [&] { ++cleared; });

    host.showSnackbar(QStringLiteral("First"), QStringLiteral("Undo"));
    QVERIFY(host.isShowing());
    QVERIFY(host.current()->hasAction());
    QCOMPARE(host.queueLength(), 0);

    // A second request queues behind the current one; a duplicate of the
    // current one is dropped.
    host.showSnackbar(QStringLiteral("Second"));
    QCOMPARE(host.queueLength(), 1);
    host.showSnackbar(QStringLiteral("First"), QStringLiteral("Undo"));
    QCOMPARE(host.queueLength(), 1);

    // The enter transition settles at full opacity.
    QTest::qWait(700);
    QVERIFY(closeTo(host.currentOpacity(), 1.0, 0.01));

    // Dismissing hands the queue over to the next request.
    host.dismissCurrent();
    QCOMPARE(dismissals, 1);
    QTest::qWait(1400);
    QVERIFY(host.isShowing());
    QCOMPARE(host.current()->message(), QStringLiteral("Second"));
    QCOMPARE(host.queueLength(), 0);

    // The second snackbar has no action: it self-dismisses after 4 s (the
    // Short duration). Give the fade-out a wide window.
    QTest::qWait(5200);
    QVERIFY(!host.isShowing());
    QCOMPARE(cleared, 1);

    // The action signal forwards at click time, before the fade.
    host.showSnackbar(QStringLiteral("Third"), QStringLiteral("Retry"));
    QTest::qWait(700);
    QVERIFY(host.current());
    QTest::mouseClick(host.current(), Qt::LeftButton, Qt::NoModifier,
                      host.current()->actionRect().center().toPoint());
    QCOMPARE(actionClicks, 1);
    QTest::qWait(1400);
    QVERIFY(!host.isShowing());
}

QTEST_MAIN(TestMd3Snackbar)
#include "TestMd3Snackbar.moc"
