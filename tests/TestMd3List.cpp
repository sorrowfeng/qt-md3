// TestMd3List — the list family: MdList + MdListItem.
//
// Numbers transcribed from material-web
// tokens/versions/latest/sass/_md-comp-list.scss (the whole export: metrics,
// shapes, colour sets, opacities), with the behaviour rows from androidx
// Compose Material3 ListItem.kt / ListItemDefaults.kt (the line-count rule,
// the shape morph order, the segmented outer corners) and the m3.material.io
// measurements (the 88dp alignment breakpoint, the leading-icon top padding)
// — the sources the token header names.

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdListItemStyle.h"
#include "styles/MdListStyle.h"
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"

#include <QtGui/QEnterEvent>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

QColor pixelColorAt(QWidget &widget, const QPointF &position)
{
    QImage image(int(widget.width()), int(widget.height()),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    // DrawChildren only: a top-level widget would otherwise be erased with its
    // opaque window background first.
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32).pixelColor(position.toPoint());
}

/// The darkest pixel in `region` — used to read a glyph's colour without
/// depending on where exactly the glyph's strokes land (antialiasing makes a
/// single sample unreliable).
QColor darkestPixelIn(QWidget &widget, const QRect &region)
{
    QImage image(int(widget.width()), int(widget.height()),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    image = image.convertToFormat(QImage::Format_ARGB32);

    const QRect bounds = region.intersected(image.rect());
    QColor darkest = Qt::white;
    long darkestLuma = -1;
    for (int y = bounds.top(); y <= bounds.bottom(); ++y) {
        for (int x = bounds.left(); x <= bounds.right(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            const long luma = pixel.red() + pixel.green() + pixel.blue();
            if (darkestLuma < 0 || luma < darkestLuma) {
                darkestLuma = luma;
                darkest = pixel;
            }
        }
    }
    return darkest;
}

long colourDistance(const QColor &a, const QColor &b)
{
    return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue());
}

} // namespace

class TestMd3List : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void theItemTokenTableMatchesTheExport();
    void theContainerTokenTableMatchesTheExport();
    void theLineCountFollowsTheContent();
    void heightsFollowTheLineCount();
    void standardIsSquareAndExpressiveMorphs();
    void theSpecAlignmentRules();
    void segmentedOuterCorners();
    void interactionStatesPaintLayers();
    void selectionDrivesTheColours();
    void theListDrivesSelectionModes();
    void keyboardNavigationWalksActivatableItems();
    void renderSmoke();

private:
    MdListItem *m_item = nullptr;
};

void TestMd3List::initTestCase()
{
    MdTheme::instance();
    m_item = new MdListItem(QStringLiteral("Headline"));
    m_item->resize(360, m_item->sizeHint().height());
    m_item->show();
}

void TestMd3List::cleanup()
{
    MdComponentTokens::global().remove(
        QStringLiteral("md.comp.list.list-item.one-line.container.height"));
    MdComponentTokens::global().remove(
        QStringLiteral("md.comp.list.list-item.hovered.container.expressive.shape"));
    m_item->componentTokens().clear();
    // Every piece of state a slot may touch has to be restored here, not just
    // the ones the happy path leaves behind: a failing QCOMPARE / QVERIFY
    // aborts its slot on the spot, so whatever the slot had set at that moment
    // is still in force when the next slot starts.
    m_item->setOverline(QString());
    m_item->setHeadline(QStringLiteral("Headline"));
    m_item->setSupporting(QString());
    m_item->setTrailingSupporting(QString());
    m_item->setLeadingIcon(QString());
    m_item->setTrailingIcon(QString());
    m_item->setAvatarLabel(QString());
    m_item->setSelected(false);
    m_item->setDragged(false);
    m_item->setEnabled(true);
    m_item->setInteractive(false);
    m_item->setVariant(MdListVariant::Standard);
    m_item->setSegmentedPosition(-1, 0);
    m_item->setLeadingKind(MdListItem::LeadingKind::None);
    m_item->setLayoutDirection(Qt::LeftToRight);
    m_item->resize(360, m_item->sizeHint().height());
}

void TestMd3List::theItemTokenTableMatchesTheExport()
{
    const MdListTokens tokens = MdListTokens::resolve(MdListVariant::Standard);

    // --- container metrics --------------------------------------------------
    QCOMPARE(tokens.oneLineHeight, 56.0);
    QCOMPARE(tokens.twoLineHeight, 72.0);
    QCOMPARE(tokens.threeLineHeight, 88.0);
    QCOMPARE(tokens.topSpace, 10.0);
    QCOMPARE(tokens.bottomSpace, 10.0);
    QCOMPARE(tokens.leadingSpace, 16.0);
    QCOMPARE(tokens.trailingSpace, 16.0);
    // The export publishes between-space 12px; material-web's SCSS hardcodes
    // 16px and loses (two sources against one hardcode).
    QCOMPARE(tokens.betweenSpace, 12.0);

    // --- slots ---------------------------------------------------------------
    QCOMPARE(tokens.leadingIconSize, 24.0);
    QCOMPARE(tokens.leadingIconExpressiveSize, 20.0);
    QCOMPARE(tokens.trailingIconSize, 24.0);
    QCOMPARE(tokens.trailingIconExpressiveSize, 20.0);
    QCOMPARE(tokens.leadingAvatarSize, 40.0);
    QCOMPARE(int(tokens.leadingAvatarShape), int(ShapeCorner::Full));
    QCOMPARE(int(tokens.leadingAvatarColor), int(ColorRole::PrimaryContainer));
    QCOMPARE(int(tokens.leadingAvatarLabelColor), int(ColorRole::OnPrimaryContainer));
    QCOMPARE(int(tokens.leadingAvatarLabelType), int(TypeStyle::TitleMedium));
    QCOMPARE(tokens.leadingImageWidth, 56.0);
    QCOMPARE(tokens.leadingImageHeight, 56.0);
    QCOMPARE(int(tokens.leadingImageShape), int(ShapeCorner::None));
    QCOMPARE(int(tokens.leadingImageExpressiveShape), int(ShapeCorner::Small));
    QCOMPARE(tokens.leadingVideoWidth, 100.0);
    QCOMPARE(tokens.leadingVideoHeight, 56.0);
    QCOMPARE(tokens.smallLeadingVideoWidth, 100.0);
    QCOMPARE(tokens.smallLeadingVideoHeight, 56.0);
    QCOMPARE(tokens.largeLeadingVideoWidth, 114.0);
    QCOMPARE(tokens.largeLeadingVideoHeight, 64.0);
    QCOMPARE(int(tokens.leadingVideoShape), int(ShapeCorner::Small));

    // --- typography ----------------------------------------------------------
    QCOMPARE(int(tokens.labelTextType), int(TypeStyle::BodyLarge));
    QCOMPARE(int(tokens.overlineType), int(TypeStyle::LabelSmall));
    QCOMPARE(int(tokens.supportingTextType), int(TypeStyle::BodyMedium));
    QCOMPARE(int(tokens.trailingSupportingTextType), int(TypeStyle::LabelSmall));

    // --- shapes and elevation -------------------------------------------------
    QCOMPARE(int(tokens.containerShape), int(ShapeCorner::None));
    QCOMPARE(int(tokens.expressiveShape), int(ShapeCorner::ExtraSmall));
    QCOMPARE(int(tokens.hoveredShape), int(ShapeCorner::Medium));
    QCOMPARE(int(tokens.focusedShape), int(ShapeCorner::Large));
    QCOMPARE(int(tokens.pressedShape), int(ShapeCorner::Large));
    QCOMPARE(int(tokens.selectedShape), int(ShapeCorner::Large));
    QCOMPARE(int(tokens.draggedShape), int(ShapeCorner::Large));
    // Two disabled rows: the unselected one drops to the extra-small corner,
    // a *selected* disabled item keeps the large one.
    QCOMPARE(int(tokens.disabledShape), int(ShapeCorner::ExtraSmall));
    QCOMPARE(int(tokens.selectedDisabledShape), int(ShapeCorner::Large));
    QCOMPARE(int(tokens.containerElevation), int(ElevationLevel::Level0));
    QCOMPARE(int(tokens.draggedElevation), int(ElevationLevel::Level4));

    // --- colours ---------------------------------------------------------------
    QCOMPARE(int(tokens.family.container), int(ColorRole::Surface));
    QCOMPARE(int(tokens.family.enabled.labelText), int(ColorRole::OnSurface));
    QCOMPARE(int(tokens.family.enabled.leadingIcon), int(ColorRole::OnSurfaceVariant));
    QCOMPARE(int(tokens.family.enabled.trailingIcon), int(ColorRole::OnSurfaceVariant));
    QCOMPARE(int(tokens.family.enabled.overline), int(ColorRole::OnSurfaceVariant));
    QCOMPARE(int(tokens.family.enabled.supportingText), int(ColorRole::OnSurfaceVariant));
    QCOMPARE(int(tokens.family.enabled.trailingSupportingText), int(ColorRole::OnSurfaceVariant));
    QCOMPARE(int(tokens.family.hovered.stateLayer), int(ColorRole::OnSurface));
    QCOMPARE(tokens.family.hovered.stateLayerOpacity, 0.08);
    QCOMPARE(tokens.family.focused.stateLayerOpacity, 0.12);
    QCOMPARE(tokens.family.pressed.stateLayerOpacity, 0.12);
    QCOMPARE(tokens.family.dragged.stateLayerOpacity, 0.16);

    QCOMPARE(int(tokens.selectedFamily.container), int(ColorRole::SecondaryContainer));
    QCOMPARE(int(tokens.selectedFamily.enabled.labelText), int(ColorRole::OnSecondaryContainer));
    QCOMPARE(int(tokens.selectedFamily.enabled.overline), int(ColorRole::OnSecondaryContainer));
    QCOMPARE(int(tokens.selectedFamily.enabled.supportingText),
             int(ColorRole::OnSecondaryContainer));
    QCOMPARE(int(tokens.selectedFamily.enabled.trailingSupportingText),
             int(ColorRole::OnSecondaryContainer));
    // The selected interaction rows drop the icons to on-surface.
    QCOMPARE(int(tokens.selectedFamily.hovered.leadingIcon), int(ColorRole::OnSurface));
    QCOMPARE(int(tokens.selectedFamily.pressed.trailingIcon), int(ColorRole::OnSurface));

    // --- the disabled rows ------------------------------------------------------
    QCOMPARE(tokens.disabled.labelTextOpacity, 0.38);
    QCOMPARE(tokens.disabled.leadingIconOpacity, 0.38);
    QCOMPARE(tokens.disabled.overlineOpacity, 0.38);
    QCOMPARE(tokens.disabled.supportingTextOpacity, 0.38);
    QCOMPARE(tokens.disabled.trailingIconOpacity, 0.38);
    QCOMPARE(tokens.disabled.stateLayerOpacity, 0.1);
    // The unselected disabled row publishes no container — it is unchanged.
    QCOMPARE(tokens.disabled.containerOpacity, 0.0);
    QCOMPARE(int(tokens.disabled.labelText), int(ColorRole::OnSurface));
    // ...but the selected one does, composited over the selected container.
    QCOMPARE(tokens.selectedDisabled.containerOpacity, 0.38);
    QCOMPARE(int(tokens.selectedDisabled.container), int(ColorRole::OnSurface));

    // --- the focus indicator ----------------------------------------------------
    // These are `md.comp.list.focus.indicator.*` rows — list-level, but the
    // *item* paints the ring, so they are carried on the item's token set.
    QCOMPARE(int(tokens.focusIndicatorColor), int(ColorRole::Secondary));
    QCOMPARE(tokens.focusIndicatorThickness, 3.0);
    // A CSS `outline-offset: -3px` with a 3px outline is the inward variant's
    // zero gap: the ring sits flush inside the container edge.
    QCOMPARE(tokens.focusIndicatorOutlineOffset, -3.0);
    QCOMPARE(tokens.focusRingGap(), 0.0);
}

void TestMd3List::theContainerTokenTableMatchesTheExport()
{
    const MdListContainerTokens tokens = MdListContainerTokens::resolve();
    QCOMPARE(int(tokens.containerColor), int(ColorRole::Surface));
    QCOMPARE(int(tokens.shape), int(ShapeCorner::Large));
    QCOMPARE(tokens.topPadding, 8.0);
    QCOMPARE(tokens.bottomPadding, 8.0);
    QCOMPARE(tokens.segmentedGap, 2.0);

    // The deprecated divider rows are carried, not rendered: the export says
    // to use the standalone divider component.
    QCOMPARE(int(tokens.dividerColor), int(ColorRole::Outline));
    QCOMPARE(tokens.dividerHeight, 1.0);
    QCOMPARE(tokens.dividerTopSpace, 0.0);
    QCOMPARE(tokens.dividerBottomSpace, 0.0);
    QCOMPARE(tokens.dividerLeadingSpace, 16.0);
    QCOMPARE(tokens.dividerTrailingSpace, 16.0);
}

void TestMd3List::theLineCountFollowsTheContent()
{
    // [compose] One line with only a headline...
    QCOMPARE(m_item->lineCount(), 1);

    // ...two with either an overline or a supporting text...
    m_item->setOverline(QStringLiteral("OVERLINE"));
    QCOMPARE(m_item->lineCount(), 2);
    m_item->setOverline(QString());
    m_item->setSupporting(QStringLiteral("Supporting"));
    QCOMPARE(m_item->lineCount(), 2);

    // ...three with both...
    m_item->setOverline(QStringLiteral("OVERLINE"));
    QCOMPARE(m_item->lineCount(), 3);

    // ...or three with a supporting text that wraps.
    m_item->setOverline(QString());
    m_item->resize(200, m_item->sizeHint().height());
    m_item->setSupporting(QStringLiteral(
        "A supporting paragraph long enough that it cannot possibly fit on one line at this "
        "narrow width, which makes the item a three-line one."));
    QCOMPARE(m_item->lineCount(), 3);

    m_item->setSupporting(QString());
    m_item->resize(360, m_item->sizeHint().height());
    QCOMPARE(m_item->lineCount(), 1);
}

void TestMd3List::heightsFollowTheLineCount()
{
    m_item->resize(360, m_item->sizeHint().height());
    const int oneLine = MdListItemStyle::measuredHeight(*m_item, m_item->tokens(), 360.0);
    QCOMPARE(oneLine, 56.0);

    m_item->setSupporting(QStringLiteral("Supporting"));
    const int twoLine = MdListItemStyle::measuredHeight(*m_item, m_item->tokens(), 360.0);
    QCOMPARE(twoLine, 72.0);

    m_item->setOverline(QStringLiteral("OVERLINE"));
    const int threeLine = MdListItemStyle::measuredHeight(*m_item, m_item->tokens(), 360.0);
    QCOMPARE(threeLine, 88.0);

    // The token heights are floors, not caps: a tall content stack grows the
    // item past them.
    m_item->setSupporting(QStringLiteral(
        "A supporting paragraph long enough that it cannot possibly fit on one line at this "
        "narrow width, which makes the item a three-line one, and then some more text so the "
        "measured height has to exceed the published floor."));
    QVERIFY(MdListItemStyle::measuredHeight(*m_item, m_item->tokens(), 200.0) > 88.0);
}

void TestMd3List::standardIsSquareAndExpressiveMorphs()
{
    // --- Standard: square in every state, whatever else is set -------------
    m_item->setVariant(MdListVariant::Standard);
    m_item->setInteractive(true);
    m_item->setSelected(true);
    QCOMPARE(int(MdListItemStyle::shapeFor(*m_item, m_item->tokens())), int(ShapeCorner::None));

    // --- Expressive: the morph ladder [compose] -----------------------------
    m_item->setVariant(MdListVariant::Expressive);
    m_item->setSelected(false);
    m_item->setInteractive(true);
    QCOMPARE(int(MdListItemStyle::shapeFor(*m_item, m_item->tokens())),
             int(ShapeCorner::ExtraSmall));

    m_item->setSelected(true);
    QCOMPARE(int(MdListItemStyle::shapeFor(*m_item, m_item->tokens())),
             int(ShapeCorner::Large));

    m_item->setSelected(false);
    m_item->setEnabled(false);
    QCOMPARE(int(MdListItemStyle::shapeFor(*m_item, m_item->tokens())),
             int(ShapeCorner::ExtraSmall));

    // A *selected* disabled item keeps the large corner: the export publishes
    // a separate `selected.disabled.container.expressive.shape` row, and the
    // plain disabled row is the unselected case only.
    m_item->setSelected(true);
    QCOMPARE(int(MdListItemStyle::shapeFor(*m_item, m_item->tokens())), int(ShapeCorner::Large));
    m_item->setSelected(false);
    m_item->setEnabled(true);

    // The state's shape resolves to per-corner radii.
    m_item->setSelected(true);
    m_item->resize(360, 88);
    const QList<qreal> radii =
        MdListItemStyle::radiiFor(*m_item, m_item->tokens(), QSizeF(360.0, 88.0));
    QCOMPARE(radii.size(), 4);
    QCOMPARE(radii.first(), MdShape::resolvedRadius(ShapeCorner::Large, QSizeF(360.0, 88.0)));
}

void TestMd3List::theSpecAlignmentRules()
{
    // [spec] Centred until 88dp, top-aligned from there; the leading icon is
    // always top-aligned with 8dp (12dp at 88dp+) top padding.
    m_item->setInteractive(true);

    // --- one line, 56dp: centred -------------------------------------------
    m_item->resize(360, 56);
    {
        const MdListItemStyle::Layout layout =
            MdListItemStyle::layoutFor(*m_item, m_item->tokens());
        QVERIFY(!layout.topAligned);
        QCOMPARE(layout.headline.center().y(), layout.container.center().y());
        QCOMPARE(layout.headline.left(), 16.0);
        QCOMPARE(layout.headline.width(), 360.0 - 32.0);
    }

    // --- three lines, 88dp: top-aligned -------------------------------------
    m_item->setOverline(QStringLiteral("OVERLINE"));
    m_item->setSupporting(QStringLiteral("Supporting"));
    m_item->resize(360, 88);
    {
        const MdListItemStyle::Layout layout =
            MdListItemStyle::layoutFor(*m_item, m_item->tokens());
        QVERIFY(layout.topAligned);
        QCOMPARE(layout.container.height(), 88.0);
        // The content starts at the 10px top space.
        QCOMPARE(layout.overline.top(), layout.container.top() + 10.0);
        QVERIFY(layout.overline.bottom() <= layout.headline.top() + 0.5);
        QVERIFY(layout.headline.bottom() <= layout.supporting.top() + 0.5);
    }

    // --- the leading icon's own top padding ---------------------------------
    m_item->setOverline(QString());
    m_item->setSupporting(QString());
    m_item->setLeadingIcon(QStringLiteral("home"));
    m_item->resize(360, 56);
    {
        const MdListItemStyle::Layout layout =
            MdListItemStyle::layoutFor(*m_item, m_item->tokens());
        QCOMPARE(layout.leading.top(), 8.0);
        QCOMPARE(layout.leading.left(), 16.0);
        QCOMPARE(layout.leading.width(), 24.0);
        // The content column starts after the slot and the 12px between-space.
        QCOMPARE(layout.headline.left(), 16.0 + 24.0 + 12.0);
    }

    m_item->resize(360, 88);
    {
        const MdListItemStyle::Layout layout =
            MdListItemStyle::layoutFor(*m_item, m_item->tokens());
        // 12dp once the item reaches the breakpoint.
        QCOMPARE(layout.leading.top(), 12.0);
    }
    m_item->setLeadingKind(MdListItem::LeadingKind::None);
    m_item->resize(360, 56);
}

void TestMd3List::segmentedOuterCorners()
{
    // [compose] `segmentedShapes`: the first item's top pair and the last
    // item's bottom pair take the list's container.shape; a single item takes
    // all four; the middle items keep their own shape.
    const QSizeF size(360.0, 56.0);
    const qreal outer = MdShape::resolvedRadius(ShapeCorner::Large, size);
    const qreal inner = MdShape::resolvedRadius(ShapeCorner::ExtraSmall, size);
    QVERIFY(outer > inner);

    m_item->setVariant(MdListVariant::Expressive);
    m_item->resize(360, 56);

    m_item->setSegmentedPosition(0, 3);
    {
        const QList<qreal> radii = MdListItemStyle::radiiFor(*m_item, m_item->tokens(), size);
        QCOMPARE(radii.at(0), outer);
        QCOMPARE(radii.at(1), outer);
        QCOMPARE(radii.at(2), inner);
        QCOMPARE(radii.at(3), inner);
    }

    m_item->setSegmentedPosition(2, 3);
    {
        const QList<qreal> radii = MdListItemStyle::radiiFor(*m_item, m_item->tokens(), size);
        QCOMPARE(radii.at(0), inner);
        QCOMPARE(radii.at(1), inner);
        QCOMPARE(radii.at(2), outer);
        QCOMPARE(radii.at(3), outer);
    }

    m_item->setSegmentedPosition(1, 3);
    {
        const QList<qreal> radii = MdListItemStyle::radiiFor(*m_item, m_item->tokens(), size);
        QCOMPARE(radii.at(0), inner);
        QCOMPARE(radii.at(3), inner);
    }

    m_item->setSegmentedPosition(0, 1);
    {
        const QList<qreal> radii = MdListItemStyle::radiiFor(*m_item, m_item->tokens(), size);
        QCOMPARE(radii.at(0), outer);
        QCOMPARE(radii.at(1), outer);
        QCOMPARE(radii.at(2), outer);
        QCOMPARE(radii.at(3), outer);
    }
}

void TestMd3List::interactionStatesPaintLayers()
{
    const QColor surface = MdTheme::instance().color(ColorRole::Surface);
    const QColor secondaryContainer =
        MdTheme::instance().color(ColorRole::SecondaryContainer);

    m_item->resize(360, 56);
    QCOMPARE(pixelColorAt(*m_item, QPointF(180, 28)), surface);

    // Hover arrives as an enter event and is cleared by a leave event. The
    // tests drive it directly rather than with QTest::mouseMove, which does not
    // synthesise an enter under the offscreen plugin on Qt 5 — the same
    // approach TestMd3Card and TestMd3SplitButton take.
    QEnterEvent enter(QPointF(4, 4), QPointF(4, 4), QPointF(4, 4));
    QEvent leave(QEvent::Leave);

    // A non-interactive item ignores hover entirely.
    m_item->setInteractive(false);
    QApplication::sendEvent(m_item, &enter);
    QCOMPARE(pixelColorAt(*m_item, QPointF(180, 28)), surface);
    QApplication::sendEvent(m_item, &leave);

    // An interactive one paints the hover layer (a blend towards on-surface).
    m_item->setInteractive(true);
    QApplication::sendEvent(m_item, &enter);
    QVERIFY(m_item->isHovered());
    const QColor hovered = pixelColorAt(*m_item, QPointF(180, 28));
    QVERIFY(colourDistance(hovered, surface) > 0);
    QApplication::sendEvent(m_item, &leave);
    QVERIFY(!m_item->isHovered());

    // Selection wins over the layer: the container itself changes.
    m_item->setSelected(true);
    const QColor selected = pixelColorAt(*m_item, QPointF(180, 28));
    QVERIFY(colourDistance(selected, secondaryContainer) < colourDistance(selected, surface));
    m_item->setSelected(false);

    // An unselected disabled item publishes *no* container row, so its
    // container is untouched — the export fades the content instead, element by
    // element at 0.38. (material-web fades the whole item with `opacity: 0.38`
    // and lands in the same place over a surface-coloured list; the two models
    // only part company on a disabled *selected* item, covered below.)
    const QColor onSurface = MdTheme::instance().color(ColorRole::OnSurface);
    // The only ink in this item is the headline, so the darkest pixel anywhere
    // in it is a glyph pixel. Scanning the whole rect rather than a fixed band
    // keeps the assertion independent of the platform's font metrics.
    const QRect glyphBand = m_item->rect();
    const QColor enabledGlyph = darkestPixelIn(*m_item, glyphBand);
    // Whether a platform draws glyphs at all is not the library's business:
    // Qt 5's offscreen plugin ships no font database and `drawText` then
    // produces no ink, so the content-fade half of this assertion only runs
    // where the enabled item demonstrably painted something. The token table
    // above pins the 0.38 opacities either way.
    const bool rendersText = colourDistance(enabledGlyph, onSurface) < 140;
    if (rendersText) {
        QVERIFY(colourDistance(enabledGlyph, surface) > 0);
    }

    m_item->setEnabled(false);
    // The container is untouched — the export publishes no container row for an
    // unselected disabled item.
    QCOMPARE(pixelColorAt(*m_item, QPointF(180, 28)), surface);
    if (rendersText) {
        // ...while the content fades: the darkest glyph pixel lands measurably
        // further from on-surface than the enabled one did.
        const QColor disabledGlyph = darkestPixelIn(*m_item, glyphBand);
        QVERIFY(colourDistance(disabledGlyph, onSurface)
                > colourDistance(enabledGlyph, onSurface));
    }
    m_item->setEnabled(true);
}

void TestMd3List::selectionDrivesTheColours()
{
    m_item->resize(360, 56);
    const QColor secondaryContainer =
        MdTheme::instance().color(ColorRole::SecondaryContainer);

    m_item->setSelected(true);
    QCOMPARE(pixelColorAt(*m_item, QPointF(180, 28)), secondaryContainer);

    // The selected disabled row composites on-surface over the selected
    // container at 0.38 — so it is neither the container nor plain surface.
    m_item->setEnabled(false);
    const QColor disabledSelected = pixelColorAt(*m_item, QPointF(180, 28));
    QVERIFY(colourDistance(disabledSelected, secondaryContainer) > 0);
    QVERIFY(colourDistance(disabledSelected, MdTheme::instance().color(ColorRole::Surface)) > 0);
    m_item->setEnabled(true);
    m_item->setSelected(false);
}

void TestMd3List::theListDrivesSelectionModes()
{
    MdList list;
    list.resize(360, 200);
    list.show();

    auto *first = new MdListItem(QStringLiteral("One"));
    auto *second = new MdListItem(QStringLiteral("Two"));
    auto *third = new MdListItem(QStringLiteral("Three"));
    for (MdListItem *item : {first, second, third}) {
        item->setInteractive(true);
        list.addItem(item);
    }
    QCOMPARE(list.itemCount(), 3);

    // --- None: activation never selects -------------------------------------
    QCOMPARE(int(list.selectionMode()), int(MdList::SelectionMode::None));
    QVERIFY(list.activateItem(second));
    QVERIFY(!second->isSelected());
    QCOMPARE(list.selectedItems().size(), 0);

    // --- Single: exactly one, and it moves ----------------------------------
    list.setSelectionMode(MdList::SelectionMode::Single);
    QVERIFY(list.activateItem(second));
    QCOMPARE(list.selectedItems().size(), 1);
    QCOMPARE(list.selectedItems().first(), second);

    QVERIFY(list.activateItem(third));
    QCOMPARE(list.selectedItems().size(), 1);
    QCOMPARE(list.selectedItems().first(), third);
    QVERIFY(!second->isSelected());

    // --- Multiple: toggles -----------------------------------------------------
    list.setSelectionMode(MdList::SelectionMode::Multiple);
    QVERIFY(list.activateItem(first));
    QCOMPARE(list.selectedItems().size(), 2);
    QVERIFY(list.activateItem(first));
    QCOMPARE(list.selectedItems().size(), 1);

    // --- Back to None: the selection is cleared --------------------------------
    list.setSelectionMode(MdList::SelectionMode::None);
    QCOMPARE(list.selectedItems().size(), 0);
}

void TestMd3List::keyboardNavigationWalksActivatableItems()
{
    MdList list;
    list.resize(360, 240);
    list.show();

    auto *text = new MdListItem(QStringLiteral("Text item")); // not interactive
    auto *firstInteractive = new MdListItem(QStringLiteral("One"));
    auto *disabled = new MdListItem(QStringLiteral("Disabled"));
    auto *last = new MdListItem(QStringLiteral("Last"));
    for (MdListItem *item : {text, firstInteractive, disabled, last}) {
        item->setInteractive(true);
    }
    text->setInteractive(false);
    disabled->setEnabled(false);
    for (MdListItem *item : {text, firstInteractive, disabled, last}) {
        list.addItem(item);
    }
    QApplication::processEvents();

    // The first activatable item takes the tab stop by default, skipping the
    // non-interactive one.
    QCOMPARE(list.activeItem(), firstInteractive);

    // ArrowDown walks to the next activatable item, skipping the disabled one.
    firstInteractive->setFocus(Qt::TabFocusReason);
    QTest::keyClick(firstInteractive, Qt::Key_Down);
    QCOMPARE(list.activeItem(), last);
    QVERIFY(last->hasFocus());

    // ...and wraps back to the first, because wrap is on by default.
    QTest::keyClick(last, Qt::Key_Down);
    QCOMPARE(list.activeItem(), firstInteractive);

    // Up wraps the other way.
    QTest::keyClick(firstInteractive, Qt::Key_Up);
    QCOMPARE(list.activeItem(), last);

    // Home / End jump to the ends.
    QTest::keyClick(last, Qt::Key_Home);
    QCOMPARE(list.activeItem(), firstInteractive);
    QTest::keyClick(firstInteractive, Qt::Key_End);
    QCOMPARE(list.activeItem(), last);

    // With wrapping off, the ends are the ends.
    list.setWrapNavigation(false);
    QTest::keyClick(last, Qt::Key_Down);
    QCOMPARE(list.activeItem(), last);
    QTest::keyClick(firstInteractive, Qt::Key_Up);
    QCOMPARE(list.activeItem(), firstInteractive);

    // The inline keys behave like the block ones for a vertical list.
    QTest::keyClick(firstInteractive, Qt::Key_Down);
    QTest::keyClick(last, Qt::Key_Left);
    QCOMPARE(list.activeItem(), firstInteractive);
}

void TestMd3List::renderSmoke()
{
    const QColor surface = MdTheme::instance().color(ColorRole::Surface);
    const QColor primaryContainer = MdTheme::instance().color(ColorRole::PrimaryContainer);

    m_item->setHeadline(QStringLiteral("Headline"));
    m_item->resize(360, 56);
    QCOMPARE(pixelColorAt(*m_item, QPointF(180, 28)), surface);

    // An avatar paints its disc — primary-container, in the 40px slot.
    m_item->setLeadingKind(MdListItem::LeadingKind::Avatar);
    m_item->resize(360, 56);
    const MdListItemStyle::Layout avatarLayout =
        MdListItemStyle::layoutFor(*m_item, m_item->tokens());
    QCOMPARE(avatarLayout.leading.size(), QSizeF(40.0, 40.0));
    QCOMPARE(pixelColorAt(*m_item, avatarLayout.leading.center()), primaryContainer);
    m_item->setLeadingKind(MdListItem::LeadingKind::None);

    // The list container paints the published container colour rounded by
    // `container.shape`.
    MdList list;
    list.resize(300, 120);
    list.show();
    QCOMPARE(pixelColorAt(list, QPointF(150, 60)), surface);
    const MdListStyle::Layout listLayout = MdListStyle::layoutFor(list, list.tokens());
    QVERIFY(listLayout.container.width() > 0);
    QCOMPARE(listLayout.radii.size(), 4);
    QCOMPARE(listLayout.radii.first(),
             MdShape::resolvedRadius(ShapeCorner::Large, listLayout.container.size()));
}

QTEST_MAIN(TestMd3List)
#include "TestMd3List.moc"
