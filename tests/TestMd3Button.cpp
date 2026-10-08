// MdButton — MD3 common buttons.
//
// Three things are pinned here and they are deliberately different kinds of
// check:
//
//   1. the token table, field by field, against the published
//      `md.comp.button.*` sets. A wrong leading/trailing space or a swapped
//      resting shape is invisible in a screenshot but wrong forever after.
//   2. the widget contract — properties, signals, geometry, mnemonics,
//      soft-disabled, and the press shape morph's state machine.
//   3. a render smoke check across every variant x size x shape combination,
//      because a style that silently paints nothing still returns a clean
//      QSize and passes 1 and 2.

#include "TestMd3Common.h"

#include "core/MdButtonTokens.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTokens.h"
#include "styles/MdButtonStyle.h"
#include "widgets/MdButton.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

struct OffscreenDefault
{
    OffscreenDefault()
    {
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
        }
    }
};

// NOLINTNEXTLINE(cert-err58-cpp) — qputenv cannot throw.
const OffscreenDefault g_offscreenDefault;

bool near(qreal a, qreal b, qreal tolerance = 0.001)
{
    return qAbs(a - b) <= tolerance;
}

/// Show, grab, hide. The paint path under test is the application-wide paint
/// hub, which only sees events once the widget is realised, so this cannot be
/// replaced with a bare QWidget::render() without testing something else.
QImage renderButton(MdButton &button, const QSize &size)
{
    button.resize(size);
    button.show();
    QTest::qWaitForWindowExposed(&button);
    const QPixmap pixmap = button.grab();
    button.hide();
    return pixmap.toImage();
}

/// Non-background pixel count. The reference colour is taken from the widget's
/// own top-left corner, which always lands in the transparent margin reserved
/// for the focus indicator and is therefore never part of the container.
int paintedPixels(const QImage &image)
{
    return mdtest::paintedPixelCount(image, image.pixel(0, 0));
}

/// Composite onto an opaque colour.
///
/// A grab of a widget that never fills its own background comes back with
/// transparent pixels, and transparent black composites to "no ink" while a
/// *translucent* tint composites to a colour that looks nothing like it. Any
/// test that compares pixel intensities has to start from a known opaque
/// backdrop or it is measuring the alpha channel by accident.
QRgb backdrop()
{
    // Deliberately a grey no M3 colour role produces, so a match cannot be
    // mistaken for a themed surface.
    return qRgb(0xF2, 0xF2, 0xF2);
}

QImage flatten(const QImage &image)
{
    QImage flat(image.size(), QImage::Format_RGB32);
    flat.fill(backdrop());
    QPainter painter(&flat);
    painter.drawImage(0, 0, image);
    painter.end();
    return flat;
}

/// Largest per-channel difference, ignoring alpha.
int channelDelta(QRgb a, QRgb b)
{
    return qMax(qMax(qAbs(qRed(a) - qRed(b)), qAbs(qGreen(a) - qGreen(b))),
                qAbs(qBlue(a) - qBlue(b)));
}

} // namespace

class TestMd3Button : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    void tokenMetricsMatchThePublishedSizeScale();
    void tokenShapesMatchThePublishedShapeTokens();
    void variantColoursMatchThePublishedTokenSets();
    void disabledUsesOnSurfaceAtThePublishedOpacities();
    void theSizeSpecificPaddingWinsOverTheBaseToken();
    void componentOverridesReachTheTokenTable();
    void propertiesRoundTripThroughTheMetaObject();
    void signalsFireOnlyOnChange();
    void sizeHintIsTheContainerPlusTheFocusRingMargin();
    void containerHeightMatchesTheSizeToken();
    void displayTextStripsMnemonics();
    void softDisabledStaysFocusableButIgnoresPointerAndKeyboard();
    void pressMorphsTheContainerTowardsThePressedShape();
    void pointerFocusShowsNoFocusIndicatorButKeyboardFocusDoes();
    void everyCombinationRendersSomething();
    void lightAndDarkPaintDifferentContainers();
    void outlinedAndTextPaintNoContainer();
};

void TestMd3Button::pointerFocusShowsNoFocusIndicatorButKeyboardFocusDoes()
{
    // The official components gate the focus indicator and the focused state
    // colours on `:focus-visible` — a Tab shows them, a mouse click does not.
    // Qt only offers the focus *reason*, so that is what the widget records.
    MdButton button(QStringLiteral("Focus"));
    button.resize(button.sizeHint());
    button.show();
    QVERIFY(QTest::qWaitForWindowExposed(&button));

    // A pointer press that moves focus must not read as keyboard focus.
    QTest::mouseClick(&button, Qt::LeftButton);
    QVERIFY(button.hasFocus());
    QVERIFY(!button.hasKeyboardFocus());
    // The pointer is still over the button, so the visible state is hovered —
    // hovered regardless of focus, which is the point: focus alone must not
    // upgrade the state when it is not the keyboard kind.
    QCOMPARE(button.paintState(), MdButtonState::Hovered);

    // Keyboard focus (a Tab) is the visible kind. setFocus() on a widget
    // that already holds focus does not re-deliver focusInEvent, so drop it
    // first — that is also what makes the reason re-classification run.
    button.clearFocus();
    button.setFocus(Qt::TabFocusReason);
    QVERIFY(button.hasFocus());
    QVERIFY(button.hasKeyboardFocus());

    // Mouse focus taking focus *away* and giving it back clears the flag —
    // and again, drop the focus first so the reason is re-delivered.
    button.clearFocus();
    button.setFocus(Qt::MouseFocusReason);
    QVERIFY(button.hasFocus());
    QVERIFY(!button.hasKeyboardFocus());

    button.clearFocus();
    QVERIFY(!button.hasKeyboardFocus());
    button.hide();
}

void TestMd3Button::cleanup()
{
    // The application-wide store is shared, so a leaked override would change
    // what every later slot resolves.
    MdComponentTokens::global().clear();
}

// ---------------------------------------------------------------------------
// 1. the token table
// ---------------------------------------------------------------------------

void TestMd3Button::tokenMetricsMatchThePublishedSizeScale()
{
    struct Row
    {
        ButtonSize size;
        qreal height;
        qreal iconSize;
        qreal iconLabelSpace;
        qreal leadingSpace;
        qreal outlineWidth;
        TypeStyle labelStyle;
    };
    const QVector<Row> rows = {
        {ButtonSize::XSmall, 32.0, 20.0, 8.0, 12.0, 1.0, TypeStyle::LabelLarge},
        {ButtonSize::Small, 40.0, 20.0, 8.0, 16.0, 1.0, TypeStyle::LabelLarge},
        {ButtonSize::Medium, 56.0, 24.0, 8.0, 24.0, 1.0, TypeStyle::TitleMedium},
        {ButtonSize::Large, 96.0, 32.0, 12.0, 48.0, 2.0, TypeStyle::HeadlineSmall},
        {ButtonSize::XLarge, 136.0, 40.0, 16.0, 64.0, 3.0, TypeStyle::HeadlineLarge},
    };

    for (const Row &row : rows) {
        const MdButtonTokens tokens =
            MdButtonTokens::resolve(ButtonVariant::Filled, row.size, ButtonShape::Round);
        const QByteArray where = md::buttonSizeName(row.size).toLatin1();

        QVERIFY2(near(tokens.containerHeight, row.height), where.constData());
        QVERIFY2(near(tokens.iconSize, row.iconSize), where.constData());
        QVERIFY2(near(tokens.iconLabelSpace, row.iconLabelSpace), where.constData());
        QVERIFY2(near(tokens.leadingSpace, row.leadingSpace), where.constData());
        // The scale is symmetric; upstream publishes both sides as one value.
        QVERIFY2(near(tokens.trailingSpace, row.leadingSpace), where.constData());
        QVERIFY2(near(tokens.outlineWidth, row.outlineWidth), where.constData());
        QVERIFY2(tokens.labelStyle == row.labelStyle, where.constData());
    }

    // Every size shares the same press spring: spring-fast-spatial.
    const MdButtonTokens any =
        MdButtonTokens::resolve(ButtonVariant::Filled, ButtonSize::Medium, ButtonShape::Round);
    QVERIFY(near(any.springStiffness, 1400.0));
    QVERIFY(near(any.springDampingRatio, 0.9));
}

void TestMd3Button::tokenShapesMatchThePublishedShapeTokens()
{
    // container.shape.round is corner-full for every size; .square steps up
    // with the size, and pressed.shape is one step below the resting square.
    struct Row
    {
        ButtonSize size;
        ShapeCorner square;
        ShapeCorner pressed;
    };
    const QVector<Row> rows = {
        {ButtonSize::XSmall, ShapeCorner::Medium, ShapeCorner::Small},
        {ButtonSize::Small, ShapeCorner::Medium, ShapeCorner::Small},
        {ButtonSize::Medium, ShapeCorner::Large, ShapeCorner::Medium},
        {ButtonSize::Large, ShapeCorner::ExtraLarge, ShapeCorner::Large},
        {ButtonSize::XLarge, ShapeCorner::ExtraLarge, ShapeCorner::Large},
    };

    for (const Row &row : rows) {
        const MdButtonTokens round =
            MdButtonTokens::resolve(ButtonVariant::Filled, row.size, ButtonShape::Round);
        const MdButtonTokens square =
            MdButtonTokens::resolve(ButtonVariant::Filled, row.size, ButtonShape::Square);

        QCOMPARE(round.restingShape, ShapeCorner::Full);
        QCOMPARE(square.restingShape, row.square);
        QCOMPARE(round.pressedShape, row.pressed);
        QCOMPARE(square.pressedShape, row.pressed);
    }
}

void TestMd3Button::variantColoursMatchThePublishedTokenSets()
{
    struct Row
    {
        ButtonVariant variant;
        ColorRole container; ///< Count == the variant paints no container
        ColorRole content;
        ColorRole outline;
        ElevationLevel enabledElevation;
        ElevationLevel hoveredElevation;
    };
    const QVector<Row> rows = {
        {ButtonVariant::Elevated, ColorRole::SurfaceContainerLow, ColorRole::Primary,
         ColorRole::Count, ElevationLevel::Level1, ElevationLevel::Level2},
        {ButtonVariant::Filled, ColorRole::Primary, ColorRole::OnPrimary, ColorRole::Count,
         ElevationLevel::Level0, ElevationLevel::Level1},
        {ButtonVariant::Tonal, ColorRole::SecondaryContainer, ColorRole::OnSecondaryContainer,
         ColorRole::Count, ElevationLevel::Level0, ElevationLevel::Level1},
        {ButtonVariant::Outlined, ColorRole::Count, ColorRole::OnSurfaceVariant,
         ColorRole::OutlineVariant, ElevationLevel::Level0, ElevationLevel::Level0},
        {ButtonVariant::Text, ColorRole::Count, ColorRole::Primary, ColorRole::Count,
         ElevationLevel::Level0, ElevationLevel::Level0},
    };

    for (const Row &row : rows) {
        const MdButtonTokens tokens =
            MdButtonTokens::resolve(row.variant, ButtonSize::Small, ButtonShape::Round);
        const QByteArray where = md::buttonVariantName(row.variant).toLatin1();

        QVERIFY2(tokens.enabled.container.role == row.container, where.constData());
        QVERIFY2(tokens.enabled.paintsContainer() == (row.container != ColorRole::Count),
                 where.constData());
        QVERIFY2(tokens.enabled.labelText.role == row.content, where.constData());
        QVERIFY2(tokens.enabled.icon.role == row.content, where.constData());
        QVERIFY2(tokens.enabled.stateLayer == row.content, where.constData());
        QVERIFY2(tokens.enabled.outline == row.outline, where.constData());
        QVERIFY2(tokens.enabled.paintsOutline() == (row.outline != ColorRole::Count),
                 where.constData());
        QVERIFY2(tokens.enabled.elevation == row.enabledElevation, where.constData());
        QVERIFY2(tokens.hovered.elevation == row.hoveredElevation, where.constData());

        // hovered / focused / pressed republish the same roles as enabled: the
        // states differ by state-layer opacity and elevation, not by colour.
        for (MdButtonState state :
             {MdButtonState::Hovered, MdButtonState::Focused, MdButtonState::Pressed}) {
            QVERIFY2(tokens.state(state).labelText.role == row.content, where.constData());
            QVERIFY2(tokens.state(state).stateLayer == row.content, where.constData());
        }
    }
}

void TestMd3Button::disabledUsesOnSurfaceAtThePublishedOpacities()
{
    for (int i = 0; i < int(ButtonVariant::Count); ++i) {
        const auto variant = ButtonVariant(i);
        const MdButtonTokens tokens = MdButtonTokens::resolve(variant, ButtonSize::Small,
                                                              ButtonShape::Round);
        const QByteArray where = md::buttonVariantName(variant).toLatin1();

        // Every variant republishes disabled.{container,label-text,icon}.color
        // as on-surface, faded by the three published opacities.
        QVERIFY2(near(tokens.disabledContainerOpacity, 0.10), where.constData());
        QVERIFY2(near(tokens.disabledLabelOpacity, 0.38), where.constData());
        QVERIFY2(near(tokens.disabledIconOpacity, 0.38), where.constData());

        QVERIFY2(tokens.disabled.labelText.role == ColorRole::OnSurface, where.constData());
        QVERIFY2(near(tokens.disabled.labelText.opacity, 0.38), where.constData());
        QVERIFY2(tokens.disabled.icon.role == ColorRole::OnSurface, where.constData());
        QVERIFY2(tokens.disabled.elevation == ElevationLevel::Level0, where.constData());

        // A disabled button takes no state layer at all.
        QVERIFY2(tokens.disabled.stateLayer == ColorRole::Count, where.constData());

        // The container-less variants stay container-less when disabled: giving
        // them a fill only while disabled would be a jump nothing specifies.
        QVERIFY2(tokens.disabled.paintsContainer() == tokens.enabled.paintsContainer(),
                 where.constData());
        if (tokens.disabled.paintsContainer()) {
            QVERIFY2(tokens.disabled.container.role == ColorRole::OnSurface, where.constData());
            QVERIFY2(near(tokens.disabled.container.opacity, 0.10), where.constData());
        }
    }
}

void TestMd3Button::theSizeSpecificPaddingWinsOverTheBaseToken()
{
    // Upstream publishes two different paddings for the same 40 px height:
    // md.comp.button.leading-space is 24px while md.comp.button.small.
    // leading-space is 16px. A caller who asked for Small gets Small's value.
    // This test exists so that "fixing" the disagreement by deleting one of the
    // two rows fails loudly. See docs/porting-todo.md.
    const MdButtonTokens small =
        MdButtonTokens::resolve(ButtonVariant::Filled, ButtonSize::Small, ButtonShape::Round);
    QVERIFY(near(small.containerHeight, 40.0));
    QVERIFY(near(small.leadingSpace, 16.0));
    QVERIFY(!near(small.leadingSpace, 24.0));
}

void TestMd3Button::componentOverridesReachTheTokenTable()
{
    // The size-qualified key wins over the generic one, and both beat the
    // published default.
    MdComponentTokens local;
    local.setValue(QStringLiteral("md.comp.button.large.container.height"), QStringLiteral("104"));
    local.setValue(QStringLiteral("md.comp.button.icon.size"), QStringLiteral("28"));
    local.setValue(QStringLiteral("md.comp.button.container.shape.square"), QStringLiteral("full"));
    // The `12px` form the SCSS export uses must parse too.
    local.setValue(QStringLiteral("md.comp.button.outlined.outline.width"), QStringLiteral("4px"));

    const MdButtonTokens tokens = MdButtonTokens::resolve(ButtonVariant::Outlined, ButtonSize::Large,
                                                          ButtonShape::Square, &local);
    QVERIFY(near(tokens.containerHeight, 104.0));
    QVERIFY(near(tokens.iconSize, 28.0));
    QCOMPARE(tokens.restingShape, ShapeCorner::Full);
    QVERIFY(near(tokens.outlineWidth, 4.0));

    // A key the size override does not cover still reads the generic one, and
    // an unrelated size is untouched.
    const MdButtonTokens xsmall = MdButtonTokens::resolve(ButtonVariant::Filled, ButtonSize::XSmall,
                                                          ButtonShape::Round, &local);
    QVERIFY(near(xsmall.containerHeight, 32.0));
    QVERIFY(near(xsmall.iconSize, 28.0));

    // A shape override is accepted under both spellings of the token name.
    MdComponentTokens prefixed;
    prefixed.setValue(QStringLiteral("md.comp.button.container.shape.round"),
                      QStringLiteral("corner-extra-small"));
    const MdButtonTokens viaPrefix = MdButtonTokens::resolve(
        ButtonVariant::Filled, ButtonSize::Small, ButtonShape::Round, &prefixed);
    QCOMPARE(viaPrefix.restingShape, ShapeCorner::ExtraSmall);

    // A typo'd shape name falls back rather than silently becoming "none".
    MdComponentTokens typo;
    typo.setValue(QStringLiteral("md.comp.button.container.shape.round"), QStringLiteral("rounded"));
    const MdButtonTokens fallback =
        MdButtonTokens::resolve(ButtonVariant::Filled, ButtonSize::Small, ButtonShape::Round, &typo);
    QCOMPARE(fallback.restingShape, ShapeCorner::Full);

    // The application-wide store is honoured when no instance bag is given.
    MdComponentTokens::global().setValue(QStringLiteral("md.comp.button.disabled.container.opacity"),
                                         QStringLiteral("0.2"));
    const MdButtonTokens globalOnly =
        MdButtonTokens::resolve(ButtonVariant::Filled, ButtonSize::Small, ButtonShape::Round);
    QVERIFY(near(globalOnly.disabledContainerOpacity, 0.2));

    // ...and an instance bag still sees it.
    MdComponentTokens empty;
    const MdButtonTokens viaInstance = MdButtonTokens::resolve(ButtonVariant::Filled,
                                                              ButtonSize::Small, ButtonShape::Round,
                                                              &empty);
    QVERIFY(near(viaInstance.disabledContainerOpacity, 0.2));
}

// ---------------------------------------------------------------------------
// 2. the widget contract
// ---------------------------------------------------------------------------

void TestMd3Button::propertiesRoundTripThroughTheMetaObject()
{
    MdButton button(QStringLiteral("Save"));

    const QMetaObject *meta = button.metaObject();
    // QVERIFY expands to an early `return`, so it cannot live inside a lambda
    // that produces a value. These two stay total and the assertions stay here.
    const auto writeProperty = [&](const char *name, const QVariant &value) {
        const int index = meta->indexOfProperty(name);
        return index >= 0 && meta->property(index).write(&button, value);
    };
    const auto readProperty = [&](const char *name) {
        const int index = meta->indexOfProperty(name);
        return index >= 0 ? meta->property(index).read(&button) : QVariant();
    };

    QVERIFY(writeProperty("variant", QVariant::fromValue(ButtonVariant::Outlined)));
    QCOMPARE(readProperty("variant").value<ButtonVariant>(), ButtonVariant::Outlined);

    QVERIFY(writeProperty("buttonSize", QVariant::fromValue(ButtonSize::XLarge)));
    QCOMPARE(readProperty("buttonSize").value<ButtonSize>(), ButtonSize::XLarge);

    QVERIFY(writeProperty("buttonShape", QVariant::fromValue(ButtonShape::Square)));
    QCOMPARE(readProperty("buttonShape").value<ButtonShape>(), ButtonShape::Square);

    QVERIFY(writeProperty("leadingIcon", QStringLiteral("add")));
    QCOMPARE(readProperty("leadingIcon").toString(), QStringLiteral("add"));

    QVERIFY(writeProperty("trailingIcon", QStringLiteral("arrow_forward")));
    QCOMPARE(readProperty("trailingIcon").toString(), QStringLiteral("arrow_forward"));

    QVERIFY(writeProperty("softDisabled", true));
    QCOMPARE(readProperty("softDisabled").toBool(), true);

    // Every property must advertise NOTIFY, per the brief's DoD item 3.
    for (const char *name : {"variant", "buttonSize", "buttonShape", "leadingIcon", "trailingIcon",
                             "softDisabled"}) {
        const int index = meta->indexOfProperty(name);
        QVERIFY2(index >= 0, name);
        QVERIFY2(meta->property(index).hasNotifySignal(), name);
        // ...and be readable through the generic path, which is what an
        // inspector relies on.
        QVERIFY2(meta->property(index).read(&button).isValid(), name);
    }
}

void TestMd3Button::signalsFireOnlyOnChange()
{
    MdButton button;

    QSignalSpy variantSpy(&button, &MdButton::variantChanged);
    QSignalSpy sizeSpy(&button, &MdButton::buttonSizeChanged);
    QSignalSpy shapeSpy(&button, &MdButton::buttonShapeChanged);
    QSignalSpy iconSpy(&button, &MdButton::leadingIconChanged);
    QSignalSpy softSpy(&button, &MdButton::softDisabledChanged);

    button.setVariant(ButtonVariant::Filled); // already the default
    button.setButtonSize(ButtonSize::Small);
    button.setButtonShape(ButtonShape::Round);
    button.setLeadingIcon(QString());
    button.setSoftDisabled(false);
    QCOMPARE(variantSpy.count(), 0);
    QCOMPARE(sizeSpy.count(), 0);
    QCOMPARE(shapeSpy.count(), 0);
    QCOMPARE(iconSpy.count(), 0);
    QCOMPARE(softSpy.count(), 0);

    button.setVariant(ButtonVariant::Text);
    button.setButtonSize(ButtonSize::Large);
    button.setButtonShape(ButtonShape::Square);
    button.setLeadingIcon(QStringLiteral("check"));
    button.setSoftDisabled(true);
    QCOMPARE(variantSpy.count(), 1);
    QCOMPARE(sizeSpy.count(), 1);
    QCOMPARE(shapeSpy.count(), 1);
    QCOMPARE(iconSpy.count(), 1);
    QCOMPARE(softSpy.count(), 1);

    QCOMPARE(variantSpy.first().at(0).value<ButtonVariant>(), ButtonVariant::Text);
    QCOMPARE(sizeSpy.first().at(0).value<ButtonSize>(), ButtonSize::Large);
    QCOMPARE(shapeSpy.first().at(0).value<ButtonShape>(), ButtonShape::Square);
    QCOMPARE(iconSpy.first().at(0).toString(), QStringLiteral("check"));
    QCOMPARE(softSpy.first().at(0).toBool(), true);
}

void TestMd3Button::sizeHintIsTheContainerPlusTheFocusRingMargin()
{
    MdButton button(QStringLiteral("Button"));
    button.setVariant(ButtonVariant::Filled);
    button.setButtonSize(ButtonSize::Small);

    const MdButtonTokens tokens = button.tokens();
    const MdFocusRingSpec spec = MdButtonStyle::focusRingSpec(tokens);
    const qreal inset = MdButtonStyle::focusRingInset(spec);

    // 2 px gap + 8 px peak stroke + 1.5 px half of the resting 3 px stroke.
    QVERIFY(near(inset, 7.5));
    QVERIFY(near(spec.width, 3.0));
    QVERIFY(near(spec.outwardOffset, 2.0));

    const QSize hint = button.sizeHint();
    QVERIFY(near(hint.height(), tokens.containerHeight + 2.0 * inset, 1.0));
    QVERIFY(hint.width() > 2.0 * inset);
}

void TestMd3Button::containerHeightMatchesTheSizeToken()
{
    // The widget is padded for the focus indicator, but the *container* — the
    // thing the token describes and the thing that must match the spec — is
    // exactly the token height.
    for (int i = 0; i < int(ButtonSize::Count); ++i) {
        const auto size = ButtonSize(i);
        MdButton button(QStringLiteral("Button"));
        button.setButtonSize(size);
        button.resize(button.sizeHint());

        const QRectF container = button.containerRect();
        const QByteArray where = md::buttonSizeName(size).toLatin1();
        QVERIFY2(near(container.height(), button.tokens().containerHeight), where.constData());
    }
}

void TestMd3Button::displayTextStripsMnemonics()
{
    MdButton button;
    button.setText(QStringLiteral("&File"));
    QCOMPARE(button.displayText(), QStringLiteral("File"));
    // The shortcut QAbstractButton installed is independent of the glyphs.
    QCOMPARE(button.shortcut().toString(), QStringLiteral("Alt+F"));

    button.setText(QStringLiteral("A && B"));
    QCOMPARE(button.displayText(), QStringLiteral("A & B"));

    button.setText(QStringLiteral("Plain"));
    QCOMPARE(button.displayText(), QStringLiteral("Plain"));
}

void TestMd3Button::softDisabledStaysFocusableButIgnoresPointerAndKeyboard()
{
    MdButton button(QStringLiteral("Paste"));
    button.setSoftDisabled(true);
    button.resize(button.sizeHint());
    button.show();
    QVERIFY(QTest::qWaitForWindowExposed(&button));

    QVERIFY(button.isEffectivelyDisabled());
    QVERIFY(button.isEnabled()); // unlike setEnabled(false), it stays enabled
    QCOMPARE(button.focusPolicy(), Qt::StrongFocus);
    QCOMPARE(button.paintState(), MdButtonState::Disabled);

    // Neither a click nor Space may activate it.
    QSignalSpy clickedSpy(&button, &QAbstractButton::clicked);
    QTest::mouseClick(&button, Qt::LeftButton);
    QTest::keyClick(&button, Qt::Key_Space);
    QCOMPARE(clickedSpy.count(), 0);

    // A hard-disabled button is the one that loses focusability.
    button.setSoftDisabled(false);
    button.setEnabled(false);
    QCOMPARE(button.paintState(), MdButtonState::Disabled);
    QVERIFY(button.isEffectivelyDisabled());
    QVERIFY(!button.isSoftDisabled());

    button.hide();
}

void TestMd3Button::pressMorphsTheContainerTowardsThePressedShape()
{
    MdButton button(QStringLiteral("Press me"));
    button.setButtonSize(ButtonSize::Small);
    button.setButtonShape(ButtonShape::Round);
    button.resize(button.sizeHint());
    button.show();
    QVERIFY(QTest::qWaitForWindowExposed(&button));

    QVERIFY(near(button.pressMorph(), 0.0));

    // A full pill at rest (corner-full on a 40 px container is r = 20).
    QList<qreal> resting = MdButtonStyle::layoutFor(button, button.tokens()).radii;
    QVERIFY(!resting.isEmpty());
    QVERIFY(near(resting.first(), 20.0));

    QTest::mousePress(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
    // spring-fast-spatial settles in roughly 120 ms; 400 ms is comfortably past.
    QTRY_VERIFY_WITH_TIMEOUT(near(button.pressMorph(), 1.0, 0.02), 2000);
    QCOMPARE(button.paintState(), MdButtonState::Pressed);

    // ...and the container's corners really did move to the pressed token,
    // corner-small on a 40 px container is r = 8.
    const QList<qreal> pressed = MdButtonStyle::layoutFor(button, button.tokens()).radii;
    QVERIFY(!pressed.isEmpty());
    QVERIFY(near(pressed.first(), 8.0, 0.5));

    QTest::mouseRelease(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
    QTRY_VERIFY_WITH_TIMEOUT(near(button.pressMorph(), 0.0, 0.02), 2000);

    button.hide();
}

// ---------------------------------------------------------------------------
// 3. render smoke checks
// ---------------------------------------------------------------------------

void TestMd3Button::everyCombinationRendersSomething()
{
    // 5 variants x 5 sizes x 2 shapes. Every one of the 50 is a published token
    // set, so every one has to paint.
    for (int v = 0; v < int(ButtonVariant::Count); ++v) {
        for (int s = 0; s < int(ButtonSize::Count); ++s) {
            for (int sh = 0; sh < int(ButtonShape::Count); ++sh) {
                MdButton button(QStringLiteral("Button"));
                button.setVariant(ButtonVariant(v));
                button.setButtonSize(ButtonSize(s));
                button.setButtonShape(ButtonShape(sh));

                const QImage image = renderButton(button, button.sizeHint());
                const QByteArray where =
                    md::buttonVariantName(ButtonVariant(v)).toLatin1() + '/'
                    + md::buttonSizeName(ButtonSize(s)).toLatin1() + '/'
                    + md::buttonShapeName(ButtonShape(sh)).toLatin1();
                QVERIFY2(paintedPixels(image) > 50, where.constData());
            }
        }
    }

    // With icons the container has to grow, otherwise the icon is drawn outside
    // it — a defect that still renders "something" and would pass the loop above.
    MdButton plain(QStringLiteral("Send"));
    MdButton withIcon(QStringLiteral("Send"));
    withIcon.setLeadingIcon(QStringLiteral("send"));
    QVERIFY(withIcon.sizeHint().width() > plain.sizeHint().width());
}

void TestMd3Button::lightAndDarkPaintDifferentContainers()
{
    MdTheme &theme = MdTheme::instance();
    const ThemeMode original = theme.themeMode();

    MdButton button(QStringLiteral("Save"));
    button.setVariant(ButtonVariant::Filled);
    button.resize(button.sizeHint());
    button.show();
    QVERIFY(QTest::qWaitForWindowExposed(&button));

    theme.setThemeMode(ThemeMode::Light);
    const QImage light = button.grab().toImage();
    theme.setThemeMode(ThemeMode::Dark);
    const QImage dark = button.grab().toImage();

    QVERIFY(paintedPixels(light) > 50);
    QVERIFY(paintedPixels(dark) > 50);
    // The same geometry, a different fill: the schemes must not collapse.
    QVERIFY(light.pixel(light.width() / 2, light.height() / 2)
            != dark.pixel(dark.width() / 2, dark.height() / 2));

    theme.setThemeMode(original);
    button.hide();
}

void TestMd3Button::outlinedAndTextPaintNoContainer()
{
    // The token model first: a container-less variant resolves no container
    // colour at all, which is a stronger statement than "nothing was drawn".
    for (int i = 0; i < int(ButtonVariant::Count); ++i) {
        const auto variant = ButtonVariant(i);
        const MdButtonTokens tokens =
            MdButtonTokens::resolve(variant, ButtonSize::Small, ButtonShape::Round);
        const QColor container = MdButtonStyle::containerColor(tokens, MdButtonState::Enabled);
        const QByteArray where = md::buttonVariantName(variant).toLatin1();
        const bool expected =
            variant != ButtonVariant::Outlined && variant != ButtonVariant::Text;
        QVERIFY2(container.isValid() == expected, where.constData());
    }

    // And the render agrees. The probe sits inside the container, in the
    // leading padding, so it is clear of both the glyph and the focus ring —
    // the ring is drawn outside the container on purpose.
    const auto interiorDelta = [](ButtonVariant variant) {
        MdButton button(QStringLiteral("X"));
        button.setVariant(variant);
        button.clearFocus();
        const QImage image = flatten(renderButton(button, button.sizeHint()));

        const QRectF container = button.containerRect();
        const int x = int(container.left() + 3.0);
        const int y = int(container.center().y());
        // Guard the probe: if the container ever became narrower than the
        // padding the sample would silently move into the glyph.
        Q_ASSERT(x > 0 && x < container.right());
        return channelDelta(image.pixel(x, y), backdrop());
    };

    // A container-less variant paints at most a state-layer tint, which is
    // md.sys.state.hover at 8%.
    QVERIFY(interiorDelta(ButtonVariant::Text) < 40);
    QVERIFY(interiorDelta(ButtonVariant::Outlined) < 40);
    // A filled variant paints an opaque container there — the control that
    // proves the probe would notice a fill.
    QVERIFY(interiorDelta(ButtonVariant::Filled) > 80);
}

QTEST_MAIN(TestMd3Button)

#include "TestMd3Button.moc"
