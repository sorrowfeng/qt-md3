#ifndef MD_BADGED_BOX_H
#define MD_BADGED_BOX_H

// MdBadgedBox — the anchor container that positions an MdBadge over content.
//
// The placement facts are Compose BadgedBox's (Badge.kt) — material-web does
// not implement the component, so there is no web behaviour to port:
//
//   dot form      the 6 px circle sits inside the anchor's top-end corner,
//                 its end edge on the anchor's end edge and its top edge on
//                 the anchor's top edge (offset 6 / overlap 6 [compose])
//   content form  the pill's start edge sits 12 px inside the anchor's end
//                 edge and its bottom edge 14 px below the anchor's top edge
//                 (offset 12 / overlap 14 [compose]) — so a 16 px pill
//                 overhangs the anchor by 4 px horizontally and 2 px on top
//
// One deliberate Qt difference, recorded in docs/porting-todo.md: Compose
// lets the badge overlap anything around the box freely, while a Qt child
// widget is clipped to its parent. MdBadgedBox therefore *reserves the
// overhang* in its own geometry — sizeHint() is the content plus the
// overhang, the content keeps its own size, and the badge is never clipped.
// The painted result is identical to Compose's; only the surrounding layout
// sees a slightly larger box.
//
// Clicks on the badge pass straight through to the content underneath —
// MdBadge is transparent for mouse events, which is what Compose's
// BadgedBox achieves by not wrapping the badge in a Surface.

#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QResizeEvent;
class QShowEvent;

namespace md {

class MdBadge;

class QT_MD3_EXPORT MdBadgedBox : public QWidget
{
    Q_OBJECT

public:
    explicit MdBadgedBox(QWidget *parent = nullptr);
    ~MdBadgedBox() override;

    /// The anchored content. Reparented into the box; a previously set
    /// content widget is left parentless for the caller to delete or reuse.
    void setContentWidget(QWidget *content);
    QWidget *contentWidget() const { return m_content; }

    /// The badge, positioned over the content. Its `text` property decides
    /// the form: empty for the dot, any text for the content pill.
    MdBadge *badge() const { return m_badge; }

    // --- geometry -----------------------------------------------------------
    /// The content's size hint plus the badge overhang the box reserves.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void layoutChildren();

    MdBadge *m_badge = nullptr;
    QWidget *m_content = nullptr;
};

} // namespace md

#endif // MD_BADGED_BOX_H
