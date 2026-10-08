// TestMd3Tooltip — the Communication family's closing pair.
//
// Pins the two exports (plain: inverse-surface / extra-small / body-small
// inverse-on-surface, NO elevation; rich: surface-container / medium /
// level-2 shadow / title-small + body-medium on-surface-variant / label-large
// primary action), the action's three state rows, the override parsing, the
// Compose-port layout geometry (padding, min/max width, baseline rows, the
// 36/8 action box, the plain-4 px fallback), the popup positioning providers,
// the caret geometry, the pointer + keyboard contract of the rich action and
// the host's trigger / mutex / auto-hide rules.

#include "core/MdTheme.h"
#include "core/MdTooltipTokens.h"
#include "core/MdTypeScale.h"
#include "styles/MdTooltipStyle.h"
#include "widgets/MdTooltip.h"
#include "widgets/MdTooltipHost.h"

#include <QtGui/QFontMetricsF>
#include <QtTest/QTest>
#include <QtWidgets/QPushButton>

namespace md {
namespace {

constexpr qreal kEps = 0.01;

bool closeTo(qreal a, qreal b, qreal eps = kEps)
{
    return qAbs(a - b) <= eps;
}

class TestMd3Tooltip : public QObject
{
    Q_OBJECT

private slots:
    void plainTokenRows();
    void richTokenRows();
    void actionStateRows();
    void overrideParsing();
    void plainLayoutGeometry();
    void richLayoutGeometry();
    void richBaselineRows();
    void plainFallbackPadding();
    void positioningProviders();
    void caretGeometry();
    void widgetSizeHints();
    void richActionPointerContract();
    void richActionKeyboardContract();
    void hostHoverTrigger();
    void hostAutoHideRules();
    void hostGlobalMutex();
    void hostEscapeDismiss();
    void hostTransitionSprings();
    void styleIsInstalled();

private:
    static void flushEvents()
    {
        QTest::qWait(1);
    }
};

void TestMd3Tooltip::plainTokenRows()
{
    const MdTooltipTokens tokens = MdTooltipTokens::resolve(MdTooltipVariant::Plain);
    QCOMPARE(tokens.plainContainerColor, ColorRole::InverseSurface);
    QCOMPARE(tokens.plainContainerShape, ShapeCorner::ExtraSmall);
    QCOMPARE(tokens.plainTextStyle, TypeStyle::BodySmall);
    QCOMPARE(tokens.plainTextColor, ColorRole::InverseOnSurface);
}

void TestMd3Tooltip::richTokenRows()
{
    const MdTooltipTokens tokens = MdTooltipTokens::resolve(MdTooltipVariant::Rich);
    QCOMPARE(tokens.richContainerColor, ColorRole::SurfaceContainer);
    QCOMPARE(tokens.richContainerElevation, ElevationLevel::Level2);
    QCOMPARE(tokens.richContainerShadowColor, ColorRole::Shadow);
    QCOMPARE(tokens.richContainerShape, ShapeCorner::Medium);
    QCOMPARE(tokens.richSubheadStyle, TypeStyle::TitleSmall);
    QCOMPARE(tokens.richSubheadColor, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.richTextStyle, TypeStyle::BodyMedium);
    QCOMPARE(tokens.richTextColor, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.richActionLabelStyle, TypeStyle::LabelLarge);
    QCOMPARE(tokens.richActionLabelColor, ColorRole::Primary);
}

void TestMd3Tooltip::actionStateRows()
{
    // md.comp.rich-tooltip.action.<state>.* — the label stays primary; the
    // state layer is primary at the md.sys.state opacities.
    const MdTooltipTokens tokens;
    const MdTooltipActionRow enabled = tokens.actionRow(MdTooltipActionState::Enabled);
    QCOMPARE(enabled.content, ColorRole::Primary);
    QCOMPARE(enabled.stateLayer, ColorRole::Primary);
    QCOMPARE(enabled.stateLayerOpacity, 0.0);

    const MdTooltipActionRow hovered = tokens.actionRow(MdTooltipActionState::Hovered);
    QCOMPARE(hovered.stateLayerOpacity, 0.08);
    const MdTooltipActionRow focused = tokens.actionRow(MdTooltipActionState::Focused);
    QCOMPARE(focused.stateLayerOpacity, 0.12);
    QCOMPARE(focused.content, ColorRole::Primary);
    const MdTooltipActionRow pressed = tokens.actionRow(MdTooltipActionState::Pressed);
    QCOMPARE(pressed.stateLayerOpacity, 0.12);
}

void TestMd3Tooltip::overrideParsing()
{
    MdComponentTokens bag;
    // Corner-size overrides resolve the shape NAME (the snackbar's rule).
    bag.setValue(QStringLiteral("md.comp.plain-tooltip.container.corner-size"),
                 QStringLiteral("corner-medium"));
    const MdTooltipTokens overridden =
        MdTooltipTokens::resolve(MdTooltipVariant::Plain, &bag);
    QCOMPARE(overridden.plainContainerShape, ShapeCorner::Medium);

    // An unparseable shape name falls back to the published value.
    bag.setValue(QStringLiteral("md.comp.plain-tooltip.container.shape"),
                 QStringLiteral("nonsense"));
    const MdTooltipTokens fallback =
        MdTooltipTokens::resolve(MdTooltipVariant::Plain, &bag);
    QCOMPARE(fallback.plainContainerShape, ShapeCorner::ExtraSmall);

    MdComponentTokens richBag;
    richBag.setValue(QStringLiteral("md.comp.rich-tooltip.container.shape"),
                     QStringLiteral("small"));
    const MdTooltipTokens richOverridden =
        MdTooltipTokens::resolve(MdTooltipVariant::Rich, &richBag);
    QCOMPARE(richOverridden.richContainerShape, ShapeCorner::Small);
}

void TestMd3Tooltip::plainLayoutGeometry()
{
    const MdTooltipTokens tokens = MdTooltipTokens::resolve(MdTooltipVariant::Plain);
    const QFont font = MdTypeScale::font(tokens.plainTextStyle);
    const QFontMetricsF metrics(font);

    // A short text: the container clamps to the 40 px minimum and the 24 px
    // minimum height; the text sits inside the 8/4 padding.
    const QRectF small(0.0, 0.0, 40.0, 24.0);
    const MdTooltipStyle::PlainLayout smallLayout =
        MdTooltipStyle::plainLayout(small, QStringLiteral("Hi"), font, tokens);
    QVERIFY(closeTo(smallLayout.textRect.left(), 8.0));
    QVERIFY(closeTo(smallLayout.textRect.top(), 4.0));
    QVERIFY(closeTo(smallLayout.textRect.width(), metrics.horizontalAdvance(QStringLiteral("Hi"))));

    // The plain container height for the minimum width is the 24 px minimum.
    QVERIFY(closeTo(MdTooltipStyle::plainHeightForWidth(40.0, QStringLiteral("Hi"), tokens),
                    24.0));

    // A long text wraps at the width minus the two 8 px paddings and grows
    // the height past the minimum.
    const QString longText = QStringLiteral("this line is definitely long enough to wrap at 120 px");
    const qreal wrappedHeight = MdTooltipStyle::plainHeightForWidth(120.0, longText, tokens);
    QVERIFY(wrappedHeight > 24.0 + kEps);

    // Width clamping: natural width clamped into [40, 200].
    QVERIFY(closeTo(MdTooltipStyle::plainWidthFor(QStringLiteral("x"), tokens), 40.0));
    QVERIFY(closeTo(MdTooltipStyle::plainWidthFor(
                        QStringLiteral("a very long tooltip message that exceeds two hundred px"),
                        tokens),
                    200.0));
}

void TestMd3Tooltip::richLayoutGeometry()
{
    const MdTooltipTokens tokens = MdTooltipTokens::resolve(MdTooltipVariant::Rich);
    const QFont subhead = MdTypeScale::font(tokens.richSubheadStyle);
    const QFont text = MdTypeScale::font(tokens.richTextStyle);
    const QFont action = MdTypeScale::font(tokens.richActionLabelStyle);

    // A text-only rich tooltip: the plain 4 px vertical padding applies.
    const QRectF bounds(0.0, 0.0, 320.0, 0.0);
    const MdTooltipStyle::RichLayout textOnly =
        MdTooltipStyle::richLayout(bounds, QString(), QStringLiteral("Body text"), QString(),
                                   subhead, text, action, tokens);
    QVERIFY(!textOnly.subheadRect.isValid());
    QVERIFY(!textOnly.actionRect.isValid());
    QVERIFY(closeTo(textOnly.textRect.top(), 4.0));
    QVERIFY(closeTo(textOnly.containerHeight, 4.0 + textOnly.textRect.height() + 4.0));

    // With a subhead and an action: 28 px to the subhead baseline, the text
    // 24 px below the subhead box + 16 bottom, and a 36/8 action box.
    const MdTooltipStyle::RichLayout full =
        MdTooltipStyle::richLayout(bounds, QStringLiteral("Title"), QStringLiteral("Body text"),
                                   QStringLiteral("Action"), subhead, text, action, tokens);
    QVERIFY(full.subheadRect.isValid());
    QVERIFY(full.actionRect.isValid());
    QVERIFY(full.actionHitRect.isValid());

    const QFontMetricsF subheadMetrics(subhead);
    QVERIFY(closeTo(full.subheadRect.top(), 28.0 - subheadMetrics.ascent()));
    QVERIFY(closeTo(full.subheadRect.left(), 16.0));

    // The text's first baseline sits 24 px below the subhead box.
    const QFontMetricsF textMetrics(text);
    const qreal subheadBottom = full.subheadRect.top() + full.subheadRect.height();
    QVERIFY(closeTo(full.textRect.top() + textMetrics.ascent(), subheadBottom + 24.0, kEps * 4));

    // The action box: min height 36 with a bottom padding of 8; the box top
    // follows the text's 16 px bottom inset.
    const QFontMetricsF boxMetrics(action);
    const qreal boxHeight = qMax<qreal>(36.0, boxMetrics.height() + 24.0);
    const qreal boxTop = full.textRect.top() + full.textRect.height() + 16.0;
    QVERIFY(closeTo(full.containerHeight, boxTop + boxHeight + 8.0, kEps * 4));
    QVERIFY(full.actionHitRect.height() >= 36.0 - kEps);

    // The hit region is the label grown by 12 px each side, label centred.
    const QFontMetricsF actionMetrics(action);
    const qreal labelWidth = actionMetrics.horizontalAdvance(QStringLiteral("Action"));
    QVERIFY(closeTo(full.actionRect.width(), labelWidth));
    QVERIFY(closeTo(full.actionHitRect.width(), labelWidth + 24.0));
    QVERIFY(closeTo(full.actionRect.left(), full.actionHitRect.left() + 12.0));
    QVERIFY(closeTo(full.actionRect.center().y(), full.actionHitRect.center().y()));

    // Width clamping: a text-only rich tooltip may already exceed the 40 px
    // minimum (the 16 px paddings); nothing may exceed the 320 px maximum.
    QVERIFY(MdTooltipStyle::richWidthFor(QString(), QStringLiteral("x"), QString(), tokens)
        >= 40.0 - kEps);
    QVERIFY(MdTooltipStyle::richWidthFor(QString(), QStringLiteral("x"), QStringLiteral("Action"),
                                         tokens)
        >= 40.0 + kEps);
}

void TestMd3Tooltip::richBaselineRows()
{
    // The rich height for a subhead + text + action layout is at least the
    // 24 px minimum and grows with the content.
    const MdTooltipTokens tokens = MdTooltipTokens::resolve(MdTooltipVariant::Rich);
    const qreal height = MdTooltipStyle::richHeightForWidth(
        320.0, QStringLiteral("Title"), QStringLiteral("Body"), QStringLiteral("Action"), tokens);
    QVERIFY(height > 24.0 + kEps);
}

void TestMd3Tooltip::plainFallbackPadding()
{
    // A rich tooltip with an action but no subhead: the text takes the 24 px
    // baseline row (not the 4 px padding), then the 16 px bottom padding.
    const MdTooltipTokens tokens = MdTooltipTokens::resolve(MdTooltipVariant::Rich);
    const QFont subhead = MdTypeScale::font(tokens.richSubheadStyle);
    const QFont text = MdTypeScale::font(tokens.richTextStyle);
    const QFont action = MdTypeScale::font(tokens.richActionLabelStyle);

    const QRectF bounds(0.0, 0.0, 320.0, 0.0);
    const MdTooltipStyle::RichLayout layout =
        MdTooltipStyle::richLayout(bounds, QString(), QStringLiteral("Body"),
                                   QStringLiteral("Action"), subhead, text, action, tokens);
    QVERIFY(!layout.subheadRect.isValid());
    QVERIFY(layout.actionRect.isValid());
    const QFontMetricsF textMetrics(text);
    QVERIFY(closeTo(layout.textRect.top() + textMetrics.ascent(), 24.0, kEps * 4));
}

void TestMd3Tooltip::positioningProviders()
{
    // Above provider: centred above the anchor, 4 px spacing.
    const QRect anchor(100, 200, 80, 40);
    const QSize size(60, 30);
    const QPoint centred =
        MdTooltipStyle::abovePopupPosition(anchor, size, QSize(1000, 800), 4.0);
    QCOMPARE(centred, QPoint(110, 166)); // 100 + (80-60)/2, 200 - 30 - 4

    // Centring falls inside the window (x = 20): the provider merely coerces
    // x into the window — no start fallback for a centred value that fits.
    const QPoint centredFits =
        MdTooltipStyle::abovePopupPosition(QRect(10, 200, 80, 40), size, QSize(1000, 800), 4.0);
    QCOMPARE(centredFits.x(), 20);

    // Centring would clip the right edge: coerced to the window edge.
    const QPoint endAligned =
        MdTooltipStyle::abovePopupPosition(QRect(950, 200, 80, 40), size, QSize(1000, 800), 4.0);
    QCOMPARE(endAligned.x(), 940);

    // No room above: below the anchor (anchor bottom 59 + 1 + spacing 4).
    const QPoint below =
        MdTooltipStyle::abovePopupPosition(QRect(100, 20, 80, 40), size, QSize(1000, 800), 4.0);
    QCOMPARE(below.y(), 64);

    // Always coerced into the window: x clamps to 0, y clamps to the window
    // height minus the tooltip height (10).
    const QPoint coerced =
        MdTooltipStyle::abovePopupPosition(QRect(0, 0, 40, 20), QSize(60, 30), QSize(50, 40), 4.0);
    QCOMPARE(coerced, QPoint(0, 10));

    // Rich provider: start-aligned, shifted left when the right edge clips,
    // centred when both would clip.
    const QPoint richStart =
        MdTooltipStyle::richPopupPosition(anchor, size, QSize(1000, 800), 4.0);
    QCOMPARE(richStart.x(), 100);
    const QPoint richShifted =
        MdTooltipStyle::richPopupPosition(QRect(950, 200, 80, 40), size, QSize(1000, 800), 4.0);
    QCOMPARE(richShifted.x(), 970); // 950 + 80 - 1 + 1 - 60
    // Rich provider centring: both a left shift and the anchor are too wide
    // for the window — the tooltip centres on the anchor instead.
    const QPoint richCentred =
        MdTooltipStyle::richPopupPosition(QRect(100, 200, 80, 40), QSize(500, 30), QSize(500, 800),
                                          4.0);
    QCOMPARE(richCentred.x(), 100 + (80 - 500) / 2);
}

void TestMd3Tooltip::caretGeometry()
{
    MdTooltip tooltip;
    tooltip.resize(100, 40);
    tooltip.setCaretSide(MdTooltipCaretSide::Bottom);
    const QRectF container = tooltip.containerRect();
    // A plain tooltip has no shadow margin, but the caret protrusion is
    // widget headroom: the container is the widget rect minus the caret
    // margin on its side.
    QVERIFY(closeTo(container.width(), 100.0));
    QVERIFY(closeTo(container.height(), 32.0));

    const QRectF caret = tooltip.caretRect();
    QVERIFY(caret.isValid());
    QVERIFY(closeTo(caret.width(), 16.0));
    QVERIFY(closeTo(caret.height(), 8.0));
    QVERIFY(closeTo(caret.center().x(), container.center().x()));
    QVERIFY(closeTo(caret.top(), container.bottom()));

    tooltip.setCaretSide(MdTooltipCaretSide::Top);
    const QRectF topCaret = tooltip.caretRect();
    QVERIFY(closeTo(topCaret.bottom(), tooltip.containerRect().top()));

    tooltip.setCaretSide(MdTooltipCaretSide::None);
    QVERIFY(!tooltip.caretRect().isValid());
}

void TestMd3Tooltip::widgetSizeHints()
{
    MdTooltip plain(QStringLiteral("Short"));
    plain.resize(plain.sizeHint());
    QVERIFY(plain.sizeHint().width() >= 40);
    QVERIFY(plain.sizeHint().height() >= 24);

    // A rich tooltip's widget rect carries the 8 px shadow margins.
    MdTooltip rich(QStringLiteral("Body"));
    rich.setVariant(MdTooltipVariant::Rich);
    rich.setTitle(QStringLiteral("Title"));
    const QSize richHint = rich.sizeHint();
    QVERIFY(richHint.width() >= 40 + 16);
    QVERIFY(richHint.height() >= 24 + 16);
    const QRectF richContainer = rich.containerRect();
    QVERIFY(closeTo(richContainer.left(), 8.0));
    QVERIFY(closeTo(richContainer.top(), 8.0));

    // A plain tooltip is exactly its container.
    const QRectF plainContainer = plain.containerRect();
    QVERIFY(closeTo(plainContainer.width(), qreal(plain.width())));
    QVERIFY(closeTo(plainContainer.height(), qreal(plain.height())));
}

void TestMd3Tooltip::richActionPointerContract()
{
    MdTooltip rich(QStringLiteral("Body text"));
    rich.setVariant(MdTooltipVariant::Rich);
    rich.setActionLabel(QStringLiteral("Action"));
    rich.resize(rich.sizeHint());

    QCOMPARE(rich.actionState(), MdTooltipActionState::Enabled);

    // Hover on the action region paints the hovered row.
    const QPointF actionCenter = rich.actionHitRect().center();
    QMouseEvent move(QEvent::MouseMove, actionCenter, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&rich, &move);
    QCOMPARE(rich.actionState(), MdTooltipActionState::Hovered);

    // Press inside starts the ripple and the pressed row.
    QTest::mousePress(&rich, Qt::LeftButton, Qt::NoModifier, actionCenter.toPoint());
    QCOMPARE(rich.actionState(), MdTooltipActionState::Pressed);
    QVERIFY(rich.actionRippleFrame().valid);

    bool clicked = false;
    connect(&rich, &MdTooltip::actionClicked, this, [&clicked] { clicked = true; });
    QTest::mouseRelease(&rich, Qt::LeftButton, Qt::NoModifier, actionCenter.toPoint());
    QVERIFY(clicked);
    QCOMPARE(rich.actionState(), MdTooltipActionState::Hovered);

    // Press outside the action is inert.
    QTest::mousePress(&rich, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
    QCOMPARE(rich.actionState(), MdTooltipActionState::Hovered);
    QTest::mouseRelease(&rich, Qt::LeftButton, Qt::NoModifier, QPoint(1, 1));
}

void TestMd3Tooltip::richActionKeyboardContract()
{
    MdTooltip rich(QStringLiteral("Body text"));
    rich.setVariant(MdTooltipVariant::Rich);
    rich.setActionLabel(QStringLiteral("Action"));
    rich.resize(rich.sizeHint());
    rich.show();
    flushEvents();
    // Window activation handed the initial focus widget a keyboard-class
    // reason; drop it before testing the :focus-visible semantics.
    rich.clearFocus();
    flushEvents();
    // Keyboard focus shows the focus row; mouse focus does not.
    rich.setFocus(Qt::MouseFocusReason);
    QCOMPARE(rich.actionState(), MdTooltipActionState::Enabled);
    // Qt suppresses a repeated FocusIn when the widget already has focus, so
    // clear first — the reason matters for :focus-visible.
    rich.clearFocus();
    flushEvents();
    rich.setFocus(Qt::TabFocusReason);
    QCOMPARE(rich.actionState(), MdTooltipActionState::Focused);

    bool clicked = false;
    connect(&rich, &MdTooltip::actionClicked, this, [&clicked] { clicked = true; });
    QTest::keyClick(&rich, Qt::Key_Return);
    QVERIFY(clicked);

    // A plain tooltip has no interactive region and takes no focus.
    MdTooltip plain(QStringLiteral("Body"));
    QCOMPARE(plain.focusPolicy(), Qt::NoFocus);
    QTest::keyClick(&plain, Qt::Key_Return); // must not crash
}

void TestMd3Tooltip::hostHoverTrigger()
{
    QWidget window;
    window.resize(600, 400);
    auto *host = new MdTooltipHost(&window);
    auto *anchor = new QPushButton(QStringLiteral("Anchor"));
    host->setAnchorWidget(anchor);
    host->setGeometry(100, 100, 120, 40);
    host->tooltip()->setText(QStringLiteral("Tooltip text"));
    window.show();
    flushEvents();

    QVERIFY(!host->isShowing());
    // Hover shows immediately and never self-dismisses. The enter transition
    // starts from opacity 0; give the 16 ms ticks a few frames.
    host->showTooltip(true);
    QVERIFY(host->isShowing());
    QVERIFY(!host->isAutoHideScheduled());
    QTest::qWait(60);
    QVERIFY(host->tooltip()->isVisible());

    host->dismiss();
    // The exit transition (~600 ms of spring settle) must finish before the
    // tooltip counts as hidden.
    QTest::qWait(700);
    QVERIFY(!host->isShowing());
    QVERIFY(!host->tooltip()->isVisible());
}

void TestMd3Tooltip::hostAutoHideRules()
{
    QWidget window;
    window.resize(600, 400);
    auto *host = new MdTooltipHost(&window);
    auto *anchor = new QPushButton(QStringLiteral("Anchor"));
    host->setAnchorWidget(anchor);
    host->setGeometry(100, 100, 120, 40);
    host->tooltip()->setText(QStringLiteral("Tooltip text"));
    window.show();
    flushEvents();

    // A non-hover show on a non-persistent tooltip schedules the 1500 ms
    // auto-hide (BasicTooltipDefaults.TooltipDuration).
    host->showTooltip(false);
    QVERIFY(host->isShowing());
    QVERIFY(host->isAutoHideScheduled());
    host->dismiss();

    // Persistent: no auto-hide; only explicit dismissal.
    host->setPersistent(true);
    host->showTooltip(false);
    QVERIFY(host->isShowing());
    QVERIFY(!host->isAutoHideScheduled());
    host->dismiss();

    // Hover show skips the timer even when non-persistent.
    host->setPersistent(false);
    host->showTooltip(true);
    QVERIFY(!host->isAutoHideScheduled());
    host->dismiss();
}

void TestMd3Tooltip::hostGlobalMutex()
{
    QWidget window;
    window.resize(600, 400);
    auto *first = new MdTooltipHost(&window);
    auto *firstAnchor = new QPushButton(QStringLiteral("First"));
    first->setAnchorWidget(firstAnchor);
    first->setGeometry(20, 20, 100, 30);
    first->tooltip()->setText(QStringLiteral("First"));

    auto *second = new MdTooltipHost(&window);
    auto *secondAnchor = new QPushButton(QStringLiteral("Second"));
    second->setAnchorWidget(secondAnchor);
    second->setGeometry(300, 20, 100, 30);
    second->tooltip()->setText(QStringLiteral("Second"));
    window.show();
    flushEvents();

    first->showTooltip(true);
    QVERIFY(first->isShowing());
    // The second show cancels the first instantly (GlobalMutatorMutex).
    second->showTooltip(true);
    QVERIFY(second->isShowing());
    QVERIFY(!first->isShowing());

    second->dismiss();
    QTest::qWait(700);
    QVERIFY(!second->isShowing());
}

void TestMd3Tooltip::hostEscapeDismiss()
{
    QWidget window;
    window.resize(600, 400);
    auto *host = new MdTooltipHost(&window);
    auto *anchor = new QPushButton(QStringLiteral("Anchor"));
    host->setAnchorWidget(anchor);
    host->setGeometry(100, 100, 120, 40);
    host->tooltip()->setText(QStringLiteral("Tooltip text"));
    window.show();
    flushEvents();

    host->showTooltip(true);
    QVERIFY(host->isShowing());

    // Escape on the anchor dismisses (the exit transition must settle).
    QTest::keyClick(anchor, Qt::Key_Escape);
    QTest::qWait(700);
    QVERIFY(!host->isShowing());

    // Escape on the popup itself dismisses too.
    host->showTooltip(true);
    flushEvents();
    QTest::keyClick(host->tooltip(), Qt::Key_Escape);
    QTest::qWait(700);
    QVERIFY(!host->isShowing());
}

void TestMd3Tooltip::hostTransitionSprings()
{
    // The enter transition runs the same fade + scale pair the snackbar host
    // uses: opacity on the effects-fast spring, scale 0.8→1 on the
    // spatial-fast spring.
    MdTooltipHost host;
    QCOMPARE(host.currentOpacity(), 1.0);
    QCOMPARE(host.currentScale(), 1.0);
    host.showTooltip(true);
    QVERIFY(host.isTransitioning());
    // A few ticks in, the opacity has left the 0 start and the scale has
    // left the 0.8 start.
    QTest::qWait(60);
    QVERIFY(host.currentOpacity() > 0.0);
    QVERIFY(host.currentScale() > 0.8);
    host.dismiss();
    QVERIFY(host.isTransitioning());
    QTest::qWait(700);
    QVERIFY(!host.isTransitioning());
    QCOMPARE(host.currentOpacity(), 0.0);
}

void TestMd3Tooltip::styleIsInstalled()
{
    // Constructing the widgets installed the paint hub filter.
    MdTooltip tooltip;
    QVERIFY(MdTooltipStyle::isInstalled());
}

} // namespace
} // namespace md

QTEST_MAIN(md::TestMd3Tooltip)
#include "TestMd3Tooltip.moc"
