#include "MdButtonGroupStyle.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "styles/MdButtonStyle.h"
#include "widgets/MdButton.h"
#include "widgets/MdButtonGroup.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>

namespace md {

MdButtonGroupStyle::MdButtonGroupStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdButtonGroupStyle *MdButtonGroupStyle::shared()
{
    static QMutex mutex;
    static MdButtonGroupStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed, for the same reason MdButtonStyle's
        // singleton is not: it outlives every widget.
        //
        // No installPaintFilter() call. A button group has no pixels of its own
        // (see the header), so filtering its paint events would exist only to
        // draw nothing.
        instance = new MdButtonGroupStyle;
    }
    return instance;
}

void MdButtonGroupStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    // Deliberately empty. The spec defines a button group as an invisible
    // container whose entire contribution is spacing and shape, and both of
    // those are applied to the *items* rather than drawn here. Anything painted
    // at this point would be a container the design system does not have.
    Q_UNUSED(painter);
    Q_UNUSED(widget);
}

QSizeF MdButtonGroupStyle::naturalContainerSize(const MdButton &item,
                                                const MdButtonGroupTokens &tokens,
                                                ButtonGroupOrientation orientation)
{
    QSizeF size = MdButtonStyle::layoutFor(item, item.tokens()).preferredContainerSize;

    // "Extra small and small connected button groups have 48 dp target areas
    // and a minimum width of 48 dp." The minimum is on the main axis, which is
    // the width when the group runs horizontally and the height when it runs
    // vertically.
    if (tokens.minimumItemExtent > 0.0) {
        if (orientation == ButtonGroupOrientation::Vertical) {
            size.setHeight(qMax(size.height(), tokens.minimumItemExtent));
        } else {
            size.setWidth(qMax(size.width(), tokens.minimumItemExtent));
        }
    }
    return size;
}

qreal MdButtonGroupStyle::itemMargin(const MdButton &button)
{
    const MdButtonTokens &tokens = button.tokens();
    return MdButtonStyle::focusRingInset(MdButtonStyle::focusRingSpec(tokens));
}

ButtonShape MdButtonGroupStyle::itemShape(const MdButtonGroup &group, bool selected)
{
    const ButtonShape base = group.groupShape();
    if (!selected) {
        return base;
    }
    // "When a toggle button is selected in a standard button group, its shape
    // should change between square and round." Which is to say: the other one.
    return base == ButtonShape::Round ? ButtonShape::Square : ButtonShape::Round;
}

QList<qreal> MdButtonGroupStyle::itemRadii(const MdButtonGroupTokens &tokens,
                                           int index,
                                           int count,
                                           ButtonGroupOrientation orientation,
                                           bool selected,
                                           bool pressed,
                                           const QSizeF &containerSize,
                                           bool rightToLeft)
{
    // The standard form never overrides corners: each item keeps the shape its
    // own button tokens give it, and the group only swaps round for square on
    // selection (see itemShape()).
    if (tokens.variant != ButtonGroupVariant::Connected || count <= 0) {
        return QList<qreal>();
    }

    const bool horizontal = orientation == ButtonGroupOrientation::Horizontal;
    const qreal crossExtent = horizontal ? containerSize.height() : containerSize.width();
    if (crossExtent <= 0.0) {
        return QList<qreal>();
    }

    const qreal outer = MdShape::resolvedRadius(tokens.outerCorner, containerSize);

    // Pressed wins over selected. The two are published as separate token sets
    // and there is no `selected.pressed.inner-corner` to consult, so the only
    // question is which state a segment that is both shows. Press is the more
    // transient and more specific of the two, and letting it win is what makes
    // pressing an already-selected segment still read as a press.
    qreal inner = 0.0;
    if (pressed) {
        inner = MdShape::resolvedRadius(tokens.pressedInnerCorner, containerSize);
    } else if (selected) {
        // `selected.inner-corner.corner-size: 50%`, i.e. half the cross-axis
        // extent — which is exactly the radius that turns those two corners
        // into the ends of a pill.
        inner = tokens.selectedInnerCornerFraction * crossExtent;
    } else {
        inner = MdShape::resolvedRadius(tokens.innerCorner, containerSize);
    }

    const qreal leading = index == 0 ? outer : inner;
    const qreal trailing = index == count - 1 ? outer : inner;

    // TL, TR, BR, BL.
    if (horizontal) {
        return rightToLeft ? QList<qreal>{trailing, leading, leading, trailing}
                           : QList<qreal>{leading, trailing, trailing, leading};
    }
    return QList<qreal>{leading, leading, trailing, trailing};
}

MdButtonGroupStyle::Layout MdButtonGroupStyle::layoutFor(const MdButtonGroup &group,
                                                         const MdButtonGroupTokens &tokens,
                                                         const QVector<qreal> &growth)
{
    Layout layout;
    const int count = group.count();
    if (count <= 0) {
        return layout;
    }

    const bool horizontal = group.orientation() == ButtonGroupOrientation::Horizontal;
    const bool rightToLeft = horizontal && MdTheme::instance().isRightToLeft();

    // Natural container extent per item, and the largest margin any item
    // reserves. The margin is an MdButton concern — room for the outward focus
    // indicator — and every item of a group is an MdButton, but the tokens are
    // per item, so it is measured per item rather than assumed equal.
    QVector<qreal> baseExtent(count, 0.0);
    qreal margin = 0.0;
    qreal maxBaseExtent = 0.0;
    qreal maxNaturalWidth = 0.0;
    for (int i = 0; i < count; ++i) {
        const MdButton *item = group.itemAt(i);
        if (item == nullptr) {
            continue;
        }
        const QSizeF natural = naturalContainerSize(*item, tokens, group.orientation());
        const qreal extent = horizontal ? natural.width() : natural.height();
        baseExtent[i] = extent;
        maxBaseExtent = qMax(maxBaseExtent, extent);
        maxNaturalWidth = qMax(maxNaturalWidth, natural.width());
        margin = qMax(margin, itemMargin(*item));
    }

    // The token height is the cross extent of a horizontal group, and the
    // *minimum* width of a vertical one. A vertical group whose items were all
    // clamped to 40 px would clip their labels, so the column is as wide as the
    // widest item needs — every item sharing that width, the way a column of
    // buttons reads.
    const qreal crossExtent = horizontal ? tokens.containerHeight
                                         : qMax(tokens.containerHeight, maxNaturalWidth);

    // Rest geometry: the row as it is with nothing pressed. This is the frame
    // every displacement below is measured against, and — importantly — it is
    // also the frame the *widget* is sized for. A pressed item grows, but the
    // row it grew out of must stay where it was, or the growth would re-centre
    // the whole group and the item would appear to slide rather than swell.
    QVector<qreal> restStart(count, 0.0);
    QVector<qreal> extra(count, 0.0);
    qreal restMain = 0.0;
    qreal totalExtra = 0.0;
    {
        qreal cursor = 0.0;
        for (int i = 0; i < count; ++i) {
            restStart[i] = cursor;
            cursor += baseExtent.at(i) + tokens.betweenSpace;
            restMain += baseExtent.at(i);
            // `pressedWidthMultiplier` is 0 for the connected form, because a
            // connected item does not move its neighbours.
            if (i < growth.size() && growth.at(i) > 0.0) {
                extra[i] = baseExtent.at(i) * growth.at(i);
            }
            totalExtra += extra.at(i);
        }
        restMain += tokens.betweenSpace * (count - 1);
    }

    // A growing item grows about its own centre. Taking half of the row's total
    // growth off the start and letting each item sit after whatever grew to its
    // left is what makes that true for every item at once: for the single
    // pressed item it is the spec's "changes the width of itself and adjacent
    // buttons" — the items before it move left by half the growth, the items
    // after it move right by half, and the pressed item's own centre does not
    // move at all.
    QVector<qreal> shift(count, 0.0);
    {
        const qreal halfTotal = totalExtra / 2.0;
        qreal before = 0.0;
        for (int i = 0; i < count; ++i) {
            shift[i] = before - halfTotal;
            before += extra.at(i);
        }
    }

    // The widget has to be big enough for the row *and* for the moment the
    // pressed item is at full growth, or Qt would clip the growing item at the
    // group's edge. One multiplier's worth of slack is all the shifts above can
    // ever need: nothing moves by more than half of the growth.
    //
    // That reserve *is* the reported size, which is why it does not change while
    // an item is pressed: a group whose sizeHint grew mid-press would ask its
    // parent to resize it, and the row would jump. The growth lives inside the
    // slack, so the widget never needs to move.
    const qreal growthReserve = maxBaseExtent * tokens.pressedWidthMultiplier;
    const qreal preferredMain = restMain + growthReserve + 2.0 * margin;
    const qreal preferredCross = crossExtent + 2.0 * margin;
    layout.preferredSize = horizontal ? QSizeF(preferredMain, preferredCross)
                                      : QSizeF(preferredCross, preferredMain);

    layout.crossExtent = crossExtent;

    // Place inside whatever the widget actually is, never smaller than what the
    // items need: overlapping containers would be worse than an over-tall
    // widget, so a squeezed group keeps its content size and simply offers less
    // slack.
    //
    // The main axis is measured against the *rest* preferred size, not the
    // grown one. Sizing it against the grown one would make the available space
    // — and with it the origin — move in step with the growth, and the pressed
    // item would slide instead of swelling.
    const QSizeF size = group.size();
    const qreal mainAvail = qMax(horizontal ? qreal(size.width()) : qreal(size.height()),
                                 preferredMain);
    const qreal crossAvail = qMax(horizontal ? qreal(size.height()) : qreal(size.width()),
                                  preferredCross);
    const qreal mainOrigin = (mainAvail - restMain - growthReserve) / 2.0;
    const qreal crossOrigin = (crossAvail - crossExtent) / 2.0;

    layout.items.reserve(count);
    QRectF bounds;
    for (int i = 0; i < count; ++i) {
        const qreal extent = baseExtent.at(i) + extra.at(i);
        const qreal start = mainOrigin + restStart.at(i) + shift.at(i);

        QRectF rect;
        if (horizontal) {
            rect = QRectF(start - margin, crossOrigin - margin, extent + 2.0 * margin,
                          crossExtent + 2.0 * margin);
            if (rightToLeft) {
                rect.moveLeft(mainAvail - (rect.left() + rect.width()));
            }
        } else {
            rect = QRectF(crossOrigin - margin, start - margin, crossExtent + 2.0 * margin,
                          extent + 2.0 * margin);
        }

        layout.items.append(rect);
        bounds = bounds.isNull() ? rect : bounds.united(rect);
    }
    layout.bounds = bounds;

    return layout;
}

} // namespace md
