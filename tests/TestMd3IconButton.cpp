// MdIconButton — MD3 icon buttons.
//
// The same three kinds of check the common button's suite makes, scaled to a
// component whose colour matrix is three families deep:
//
//   1. the token table, field by field, against the published
//      `md.comp.icon-button.*` sets — metrics and shapes per size, and the
//      plain / selected / unselected colour families per style.
//   2. the widget contract — properties, signals, the toggle/selected pair,
//      the shape the selection implies, and the press morph.
//   3. a render smoke check across every style x size x shape combination,
//      because a style that silently paints nothing still returns a clean
//      QSize and passes 1 and 2.

#include "TestMd3Common.h"

#include "core/MdIconButtonTokens.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "styles/MdIconButtonStyle.h"
#include "widgets/MdIconButton.h"

#include <QtGui/QImage>
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

QImage renderButton(MdIconButton &button, const QSize &size)
{
    button.resize(size);
    button.show();
    QTest::qWaitForWindowExposed(&button);
    const QPixmap pixmap = button.grab();
    button.hide();
    return pixmap.toImage();
}

int paintedPixels(const QImage &image)
{
    return mdtest::paintedPixelCount(image, image.pixel(0, 0));
}

/// The radius a shape token resolves to for a square box of `height`.
qreal resolvedRadius(ShapeCorner corner, qreal height)
{
    return MdShape::resolvedRadius(corner, QSizeF(height, height));
}

} // namespace

class TestMd3IconButton : public QObject
{
    Q_OBJECT

private slots:
    // --- token table -------------------------------------------------------

    void sizeTableMatchesTheExport();
    void standardHasNoContainerAndSelectionTurnsTheIconPrimary();
    void filledToggleShowsTheSurfaceContainerChipWhenUnchecked();
    void tonalSelectedFillsWithSecondary();
    void outlinedSelectedFillsWithInverseSurfaceAndDropsTheStroke();
    void familiesFallBackToPlainForNonToggles();
    void overridesReachTheResolver();

    // --- widget contract ----------------------------------------------------

    void defaultGeometryMatchesTheTokens();
    void selectionSwapsTheRestingCorners();
    void pressMorphRunsAndSettles();
    void toggleableDrivesSelectedAndTheSignals();
    void iconNameIsTheAccessibleName();
    void spaceTrackChangesTheContainerWidth();

    // --- render smoke -------------------------------------------------------

    void everyCombinationRendersInk();
};

void TestMd3IconButton::sizeTableMatchesTheExport()
{
    // md.comp.icon-button.<size>: container.height, icon.size, the three
    // space tracks, outlined.outline.width and the five shape slots. Every
    // number below is transcribed from the size files.
    struct Row
    {
        ButtonSize size;
        qreal height;
        qreal iconSize;
        qreal defaultSpace;
        qreal narrowSpace;
        qreal wideSpace;
        qreal outlineWidth;
        ShapeCorner squareResting;
        ShapeCorner pressed;
        ShapeCorner selectedRound;
    };
    const Row rows[] = {
        {ButtonSize::XSmall, 32.0, 20.0, 6.0, 4.0, 10.0, 1.0, ShapeCorner::Medium,
         ShapeCorner::Small, ShapeCorner::Medium},
        {ButtonSize::Small, 40.0, 24.0, 8.0, 4.0, 14.0, 1.0, ShapeCorner::Medium,
         ShapeCorner::Small, ShapeCorner::Medium},
        {ButtonSize::Medium, 56.0, 24.0, 16.0, 12.0, 24.0, 1.0, ShapeCorner::Large,
         ShapeCorner::Medium, ShapeCorner::Large},
        {ButtonSize::Large, 96.0, 32.0, 32.0, 16.0, 48.0, 2.0, ShapeCorner::ExtraLarge,
         ShapeCorner::Large, ShapeCorner::ExtraLarge},
        {ButtonSize::XLarge, 136.0, 40.0, 48.0, 32.0, 72.0, 3.0, ShapeCorner::ExtraLarge,
         ShapeCorner::Large, ShapeCorner::ExtraLarge},
    };

    for (const Row &row : rows) {
        const MdIconButtonTokens filled =
            MdIconButtonTokens::resolve(IconButtonVariant::Filled, row.size, ButtonShape::Round,
                                        IconButtonSpaceTrack::Default);
        QCOMPARE(filled.containerHeight, row.height);
        QCOMPARE(filled.iconSize, row.iconSize);
        QCOMPARE(filled.defaultLeadingSpace, row.defaultSpace);
        QCOMPARE(filled.defaultTrailingSpace, row.defaultSpace);
        QCOMPARE(filled.narrowLeadingSpace, row.narrowSpace);
        QCOMPARE(filled.wideLeadingSpace, row.wideSpace);
        QCOMPARE(filled.outlineWidth, row.outlineWidth);
        QVERIFY(filled.roundShape == ShapeCorner::Full);
        QVERIFY(filled.squareShape == row.squareResting);
        QVERIFY(filled.pressedShape == row.pressed);
        QVERIFY(filled.selectedRoundShape == row.selectedRound);
        QVERIFY(filled.selectedSquareShape == ShapeCorner::Full);
        QCOMPARE(filled.springStiffness, 1400.0);
        QCOMPARE(filled.springDampingRatio, 0.9);
        QCOMPARE(filled.focusIndicator, ColorRole::Secondary);
        QCOMPARE(filled.focusIndicatorThickness, 3.0);
        QCOMPARE(filled.focusIndicatorOffset, 2.0);

        // The default track's arithmetic is the square: space + icon + space.
        QVERIFY(near(filled.defaultLeadingSpace + filled.iconSize + filled.defaultTrailingSpace,
                     filled.containerHeight));
    }
}

void TestMd3IconButton::standardHasNoContainerAndSelectionTurnsTheIconPrimary()
{
    const MdIconButtonTokens standard =
        MdIconButtonTokens::resolve(IconButtonVariant::Standard, ButtonSize::Small,
                                    ButtonShape::Round, IconButtonSpaceTrack::Default);

    // md.comp.icon-button.standard: icon-color on-surface-variant, no
    // container token at all — absence is the published fact.
    QVERIFY(!standard.plain.enabled.container.isPresent());
    QVERIFY(standard.plain.enabled.icon.role == ColorRole::OnSurfaceVariant);
    QVERIFY(standard.plain.enabled.stateLayer == ColorRole::OnSurfaceVariant);
    QVERIFY(!standard.plain.disabled.container.isPresent());
    QVERIFY(standard.plain.disabled.icon.role == ColorRole::OnSurface);
    QCOMPARE(standard.plain.disabled.icon.opacity, 0.38);

    // selected-*: icon and state layer primary, and still no container.
    QVERIFY(!standard.selected.enabled.container.isPresent());
    QVERIFY(standard.selected.enabled.icon.role == ColorRole::Primary);
    QVERIFY(standard.selected.enabled.stateLayer == ColorRole::Primary);
    QVERIFY(standard.selected.disabled.icon.role == ColorRole::OnSurface);
}

void TestMd3IconButton::filledToggleShowsTheSurfaceContainerChipWhenUnchecked()
{
    const MdIconButtonTokens filled =
        MdIconButtonTokens::resolve(IconButtonVariant::Filled, ButtonSize::Small,
                                    ButtonShape::Round, IconButtonSpaceTrack::Default);

    // The plain family and the base file agree: primary + on-primary.
    QVERIFY(filled.plain.enabled.container.role == ColorRole::Primary);
    QVERIFY(filled.plain.enabled.icon.role == ColorRole::OnPrimary);
    QVERIFY(filled.plain.enabled.stateLayer == ColorRole::OnPrimary);

    // md.comp.icon-button.filled.unselected.container.color:
    // surface-container with an on-surface-variant icon.
    QVERIFY(filled.unselected.enabled.container.role == ColorRole::SurfaceContainer);
    QVERIFY(filled.unselected.enabled.icon.role == ColorRole::OnSurfaceVariant);
    QVERIFY(filled.unselected.enabled.stateLayer == ColorRole::OnSurfaceVariant);

    // The checked toggle is the plain chip again: primary + on-primary.
    QVERIFY(filled.selected.enabled.container.role == ColorRole::Primary);
    QVERIFY(filled.selected.enabled.icon.role == ColorRole::OnPrimary);

    // Disabled: on-surface at the two published opacities, container only
    // where a container exists.
    QVERIFY(filled.plain.disabled.container.role == ColorRole::OnSurface);
    QCOMPARE(filled.plain.disabled.container.opacity, 0.10);
    QVERIFY(filled.unselected.disabled.container.role == ColorRole::OnSurface);
    QVERIFY(filled.selected.disabled.container.role == ColorRole::OnSurface);
    QCOMPARE(filled.selected.disabled.container.opacity, 0.10);
}

void TestMd3IconButton::tonalSelectedFillsWithSecondary()
{
    const MdIconButtonTokens tonal =
        MdIconButtonTokens::resolve(IconButtonVariant::Tonal, ButtonSize::Small,
                                    ButtonShape::Round, IconButtonSpaceTrack::Default);

    QVERIFY(tonal.plain.enabled.container.role == ColorRole::SecondaryContainer);
    QVERIFY(tonal.plain.enabled.icon.role == ColorRole::OnSecondaryContainer);
    QVERIFY(tonal.selected.enabled.container.role == ColorRole::Secondary);
    QVERIFY(tonal.selected.enabled.icon.role == ColorRole::OnSecondary);
    // md.comp.icon-button.tonal.unselected.* re-state what the plain family
    // already says, and the transcription keeps them.
    QVERIFY(tonal.unselected.enabled.container.role == ColorRole::SecondaryContainer);
}

void TestMd3IconButton::outlinedSelectedFillsWithInverseSurfaceAndDropsTheStroke()
{
    const MdIconButtonTokens outlined =
        MdIconButtonTokens::resolve(IconButtonVariant::Outlined, ButtonSize::Small,
                                    ButtonShape::Round, IconButtonSpaceTrack::Default);

    QVERIFY(!outlined.plain.enabled.container.isPresent());
    QVERIFY(outlined.plain.enabled.outline == ColorRole::OutlineVariant);
    QVERIFY(outlined.plain.enabled.icon.role == ColorRole::OnSurfaceVariant);
    QCOMPARE(outlined.outlineWidth, 1.0);

    // The checked toggle: inverse-surface chip, inverse-on-surface icon, and
    // no outline token in the selected set — which is a real absence, the
    // checked button is a filled chip rather than a stroked one.
    QVERIFY(outlined.selected.enabled.container.role == ColorRole::InverseSurface);
    QVERIFY(outlined.selected.enabled.icon.role == ColorRole::InverseOnSurface);
    QVERIFY(outlined.selected.enabled.outline == ColorRole::Count);

    // It is also the one style with selected-disabled container tokens:
    // on-surface at the published 0.1 opacity.
    QVERIFY(outlined.selected.disabled.container.role == ColorRole::OnSurface);
    QCOMPARE(outlined.selected.disabled.container.opacity, 0.10);

    // The unchecked toggle keeps its stroke while disabled.
    QVERIFY(outlined.unselected.disabled.outline == ColorRole::OutlineVariant);
}

void TestMd3IconButton::familiesFallBackToPlainForNonToggles()
{
    const MdIconButtonTokens tokens =
        MdIconButtonTokens::resolve(IconButtonVariant::Filled, ButtonSize::Small,
                                    ButtonShape::Round, IconButtonSpaceTrack::Default);
    // A non-toggle paints the plain family whichever way `selected` points —
    // familyFor is the only place that choice is made.
    QVERIFY(tokens.familyFor(false, false).enabled.container.role
            == tokens.plain.enabled.container.role);
    QVERIFY(tokens.familyFor(false, true).enabled.container.role
            == tokens.plain.enabled.container.role);
    QVERIFY(tokens.familyFor(true, true).enabled.container.role == ColorRole::Primary);
    QVERIFY(tokens.familyFor(true, false).enabled.container.role == ColorRole::SurfaceContainer);
}

void TestMd3IconButton::overridesReachTheResolver()
{
    MdComponentTokens overrides;
    overrides.setValue(QStringLiteral("md.comp.icon-button.small.container.height"),
                       QStringLiteral("48"));
    overrides.setValue(QStringLiteral("md.comp.icon-button.container.height"),
                       QStringLiteral("44"));
    const MdIconButtonTokens tokens =
        MdIconButtonTokens::resolve(IconButtonVariant::Filled, ButtonSize::Small,
                                    ButtonShape::Round, IconButtonSpaceTrack::Default,
                                    &overrides);
    // The size-qualified key wins over the generic one.
    QCOMPARE(tokens.containerHeight, 48.0);

    // A shape override accepts both spellings, like the other components.
    MdComponentTokens shapeOverrides;
    shapeOverrides.setValue(QStringLiteral("md.comp.icon-button.small.container.shape.round"),
                            QStringLiteral("corner-large"));
    const MdIconButtonTokens shaped =
        MdIconButtonTokens::resolve(IconButtonVariant::Filled, ButtonSize::Small,
                                    ButtonShape::Round, IconButtonSpaceTrack::Default,
                                    &shapeOverrides);
    QVERIFY(shaped.roundShape == ShapeCorner::Large);
}

void TestMd3IconButton::defaultGeometryMatchesTheTokens()
{
    MdIconButton button;
    button.setVariant(IconButtonVariant::Filled);
    button.setIconName(QStringLiteral("add"));
    button.resize(button.sizeHint());

    const MdIconButtonTokens &tokens = button.tokens();
    const MdIconButtonStyle::Layout layout = MdIconButtonStyle::layoutFor(button, tokens);
    const qreal inset = MdIconButtonStyle::focusRingInset(tokens);

    // The widget is the square container plus the focus margin on every side;
    // for a small icon button that is 40 + 2 * 7.5.
    QCOMPARE(button.sizeHint().width(), int(std::ceil(40.0 + 2.0 * inset)));
    QCOMPARE(button.sizeHint().height(), int(std::ceil(40.0 + 2.0 * inset)));
    QVERIFY(near(layout.container.width(), 40.0));
    QVERIFY(near(layout.container.height(), 40.0));

    // The icon box is the token icon size, centred.
    QVERIFY(near(layout.icon.width(), 24.0));
    QVERIFY(near(layout.icon.height(), 24.0));
    QVERIFY(near(layout.icon.center().x(), layout.container.center().x()));
    QVERIFY(near(layout.icon.center().y(), layout.container.center().y()));

    // The resting corners of the round shape are the token full corner: half
    // the box, a pill.
    const QList<qreal> radii = layout.radii;
    QCOMPARE(radii.size(), 4);
    QVERIFY(near(radii.first(), resolvedRadius(ShapeCorner::Full, 40.0)));
}

void TestMd3IconButton::selectionSwapsTheRestingCorners()
{
    MdIconButton button;
    button.setVariant(IconButtonVariant::Filled);
    button.setIconName(QStringLiteral("add"));
    button.setToggleable(true);
    button.resize(button.sizeHint());

    // Unchecked round: corner-full, the pill.
    qreal radius = MdIconButtonStyle::layoutFor(button, button.tokens()).radii.first();
    QVERIFY(near(radius, resolvedRadius(ShapeCorner::Full, 40.0)));

    // Checked round: the published *other* knob — selected round is
    // corner-medium, 12 px for this box.
    button.setSelected(true);
    radius = MdIconButtonStyle::layoutFor(button, button.tokens()).radii.first();
    QVERIFY(near(radius, resolvedRadius(ShapeCorner::Medium, 40.0)));

    // The square knob follows the same swap: resting square is corner-medium,
    // selected square is corner-full.
    button.setSelected(false);
    button.setButtonShape(ButtonShape::Square);
    radius = MdIconButtonStyle::layoutFor(button, button.tokens()).radii.first();
    QVERIFY(near(radius, resolvedRadius(ShapeCorner::Medium, 40.0)));
    button.setSelected(true);
    radius = MdIconButtonStyle::layoutFor(button, button.tokens()).radii.first();
    QVERIFY(near(radius, resolvedRadius(ShapeCorner::Full, 40.0)));
}

void TestMd3IconButton::pressMorphRunsAndSettles()
{
    MdIconButton button;
    button.setVariant(IconButtonVariant::Filled);
    button.setIconName(QStringLiteral("add"));
    button.resize(button.sizeHint());
    button.show();
    QTest::qWaitForWindowExposed(&button);

    QCOMPARE(button.pressMorph(), 0.0);
    QTest::mousePress(&button, Qt::LeftButton);
    QVERIFY(button.isDown());
    // The spring takes a few frames; it must be on its way immediately.
    QTest::qWait(50);
    QVERIFY(button.pressMorph() > 0.1);

    QTest::mouseRelease(&button, Qt::LeftButton);
    // ...and back. The settle bound is what keeps this from spinning forever.
    QTest::qWait(600);
    QVERIFY(qFuzzyCompare(button.pressMorph(), 0.0));

    button.hide();
}

void TestMd3IconButton::toggleableDrivesSelectedAndTheSignals()
{
    MdIconButton button;
    button.setVariant(IconButtonVariant::Filled);
    button.setIconName(QStringLiteral("bookmark"));
    button.resize(button.sizeHint());

    QSignalSpy toggleSpy(&button, &MdIconButton::toggleableChanged);
    QSignalSpy selectedSpy(&button, &MdIconButton::selectedChanged);

    QVERIFY(!button.isToggleable());
    QVERIFY(!button.isSelected());

    button.setToggleable(true);
    QCOMPARE(toggleSpy.count(), 1);
    QVERIFY(button.isToggleable());
    // QAbstractButton's checkable machinery is what backs the selection, so a
    // click toggles it without any second code path here.
    QTest::mouseClick(&button, Qt::LeftButton);
    QVERIFY(button.isSelected());
    QCOMPARE(selectedSpy.count(), 1);
    QVERIFY(button.isChecked());

    button.setSelected(false);
    QCOMPARE(selectedSpy.count(), 2);
    QVERIFY(!button.isSelected());
}

void TestMd3IconButton::iconNameIsTheAccessibleName()
{
    MdIconButton button;
    button.setIconName(QStringLiteral("settings"));
    QCOMPARE(button.iconName(), QStringLiteral("settings"));
    // An icon-only control with no accessible name is unusable with a screen
    // reader; the icon name doubles as it.
    QCOMPARE(button.accessibleName(), QStringLiteral("settings"));
}

void TestMd3IconButton::spaceTrackChangesTheContainerWidth()
{
    MdIconButton button;
    button.setVariant(IconButtonVariant::Filled);
    button.setIconName(QStringLiteral("add"));
    button.resize(button.sizeHint());

    const qreal defaultWidth = button.containerRect().width();
    QVERIFY(near(defaultWidth, 40.0));

    // md.comp.icon-button.<size>.narrow-leading/trailing-space: 4 px at small,
    // so the container narrows while the height stays the token height.
    button.setSpaceTrack(IconButtonSpaceTrack::Narrow);
    QVERIFY(button.containerRect().width() < defaultWidth);
    QVERIFY(near(button.containerRect().height(), 40.0));
    QVERIFY(near(button.containerRect().width(), 4.0 + 24.0 + 4.0));

    // md.comp.icon-button.<size>.wide-leading/trailing-space: 14 px at small.
    button.setSpaceTrack(IconButtonSpaceTrack::Wide);
    QVERIFY(near(button.containerRect().width(), 14.0 + 24.0 + 14.0));
}

void TestMd3IconButton::everyCombinationRendersInk()
{
    // 4 styles x 5 sizes x 2 shapes, each rendered in the enabled and the
    // selected toggle state. Ink somewhere in the frame is the claim: the
    // standard variant paints only an icon, the others paint a container, and
    // a regression that silently paints nothing fails here.
    int combinations = 0;
    for (int v = 0; v < int(IconButtonVariant::Count); ++v) {
        for (int s = 0; s < int(ButtonSize::Count); ++s) {
            for (int shape = 0; shape < int(ButtonShape::Count); ++shape) {
                MdIconButton button;
                button.setVariant(IconButtonVariant(v));
                button.setButtonSize(ButtonSize(s));
                button.setButtonShape(ButtonShape(shape));
                button.setIconName(QStringLiteral("add"));
                const QImage unselected = renderButton(button, button.sizeHint());
                QVERIFY(paintedPixels(unselected) > 0);

                button.setToggleable(true);
                button.setSelected(true);
                const QImage selected = renderButton(button, button.sizeHint());
                QVERIFY(paintedPixels(selected) > 0);
                ++combinations;
            }
        }
    }
    QCOMPARE(combinations, 40);
}

QTEST_MAIN(TestMd3IconButton)
#include "TestMd3IconButton.moc"
