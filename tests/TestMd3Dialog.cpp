#include "TestMd3Common.h"

#include "core/MdDialogTokens.h"
#include "core/MdShape.h"
#include "core/MdTypeScale.h"
#include "styles/MdDialogStyle.h"
#include "widgets/MdButton.h"
#include "widgets/MdDialog.h"
#include "widgets/MdDialogHost.h"

#include <QtGui/QKeyEvent>
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

/// Build a ContentSpec the way MdDialog does, for the pure layout functions.
MdDialogStyle::ContentSpec spec(bool icon, const QString &title, const QString &text,
                                const QList<qreal> &actions = {})
{
    MdDialogStyle::ContentSpec content;
    content.hasIcon = icon;
    content.title = title;
    content.text = text;
    content.actionWidths = actions;
    return content;
}

} // namespace

class TestMd3Dialog : public QObject
{
    Q_OBJECT

private slots:
    void tokenRows();
    void actionStateRows();
    void overrideParsing();
    void layoutGeometry();
    void layoutWithIcon();
    void layoutEdgeCases();
    void actionsWrapping();
    void widthBounds();
    void renderSmoke();
    void pointerContract();
    void keyboardContract();
    void hostFade();
    void hostGeometry();
    void styleIsInstalled();
};

void TestMd3Dialog::tokenRows()
{
    const MdDialogTokens tokens = MdDialogTokens::resolve();

    // Container: surface-container-high, level3, corner-extra-large.
    QCOMPARE(tokens.containerColor, ColorRole::SurfaceContainerHigh);
    QCOMPARE(tokens.containerElevation, ElevationLevel::Level3);
    QCOMPARE(tokens.containerShape, ShapeCorner::ExtraLarge);
    QCOMPARE(tokens.containerShadowColor, ColorRole::Shadow);

    // Headline / supporting text / icon / action.
    QCOMPARE(tokens.headlineStyle, TypeStyle::HeadlineSmall);
    QCOMPARE(tokens.headlineColor, ColorRole::OnSurface);
    QCOMPARE(tokens.supportingTextStyle, TypeStyle::BodyMedium);
    QCOMPARE(tokens.supportingTextColor, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.iconSize, 24.0);
    QCOMPARE(tokens.iconColor, ColorRole::Secondary);
    QCOMPARE(tokens.actionLabelStyle, TypeStyle::LabelLarge);
    QCOMPARE(tokens.actionLabelColor, ColorRole::Primary);

    // Deprecated-but-published divider rows.
    QCOMPARE(tokens.dividerHeight, 1.0);
    QCOMPARE(tokens.dividerColor, ColorRole::Outline);

    // Compose's private layout constants.
    QCOMPARE(MdDialogTokens::kContainerPadding, 24.0);
    QCOMPARE(MdDialogTokens::kTextBottomPadding, 24.0);
    QCOMPARE(MdDialogTokens::kIconBottomPadding, 16.0);
    QCOMPARE(MdDialogTokens::kTitleBottomPadding, 16.0);
    QCOMPARE(MdDialogTokens::kActionsSpacing, 8.0);
    QCOMPARE(MdDialogTokens::kMinWidth, 280.0);
    QCOMPARE(MdDialogTokens::kMaxWidth, 560.0);
    QCOMPARE(MdDialogTokens::kScrimOpacity, 0.32);
}

void TestMd3Dialog::actionStateRows()
{
    const MdDialogTokens tokens = MdDialogTokens::resolve();

    // The action rows repeat the primary label colour; only the state-layer
    // opacity moves: 0 / 0.08 / 0.12 / 0.12 (md.sys.state's own opacities).
    const MdDialogActionRow enabled = tokens.actionRow(MdDialogActionState::Enabled);
    QCOMPARE(enabled.content, ColorRole::Primary);
    QCOMPARE(enabled.stateLayer, ColorRole::Primary);
    QCOMPARE(enabled.stateLayerOpacity, 0.0);

    const MdDialogActionRow hovered = tokens.actionRow(MdDialogActionState::Hovered);
    QCOMPARE(hovered.content, ColorRole::Primary);
    QCOMPARE(hovered.stateLayer, ColorRole::Primary);
    QCOMPARE(hovered.stateLayerOpacity, 0.08);

    const MdDialogActionRow focused = tokens.actionRow(MdDialogActionState::Focused);
    QCOMPARE(focused.stateLayerOpacity, 0.12);

    const MdDialogActionRow pressed = tokens.actionRow(MdDialogActionState::Pressed);
    QCOMPARE(pressed.stateLayerOpacity, 0.12);
}

void TestMd3Dialog::overrideParsing()
{
    MdComponentTokens overrides;
    overrides.setValue(QStringLiteral("md.comp.dialog.container.shape"),
                       QStringLiteral("medium"));
    overrides.setValue(QStringLiteral("md.comp.dialog.icon.size"), QStringLiteral("56px"));
    overrides.setValue(QStringLiteral("md.comp.dialog.divider.height"), QStringLiteral("3"));

    const MdDialogTokens tokens = MdDialogTokens::resolve(&overrides);
    QCOMPARE(tokens.containerShape, ShapeCorner::Medium);
    QCOMPARE(tokens.iconSize, 56.0);
    QCOMPARE(tokens.dividerHeight, 3.0);

    // An unparseable override is ignored, not coerced.
    MdComponentTokens bad;
    bad.setValue(QStringLiteral("md.comp.dialog.container.shape"), QStringLiteral("blob"));
    bad.setValue(QStringLiteral("md.comp.dialog.icon.size"), QStringLiteral("tiny"));
    const MdDialogTokens fallback = MdDialogTokens::resolve(&bad);
    QCOMPARE(fallback.containerShape, ShapeCorner::ExtraLarge);
    QCOMPARE(fallback.iconSize, 24.0);
}

void TestMd3Dialog::layoutGeometry()
{
    const MdDialogTokens tokens = MdDialogTokens::resolve();
    const MdDialogStyle::ContentSpec content =
        spec(false, QStringLiteral("Title"), QStringLiteral("Body text"),
             { 96.0, 96.0 });

    const qreal widgetWidth = 360.0;
    const MdDialogStyle::Layout layout =
        MdDialogStyle::layoutFor(widgetWidth, content, tokens);

    // The container is the widget minus the shadow margin.
    QCOMPARE(layout.container, QRectF(8.0, 8.0, widgetWidth - 16.0, layout.container.height()));
    const QList<qreal> radii =
        MdShape::resolvedRadii(ShapeCorner::ExtraLarge, layout.container.size());
    QVERIFY(layout.radii.first() > 0.0);
    QCOMPARE(layout.radii.first(), radii.first());

    // No icon: the title starts at the container's top padding.
    QVERIFY(!layout.iconRect.isValid());
    QVERIFY(layout.titleRect.isValid());
    QCOMPARE(layout.titleRect.top(), 8.0 + 24.0);
    QCOMPARE(layout.titleRect.left(), 8.0 + 24.0);
    QCOMPARE(layout.titleRect.width(), widgetWidth - 16.0 - 48.0);

    // Text follows the title plus its 16 px bottom padding.
    QVERIFY(layout.textRect.isValid());
    QCOMPARE(layout.textRect.top(),
             layout.titleRect.top() + layout.titleRect.height() + 16.0);

    // One action row, end-aligned: dismiss left of confirm.
    QCOMPARE(layout.actionRows.size(), 1);
    QCOMPARE(layout.actionRows.first().size(), 2);
    const QRectF dismiss = layout.actionRows.first().first();
    const QRectF confirm = layout.actionRows.first().last();
    QVERIFY(dismiss.right() < confirm.left());
    QCOMPARE(confirm.right(), layout.container.right() - 24.0);
    QCOMPARE(confirm.height(), 40.0); // ButtonDefaults.MinHeight
    QCOMPARE(confirm.right() - dismiss.left(), 96.0 + 8.0 + 96.0);

    // Total height: 24 top + title + 16 + text + 24 + row + 24 bottom.
    const QFontMetricsF headline(MdTypeScale::font(TypeStyle::HeadlineSmall));
    const QFontMetricsF body(MdTypeScale::font(TypeStyle::BodyMedium));
    const qreal expected =
        24.0 + headline.height() + 16.0 + body.height() + 24.0 + 40.0 + 24.0;
    QVERIFY(closeTo(layout.container.height(), expected, 1e-3));

    // The widget height adds the shadow margins.
    QVERIFY(closeTo(MdDialogStyle::heightForWidth(widgetWidth, content, tokens),
                    layout.container.height() + 16.0, 1e-3));
}

void TestMd3Dialog::layoutWithIcon()
{
    const MdDialogTokens tokens = MdDialogTokens::resolve();
    const MdDialogStyle::ContentSpec content =
        spec(true, QStringLiteral("Title"), QString(), { 96.0 });

    const MdDialogStyle::Layout layout =
        MdDialogStyle::layoutFor(360.0, content, tokens);

    // The icon is centred and the title follows it.
    QVERIFY(layout.iconRect.isValid());
    QCOMPARE(layout.iconRect.size(), QSizeF(24.0, 24.0));
    QCOMPARE(layout.iconRect.center().x(), layout.container.center().x());
    QCOMPARE(layout.iconRect.top(), 8.0 + 24.0);
    QCOMPARE(layout.titleRect.top(), layout.iconRect.top() + layout.iconRect.height() + 16.0);
    QVERIFY(!layout.textRect.isValid());

    // With no text, the row follows the title's 16 px padding.
    QCOMPARE(layout.actionRows.first().first().top(),
             layout.titleRect.top() + layout.titleRect.height() + 16.0);
}

void TestMd3Dialog::layoutEdgeCases()
{
    const MdDialogTokens tokens = MdDialogTokens::resolve();

    // Textless and titleless: the icon (or actions) carry the whole flow.
    const MdDialogStyle::Layout bare =
        MdDialogStyle::layoutFor(360.0, spec(false, QString(), QString(), { 96.0 }), tokens);
    QVERIFY(!bare.titleRect.isValid());
    QVERIFY(!bare.textRect.isValid());
    QCOMPARE(bare.actionRows.size(), 1);

    // Long supporting text wraps taller than one line but never exceeds the
    // content width.
    const QString longText = QStringLiteral(
        "A rather long supporting text that must wrap onto several lines inside the "
        "content width so the dialog grows downwards instead of sideways.");
    const MdDialogStyle::Layout wrapped =
        MdDialogStyle::layoutFor(360.0, spec(false, QStringLiteral("T"), longText), tokens);
    QVERIFY(wrapped.textRect.height() > QFontMetricsF(MdTypeScale::font(TypeStyle::BodyMedium))
                                            .height());
    QVERIFY(wrapped.textRect.width() <= 360.0 - 16.0 - 48.0 + kEpsilon);
}

void TestMd3Dialog::actionsWrapping()
{
    const MdDialogTokens tokens = MdDialogTokens::resolve();

    // Two wide buttons that cannot share one row wrap: the confirm takes the
    // first row, the dismiss the second — both end-aligned (the RTL FlowRow
    // trick, restated).
    const MdDialogStyle::ContentSpec content =
        spec(false, QStringLiteral("T"), QString(), { 200.0, 200.0 });
    const MdDialogStyle::Layout layout = MdDialogStyle::layoutFor(360.0, content, tokens);

    QCOMPARE(layout.actionRows.size(), 2);
    const QRectF confirm = layout.actionRows.first().first();
    const QRectF dismiss = layout.actionRows.last().first();
    QCOMPARE(confirm.width(), 200.0);
    QCOMPARE(dismiss.width(), 200.0);
    QCOMPARE(confirm.right(), layout.container.right() - 24.0);
    QCOMPARE(dismiss.right(), layout.container.right() - 24.0);
    QCOMPARE(dismiss.top(),
             confirm.top() + confirm.height() + MdDialogTokens::kActionsSpacing);
    // Confirm above dismiss.
    QVERIFY(confirm.top() + confirm.height() < dismiss.top());
}

void TestMd3Dialog::widthBounds()
{
    const MdDialogTokens tokens = MdDialogTokens::resolve();
    const MdDialogStyle::ContentSpec content =
        spec(false, QStringLiteral("T"), QStringLiteral("Body"), { 96.0 });

    // A dialog narrower than the 280 floor still lays out (the host clamps);
    // the content width just follows the container.
    const MdDialogStyle::Layout narrow =
        MdDialogStyle::layoutFor(200.0, content, tokens);
    QCOMPARE(narrow.container.width(), 200.0 - 16.0);

    // The host clamps its dialog's widget width into [280, 560] + margins.
    QWidget parentWidgetArea;
    parentWidgetArea.resize(800, 600);
    MdDialogHost host(&parentWidgetArea);
    host.dialog()->setTitle(QStringLiteral("T"));
    host.dialog()->setText(QStringLiteral("Body"));
    auto *confirm = new MdButton(QStringLiteral("OK"));
    confirm->setVariant(ButtonVariant::Text);
    host.dialog()->setConfirmButton(confirm);
    parentWidgetArea.show();
    QTest::qWaitForWindowExposed(&parentWidgetArea);
    host.show();
    const int widgetWidth = host.dialog()->width();
    QVERIFY(widgetWidth >= int(MdDialogTokens::kMinWidth));
    QVERIFY(widgetWidth <= int(MdDialogTokens::kMaxWidth + MdDialogTokens::kShadowMargin * 2.0));

    // A host narrower than the minimum clamps to the host.
    QWidget small;
    small.resize(220, 300);
    MdDialogHost tightHost(&small);
    small.show();
    QTest::qWaitForWindowExposed(&small);
    tightHost.show();
    QVERIFY(tightHost.dialog()->width() <= 220);
}

void TestMd3Dialog::renderSmoke()
{
    MdDialog dialog;
    dialog.setTitle(QStringLiteral("Reset settings?"));
    dialog.setText(QStringLiteral("This will reset all settings to their defaults."));
    auto *confirm = new MdButton(QStringLiteral("Accept"));
    confirm->setVariant(ButtonVariant::Text);
    dialog.setConfirmButton(confirm);
    auto *dismiss = new MdButton(QStringLiteral("Decline"));
    dismiss->setVariant(ButtonVariant::Text);
    dialog.setDismissButton(dismiss);
    dialog.resize(360, dialog.heightForWidth(360));
    dialog.show();

    // Painting must not crash at several opacities (the host fade drives it).
    for (const qreal opacity : { 1.0, 0.5, 0.0 }) {
        dialog.setPaintOpacity(opacity);
        const QImage image = dialog.grab().toImage();
        QVERIFY(!image.isNull());
    }

    // Content changes reflow without crashing.
    dialog.setText(QStringLiteral("Short."));
    QVERIFY(dialog.heightForWidth(360) < 1000);
}

void TestMd3Dialog::pointerContract()
{
    QWidget parent;
    parent.resize(600, 400);
    MdDialogHost host(&parent);
    host.dialog()->setTitle(QStringLiteral("T"));
    host.dialog()->setText(QStringLiteral("B"));

    int dismissals = 0;
    QObject::connect(&host, &MdDialogHost::dismissed, [&dismissals] { ++dismissals; });

    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    host.show();
    QVERIFY(host.isShowing());
    QCOMPARE(dismissals, 0);

    // A click on the scrim runs the onDismissRequest flow.
    QTest::mousePress(&host, Qt::LeftButton, Qt::NoModifier,
                      QPoint(2, host.height() / 2));
    QCOMPARE(dismissals, 1);
    QVERIFY(host.isTransitioning() || !host.isShowing());

    // Settled: the dialog is hidden again.
    QTest::qWait(400);
    QVERIFY(!host.isShowing());
    QCOMPARE(host.currentOpacity(), 0.0);

    // Re-show, and the fade completes to full opacity.
    host.show();
    QTest::qWait(400);
    QVERIFY(host.isShowing());
    QCOMPARE(host.currentOpacity(), 1.0);

    // A click inside the dialog does not dismiss (the dialog child swallows
    // it; the host sees nothing).
    const QPoint inside = host.dialog()->geometry().center();
    QMouseEvent insidePress(QEvent::MouseButtonPress, QPointF(inside),
                            QPointF(host.mapToGlobal(inside)), Qt::LeftButton,
                            Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&host, &insidePress);
    QCOMPARE(dismissals, 1);
    QVERIFY(host.isShowing());

    // An explicit dismiss() also emits exactly once per request.
    host.dismiss();
    QCOMPARE(dismissals, 2);
    QTest::qWait(400);
    QVERIFY(!host.isShowing());
}

void TestMd3Dialog::keyboardContract()
{
    QWidget parent;
    parent.resize(600, 400);
    MdDialogHost host(&parent);
    host.dialog()->setTitle(QStringLiteral("T"));

    int dismissals = 0;
    QObject::connect(&host, &MdDialogHost::dismissed, [&dismissals] { ++dismissals; });

    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    host.show();
    QTest::qWait(100);

    // Escape on the dialog runs the onDismissRequest flow.
    QTest::keyClick(host.dialog(), Qt::Key_Escape);
    QCOMPARE(dismissals, 1);
    QTest::qWait(400);
    QVERIFY(!host.isShowing());
}

void TestMd3Dialog::hostFade()
{
    QWidget parent;
    parent.resize(600, 400);
    MdDialogHost host(&parent);
    host.dialog()->setTitle(QStringLiteral("T"));
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));

    // The fade starts at 0 and settles at 1 on the effects-fast spring.
    host.show();
    QVERIFY(host.isTransitioning());
    QVERIFY(host.currentOpacity() >= 0.0);
    QTest::qWait(400);
    QCOMPARE(host.currentOpacity(), 1.0);
    QVERIFY(!host.isTransitioning());

    // The leaving phase runs the fade back down to 0.
    host.dismiss();
    QVERIFY(host.isTransitioning());
    QTest::qWait(400);
    QCOMPARE(host.currentOpacity(), 0.0);
    QVERIFY(!host.isShowing());

    // Showing while already showing is a no-op.
    host.show();
    QTest::qWait(400);
    host.show();
    QCOMPARE(host.currentOpacity(), 1.0);
}

void TestMd3Dialog::hostGeometry()
{
    QWidget parent;
    parent.resize(800, 600);
    MdDialogHost host(&parent);
    host.dialog()->setTitle(QStringLiteral("T"));
    host.dialog()->setText(QStringLiteral("B"));
    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    host.show();
    QTest::qWait(50);

    // Centred in the host.
    const QRect dialogGeometry = host.dialog()->geometry();
    QVERIFY(closeTo(dialogGeometry.center().x(), host.width() / 2.0, 1.5));
    QVERIFY(closeTo(dialogGeometry.center().y(), host.height() / 2.0, 1.5));

    // Width clamped to the 560 cap (+ shadow margins); height follows the
    // dialog's own height-for-width.
    QVERIFY(dialogGeometry.width() <= int(MdDialogTokens::kMaxWidth + 16.0));
    QCOMPARE(dialogGeometry.height(),
             host.dialog()->heightForWidth(dialogGeometry.width()));
}

void TestMd3Dialog::styleIsInstalled()
{
    // Constructing a dialog installs the paint filter.
    MdDialog dialog;
    QVERIFY(MdDialogStyle::isInstalled());
}

QTEST_MAIN(TestMd3Dialog)

#include "TestMd3Dialog.moc"
