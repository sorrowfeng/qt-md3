#ifndef MD_CHILD_BOX_H
#define MD_CHILD_BOX_H

// Where a child widget actually paints, and the geometry that puts that where
// the tokens say it goes.
//
// Every qt-md3 component that can show a focus indicator is an *outward* one:
// the indicator's outer edge sits at `offset + activeWidth / 2 + width / 2`
// beyond the container — 7.5 px at the published 3 px stroke, 2 px gap and
// 8 px active width. Qt clips a child widget to its own rectangle, so that
// margin cannot live outside the widget the way Compose's `drawWithContent`
// overlay does; the component has to reserve it *inside* itself and report a
// `sizeHint` of `container + 2 * margin`.
//
// That makes `sizeHint()` the wrong thing for a *container* to lay a child out
// by. `MdIconButton` is a 55 x 55 widget around a 40 x 40 container and
// `MdFab` a 71 x 71 widget around a 56 x 56 one, so placing the widget on a
// token box parks the visible container 7.5 px inside it and spaces siblings
// `container + 15` apart instead of `container` apart. `MdButtonGroup` was the
// first component to hit this and established the rule, recorded in
// docs/porting-todo.md — *place the container, not the widget, and accept that
// two neighbouring widget rects then overlap by `2 * margin - gap`* — and did
// the arithmetic inline in `MdButtonGroupStyle::layoutFor`. The app bars are
// the second and third containers to need it, so the rule lives here now
// rather than being re-derived per component.
//
// A widget with no such concept — anything that is not one of the components
// `measure()` knows about — answers with its own rectangle, so the naive answer
// is the fallback rather than an error. Add a component to the dispatch when it
// grows a `containerRect()`.

#include "core/QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtCore/QSize>

class QWidget;

namespace md {

/// A child widget's painted container, measured.
class QT_MD3_EXPORT MdChildBox
{
public:
    /// Sizes `widget` to the size it asks for, then measures the container it
    /// paints inside itself.
    ///
    /// Sizing first is not optional: `containerRect()` is a function of the
    /// widget's own rect, so measuring against a stale one answers against a
    /// rect the widget will never be given — silently, and smaller. `resize()`
    /// to the size a widget already has is a no-op in Qt, so calling this from
    /// both a layout and a placement pass is safe.
    static MdChildBox measure(QWidget *widget);

    bool isValid() const { return m_widget != nullptr; }

    /// The size the widget must be given — its own `sizeHint()`, margin included.
    QSize widgetSize() const { return m_widgetSize; }

    /// Where the container lands inside the widget, in the widget's coordinates.
    QRectF container() const { return m_container; }

    /// The container's natural size. This — not `widgetSize()` — is the extent
    /// a container advances its cursor by, so that siblings end up `container`
    /// apart rather than `container + 2 * margin`.
    QSizeF containerSize() const { return m_container.size(); }

    /// The geometry that puts the container's top-left on `box`'s, leaving the
    /// widget's own margin overhanging. Empty for an invalid box.
    QRect geometryOn(const QRectF &box) const;

private:
    QWidget *m_widget = nullptr;
    QSize m_widgetSize;
    QRectF m_container;
};

} // namespace md

#endif // MD_CHILD_BOX_H
