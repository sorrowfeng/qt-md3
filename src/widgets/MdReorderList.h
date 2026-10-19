#ifndef MD_REORDER_LIST_H
#define MD_REORDER_LIST_H

// MdReorderList — Stage 2 Qt extension: an MdList with item reordering.
//
// MD3 has no reorder-list family. The control is the standard "queue"
// composition: a list whose rows can be moved up / down (keyboard and
// buttons) and, when `dragEnabled`, dragged to a new slot. The visuals are
// the Stage 1 list family (`md.comp.list.*` / `md.comp.list.list-item.*`);
// this class only owns the ordering model.
//
// Reordering emits `itemMoved(from, to)` after the items have been swapped,
// so a host can keep its model in sync. No QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include "MdList.h"

namespace md {

class QT_MD3_EXPORT MdReorderList : public MdList
{
    Q_OBJECT

    Q_PROPERTY(bool dragEnabled READ isDragEnabled WRITE setDragEnabled NOTIFY dragEnabledChanged)
    Q_PROPERTY(bool moveButtonsVisible READ isMoveButtonsVisible WRITE setMoveButtonsVisible NOTIFY
                   moveButtonsVisibleChanged)

public:
    explicit MdReorderList(QWidget *parent = nullptr);
    ~MdReorderList() override;

    bool isDragEnabled() const { return m_dragEnabled; }
    void setDragEnabled(bool enabled);

    /// Show small up/down icon buttons on each interactive row's trailing
    /// slot. Off by default; hosts that supply their own affordances can
    /// leave it off and call moveItem() themselves.
    bool isMoveButtonsVisible() const { return m_moveButtonsVisible; }
    void setMoveButtonsVisible(bool visible);

    /// Move the item at `index` by `delta` slots. Returns the new index, or
    /// the old one when the move would leave the list.
    int moveItem(int index, int delta);

    /// Move the currently active item. Returns true when a move happened.
    bool moveActive(int delta);

public slots:
    void moveUp();
    void moveDown();

signals:
    void dragEnabledChanged(bool enabled);
    void moveButtonsVisibleChanged(bool visible);
    /// The item at `from` now lives at `to` (both pre-move indices are
    /// resolved after the swap).
    void itemMoved(int from, int to);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void init();
    void refreshMoveButtons();
    int indexAt(const QPointF &pos) const;

    bool m_dragEnabled = true;
    bool m_moveButtonsVisible = false;
    bool m_dragging = false;
    int m_dragIndex = -1;
    int m_dropIndex = -1;
    QPointF m_pressPos;
};

} // namespace md

#endif // MD_REORDER_LIST_H
