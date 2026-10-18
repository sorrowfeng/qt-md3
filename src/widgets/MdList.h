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
// at a time"). Single clears the others, Multiple toggles; `None` is the
// plain action list — activation still selects the row so the click has a
// visible response, and switching *to* `None` clears any managed selection.
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
    /// Realise only the rows the viewport can see. Off by default so the
    /// classic widget-per-item layout is untouched; turn it on for lists with
    /// hundreds of rows where laying out and painting every item dominates.
    Q_PROPERTY(bool virtualized READ isVirtualized WRITE setVirtualized NOTIFY virtualizedChanged)

public:
    /// Compose's three selection shapes, minus "required": a list may hold
    /// none. `None` is the plain single-action list (activation still selects
    /// the row so a click is visible).
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

    // --- virtualisation ----------------------------------------------------------
    /// True when only the rows near the viewport are shown and laid out.
    ///
    /// The API is unchanged either way: items are still added as widgets and
    /// `heightForWidth()` still reports the full stack height, so a host scroll
    /// area keeps a correct scrollbar. What changes is that rows outside the
    /// realised window are `hide()`n, so Qt skips their layout and paint, and
    /// each row's height is cached rather than re-measured on every pass.
    bool isVirtualized() const { return m_virtualized; }
    void setVirtualized(bool virtualized);

    /// The window of rows currently realised when `virtualized` is on —
    /// [first, last], inclusive, or (-1, -1) when nothing is realised. With
    /// virtualisation off every row is realised and this reports the full span.
    void realisedRange(int *first, int *last) const;

    /// Scroll the nearest ancestor `QAbstractScrollArea` (if any) so `item`
    /// is inside the viewport. No-op when the item is null or not ours.
    void scrollToItem(MdListItem *item);

    // --- tokens ------------------------------------------------------------------
    const MdListContainerTokens &tokens() const;

    /// Stack height of the items at `width`, including the container padding.
    /// The list has height-for-width: a multi-line item grows when the list
    /// narrows, so a single preferred height would let the band clip or
    /// overlap the rows.
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

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
    void virtualizedChanged(bool virtualized);
    /// An item was activated by click or Space/Enter.
    void itemActivated(md::MdListItem *item);
    /// An item's selection changed because of activation.
    void selectionChanged(md::MdListItem *item, bool selected);

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onThemeChanged();
    void onScrollChanged();

private:
    void init();
    /// Re-apply the token-driven layout margins/spacing and the segmented
    /// positions.
    void syncLayout();
    /// Keep the roving tab stop valid after the item set changes.
    void syncActiveItem();
    /// One item's activation, wired from the item's `activated()` signal.
    void handleItemActivated(MdListItem *item);
    /// Re-measure and re-show the virtualised rows around the viewport.
    void syncVirtualWindow();
    /// The viewport band (in list coordinates) a virtualised list realises
    /// rows for: the ancestor scroll area's visible rect, or the whole widget
    /// when there is none.
    QRect virtualViewportRect() const;
    /// The cached measured height of the row at `index`, recomputed when the
    /// cache is stale.
    int virtualItemHeight(int index, int width) const;
    /// Drop the per-row height cache (items changed, width changed, theme).
    void invalidateVirtualHeights() const;
    /// Connect the ancestor scroll area's scrollbars to `onScrollChanged`.
    void installScrollHook();

    mutable MdListContainerTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    QVBoxLayout *m_layout = nullptr;
    SelectionMode m_selectionMode = SelectionMode::None;
    bool m_wrapNavigation = true;
    bool m_segmented = false;
    bool m_virtualized = false;
    MdListItem *m_activeItem = nullptr;

    /// Virtualisation: items kept in layout order independent of the layout,
    /// with a per-row height cache so a1000-row pass does not re-measure
    /// every row on every scroll tick.
    QList<MdListItem *> m_items;
    mutable QList<int> m_virtualHeights;
    mutable int m_virtualWidth = -1;
    mutable bool m_virtualHeightsDirty = true;
    int m_realisedFirst = -1;
    int m_realisedLast = -1;
    bool m_scrollFilterInstalled = false;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdList::SelectionMode)

// Incomplete QObject subclass in itemActivated / selectionChanged.
// Opaque + registered: either macro alone leaves the pointer unusable on Qt 5.
Q_DECLARE_OPAQUE_POINTER(md::MdListItem *)
Q_DECLARE_METATYPE(md::MdListItem *)

#endif // MD_LIST_H
