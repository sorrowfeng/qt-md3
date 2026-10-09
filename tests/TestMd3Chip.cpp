// TestMd3Chip — the four published chip families over one widget.
//
// Numbers transcribed from material-web tokens/versions/latest/sass —
// _md-comp-assist-chip.scss, -filter-chip.scss, -input-chip.scss and
// -suggestion-chip.scss — behaviour cross-checked against androidx Compose
// Material3 Chip.kt (the shared `Chip` / `SelectableChip` and the
// per-family Defaults objects).
//
// The assertions worth keeping are the ones a token table cannot express:
//
//   * the **pressed state-layer swap** — a filter chip's unselected press
//     ripples `on-secondary-container` while a selected press ripples
//     `on-surface-variant`; input chips do not swap;
//   * the **two-spacing arrangement** — Compose's `ChipArrangement` puts the
//     label after the leading slot's spacing and right-aligns the trailing
//     slot, and filter/input tighten the leading gap to 4 px;
//   * the **trailing slot never moves** — a stretched chip widens the label
//     area, exactly as the weighted label does upstream;
//   * assist and suggestion are **not checkable**; filter and input are;
//   * the **flat selected filter chip rises to level 1 on hover** — the one
//     flat elevation row in the family;
//   * input is **flat only** — the elevated kind mirrors the flat one
//     because no `ElevatedInputChip` exists upstream.

#include "core/MdChipTokens.h"
#include "core/MdFocusRing.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdChipStyle.h"
#include "widgets/MdChip.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

QColor pixelColorAt(QWidget &widget, const QPointF &position)
{
    QImage image(int(widget.width()), int(widget.height()), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32).pixelColor(position.toPoint());
}

/// One synchronous paint — what makes the ripple's colour observable right
/// after a press.
void paintOnce(QWidget &widget)
{
    pixelColorAt(widget, QPointF(0, 0));
}

} // namespace

class TestMd3Chip : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    // --- token tables ---------------------------------------------------------
    void metricTable();
    void assistTable();
    void suggestionTable();
    void filterTable();
    void inputTable();
    void clickOnlyFamiliesAreNotSelectable();

    // --- geometry and arrangement -----------------------------------------------
    void heightIsThirtyTwo();
    void filterTightensTheLeadingGap();
    void trailingStaysRightWhenStretched();
    void avatarIsTwentyFourFull();

    // --- selection -----------------------------------------------------------------
    void clickOnlyChipDoesNotToggle();
    void filterChipToggles();

    // --- the paint ---------------------------------------------------------------------
    void assistFlatPaintsOutlineOnly();
    void assistElevatedPaintsContainerLow();
    void filterSelectedPaintsSecondaryContainer();
    void disabledSelectedContainerFadesToTwelvePercent();
    void pressRipplesInTheSwappedColour();
    void focusRingOnlyOnKeyboardFocus();

    // --- the style -----------------------------------------------------------------------
    void styleIsInstalled();
};

void TestMd3Chip::cleanup()
{
    // No shared mutable state; each test builds its own widgets on the stack.
}

// ---------------------------------------------------------------------------
// Token tables
// ---------------------------------------------------------------------------

void TestMd3Chip::metricTable()
{
    for (const MdChipVariant variant : { MdChipVariant::Assist, MdChipVariant::Filter,
                                         MdChipVariant::Input, MdChipVariant::Suggestion }) {
        const MdChipVariantTokens tokens = MdChipVariantTokens::resolve(variant);
        QCOMPARE(tokens.containerHeight, 32.0);
        QCOMPARE(tokens.containerShape, ShapeCorner::Small);
        QCOMPARE(tokens.iconSize, 18.0);
        QCOMPARE(tokens.labelTextType, TypeStyle::LabelLarge);
        QCOMPARE(tokens.focusIndicatorColor, ColorRole::Secondary);
        QCOMPARE(tokens.focusIndicatorOuterOffset, 2.0);
        QCOMPARE(tokens.focusIndicatorThickness, 3.0);
        // Compose's behaviour constants.
        QCOMPARE(tokens.contentPadding, 8.0);
        QCOMPARE(tokens.elementSpacing, 8.0);
        QCOMPARE(tokens.compactSpacing, 4.0);
        QCOMPARE(tokens.disabledLabelTextOpacity, 0.38);
        QCOMPARE(tokens.disabledIconOpacity, 0.38);
        QCOMPARE(tokens.disabledContainerOpacity, 0.12);
        QCOMPARE(tokens.draggedStateLayerOpacity, 0.16);
    }
    // The avatar is the input family's own metric.
    const MdChipVariantTokens input = MdChipVariantTokens::resolve(MdChipVariant::Input);
    QCOMPARE(input.avatarSize, 24.0);
    QCOMPARE(input.avatarShape, ShapeCorner::Full);
}

void TestMd3Chip::assistTable()
{
    const MdChipVariantTokens tokens = MdChipVariantTokens::resolve(MdChipVariant::Assist);
    const int enabled = int(MdChipInteraction::Enabled);
    const int hovered = int(MdChipInteraction::Hovered);
    const int focused = int(MdChipInteraction::Focused);
    const int pressed = int(MdChipInteraction::Pressed);
    const int dragged = int(MdChipInteraction::Dragged);
    const int disabled = int(MdChipInteraction::Disabled);
    const int unselected = 0;

    // The flat outline: 1 px `outline-variant`, lifting to `on-surface` on
    // focus, fading to 0.12 disabled.
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].outlineWidth[unselected], 1.0);
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].outline[unselected][enabled].role,
             ColorRole::OutlineVariant);
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].outline[unselected][focused].role,
             ColorRole::OnSurface);
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].outline[unselected][disabled].role,
             ColorRole::OnSurface);
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].outline[unselected][disabled].opacity, 0.12);

    // Label `on-surface`; the leading icon `primary`; no pressed-swap. The
    // state layer only exists in the interaction states — the export publishes
    // no resting (`enabled`) row, so that slot stays absent.
    for (int i : {hovered, focused, pressed, dragged}) {
        QCOMPARE(tokens.surface[int(MdChipKind::Flat)].label[unselected][i].role,
                 ColorRole::OnSurface);
        QCOMPARE(tokens.surface[int(MdChipKind::Flat)].leadingIcon[unselected][i].role,
                 ColorRole::Primary);
        QCOMPARE(tokens.surface[int(MdChipKind::Flat)].stateLayer[unselected][i].role,
                 ColorRole::OnSurface);
    }
    QVERIFY(!tokens.surface[int(MdChipKind::Flat)].stateLayer[unselected][enabled].isPresent());
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].label[unselected][disabled].role,
             ColorRole::OnSurface);
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].label[unselected][disabled].opacity, 0.38);

    // The elevated ladder: 1 / 2 / 1 / 1 / 4 / 0, container `low`, disabled
    // `on-surface` at 0.12.
    const MdChipSurfaceTokens &elevated = tokens.surface[int(MdChipKind::Elevated)];
    QCOMPARE(elevated.container[unselected][enabled].role, ColorRole::SurfaceContainerLow);
    QCOMPARE(elevated.container[unselected][disabled].role, ColorRole::OnSurface);
    QCOMPARE(elevated.container[unselected][disabled].opacity, 0.12);
    QCOMPARE(elevated.containerElevation[enabled], ElevationLevel::Level1);
    QCOMPARE(elevated.containerElevation[hovered], ElevationLevel::Level2);
    QCOMPARE(elevated.containerElevation[focused], ElevationLevel::Level1);
    QCOMPARE(elevated.containerElevation[pressed], ElevationLevel::Level1);
    QCOMPARE(elevated.containerElevation[dragged], ElevationLevel::Level4);
    QCOMPARE(elevated.containerElevation[disabled], ElevationLevel::Level0);
}

void TestMd3Chip::suggestionTable()
{
    const MdChipVariantTokens tokens = MdChipVariantTokens::resolve(MdChipVariant::Suggestion);
    const int enabled = int(MdChipInteraction::Enabled);
    const int focused = int(MdChipInteraction::Focused);

    // The muted label, and the `on-surface-variant` focus outline lift (assist
    // lifts to `on-surface`). The state layer has no resting row — its first
    // published state is hover.
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].label[0][enabled].role,
             ColorRole::OnSurfaceVariant);
    QVERIFY(!tokens.surface[int(MdChipKind::Flat)].stateLayer[0][enabled].isPresent());
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].stateLayer[0][int(MdChipInteraction::Hovered)]
                 .role,
             ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].outline[0][focused].role,
             ColorRole::OnSurfaceVariant);
    // The leading icon stays `primary`.
    QCOMPARE(tokens.surface[int(MdChipKind::Flat)].leadingIcon[0][enabled].role,
             ColorRole::Primary);
}

void TestMd3Chip::filterTable()
{
    const MdChipVariantTokens tokens = MdChipVariantTokens::resolve(MdChipVariant::Filter);
    const int enabled = int(MdChipInteraction::Enabled);
    const int hovered = int(MdChipInteraction::Hovered);
    const int pressed = int(MdChipInteraction::Pressed);
    const int disabled = int(MdChipInteraction::Disabled);
    const MdChipSurfaceTokens &flat = tokens.surface[int(MdChipKind::Flat)];

    // The pressed state-layer swap — the family's signature special case.
    QCOMPARE(flat.stateLayer[0][pressed].role, ColorRole::OnSecondaryContainer);
    QCOMPARE(flat.stateLayer[1][pressed].role, ColorRole::OnSurfaceVariant);
    // Hover and focus read each side's own colour.
    QCOMPARE(flat.stateLayer[0][hovered].role, ColorRole::OnSurfaceVariant);
    QCOMPARE(flat.stateLayer[1][hovered].role, ColorRole::OnSecondaryContainer);

    // Selected: `secondary-container` fill, no outline, `on-secondary-container`
    // content.
    QCOMPARE(flat.container[1][enabled].role, ColorRole::SecondaryContainer);
    QVERIFY(!flat.outline[1][enabled].isPresent());
    QCOMPARE(flat.outlineWidth[1], 0.0);
    QCOMPARE(flat.label[1][enabled].role, ColorRole::OnSecondaryContainer);
    // Unselected: the leading icon `primary`, 1 px `outline-variant`.
    QCOMPARE(flat.leadingIcon[0][enabled].role, ColorRole::Primary);
    QCOMPARE(flat.outline[0][enabled].role, ColorRole::OutlineVariant);
    QCOMPARE(flat.outlineWidth[0], 1.0);

    // The one flat elevation row: a selected chip rises on hover.
    QCOMPARE(flat.containerElevation[hovered], ElevationLevel::Level1);
    QCOMPARE(flat.containerElevation[enabled], ElevationLevel::Level0);

    // Disabled: the selected container fades to `on-surface` @ 0.12; the
    // unselected side is transparent with the outline faded.
    QCOMPARE(flat.container[1][disabled].role, ColorRole::OnSurface);
    QCOMPARE(flat.container[1][disabled].opacity, 0.12);
    QVERIFY(!flat.container[0][disabled].isPresent());
    QCOMPARE(flat.outline[0][disabled].opacity, 0.12);

    // Elevated: `secondary-container` when selected, `low` when not.
    const MdChipSurfaceTokens &elevated = tokens.surface[int(MdChipKind::Elevated)];
    QCOMPARE(elevated.container[0][enabled].role, ColorRole::SurfaceContainerLow);
    QCOMPARE(elevated.container[1][enabled].role, ColorRole::SecondaryContainer);
    QCOMPARE(elevated.containerElevation[hovered], ElevationLevel::Level2);
}

void TestMd3Chip::inputTable()
{
    const MdChipVariantTokens tokens = MdChipVariantTokens::resolve(MdChipVariant::Input);
    const int enabled = int(MdChipInteraction::Enabled);
    const int hovered = int(MdChipInteraction::Hovered);
    const int focused = int(MdChipInteraction::Focused);
    const int pressed = int(MdChipInteraction::Pressed);
    const int dragged = int(MdChipInteraction::Dragged);
    const MdChipSurfaceTokens &flat = tokens.surface[int(MdChipKind::Flat)];

    // No pressed swap: both sides press in the colour they have.
    QCOMPARE(flat.stateLayer[0][pressed].role, ColorRole::OnSurfaceVariant);
    QCOMPARE(flat.stateLayer[1][pressed].role, ColorRole::OnSecondaryContainer);

    // The leading icon lifts to `primary` under every interaction...
    for (int i : {hovered, focused, pressed, dragged}) {
        QCOMPARE(flat.leadingIcon[0][i].role, ColorRole::Primary);
    }
    QCOMPARE(flat.leadingIcon[0][enabled].role, ColorRole::OnSurfaceVariant);
    // ...and the trailing icon carries its own pressed override.
    QCOMPARE(flat.trailingIcon[0][pressed].role, ColorRole::Primary);
    QCOMPARE(flat.trailingIcon[0][enabled].role, ColorRole::OnSurfaceVariant);

    // Selected: `secondary-container`, no outline.
    QCOMPARE(flat.container[1][enabled].role, ColorRole::SecondaryContainer);
    QCOMPARE(flat.outlineWidth[1], 0.0);

    // The dragged elevation rides the flat ladder too.
    QCOMPARE(flat.containerElevation[dragged], ElevationLevel::Level4);
    QCOMPARE(flat.containerElevation[hovered], ElevationLevel::Level0);

    // Input publishes no elevated rows: the elevated kind mirrors the flat one.
    const MdChipSurfaceTokens &elevated = tokens.surface[int(MdChipKind::Elevated)];
    QCOMPARE(elevated.container[1][enabled].role, ColorRole::SecondaryContainer);
    QCOMPARE(elevated.containerElevation[hovered], ElevationLevel::Level0);
}

void TestMd3Chip::clickOnlyFamiliesAreNotSelectable()
{
    QVERIFY(!MdChipVariantTokens::resolve(MdChipVariant::Assist).isSelectable());
    QVERIFY(!MdChipVariantTokens::resolve(MdChipVariant::Suggestion).isSelectable());
    QVERIFY(MdChipVariantTokens::resolve(MdChipVariant::Filter).isSelectable());
    QVERIFY(MdChipVariantTokens::resolve(MdChipVariant::Input).isSelectable());
}

// ---------------------------------------------------------------------------
// Geometry and arrangement
// ---------------------------------------------------------------------------

void TestMd3Chip::heightIsThirtyTwo()
{
    MdChip chip(QStringLiteral("A"));
    chip.resize(chip.sizeHint());
    QCOMPARE(chip.sizeHint().height(), 32);
    QCOMPARE(MdChipStyle::cornerRadius(chip.chipTokens()), MdShape::radius(ShapeCorner::Small));
}

void TestMd3Chip::filterTightensTheLeadingGap()
{
    // A filter chip with leading and trailing icons spaces the label 4 px from
    // the leading icon; an assist chip keeps 8 px everywhere.
    MdChip assist(QStringLiteral("A"));
    assist.setIconName(QStringLiteral("mail"));
    assist.setTrailingIconName(QStringLiteral("close"));
    const MdChip::Boxes assistBoxes = assist.boxes();
    QCOMPARE(assistBoxes.leadingIcon.right() + 8.0, assistBoxes.label.left());

    MdChip filter(QStringLiteral("A"));
    filter.setVariant(MdChipVariant::Filter);
    filter.setIconName(QStringLiteral("mail"));
    filter.setTrailingIconName(QStringLiteral("close"));
    const MdChip::Boxes filterBoxes = filter.boxes();
    QCOMPARE(filterBoxes.leadingIcon.right() + 4.0, filterBoxes.label.left());
    QCOMPARE(filterBoxes.label.right() + 4.0, filterBoxes.trailingIcon.left());
}

void TestMd3Chip::trailingStaysRightWhenStretched()
{
    MdChip chip(QStringLiteral("A"));
    chip.setTrailingIconName(QStringLiteral("close"));
    chip.resize(chip.sizeHint());
    const MdChip::Boxes natural = chip.boxes();
    const qreal naturalRight = natural.trailingIcon.right();

    chip.resize(chip.sizeHint() + QSize(60, 0));
    const MdChip::Boxes stretched = chip.boxes();
    // The trailing icon's right edge is the content padding inside the widget,
    // and the label area is what grew.
    QCOMPARE(stretched.trailingIcon.right(), qreal(chip.width()) - 8.0);
    QCOMPARE(stretched.trailingIcon.left(), natural.trailingIcon.left() + 60.0);
    QVERIFY(stretched.label.width() > natural.label.width());
}

void TestMd3Chip::avatarIsTwentyFourFull()
{
    MdChip chip(QStringLiteral("A"));
    chip.setVariant(MdChipVariant::Input);
    chip.setAvatarIconName(QStringLiteral("person"));
    const MdChip::Boxes boxes = chip.boxes();
    QVERIFY(boxes.hasAvatar);
    QCOMPARE(boxes.avatar.size(), QSizeF(24.0, 24.0));
    // The avatar displaces the icon slot: both set means the avatar shows.
    chip.setIconName(QStringLiteral("mail"));
    const MdChip::Boxes displaced = chip.boxes();
    QVERIFY(displaced.hasAvatar);
    QVERIFY(!displaced.hasLeadingIcon || displaced.leadingIcon.isNull());
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

void TestMd3Chip::clickOnlyChipDoesNotToggle()
{
    MdChip chip(QStringLiteral("A"));
    QVERIFY(!chip.isCheckable());
    chip.click();
    QVERIFY(!chip.isChecked());
}

void TestMd3Chip::filterChipToggles()
{
    MdChip chip(QStringLiteral("A"));
    chip.setVariant(MdChipVariant::Filter);
    QVERIFY(chip.isCheckable());
    chip.click();
    QVERIFY(chip.isChecked());
    chip.click();
    QVERIFY(!chip.isChecked());
}

// ---------------------------------------------------------------------------
// The paint
// ---------------------------------------------------------------------------

void TestMd3Chip::assistFlatPaintsOutlineOnly()
{
    MdChip chip(QStringLiteral("A"));
    chip.resize(chip.sizeHint());

    const QColor centre = pixelColorAt(chip, QPointF(chip.width() / 2.0, 4.0));
    QCOMPARE(centre.alpha(), 0);
    // The stroke is inset half a width, hugging the top edge — pixel row 0.
    // (Sampling at y=0.5 rounds to row 1, *outside* the 1 px stroke.)
    const QColor edge = pixelColorAt(chip, QPointF(chip.width() / 2.0, 0.0));
    const QColor expectedEdge = MdTheme::instance().color(ColorRole::OutlineVariant);
    QVERIFY2(qAbs(edge.alpha() - expectedEdge.alpha()) <= 2
                 && qAbs(edge.red() - expectedEdge.red()) <= 2
                 && qAbs(edge.green() - expectedEdge.green()) <= 2
                 && qAbs(edge.blue() - expectedEdge.blue()) <= 2,
             qPrintable(QStringLiteral("edge %1 vs expected %2")
                            .arg(edge.name(QColor::HexArgb))
                            .arg(expectedEdge.name(QColor::HexArgb))));
}

void TestMd3Chip::assistElevatedPaintsContainerLow()
{
    MdChip chip(QStringLiteral("A"));
    chip.setKind(MdChipKind::Elevated);
    chip.resize(chip.sizeHint());

    const QColor centre = pixelColorAt(chip, QPointF(chip.width() / 2.0, 4.0));
    QCOMPARE(centre, MdTheme::instance().color(ColorRole::SurfaceContainerLow));
}

void TestMd3Chip::filterSelectedPaintsSecondaryContainer()
{
    MdChip chip(QStringLiteral("A"));
    chip.setVariant(MdChipVariant::Filter);
    chip.setSelected(true);
    chip.resize(chip.sizeHint());

    const QColor centre = pixelColorAt(chip, QPointF(chip.width() / 2.0, 4.0));
    QCOMPARE(centre, MdTheme::instance().color(ColorRole::SecondaryContainer));
}

void TestMd3Chip::disabledSelectedContainerFadesToTwelvePercent()
{
    MdChip chip(QStringLiteral("A"));
    chip.setVariant(MdChipVariant::Filter);
    chip.setSelected(true);
    chip.setEnabled(false);
    chip.resize(chip.sizeHint());

    const QColor onSurface = MdTheme::instance().color(ColorRole::OnSurface);
    QColor expected = onSurface;
    expected.setAlphaF(onSurface.alphaF() * 0.12);
    // The 0.12 fill rides the premultiplied render target: the round-trip
    // at alpha ~31 costs up to ~4 counts per channel. Widen past it.
    const QColor centre = pixelColorAt(chip, QPointF(chip.width() / 2.0, 4.0));
    QVERIFY2(qAbs(centre.alpha() - expected.alpha()) <= 6
                 && qAbs(centre.red() - expected.red()) <= 6,
             qPrintable(QStringLiteral("centre %1 vs expected %2")
                            .arg(centre.name(QColor::HexArgb))
                            .arg(expected.name(QColor::HexArgb))));
}

void TestMd3Chip::pressRipplesInTheSwappedColour()
{
    MdChip chip(QStringLiteral("A"));
    chip.setVariant(MdChipVariant::Filter);
    chip.resize(chip.sizeHint());
    chip.show();
    QVERIFY(QTest::qWaitForWindowExposed(&chip));

    // An unselected filter press ripples `on-secondary-container` — the colour
    // the chip is about to earn.
    const QPointF centre(chip.width() / 2.0, chip.height() / 2.0);
    QTest::mousePress(&chip, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    QVERIFY(chip.rippleController()->isActive());
    paintOnce(chip);
    QCOMPARE(chip.rippleController()->contentColor(),
             MdTheme::instance().color(ColorRole::OnSecondaryContainer));
    QTest::mouseRelease(&chip, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    QVERIFY(chip.isChecked());

    // A selected press ripples the other side of the swap: `on-surface-variant`.
    QTest::mousePress(&chip, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    paintOnce(chip);
    QCOMPARE(chip.rippleController()->contentColor(),
             MdTheme::instance().color(ColorRole::OnSurfaceVariant));
    QTest::mouseRelease(&chip, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
}

void TestMd3Chip::focusRingOnlyOnKeyboardFocus()
{
    MdChip chip(QStringLiteral("A"));
    chip.resize(chip.sizeHint());
    chip.show();
    QVERIFY(QTest::qWaitForWindowExposed(&chip));

    chip.clearFocus();
    chip.setFocus(Qt::MouseFocusReason);
    QVERIFY(chip.hasFocus());
    QVERIFY(!chip.hasKeyboardFocus());

    chip.clearFocus();
    chip.setFocus(Qt::TabFocusReason);
    QVERIFY(chip.hasKeyboardFocus());
    QVERIFY(chip.focusRingController() != nullptr);
}

void TestMd3Chip::styleIsInstalled()
{
    MdChip chip;
    QVERIFY(MdChipStyle::isInstalled());
}

QTEST_MAIN(TestMd3Chip)
#include "TestMd3Chip.moc"
