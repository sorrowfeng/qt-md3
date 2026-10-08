// MdExtendedFab — MD3 extended floating action buttons.
//
// The same three kinds of check the button families' suites make:
//
//   1. the token table, field by field, against the published
//      `md.comp.extended-fab.*` sets — metrics, shape and label type scale
//      per size, the six colour sets, the lowered elevation rows, and the
//      disabled row that fills the export's gap from the spec's state table.
//   2. the widget contract — properties, signals, content-derived geometry,
//      the ripple on press, and the `:focus-visible` rule.
//   3. a render smoke check across every colour set x size x lowered
//      combination, because a style that silently paints nothing still
//      returns a clean QSize and passes 1 and 2.

#include "TestMd3Common.h"

#include "core/MdExtendedFabTokens.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdExtendedFabStyle.h"
#include "widgets/MdExtendedFab.h"

#include <QtCore/QElapsedTimer>
#include <QtGui/QFontMetrics>
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

QImage renderExtendedFab(MdExtendedFab &fab)
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

class TestMd3ExtendedFab : public QObject
{
    Q_OBJECT

private slots:
    // --- token table -------------------------------------------------------

    void sizeTableMatchesTheExport();
    void colourSetsMatchTheExport();
    void loweredChangesElevationOnly();
    void disabledRowFillsTheExportGapFromTheSpecTable();
    void overridesReachTheResolver();

    // --- widget contract ----------------------------------------------------

    void defaultGeometryMatchesTheTokens();
    void propertiesAndSignalsFire();
    void iconAndLabelAreTheAccessibleName();
    void pressRunsTheRippleAndReleaseFadesIt();
    void pointerFocusShowsNoRingKeyboardFocusDoes();

    // --- render smoke -------------------------------------------------------

    void everyCombinationRendersInk();
};

void TestMd3ExtendedFab::sizeTableMatchesTheExport()
{
    // md.comp.extended-fab.<size>: container.height, icon.size,
    // icon.label.space, leading/trailing.space, container.shape and the label
    // type-scale row. Every number below is transcribed from the size files
    // (_md-comp-extended-fab-small/medium/large.scss).
    struct Row
    {
        ExtendedFabSize size;
        qreal height;
        qreal iconSize;
        qreal iconLabelSpace;
        qreal leadingSpace;
        qreal trailingSpace;
        ShapeCorner shape;
        TypeStyle labelStyle;
    };
    const Row rows[] = {
        {ExtendedFabSize::Small, 56.0, 24.0, 8.0, 16.0, 16.0, ShapeCorner::Large,
         TypeStyle::TitleMedium},
        {ExtendedFabSize::Medium, 80.0, 28.0, 12.0, 26.0, 26.0, ShapeCorner::LargeIncreased,
         TypeStyle::TitleLarge},
        {ExtendedFabSize::Large, 96.0, 36.0, 16.0, 28.0, 28.0, ShapeCorner::ExtraLarge,
         TypeStyle::HeadlineSmall},
    };

    for (const Row &row : rows) {
        const MdExtendedFabTokens tokens =
            MdExtendedFabTokens::resolve(ExtendedFabVariant::Primary, row.size, false);
        QCOMPARE(tokens.containerHeight, row.height);
        QCOMPARE(tokens.iconSize, row.iconSize);
        QCOMPARE(tokens.iconLabelSpace, row.iconLabelSpace);
        QCOMPARE(tokens.leadingSpace, row.leadingSpace);
        QCOMPARE(tokens.trailingSpace, row.trailingSpace);
        QVERIFY(tokens.containerShape == row.shape);
        QVERIFY(tokens.labelStyle == row.labelStyle);
    }
}

void TestMd3ExtendedFab::colourSetsMatchTheExport()
{
    // md.comp.extended-fab.<variant>: container.color, icon.color and the
    // (flat) hovered/focused/pressed state-layer colour, which restates the
    // enabled roles in every set. In every published set the icon, the label
    // text and the state layer share one role.
    struct Row
    {
        ExtendedFabVariant variant;
        ColorRole container;
        ColorRole content;
    };
    const Row rows[] = {
        {ExtendedFabVariant::Primary, ColorRole::Primary, ColorRole::OnPrimary},
        {ExtendedFabVariant::Secondary, ColorRole::Secondary, ColorRole::OnSecondary},
        {ExtendedFabVariant::Tertiary, ColorRole::Tertiary, ColorRole::OnTertiary},
        {ExtendedFabVariant::PrimaryContainer, ColorRole::PrimaryContainer,
         ColorRole::OnPrimaryContainer},
        {ExtendedFabVariant::SecondaryContainer, ColorRole::SecondaryContainer,
         ColorRole::OnSecondaryContainer},
        {ExtendedFabVariant::TertiaryContainer, ColorRole::TertiaryContainer,
         ColorRole::OnTertiaryContainer},
    };

    for (const Row &row : rows) {
        const MdExtendedFabTokens tokens =
            MdExtendedFabTokens::resolve(row.variant, ExtendedFabSize::Small, false);
        QVERIFY(tokens.family.enabled.container.isPresent());
        QVERIFY(tokens.family.enabled.container.role == row.container);
        QVERIFY(tokens.family.enabled.icon.role == row.content);

        // The interactive rows are flat: hovered, focused and pressed restate
        // the enabled roles.
        for (const MdFabState state :
             {MdFabState::Hovered, MdFabState::Focused, MdFabState::Pressed}) {
            QVERIFY(tokens.family.state(state).container.role == row.container);
            QVERIFY(tokens.family.state(state).icon.role == row.content);
            QVERIFY(tokens.family.state(state).stateLayer == row.content);
        }
    }
}

void TestMd3ExtendedFab::loweredChangesElevationOnly()
{
    // Raised: enabled/focused/pressed level3, hovered level4.
    const MdExtendedFabTokens raised =
        MdExtendedFabTokens::resolve(ExtendedFabVariant::Primary, ExtendedFabSize::Small, false);
    QVERIFY(raised.family.elevation(MdFabState::Enabled) == ElevationLevel::Level3);
    QVERIFY(raised.family.elevation(MdFabState::Hovered) == ElevationLevel::Level4);
    QVERIFY(raised.family.elevation(MdFabState::Focused) == ElevationLevel::Level3);
    QVERIFY(raised.family.elevation(MdFabState::Pressed) == ElevationLevel::Level3);

    // Lowered: enabled/focused/pressed level1, hovered level2.
    const MdExtendedFabTokens lowered =
        MdExtendedFabTokens::resolve(ExtendedFabVariant::Primary, ExtendedFabSize::Small, true);
    QVERIFY(lowered.family.elevation(MdFabState::Enabled) == ElevationLevel::Level1);
    QVERIFY(lowered.family.elevation(MdFabState::Hovered) == ElevationLevel::Level2);
    QVERIFY(lowered.family.elevation(MdFabState::Focused) == ElevationLevel::Level1);
    QVERIFY(lowered.family.elevation(MdFabState::Pressed) == ElevationLevel::Level1);

    // Unlike the FAB family there is no lowered container colour: lowered
    // changes elevation only, for every colour set.
    for (const ExtendedFabVariant variant :
         {ExtendedFabVariant::Primary, ExtendedFabVariant::PrimaryContainer}) {
        QVERIFY(MdExtendedFabTokens::resolve(variant, ExtendedFabSize::Small, false)
                    .family.enabled.container.role
                == MdExtendedFabTokens::resolve(variant, ExtendedFabSize::Small, true)
                       .family.enabled.container.role);
    }
}

void TestMd3ExtendedFab::disabledRowFillsTheExportGapFromTheSpecTable()
{
    // The token export publishes no disabled rows; the spec's disabled state
    // table (the same disabled row every push-button family shows) fills
    // them: on-surface @12% container, on-surface @38% icon and label,
    // level0, and no state layer. Recorded in docs/porting-todo.md.
    const MdExtendedFabTokens tokens =
        MdExtendedFabTokens::resolve(ExtendedFabVariant::Primary, ExtendedFabSize::Small, false);
    const MdFabStateColours &disabled = tokens.family.disabled;

    QVERIFY(disabled.container.isPresent());
    QVERIFY(disabled.container.role == ColorRole::OnSurface);
    QCOMPARE(disabled.container.opacity, 0.12);
    QVERIFY(disabled.icon.role == ColorRole::OnSurface);
    QCOMPARE(disabled.icon.opacity, 0.38);
    QVERIFY(disabled.stateLayer == ColorRole::Count);
    QVERIFY(tokens.family.elevation(MdFabState::Disabled) == ElevationLevel::Level0);
}

void TestMd3ExtendedFab::overridesReachTheResolver()
{
    MdComponentTokens overrides;
    overrides.setValue(QStringLiteral("md.comp.extended-fab.small.container.height"),
                       QStringLiteral("64"));
    overrides.setValue(QStringLiteral("md.comp.extended-fab.container.height"),
                       QStringLiteral("60"));
    const MdExtendedFabTokens tokens = MdExtendedFabTokens::resolve(
        ExtendedFabVariant::Primary, ExtendedFabSize::Small, false, &overrides);
    // The size-qualified key wins over the generic one.
    QCOMPARE(tokens.containerHeight, 64.0);

    // The spacing metrics are overridable too.
    MdComponentTokens spaceOverrides;
    spaceOverrides.setValue(QStringLiteral("md.comp.extended-fab.small.leading.space"),
                            QStringLiteral("24"));
    const MdExtendedFabTokens spaced =
        MdExtendedFabTokens::resolve(ExtendedFabVariant::Primary, ExtendedFabSize::Small, false,
                                     &spaceOverrides);
    QCOMPARE(spaced.leadingSpace, 24.0);

    // A shape override accepts both spellings, like the other components.
    MdComponentTokens shapeOverrides;
    shapeOverrides.setValue(QStringLiteral("md.comp.extended-fab.small.container.shape"),
                            QStringLiteral("corner-full"));
    const MdExtendedFabTokens shaped =
        MdExtendedFabTokens::resolve(ExtendedFabVariant::Primary, ExtendedFabSize::Small, false,
                                     &shapeOverrides);
    QVERIFY(shaped.containerShape == ShapeCorner::Full);
}

void TestMd3ExtendedFab::defaultGeometryMatchesTheTokens()
{
    MdExtendedFab fab(QStringLiteral("add"), QStringLiteral("Create"));

    // The focus-indicator margin: offset 2 + activeWidth/2 + width/2 = 7.5 px.
    const MdExtendedFabTokens tokens = fab.tokens();
    const qreal inset = MdExtendedFabStyle::focusRingInset(tokens);
    QCOMPARE(inset, 7.5);

    fab.resize(fab.sizeHint());

    // The container width is derived: leading 16 + icon 24 + gap 8 + label +
    // trailing 16, all in the size set's title-medium font.
    const QFontMetricsF metrics(MdTypeScale::font(tokens.labelStyle));
    const qreal labelWidth = metrics.horizontalAdvance(QStringLiteral("Create"));
    const qreal containerWidth = 16.0 + 24.0 + 8.0 + labelWidth + 16.0;

    const QRectF container = fab.containerRect();
    QCOMPARE(container.height(), 56.0);
    QVERIFY(qAbs(container.width() - containerWidth) < 0.75);
    // Horizontally the size hint's ceil() leaves the fractional remainder to
    // the centring (≤ 0.5 px of slack); vertically the height is integral.
    QVERIFY(qAbs(container.left() - inset) < 0.5);
    QCOMPARE(container.top(), inset);

    // The size hint is the container plus the same margin on both axes.
    QCOMPARE(fab.sizeHint().height(), 71);
    QVERIFY(qAbs(fab.sizeHint().width() - (containerWidth + 15.0)) < 1.5);

    // The medium size changes height, spacing and the label font.
    fab.setFabSize(ExtendedFabSize::Medium);
    QCOMPARE(fab.containerRect().height(), 80.0);
    QVERIFY(fab.tokens().labelStyle == TypeStyle::TitleLarge);
}

void TestMd3ExtendedFab::propertiesAndSignalsFire()
{
    MdExtendedFab fab(QStringLiteral("add"), QStringLiteral("Create"));

    QSignalSpy variantSpy(&fab, &MdExtendedFab::variantChanged);
    QSignalSpy sizeSpy(&fab, &MdExtendedFab::fabSizeChanged);
    QSignalSpy loweredSpy(&fab, &MdExtendedFab::loweredChanged);
    QSignalSpy iconSpy(&fab, &MdExtendedFab::iconNameChanged);

    fab.setVariant(ExtendedFabVariant::TertiaryContainer);
    QCOMPARE(fab.variant(), ExtendedFabVariant::TertiaryContainer);
    QCOMPARE(variantSpy.count(), 1);

    fab.setFabSize(ExtendedFabSize::Large);
    QCOMPARE(fab.fabSize(), ExtendedFabSize::Large);
    QCOMPARE(sizeSpy.count(), 1);

    fab.setLowered(true);
    QVERIFY(fab.isLowered());
    QCOMPARE(loweredSpy.count(), 1);

    fab.setIconName(QStringLiteral("edit"));
    QCOMPARE(fab.iconName(), QStringLiteral("edit"));
    QCOMPARE(iconSpy.count(), 1);

    // No-op writes stay silent.
    fab.setVariant(ExtendedFabVariant::TertiaryContainer);
    fab.setFabSize(ExtendedFabSize::Large);
    fab.setLowered(true);
    QCOMPARE(variantSpy.count(), 1);
    QCOMPARE(sizeSpy.count(), 1);
    QCOMPARE(loweredSpy.count(), 1);
}

void TestMd3ExtendedFab::iconAndLabelAreTheAccessibleName()
{
    // The accessible name reads "icon, text" the way a screen reader does.
    MdExtendedFab fab(QStringLiteral("add"), QStringLiteral("Create"));
    QCOMPARE(fab.accessibleName(), QStringLiteral("add Create"));

    fab.setIconName(QStringLiteral("search"));
    QCOMPARE(fab.accessibleName(), QStringLiteral("search Create"));

    // A label-only construction still names itself.
    MdExtendedFab labelOnly(QStringLiteral("Compose"));
    QCOMPARE(labelOnly.accessibleName(), QStringLiteral("Compose"));
}

void TestMd3ExtendedFab::pressRunsTheRippleAndReleaseFadesIt()
{
    MdExtendedFab fab(QStringLiteral("add"), QStringLiteral("Create"));
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

void TestMd3ExtendedFab::pointerFocusShowsNoRingKeyboardFocusDoes()
{
    MdExtendedFab fab(QStringLiteral("add"), QStringLiteral("Create"));
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

void TestMd3ExtendedFab::everyCombinationRendersInk()
{
    // 6 colour sets x 3 sizes x lowered/raised — every combination paints a
    // container (this family has no container-less variant), so every grab
    // must be more than a third ink.
    for (const ExtendedFabVariant variant :
         {ExtendedFabVariant::Primary, ExtendedFabVariant::Secondary,
          ExtendedFabVariant::Tertiary, ExtendedFabVariant::PrimaryContainer,
          ExtendedFabVariant::SecondaryContainer, ExtendedFabVariant::TertiaryContainer}) {
        for (const ExtendedFabSize size :
             {ExtendedFabSize::Small, ExtendedFabSize::Medium, ExtendedFabSize::Large}) {
            for (const bool lowered : {false, true}) {
                MdExtendedFab fab(QStringLiteral("add"), QStringLiteral("Create"));
                fab.setVariant(variant);
                fab.setFabSize(size);
                fab.setLowered(lowered);
                const QImage image = renderExtendedFab(fab);
                const int ink = paintedPixels(image);
                // Report the combination so a failure names its case.
                const QString label = QStringLiteral("%1/%2/%3")
                                          .arg(extendedFabVariantName(variant),
                                               extendedFabSizeName(size),
                                               lowered ? "lowered" : "raised");
                QVERIFY2(ink > image.width() * image.height() / 3,
                         qPrintable(QStringLiteral("%1: only %2 ink pixels of %3")
                                        .arg(label)
                                        .arg(ink)
                                        .arg(image.width() * image.height())));
            }
        }
    }
}

QTEST_MAIN(TestMd3ExtendedFab)
#include "TestMd3ExtendedFab.moc"
