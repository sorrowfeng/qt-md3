// MdFab — MD3 floating action buttons.
//
// The same three kinds of check the button families' suites make:
//
//   1. the token table, field by field, against the published
//      `md.comp.fab.*` sets — metrics and shape per size, the four colour
//      sets, the lowered elevation rows, and the disabled row that fills the
//      export's gap from the spec's state table.
//   2. the widget contract — properties, signals, geometry, the ripple on
//      press, and the `:focus-visible` rule.
//   3. a render smoke check across every variant x size x lowered
//      combination, because a style that silently paints nothing still
//      returns a clean QSize and passes 1 and 2.

#include "TestMd3Common.h"

#include "core/MdFabTokens.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "styles/MdFabStyle.h"
#include "widgets/MdFab.h"

#include <QtCore/QElapsedTimer>
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

QImage renderFab(MdFab &fab)
{
    fab.resize(fab.sizeHint());
    fab.show();
    QTest::qWaitForWindowExposed(&fab);
    const QPixmap pixmap = fab.grab();
    fab.hide();
    return pixmap.toImage();
}

int paintedPixels(const QImage &image)
{
    return mdtest::paintedPixelCount(image, image.pixel(0, 0));
}

} // namespace

class TestMd3Fab : public QObject
{
    Q_OBJECT

private slots:
    // --- token table -------------------------------------------------------

    void sizeTableMatchesTheExport();
    void colourSetsMatchTheExport();
    void loweredChangesElevationAndTheSurfaceContainer();
    void disabledRowFillsTheExportGapFromTheSpecTable();
    void overridesReachTheResolver();

    // --- widget contract ----------------------------------------------------

    void defaultGeometryMatchesTheTokens();
    void propertiesAndSignalsFire();
    void iconNameIsTheAccessibleName();
    void pressRunsTheRippleAndReleaseFadesIt();
    void pointerFocusShowsNoRingKeyboardFocusDoes();

    // --- render smoke -------------------------------------------------------

    void everyCombinationRendersInk();
};

void TestMd3Fab::sizeTableMatchesTheExport()
{
    // md.comp.fab.<size>: container.height/width, icon.size and
    // container.shape. Every number below is transcribed from the size files
    // (_md-comp-fab-small/medium(base)/large.scss).
    struct Row
    {
        FabSize size;
        qreal height;
        qreal width;
        qreal iconSize;
        ShapeCorner shape;
    };
    const Row rows[] = {
        {FabSize::Small, 40.0, 40.0, 24.0, ShapeCorner::Medium},
        {FabSize::Medium, 56.0, 56.0, 24.0, ShapeCorner::Large},
        {FabSize::Large, 96.0, 96.0, 36.0, ShapeCorner::ExtraLarge},
    };

    for (const Row &row : rows) {
        const MdFabTokens tokens = MdFabTokens::resolve(FabVariant::Primary, row.size, false);
        QCOMPARE(tokens.containerHeight, row.height);
        QCOMPARE(tokens.containerWidth, row.width);
        QCOMPARE(tokens.iconSize, row.iconSize);
        QVERIFY(tokens.containerShape == row.shape);
    }

    // The base set (md.comp.fab without a size segment) is the medium set.
    const MdFabTokens base = MdFabTokens::resolve(FabVariant::Primary, FabSize::Medium, false);
    QCOMPARE(base.containerHeight, 56.0);
    QVERIFY(base.containerShape == ShapeCorner::Large);
}

void TestMd3Fab::colourSetsMatchTheExport()
{
    // md.comp.fab.<variant>: container.color, icon.color and the (flat)
    // hovered/focused/pressed state-layer colour, which restates the enabled
    // icon role in every variant.
    struct Row
    {
        FabVariant variant;
        ColorRole container;
        ColorRole icon;
        ColorRole stateLayer;
    };
    const Row rows[] = {
        {FabVariant::Surface, ColorRole::SurfaceContainerHigh, ColorRole::Primary,
         ColorRole::Primary},
        {FabVariant::Primary, ColorRole::Primary, ColorRole::OnPrimary, ColorRole::OnPrimary},
        {FabVariant::Secondary, ColorRole::Secondary, ColorRole::OnSecondary,
         ColorRole::OnSecondary},
        {FabVariant::Tertiary, ColorRole::Tertiary, ColorRole::OnTertiary,
         ColorRole::OnTertiary},
    };

    for (const Row &row : rows) {
        const MdFabTokens tokens = MdFabTokens::resolve(row.variant, FabSize::Medium, false);
        QVERIFY(tokens.family.enabled.container.isPresent());
        QVERIFY(tokens.family.enabled.container.role == row.container);
        QVERIFY(tokens.family.enabled.icon.role == row.icon);

        // The interactive rows are flat: hovered, focused and pressed restate
        // the enabled roles.
        for (const MdFabState state :
             {MdFabState::Hovered, MdFabState::Focused, MdFabState::Pressed}) {
            QVERIFY(tokens.family.state(state).container.role == row.container);
            QVERIFY(tokens.family.state(state).icon.role == row.icon);
            QVERIFY(tokens.family.state(state).stateLayer == row.stateLayer);
        }
    }
}

void TestMd3Fab::loweredChangesElevationAndTheSurfaceContainer()
{
    // Raised: enabled/focused/pressed level3, hovered level4.
    const MdFabTokens raised = MdFabTokens::resolve(FabVariant::Primary, FabSize::Medium, false);
    QVERIFY(raised.family.elevation(MdFabState::Enabled) == ElevationLevel::Level3);
    QVERIFY(raised.family.elevation(MdFabState::Hovered) == ElevationLevel::Level4);
    QVERIFY(raised.family.elevation(MdFabState::Focused) == ElevationLevel::Level3);
    QVERIFY(raised.family.elevation(MdFabState::Pressed) == ElevationLevel::Level3);

    // Lowered: enabled/focused/pressed level1, hovered level2.
    const MdFabTokens lowered = MdFabTokens::resolve(FabVariant::Primary, FabSize::Medium, true);
    QVERIFY(lowered.family.elevation(MdFabState::Enabled) == ElevationLevel::Level1);
    QVERIFY(lowered.family.elevation(MdFabState::Hovered) == ElevationLevel::Level2);
    QVERIFY(lowered.family.elevation(MdFabState::Focused) == ElevationLevel::Level1);
    QVERIFY(lowered.family.elevation(MdFabState::Pressed) == ElevationLevel::Level1);

    // `md.comp.fab.surface.lowered.container.color` is surface-container-low;
    // the other variants publish no lowered container and keep the raised one.
    const MdFabTokens loweredSurface =
        MdFabTokens::resolve(FabVariant::Surface, FabSize::Medium, true);
    QVERIFY(loweredSurface.family.enabled.container.role == ColorRole::SurfaceContainerLow);

    const MdFabTokens loweredPrimary =
        MdFabTokens::resolve(FabVariant::Primary, FabSize::Medium, true);
    QVERIFY(loweredPrimary.family.enabled.container.role == ColorRole::Primary);
}

void TestMd3Fab::disabledRowFillsTheExportGapFromTheSpecTable()
{
    // The token export publishes no disabled rows; the spec's disabled state
    // table (the same disabled row every push-button family shows) fills
    // them: on-surface @12% container, on-surface @38% icon, level0, and no
    // state layer. Recorded in docs/porting-todo.md.
    const MdFabTokens tokens = MdFabTokens::resolve(FabVariant::Primary, FabSize::Medium, false);
    const MdFabStateColours &disabled = tokens.family.disabled;

    QVERIFY(disabled.container.isPresent());
    QVERIFY(disabled.container.role == ColorRole::OnSurface);
    QCOMPARE(disabled.container.opacity, 0.12);
    QVERIFY(disabled.icon.role == ColorRole::OnSurface);
    QCOMPARE(disabled.icon.opacity, 0.38);
    QVERIFY(disabled.stateLayer == ColorRole::Count);
    QVERIFY(tokens.family.elevation(MdFabState::Disabled) == ElevationLevel::Level0);
}

void TestMd3Fab::overridesReachTheResolver()
{
    MdComponentTokens overrides;
    overrides.setValue(QStringLiteral("md.comp.fab.small.container.height"),
                       QStringLiteral("48"));
    overrides.setValue(QStringLiteral("md.comp.fab.container.height"), QStringLiteral("44"));
    const MdFabTokens tokens =
        MdFabTokens::resolve(FabVariant::Primary, FabSize::Small, false, &overrides);
    // The size-qualified key wins over the generic one.
    QCOMPARE(tokens.containerHeight, 48.0);

    // A shape override accepts both spellings, like the other components.
    MdComponentTokens shapeOverrides;
    shapeOverrides.setValue(QStringLiteral("md.comp.fab.small.container.shape"),
                            QStringLiteral("corner-full"));
    const MdFabTokens shaped =
        MdFabTokens::resolve(FabVariant::Primary, FabSize::Small, false, &shapeOverrides);
    QVERIFY(shaped.containerShape == ShapeCorner::Full);
}

void TestMd3Fab::defaultGeometryMatchesTheTokens()
{
    MdFab fab(QStringLiteral("add"));
    fab.resize(fab.sizeHint());

    // The focus-indicator margin: offset 2 + activeWidth/2 + width/2 = 7.5 px
    // on a 56 px container → a 71 px size hint, container centred.
    const MdFabTokens tokens = fab.tokens();
    const qreal inset = MdFabStyle::focusRingInset(tokens);
    QCOMPARE(inset, 7.5);

    QCOMPARE(fab.sizeHint(), QSize(71, 71));
    const QRectF container = fab.containerRect();
    QCOMPARE(container.size(), QSizeF(56.0, 56.0));
    QCOMPARE(container.topLeft(), QPointF(7.5, 7.5));

    // The large size resizes the hint with the container plus the same margin.
    fab.setFabSize(FabSize::Large);
    QCOMPARE(fab.sizeHint(), QSize(111, 111));
}

void TestMd3Fab::propertiesAndSignalsFire()
{
    MdFab fab(QStringLiteral("add"));

    QSignalSpy variantSpy(&fab, &MdFab::variantChanged);
    QSignalSpy sizeSpy(&fab, &MdFab::fabSizeChanged);
    QSignalSpy loweredSpy(&fab, &MdFab::loweredChanged);
    QSignalSpy iconSpy(&fab, &MdFab::iconNameChanged);

    fab.setVariant(FabVariant::Tertiary);
    QCOMPARE(fab.variant(), FabVariant::Tertiary);
    QCOMPARE(variantSpy.count(), 1);

    fab.setFabSize(FabSize::Small);
    QCOMPARE(fab.fabSize(), FabSize::Small);
    QCOMPARE(sizeSpy.count(), 1);

    fab.setLowered(true);
    QVERIFY(fab.isLowered());
    QCOMPARE(loweredSpy.count(), 1);

    fab.setIconName(QStringLiteral("edit"));
    QCOMPARE(fab.iconName(), QStringLiteral("edit"));
    QCOMPARE(iconSpy.count(), 1);

    // No-op writes stay silent.
    fab.setVariant(FabVariant::Tertiary);
    fab.setFabSize(FabSize::Small);
    fab.setLowered(true);
    QCOMPARE(variantSpy.count(), 1);
    QCOMPARE(sizeSpy.count(), 1);
    QCOMPARE(loweredSpy.count(), 1);
}

void TestMd3Fab::iconNameIsTheAccessibleName()
{
    MdFab fab(QStringLiteral("add"));
    QCOMPARE(fab.accessibleName(), QStringLiteral("add"));
    QCOMPARE(fab.toolTip(), QStringLiteral("add"));

    fab.setIconName(QStringLiteral("search"));
    QCOMPARE(fab.accessibleName(), QStringLiteral("search"));
}

void TestMd3Fab::pressRunsTheRippleAndReleaseFadesIt()
{
    MdFab fab(QStringLiteral("add"));
    fab.resize(fab.sizeHint());
    fab.show();
    QVERIFY(QTest::qWaitForWindowExposed(&fab));

    // Press at the centre: the ripple starts and produces frames.
    QTest::mousePress(&fab, Qt::LeftButton, Qt::NoModifier, fab.rect().center());
    QVERIFY(fab.isDown());
    QVERIFY(fab.rippleController() != nullptr);
    QVERIFY(fab.rippleController()->currentFrame().valid);

    // Release: the ripple moves to its fade-out phase, still animated.
    QTest::mouseRelease(&fab, Qt::LeftButton, Qt::NoModifier, fab.rect().center());
    QVERIFY(!fab.isDown());

    // After the fade completes the frame goes invalid. The controller's
    // lifetime is max(press, minimum-press 225 ms) + fade 375 ms = 600 ms,
    // measured in its own 16 ms ticks — which can lag the wall clock — so
    // poll for the fade to finish rather than sleeping a fixed amount.
    QElapsedTimer fadeClock;
    fadeClock.start();
    while (fab.rippleController()->isActive() && fadeClock.elapsed() < 2000) {
        QTest::qWait(50);
    }
    QVERIFY(!fab.rippleController()->isActive());
    QVERIFY(!fab.rippleController()->currentFrame().valid);
    fab.hide();
}

void TestMd3Fab::pointerFocusShowsNoRingKeyboardFocusDoes()
{
    MdFab fab(QStringLiteral("add"));
    fab.resize(fab.sizeHint());
    fab.show();
    QVERIFY(QTest::qWaitForWindowExposed(&fab));

    // `:focus-visible` semantics, pinned here so a regression back to
    // "any focus shows the ring" cannot slip through.
    QTest::mouseClick(&fab, Qt::LeftButton);
    QVERIFY(fab.hasFocus());
    QVERIFY(!fab.hasKeyboardFocus());

    // setFocus() on a widget that already holds focus does not re-deliver
    // focusInEvent, so drop it first.
    fab.clearFocus();
    fab.setFocus(Qt::TabFocusReason);
    QVERIFY(fab.hasKeyboardFocus());

    fab.clearFocus();
    fab.setFocus(Qt::MouseFocusReason);
    QVERIFY(fab.hasFocus());
    QVERIFY(!fab.hasKeyboardFocus());

    fab.hide();
}

void TestMd3Fab::everyCombinationRendersInk()
{
    // 4 colour sets x 3 sizes x lowered/raised — every combination paints a
    // container (a FAB has no container-less variant), so every grab must be
    // more than half ink.
    for (const FabVariant variant :
         {FabVariant::Surface, FabVariant::Primary, FabVariant::Secondary, FabVariant::Tertiary}) {
        for (const FabSize size : {FabSize::Small, FabSize::Medium, FabSize::Large}) {
            for (const bool lowered : {false, true}) {
                MdFab fab(QStringLiteral("add"));
                fab.setVariant(variant);
                fab.setFabSize(size);
                fab.setLowered(lowered);
                const QImage image = renderFab(fab);
                const int ink = paintedPixels(image);
                // Report the combination so a failure names its case.
                const QString label = QStringLiteral("%1/%2/%3")
                                          .arg(fabVariantName(variant), fabSizeName(size),
                                               lowered ? "lowered" : "raised");
                QVERIFY2(ink > image.width() * image.height() / 2,
                         qPrintable(QStringLiteral("%1: only %2 ink pixels of %3")
                                        .arg(label)
                                        .arg(ink)
                                        .arg(image.width() * image.height())));
            }
        }
    }
}

QTEST_MAIN(TestMd3Fab)
#include "TestMd3Fab.moc"
