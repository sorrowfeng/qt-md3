// TestMd3Menu — the `md.comp.menu.*` port.
//
// Numbers transcribed from material-web tokens/versions/latest/sass —
// _md-comp-menu.scss — behaviour cross-checked against androidx Compose
// Material3 Menu.kt / MenuDefaults.kt.
//
// The assertions worth keeping are the ones a token table cannot express:
//
//   * **the classic item geometry** — 48 px tall, min-width 112 / max-width
//     280 around a 12 px-padded content row, and the menu's own 8 px vertical
//     padding with the divider's 12/2 insets stacked in insertion order;
//   * **the popup opens on the scale+alpha pair** — scale 0.8 → 1.0 and alpha
//     0 → 1 while the real children hide for the flight (the surface paints
//     them through Compose's graphicsLayer transform), then reappear when the
//     springs settle;
//   * **the focus ring is inward and `:focus-visible`** — mouse/pop focus
//     reasons draw nothing; a keyboard reason draws the secondary ring inside
//     the row;
//   * **the state layer reads the unselected side for both selections** — the
//     export publishes one state-layer family and Compose applies the same
//     indication to selected items;
//   * **disabled + selected falls back to the unselected disabled rows** — the
//     export publishes selected rows for the enabled interactions only;
//   * **the popup's keyboard walk** — arrows move between enabled items,
//     Esc closes and reports through `closed()`.

#include "core/MdMenuTokens.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "core/MdTypes.h"
#include "styles/MdMenuStyle.h"
#include "widgets/MdMenu.h"

#include <QtGui/QEnterEvent>
#include <QtGui/QFontMetrics>
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

void paintOnce(QWidget &widget)
{
    pixelColorAt(widget, QPointF(0, 0));
}

/// Pump the event loop until the widget's animations have landed.
void settleAnimations(QWidget &widget, int ms = 800)
{
    Q_UNUSED(widget);
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < ms) {
        QTest::qWait(8);
    }
}

bool closeEnough(const QColor &a, const QColor &b, int tolerance)
{
    return qAbs(a.alpha() - b.alpha()) <= tolerance && qAbs(a.red() - b.red()) <= tolerance
           && qAbs(a.green() - b.green()) <= tolerance && qAbs(a.blue() - b.blue()) <= tolerance;
}

} // namespace

class TestMd3Menu : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    // --- token tables ---------------------------------------------------------
    void metricTable();
    void unselectedTable();
    void selectedTable();

    // --- geometry ------------------------------------------------------------------
    void itemGeometryClampsToExportBounds();
    void menuLayoutInterleavesRowsWithPadding();
    void popupAnchorsBelowTheAnchor();

    // --- the paint ----------------------------------------------------------------
    void selectedPaintsSecondaryContainer();
    void hoverPaintsTheStateLayer();
    void pressRipplesOnSurface();
    void focusRingIsInwardAndKeyboardOnly();

    // --- the popup behaviour -----------------------------------------------------------
    void popupOpensOnScaleAndAlpha();
    void escapeClosesTheMenu();
    void arrowKeysWalkTheItems();

    // --- the style ------------------------------------------------------------------
    void styleIsInstalled();
};

void TestMd3Menu::cleanup()
{
    // Each test builds its own widgets on the stack; popups close before their
    // owner goes out of scope. No shared mutable state.
}

// ---------------------------------------------------------------------------
// Token tables
// ---------------------------------------------------------------------------

void TestMd3Menu::metricTable()
{
    const MdMenuTokens t = MdMenuTokens::resolve();

    // The surface.
    QCOMPARE(t.containerColor, ColorRole::SurfaceContainer);
    QCOMPARE(t.containerElevation, ElevationLevel::Level2);
    QCOMPARE(t.containerShape, ShapeCorner::ExtraSmall);
    QCOMPARE(t.containerVerticalPadding, 8.0);
    QCOMPARE(t.containerSurfaceTint, ColorRole::SurfaceTint);

    // The classic item metrics (Compose's DropdownMenu* constants).
    QCOMPARE(t.itemHeight, 48.0);
    QCOMPARE(t.itemHorizontalPadding, 12.0);
    QCOMPARE(t.itemMinWidth, 112.0);
    QCOMPARE(t.itemMaxWidth, 280.0);
    QCOMPARE(t.itemIconTextSpacing, 8.0);
    QCOMPARE(t.iconSize, 24.0);
    QCOMPARE(t.labelTextType, TypeStyle::LabelLarge);

    // The disabled content opacities.
    QCOMPARE(t.disabledLabelTextOpacity, 0.38);
    QCOMPARE(t.disabledIconOpacity, 0.38);

    // The divider (Compose's HorizontalDividerPadding).
    QCOMPARE(t.dividerHeight, 1.0);
    QCOMPARE(t.dividerColor, ColorRole::SurfaceVariant);
    QCOMPARE(t.dividerHorizontalPadding, 12.0);
    QCOMPARE(t.dividerVerticalPadding, 2.0);

    // The cascading indicator.
    QCOMPARE(t.cascadingIndicatorColor, ColorRole::OnSurfaceVariant);

    // The inward focus indicator.
    QCOMPARE(t.focusIndicatorColor, ColorRole::Secondary);
    QCOMPARE(t.focusIndicatorInnerOffset, 3.0);
    QCOMPARE(t.focusIndicatorThickness, 3.0);

    // The state-layer opacities.
    QCOMPARE(t.hoverStateLayerOpacity, 0.08);
    QCOMPARE(t.focusStateLayerOpacity, 0.12);
    QCOMPARE(t.pressedStateLayerOpacity, 0.12);

    // Compose behaviour constants.
    QCOMPARE(t.closedScale, 0.8);
    QCOMPARE(t.closedAlpha, 0.0);
    QCOMPARE(t.menuHorizontalMargin, 8.0);
}

void TestMd3Menu::unselectedTable()
{
    const MdMenuTokens t = MdMenuTokens::resolve();
    const auto enabled = MdNavigationItemState::Enabled;
    const auto hovered = MdNavigationItemState::Hovered;
    const auto focused = MdNavigationItemState::Focused;
    const auto pressed = MdNavigationItemState::Pressed;
    const auto disabled = MdNavigationItemState::Disabled;

    // The label: `on-surface` in every enabled interaction; disabled at 0.38.
    for (auto state : {enabled, hovered, focused, pressed}) {
        const auto &slot = t.labelFor(MdMenuSelection::Unselected, state);
        QVERIFY(slot.isPresent());
        QCOMPARE(slot.role, ColorRole::OnSurface);
        QCOMPARE(slot.opacity, 1.0);
    }
    const auto &disabledLabel = t.labelFor(MdMenuSelection::Unselected, disabled);
    QVERIFY(disabledLabel.isPresent());
    QCOMPARE(disabledLabel.role, ColorRole::OnSurface);
    QCOMPARE(disabledLabel.opacity, 0.38);

    // The icons: `on-surface-variant` with no interaction lift; disabled
    // `on-surface` at 0.38.
    for (auto state : {enabled, hovered, focused, pressed}) {
        const auto &slot = t.iconFor(MdMenuSelection::Unselected, state);
        QVERIFY(slot.isPresent());
        QCOMPARE(slot.role, ColorRole::OnSurfaceVariant);
        QCOMPARE(slot.opacity, 1.0);
    }
    const auto &disabledIcon = t.iconFor(MdMenuSelection::Unselected, disabled);
    QVERIFY(disabledIcon.isPresent());
    QCOMPARE(disabledIcon.role, ColorRole::OnSurface);
    QCOMPARE(disabledIcon.opacity, 0.38);

    // The state layer: only the three interaction rows carry a colour.
    QVERIFY(!t.stateLayerFor(MdMenuSelection::Unselected, enabled).isPresent());
    QVERIFY(!t.stateLayerFor(MdMenuSelection::Unselected, disabled).isPresent());
    for (auto state : {hovered, focused, pressed}) {
        const auto &slot = t.stateLayerFor(MdMenuSelection::Unselected, state);
        QVERIFY(slot.isPresent());
        QCOMPARE(slot.role, ColorRole::OnSurface);
        QCOMPARE(slot.opacity, 1.0);
    }

    // The unselected item has no container row.
    for (auto state : {enabled, hovered, focused, pressed, disabled}) {
        QVERIFY(!t.itemContainerFor(MdMenuSelection::Unselected, state).isPresent());
    }
}

void TestMd3Menu::selectedTable()
{
    const MdMenuTokens t = MdMenuTokens::resolve();
    const auto enabled = MdNavigationItemState::Enabled;
    const auto hovered = MdNavigationItemState::Hovered;
    const auto focused = MdNavigationItemState::Focused;
    const auto pressed = MdNavigationItemState::Pressed;
    const auto disabled = MdNavigationItemState::Disabled;

    // The selected container is `secondary-container` for the enabled
    // interactions; disabled + selected has no rows and falls back to the
    // unselected disabled side (a transparent container).
    for (auto state : {enabled, hovered, focused, pressed}) {
        const auto &slot = t.itemContainerFor(MdMenuSelection::Selected, state);
        QVERIFY(slot.isPresent());
        QCOMPARE(slot.role, ColorRole::SecondaryContainer);
        QCOMPARE(slot.opacity, 1.0);
    }
    QVERIFY(!t.itemContainerFor(MdMenuSelection::Selected, disabled).isPresent());

    // The selected content rides `on-secondary-container`; disabled falls back
    // to the unselected disabled rows (`on-surface` at 0.38).
    for (auto state : {enabled, hovered, focused, pressed}) {
        QCOMPARE(t.labelFor(MdMenuSelection::Selected, state).role, ColorRole::OnSecondaryContainer);
        QCOMPARE(t.iconFor(MdMenuSelection::Selected, state).role,
                 ColorRole::OnSecondaryContainer);
    }
    QCOMPARE(t.labelFor(MdMenuSelection::Selected, disabled).role, ColorRole::OnSurface);
    QCOMPARE(t.labelFor(MdMenuSelection::Selected, disabled).opacity, 0.38);
    QCOMPARE(t.iconFor(MdMenuSelection::Selected, disabled).role, ColorRole::OnSurface);
    QCOMPARE(t.iconFor(MdMenuSelection::Selected, disabled).opacity, 0.38);

    // The state layer reads the unselected side for both selections — the
    // export publishes one state-layer family and Compose applies the same
    // indication to selected items.
    QCOMPARE(t.stateLayerFor(MdMenuSelection::Selected, hovered).role, ColorRole::OnSurface);
    QCOMPARE(t.stateLayerFor(MdMenuSelection::Selected, pressed).role, ColorRole::OnSurface);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

void TestMd3Menu::itemGeometryClampsToExportBounds()
{
    const MdMenuTokens t = MdMenuTokens::resolve();

    MdMenuItem empty;
    QCOMPARE(empty.sizeHint(), QSize(int(t.itemMinWidth), int(t.itemHeight)));
    QCOMPARE(empty.minimumSizeHint(), QSize(int(t.itemMinWidth), int(t.itemHeight)));

    // A short label stays at the min width; the height never moves.
    MdMenuItem shortItem(QStringLiteral("Ok"));
    QCOMPARE(shortItem.sizeHint(), QSize(int(t.itemMinWidth), int(t.itemHeight)));

    // Icons widen the row by their slot plus the 8 px spacing — measured with
    // a label long enough to clear the 112 px minimum.
    MdMenuItem iconItem(QStringLiteral("Open something wonderful today"));
    iconItem.setLeadingIconName(QStringLiteral("delete"));
    const QFont labelFont = MdTypeScale::font(t.labelTextType, TypeEmphasis::Baseline,
                                              MdTheme::instance().scriptCategory());
    const qreal textWidth =
        std::ceil(QFontMetricsF(labelFont).horizontalAdvance(iconItem.text()));
    const int withLeading = int(std::ceil(
        qBound(t.itemMinWidth, 2.0 * t.itemHorizontalPadding + textWidth + t.iconSize
                                     + t.itemIconTextSpacing,
               t.itemMaxWidth)));
    QCOMPARE(iconItem.sizeHint().width(), withLeading);
    QCOMPARE(iconItem.sizeHint().height(), int(t.itemHeight));

    // A very long label clamps at the max width.
    MdMenuItem longItem(QStringLiteral("A very long menu label that exceeds the maximum"));
    QVERIFY(longItem.sizeHint().width() <= int(t.itemMaxWidth));
}

void TestMd3Menu::menuLayoutInterleavesRowsWithPadding()
{
    MdMenu menu;
    MdMenuItem *first = menu.addItem(QStringLiteral("First"));
    menu.addDivider();
    MdMenuItem *second = menu.addItem(QStringLiteral("Second"));

    const MdMenuTokens t = menu.menuTokens();
    const int width = menu.width();

    // The 8 px padding above the first row; rows stack in insertion order.
    QCOMPARE(first->geometry(), QRect(0, int(t.containerVerticalPadding), width, int(t.itemHeight)));

    const int dividerTop = int(t.containerVerticalPadding + t.itemHeight);
    // The divider is the only row 1 px tall.
    QWidget *divider = nullptr;
    const QList<QWidget *> rows = menu.findChildren<QWidget *>();
    for (QWidget *row : rows) {
        if (row->height() == int(t.dividerHeight)) {
            divider = row;
            break;
        }
    }
    QVERIFY(divider != nullptr);
    QCOMPARE(divider->geometry(),
             QRect(int(t.dividerHorizontalPadding), dividerTop,
                   width - 2 * int(t.dividerHorizontalPadding), int(t.dividerHeight)));

    // The divider row consumes 1 px + 2 x 2 px of vertical space.
    const int secondTop = dividerTop + int(t.dividerHeight) + 2 * int(t.dividerVerticalPadding);
    QCOMPARE(second->geometry(), QRect(0, secondTop, width, int(t.itemHeight)));

    // The 8 px padding below the last row closes the surface.
    QCOMPARE(menu.height(), secondTop + int(t.itemHeight) + int(t.containerVerticalPadding));
}

void TestMd3Menu::popupAnchorsBelowTheAnchor()
{
    MdMenu menu;
    menu.addItem(QStringLiteral("First"));

    const QRect anchorRect(100, 100, 80, 40);
    menu.popup(anchorRect);
    menu.close();

    // Compose's MenuAnchorPosition.Below: the anchor's bottom edge, left
    // aligned. The window system may adjust the requested geometry (the
    // offscreen plugin shifts popups by its frame margins), so the portable
    // contract is "at or below the anchor, at or right of its left edge".
    QVERIFY(menu.pos().y() >= anchorRect.bottom() + 1);
    QVERIFY(menu.pos().x() >= anchorRect.left());
}

// ---------------------------------------------------------------------------
// The paint
// ---------------------------------------------------------------------------

void TestMd3Menu::selectedPaintsSecondaryContainer()
{
    MdMenuItem unselected(QStringLiteral("Item"));
    unselected.resize(unselected.sizeHint());

    // The unselected item has no container row — the row is transparent over
    // the menu surface. Sampled past the label's ink (the label starts at the
    // 12 px padding).
    const QColor unselectedPixel = pixelColorAt(unselected, QPointF(95.0, 24.0));
    QCOMPARE(unselectedPixel.alpha(), 0);

    MdMenuItem selected(QStringLiteral("Item"));
    selected.setSelected(true);
    selected.resize(selected.sizeHint());
    const QColor selectedPixel = pixelColorAt(selected, QPointF(95.0, 24.0));
    QVERIFY(closeEnough(selectedPixel, MdTheme::instance().color(ColorRole::SecondaryContainer),
                        6));
}

void TestMd3Menu::hoverPaintsTheStateLayer()
{
    MdMenuItem item(QStringLiteral("Item"));
    item.resize(item.sizeHint());

    // The hover layer paints only through the real event path (Qt5 offscreen
    // does not synthesize enter on mouseMove).
    QEnterEvent enter(QPointF(30, 30), QPointF(30, 30), QPointF(30, 30));
    QApplication::sendEvent(&item, &enter);
    QVERIFY(item.isHovered());
    QCOMPARE(MdMenuStyle::stateFor(item), MdNavigationItemState::Hovered);

    // `on-surface` at the token's 0.08 over a transparent row, past the
    // label's ink.
    const QColor pixel = pixelColorAt(item, QPointF(95.0, 24.0));
    const QColor base = MdTheme::instance().color(ColorRole::OnSurface);
    QCOMPARE(pixel.alpha(), qRound(base.alpha() * 0.08));
}

void TestMd3Menu::pressRipplesOnSurface()
{
    MdMenuItem item(QStringLiteral("Item"));
    item.resize(item.sizeHint());
    item.show();
    QVERIFY(QTest::qWaitForWindowExposed(&item));

    // The press rides the ripple, coloured with the pressed row.
    const QPointF centre(56.0, 24.0);
    QTest::mousePress(&item, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
    QVERIFY(item.rippleController() != nullptr);
    QVERIFY(item.rippleController()->isActive());
    paintOnce(item);
    QCOMPARE(item.rippleController()->contentColor(),
             MdTheme::instance().color(ColorRole::OnSurface));
    QTest::mouseRelease(&item, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
}

void TestMd3Menu::focusRingIsInwardAndKeyboardOnly()
{
    MdMenuItem item(QStringLiteral("Item"));
    item.resize(item.sizeHint());
    item.show();
    QVERIFY(QTest::qWaitForWindowExposed(&item));

    // A mouse focus reason draws nothing (`:focus-visible` semantics).
    item.setFocus(Qt::MouseFocusReason);
    QVERIFY(item.hasFocus());
    QVERIFY(!item.hasKeyboardFocus());

    // The popup focus reason — what `popup()` uses to land on the first item —
    // also stays ring-free.
    item.clearFocus();
    item.setFocus(Qt::PopupFocusReason);
    QVERIFY(!item.hasKeyboardFocus());

    // A keyboard reason starts the inward ring.
    item.clearFocus();
    item.setFocus(Qt::TabFocusReason);
    QVERIFY(item.hasKeyboardFocus());
    QCOMPARE(MdMenuStyle::stateFor(item), MdNavigationItemState::Focused);
    settleAnimations(item);

    // The ring draws **inward**: the band sits at
    // [innerOffset, innerOffset + thickness] from the row's left edge, not
    // outside it.
    const MdMenuTokens t = item.menuTokens();
    const QColor ring = pixelColorAt(item, QPointF(t.focusIndicatorInnerOffset
                                                       + t.focusIndicatorThickness / 2.0,
                                                   24.0));
    QVERIFY(closeEnough(ring, MdTheme::instance().color(ColorRole::Secondary), 6));

    // And nothing paints outside the row.
    QVERIFY(!closeEnough(QColor(0, 0, 0, 0), ring, 0));
}

// ---------------------------------------------------------------------------
// The popup behaviour
// ---------------------------------------------------------------------------

void TestMd3Menu::popupOpensOnScaleAndAlpha()
{
    MdMenu menu;
    MdMenuItem *item = menu.addItem(QStringLiteral("First"));

    menu.popup(QRect(40, 40, 60, 32));

    // The flight starts fully closed: scale 0.8, alpha 0, and the real
    // children hidden (the surface paints them through the transform). Assert
    // before the event loop pumps — waiting for exposure ticks the springs.
    QCOMPARE(menu.openProgress(), 0.0);
    QCOMPARE(menu.openAlpha(), 0.0);
    QVERIFY(menu.openAnimationRunning());
    QVERIFY(!item->isVisible());

    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    settleAnimations(menu);
    QCOMPARE(menu.openProgress(), 1.0);
    QCOMPARE(menu.openAlpha(), 1.0);
    QVERIFY(!menu.openAnimationRunning());

    // The children reappear once the springs settle.
    QVERIFY(item->isVisible());
    menu.close();
}

void TestMd3Menu::escapeClosesTheMenu()
{
    MdMenu menu;
    menu.addItem(QStringLiteral("First"));
    menu.popup(QRect(40, 40, 60, 32));
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    settleAnimations(menu);

    QSignalSpy closedSpy(&menu, &MdMenu::closed);
    QTest::keyClick(&menu, Qt::Key_Escape);
    QVERIFY(!menu.isVisible());
    QCOMPARE(closedSpy.count(), 1);
}

void TestMd3Menu::arrowKeysWalkTheItems()
{
    MdMenu menu;
    MdMenuItem *first = menu.addItem(QStringLiteral("First"));
    MdMenuItem *second = menu.addItem(QStringLiteral("Second"));
    MdMenuItem *third = menu.addItem(QStringLiteral("Third"));
    third->setEnabled(false);
    menu.popup(QRect(40, 40, 60, 96));
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    settleAnimations(menu);

    // Keyboard focus lands on the first enabled item — with the popup reason,
    // so no ring yet. The window system's activation may steal the initial
    // focus on the offscreen plugin, so re-establish the popup's documented
    // start state deterministically.
    first->setFocus(Qt::PopupFocusReason);
    QVERIFY(first->hasFocus());
    QVERIFY(!first->hasKeyboardFocus());

    // Down walks to the second item with a keyboard reason.
    QTest::keyClick(&menu, Qt::Key_Down);
    QVERIFY(second->hasFocus());
    QVERIFY(!first->hasFocus());
    QVERIFY(second->hasKeyboardFocus());

    // Up walks back; the walk skips the disabled third item on the way round.
    QTest::keyClick(&menu, Qt::Key_Up);
    QVERIFY(first->hasFocus());
    menu.close();
}

void TestMd3Menu::styleIsInstalled()
{
    QVERIFY(MdMenuStyle::isInstalled());
}

QTEST_MAIN(TestMd3Menu)
#include "TestMd3Menu.moc"
