#ifndef MD_BUTTON_GROUP_STYLE_H
#define MD_BUTTON_GROUP_STYLE_H

// MdButtonGroupStyle — the geometry and shape authority for an MdButtonGroup.
//
// This one is unusual, and the reason is worth stating plainly because the
// alternative looks like an oversight:
//
//   **A button group paints nothing.** The design spec says so twice over —
//   "Button groups are invisible containers that add padding between buttons
//   and modify button shape. They don't contain any buttons by default." and
//   "Button groups have no color properties."
//
// So `drawWidget()` is deliberately empty and no paint filter is installed: a
// filter would intercept paint events in order to draw nothing, which would
// only make the absence of paint look like a bug. What the class *does* own is
// the arithmetic — where each item sits, and what corner each item shows
// towards each neighbour — because that is the whole of what a group
// contributes and it has to be identical for measurement and for placement.
//
// Everything here is static and pure: it takes the group and its tokens and
// returns rectangles and radii. That is what makes the layout testable without
// a window, and what lets the group and its style never disagree.

#include "core/MdButtonGroupTokens.h"
#include "core/MdTypes.h"
#include "styles/MdStyleBase.h"

#include <QtCore/QList>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtCore/QVector>

class QWidget;

namespace md {

class MdButtonGroup;
class MdButton;

class QT_MD3_EXPORT MdButtonGroupStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdButtonGroupStyle(QObject *parent = nullptr);

    /// The single instance. Created on first use and never destroyed, for the
    /// same reason MdButtonStyle::shared() is: it has to outlive every widget.
    ///
    /// Unlike MdButtonStyle it does **not** install a paint filter — see the
    /// header note.
    static MdButtonGroupStyle *shared();

    /// Intentionally empty. A button group has no pixels of its own; it only
    /// changes the geometry and shape of the buttons inside it.
    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// Where every item goes, in group-local widget coordinates.
    struct Layout
    {
        /// One widget rect per item, in item order.
        QVector<QRectF> items;
        /// The bounding box of `items`, which is also what sizeHint() reports.
        QRectF bounds;
        /// The container cross-axis extent (the token height).
        qreal crossExtent = 0.0;
        /// The union the group's own sizeHint() should request.
        QSizeF preferredSize;
    };

    /// Lay the group out for its current size.
    ///
    /// `growth` is the extra *main-axis* extent each item is currently showing
    /// beyond its natural size, as a multiplier in [0, pressedWidthMultiplier].
    /// An empty vector means "everything at rest".
    static Layout layoutFor(const MdButtonGroup &group,
                            const MdButtonGroupTokens &tokens,
                            const QVector<qreal> &growth = QVector<qreal>());

    /// Corner radii, TL / TR / BR / BL, for one item.
    ///
    /// Empty when the item should keep the shape its own button tokens give it,
    /// which is the whole of the *standard* form: a standard group only ever
    /// changes an item's shape by swapping round for square, and never its
    /// corner radii — the spec states the standard form "changes the width,
    /// shape, and padding" of a button, and the shape in question is the
    /// button's own round/square token, not a per-corner one.
    ///
    /// `containerSize` is the item's container box. The leading item's leading
    /// side and the trailing item's trailing side take the group's
    /// `container.shape`; every other edge takes the inner corner, or the
    /// fraction the token publishes while the item is selected.
    static QList<qreal> itemRadii(const MdButtonGroupTokens &tokens,
                                  int index,
                                  int count,
                                  ButtonGroupOrientation orientation,
                                  bool selected,
                                  bool pressed,
                                  const QSizeF &containerSize,
                                  bool rightToLeft);

    /// The rounding the group gives its items, as a button shape token.
    ///
    /// The spec: "When a toggle button is selected in a standard button group,
    /// its shape should change between square and round." So an item that is
    /// not selected gets the group's shape and a selected item gets the other
    /// one.
    static ButtonShape itemShape(const MdButtonGroup &group, bool selected);

    /// The margin an item button reserves around its container for the outward
    /// focus indicator. Qt clips a child to its own rectangle, so the group has
    /// to overlap the item rects by this amount to get the containers the
    /// token's `between-space` apart.
    static qreal itemMargin(const MdButton &button);

    /// The container box one item naturally wants, before any press growth.
    ///
    /// This is the single place `minimumItemExtent` is applied — "Extra small
    /// and small connected button groups have 48 dp target areas and a minimum
    /// width of 48 dp" — so that measuring the row and resolving an item's
    /// inner corners can never disagree about how big the item is.
    static QSizeF naturalContainerSize(const MdButton &item,
                                       const MdButtonGroupTokens &tokens,
                                       ButtonGroupOrientation orientation);
};

} // namespace md

#endif // MD_BUTTON_GROUP_STYLE_H
