#include "MdNavigationRailStyle.h"

#include "styles/MdChildBox.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdNavigationBarItem.h"
#include "widgets/MdNavigationRail.h"

#include <QtGui/QPainter>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

namespace md {

MdNavigationRailStyle::MdNavigationRailStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdNavigationRailStyle *MdNavigationRailStyle::shared()
{
    static QMutex mutex;
    static MdNavigationRailStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdNavigationRailStyle;
        installPaintFilter<MdNavigationRail>(instance);
    }
    return instance;
}

bool MdNavigationRailStyle::isInstalled()
{
    return hasPaintFilter(&MdNavigationRail::staticMetaObject);
}

MdNavigationRailStyle::Layout
MdNavigationRailStyle::layoutFor(const MdNavigationRail &rail, const MdNavigationRailTokens &tokens,
                                 bool expanded, bool modal)
{
    Q_UNUSED(modal);

    Layout layout;

    const QRectF rect(rail.rect());
    layout.container = rect;

    const MdNavigationRailVariantTokens &v = tokens.forVariant(rail.variant());

    layout.radii = MdShape::resolvedRadii(expanded && v.supportsExpanded() && rail.isModal()
                                              ? v.modalContainerShape
                                              : v.containerShape,
                                          rect.size());

    // Where the content starts. The flexible rail insets everything by its
    // published 44; the baseline rail's items start after its hard-coded 4.
    const qreal contentY = v.supportsExpanded() ? v.containerTopSpace : v.containerVerticalPadding;

    // The header first, if there is one. Compose places it at the content's top
    // and pushes the items down by its height plus the header gap (8
    // hard-coded in the baseline behaviour, the baseline *item* family's
    // `header-space-minimum` of 40 in the wide one).
    //
    // Read through `sizeHint()` only: this function runs from the *paint* path
    // too, and `MdChildBox::measure` would resize the widgets mid-paint and
    // leave them shrunk to their hints. (`placeChildren` does the real
    // measuring-and-placing, where every measure is followed by its
    // `setGeometry`.)
    qreal itemsY = contentY;
    if (rail.header() != nullptr && !rail.header()->isHidden()) {
        const qreal headerHeight = qreal(rail.header()->sizeHint().height());
        layout.headerBox = QRectF(0.0, contentY, rect.width(), headerHeight);
        itemsY = contentY + headerHeight + v.headerSpace;
    }

    const int count = int(rail.items().size());
    if (count <= 0) {
        return layout;
    }

    // The gap between items, by state: the flexible rail's collapsed 4 gives
    // way to the expanded family's own `between-item-space` (0) when expanded.
    // The baseline rail has one hard-coded 4.
    const qreal gap = (expanded && v.supportsExpanded()) ? v.expandedBetweenItemSpace
                                                         : v.itemVerticalSpace;

    // Item geometry. Collapsed items take the full width — centring their
    // content is the shared item's own arithmetic. Expanded items are held at
    // their natural width on the leading edge, with the hard-coded trailing
    // room left over: Compose constrains the item to `maxWidth - 20` and
    // `placeRelative(0, y)`. The item is its own container, so its container
    // size *is* its `sizeHint()` — read, not measured.
    const bool itemsExpanded = expanded && v.supportsExpanded();
    qreal totalHeight = 0.0;
    for (MdNavigationBarItem *item : rail.items()) {
        const QSizeF hint(item->sizeHint());
        const qreal width = itemsExpanded ? hint.width() : rect.width();
        layout.itemBoxes.append(QRectF(0.0, 0.0, width, hint.height()));
        totalHeight += hint.height();
    }
    totalHeight += gap * qreal(count - 1);

    // `Arrangement.Center` centres the run in the container's whole height,
    // ignoring the top inset and the header — Compose arranges against
    // `height + paddings` and negatively offsets the top padding. Otherwise the
    // run starts after the header.
    qreal y = itemsY;
    if (rail.arrangement() == MdNavigationRailArrangement::Center) {
        y = qMax<qreal>(0.0, (rect.height() - totalHeight) / 2.0);
    }

    for (QRectF &box : layout.itemBoxes) {
        box.moveTop(y);
        y += box.height() + gap;
    }

    return layout;
}

qreal MdNavigationRailStyle::expandedWidthFor(const MdNavigationRail &rail,
                                              const MdNavigationRailTokens &tokens)
{
    const MdNavigationRailVariantTokens &v = tokens.forVariant(rail.variant());
    if (!v.supportsExpanded()) {
        return v.containerWidth;
    }

    // The width follows the content: the widest element — header or the widest
    // item plus the expanded padding — inside the published bounds. Compose
    // measures exactly this (`expandedItemMaxWidth`, `headerPlaceable.width`)
    // and coerces it to `ContainerWidthMinimum` and the constraints. Read
    // through `sizeHint()` for the same no-side-effect reason as `layoutFor`.
    qreal widest = 0.0;
    if (rail.header() != nullptr && !rail.header()->isHidden()) {
        widest = qreal(rail.header()->sizeHint().width());
    }
    for (MdNavigationBarItem *item : rail.items()) {
        widest = qMax(widest, qreal(item->sizeHint().width()) + v.expandedItemPadding);
    }

    return qBound(v.expandedWidthMinimum, qMax(widest, v.expandedWidthMinimum),
                  v.expandedWidthMaximum);
}

void MdNavigationRailStyle::paintNavigationRail(QPainter &painter, const MdNavigationRail &rail,
                                                const MdNavigationRailTokens &tokens,
                                                const Layout &layout, bool modal)
{
    const MdNavigationRailVariantTokens &v = tokens.forVariant(rail.variant());

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(MdTheme::instance().color(modal && v.supportsExpanded()
                                                   ? v.modalContainerColor
                                                   : v.containerColor));
    painter.drawPath(MdShape::roundedRect(layout.container, layout.radii));
    painter.restore();

    // The container elevations (level0 standard — nothing to draw — and the
    // modal level2, which falls outside a child rail's rect) are carried and
    // not painted; see the header note and docs/porting-todo.md.
}

void MdNavigationRailStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *rail = qobject_cast<MdNavigationRail *>(widget);
    if (painter == nullptr || rail == nullptr) {
        return;
    }
    const MdNavigationRailTokens &tokens = rail->tokens();
    paintNavigationRail(*painter, *rail, tokens,
                        layoutFor(*rail, tokens, rail->isExpanded(), rail->isModal()),
                        rail->isModal());
}

} // namespace md
