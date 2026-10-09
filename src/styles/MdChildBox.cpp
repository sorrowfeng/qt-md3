#include "MdChildBox.h"

#include "widgets/MdButton.h"
#include "widgets/MdCard.h"
#include "widgets/MdExtendedFab.h"
#include "widgets/MdFab.h"
#include "widgets/MdFabMenuItem.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdListItem.h"
#include "widgets/MdSplitButton.h"

#include <QtWidgets/QWidget>

#include <cmath>

namespace md {

namespace {

/// One component of the dispatch: succeeds only when `widget` really is one of
/// them. Written as a template so the list below reads as a list of types
/// rather than a list of near-identical branches.
template <typename Component>
bool containerRectOf(const QWidget *widget, QRectF *out)
{
    if (const auto *component = qobject_cast<const Component *>(widget)) {
        *out = component->containerRect();
        return true;
    }
    return false;
}

} // namespace

MdChildBox MdChildBox::measure(QWidget *widget)
{
    MdChildBox box;
    if (widget == nullptr) {
        return box;
    }

    box.m_widget = widget;

    // A widget that has never been given a size answers against a 0 x 0 rect,
    // which every one of these layouts turns into "no room for the container".
    const QSize hint = widget->sizeHint();
    if (widget->size() != hint) {
        widget->resize(hint);
    }
    box.m_widgetSize = widget->size();

    // The closed set of components that reserve a focus margin. Ordered from
    // the most common slot filler outwards; the order only matters for types
    // that inherit from each other, and none of these do.
    QRectF container;
    const bool known = containerRectOf<MdIconButton>(widget, &container)
                       || containerRectOf<MdButton>(widget, &container)
                       || containerRectOf<MdFab>(widget, &container)
                       || containerRectOf<MdExtendedFab>(widget, &container)
                       || containerRectOf<MdSplitButton>(widget, &container)
                       || containerRectOf<MdFabMenuItem>(widget, &container)
                       || containerRectOf<MdCard>(widget, &container)
                       || containerRectOf<MdListItem>(widget, &container);

    box.m_container = known ? container : QRectF(QPointF(0.0, 0.0), QSizeF(box.m_widgetSize));
    return box;
}

QRect MdChildBox::geometryOn(const QRectF &box) const
{
    if (m_widget == nullptr || !box.isValid()) {
        return QRect();
    }
    return QRect(int(std::lround(box.left() - m_container.left())),
                 int(std::lround(box.top() - m_container.top())), m_widgetSize.width(),
                 m_widgetSize.height());
}

} // namespace md
