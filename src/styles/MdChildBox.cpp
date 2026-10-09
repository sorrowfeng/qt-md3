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

QRect MdChildBox::resizedGeometryOn(const QRectF &box) const
{
    if (m_widget == nullptr || !box.isValid()) {
        return QRect();
    }
    // `m_widgetSize - m_container.size()` is exactly the margin the component
    // reserves a side, which is what has to be re-added around the new box.
    const QSizeF margin = QSizeF(m_widgetSize) - m_container.size();
    const QSizeF size = box.size() + margin;

    // The widget's **centre** goes on the box's centre, not its top-left on
    // the box's. That is the one rule that lands the container on the box for
    // both kinds of component: one whose container fills `widget - margin`
    // (then the two centres coincide anyway), and one whose container is a
    // fixed token size centred inside a larger widget — `MdFab`'s container is
    // always `containerWidth x containerHeight`, so a top-left alignment would
    // let it drift towards the middle of an oversized widget, and centring at
    // least leaves it centred on the box it could not fill.
    return QRect(int(std::lround(box.center().x() - size.width() / 2.0)),
                 int(std::lround(box.center().y() - size.height() / 2.0)),
                 int(std::lround(size.width())), int(std::lround(size.height())));
}

} // namespace md
