#ifndef MD_LIST_H
#define MD_LIST_H

// MdList — the MD3 list container.
//
// A list is a continuous vertical index of items: the export's container rows
// (`md.comp.list.container.color` / `.shape`, the 8px vertical padding, the
// 2px segmented gap) plus the *navigation* behaviour the web implementation
// owns (material-web's `ListController`):
//
//   * roving tabindex — exactly one interactive item is reachable by Tab;
//     arrows move focus *and* the tab stop, Home/End jump to the ends, and
//     navigation wraps by default (material-web's `wrapNavigation ?? true`).
//   * ArrowDown/ArrowUp are the primary keys; the inline keys (ArrowRight /
//     ArrowLeft, mirrored in RTL) are treated as the same pair, which is what
//     the web controller does for a vertical list.
//   * disabled and non-interactive items are skipped, never "activated".
//   * Space / Enter on a focused item run the activation flow — the item
//     emits `activated()`, which this container turns into selection and the
//     `itemActivated()` signal.
//
// Selection is the *container's* business, not the item's (Compose's
// `selectable` overloads; the spec's "a list can have only one selection mode
// at a time"): None leaves every item as the caller set it, Single clears the
// others, Multiple toggles.
//
// The container paints the export's container colour rounded by
// `container.shape`; items are laid out through the owned vertical layout,
// with `segmentedGap` spacing when the list is segmented.

#include "core/MdListTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtWidgets/QWidget>

class QKeyEvent;
class QVBoxLayout;

namespace md {

class MdListItem;
class MdListStyle;

class QT_MD3_EXPORT MdList : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE setSelectionMode NOTIFY
                   selectionModeChanged)
    Q_PROPERTY(bool wrapNavigation READ wrapsNavigation WRITE setWrapNavigation NOTIFY
                   wrapNavigationChanged)
    Q_PROPERTY(bool segmented READ isSegmented WRITE setSegmented NOTIFY segmentedChanged)

public:
    /// Compose's three selection shapes, minus "required": a list may hold
    /// none. `None` is the plain single-action list.
    enum class SelectionMode
    {
        None,
        Single,
        Multiple,
    };
    Q_ENUM(SelectionMode)

    explicit MdList(QWidget *parent = nullptr);
    ~MdList() override;

    // --- items ---------------------------------------------------------------
    /// Append an item. The list takes ownership through the Qt parent
    /// relationship and lays it out through the owned layout.
    void addItem(MdListItem *item);
    void insertItem(int index, MdListItem *item);
    void removeItem(MdListItem *item);
    /// The attached items, in layout order.
    QList<MdListItem *> items() const;
    int itemCount() const;

    // --- selection ------------------------------------------------------------
    SelectionMode selectionMode() const { return m_selectionMode; }
    void setSelectionMode(SelectionMode mode);
    /// The selected items, in layout order.
    QList<MdListItem *> selectedItems() const;
    void clearSelection();

    // --- navigation ------------------------------------------------------------
    bool wrapsNavigation() const { return m_wrapNavigation; }
    void setWrapNavigation(bool wrap);

    /// The item that currently holds the roving tab stop (nullptr when none
    /// is set). Update it when focus moves so Tab comes back where it left.
    MdListItem *activeItem() const { return m_activeItem; }
    void setActiveItem(MdListItem *item);

    /// The next (step > 0) or previous (step < 0) *activatable* item from
    /// `from` — interactive, enabled items only. Honours the wrap setting.
    MdListItem *nextItem(MdListItem *from, int step) const;

    /// Focus (and activate) an item the way a click would: selects per the
    /// selection mode, moves the tab stop and emits `itemActivated()`.
    bool activateItem(MdListItem *item);

    /// True when the item can be navigated to: interactive and enabled.
    static bool isActivatable(const MdListItem *item);

    // --- segmented lists --------------------------------------------------------
    /// A segmented list keeps a 2px gap between items and hands each item its
    /// index/count so its *outer* corners take the list's shape.
    bool isSegmented() const { return m_segmented; }
    void setSegmented(bool segmented);

    // --- tokens ------------------------------------------------------------------
    const MdListContainerTokens &tokens() const;

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

signals:
    void selectionModeChanged(md::MdList::SelectionMode mode);
    void wrapNavigationChanged(bool wrap);
    void segmentedChanged(bool segmented);
    /// An item was activated by click or Space/Enter.
    void itemActivated(md::MdListItem *item);
    /// An item's selection changed because of activation.
    void selectionChanged(md::MdListItem *item, bool selected);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    /// Re-apply the token-driven layout margins/spacing and the segmented
    /// positions.
    void syncLayout();
    /// Keep the roving tab stop valid after the item set changes.
    void syncActiveItem();
    /// One item's activation, wired from the item's `activated()` signal.
    void handleItemActivated(MdListItem *item);

    mutable MdListContainerTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    QVBoxLayout *m_layout = nullptr;
    SelectionMode m_selectionMode = SelectionMode::None;
    bool m_wrapNavigation = true;
    bool m_segmented = false;
    MdListItem *m_activeItem = nullptr;
};

} // namespace md

#endif // MD_LIST_H
