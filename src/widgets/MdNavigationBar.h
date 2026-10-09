#ifndef MD_NAVIGATION_BAR_H
#define MD_NAVIGATION_BAR_H

// MdNavigationBar — the MD3 navigation bar, both published variants.
//
// "Navigation bars let people switch between UI views on smaller devices"
// (m3.material.io/components/navigation-bar). It is a row of three to five
// destinations of equal importance, one of which is selected.
//
// ## The variant is not a size knob
//
// `variant()` selects between two **unrelated token families**, not two
// snapshots of one. The baseline family is 80 px tall with a 64 x 32 pill and
// an `on-surface` active label; the flexible family is 64 px tall with a
// 56 x 32 pill and a `secondary` one, and it is the only one whose items can be
// laid out horizontally. The spec replaced the first with the second in May
// 2025 — see the long note in `MdNavigationBarTokens.h` for how the two were
// told apart and where the sources disagree.
//
// ## Two things the classes decide, not the items
//
//   * **The selection.** The bar owns `currentIndex` and is the only thing that
//     may change it; an item's `selected` is a mirror. Compose expresses the
//     same thing by making the item `selectable` inside a `selectableGroup`,
//     and Qt by making the items checkable and the bar exclusive.
//   * **The variant.** Items are told which family they belong to whenever
//     they are added and whenever the variant moves, because an item that
//     resolved its own tokens would silently keep the old numbers.
//
// The bar itself takes no focus. Its items do; the row is a group of Tab stops
// exactly as the buttons in a `MdButtonGroup` are.

#include "core/MdNavigationBarTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtWidgets/QWidget>

namespace md {

class MdNavigationBarItem;
class MdNavigationBarStyle;

class QT_MD3_EXPORT MdNavigationBar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(md::MdNavigationBarVariant variant READ variant WRITE setVariant NOTIFY
                   variantChanged)
    Q_PROPERTY(md::MdNavigationBarArrangement arrangement READ arrangement WRITE setArrangement
                   NOTIFY arrangementChanged)
    Q_PROPERTY(md::MdNavigationItemIconPosition itemLayout READ itemLayout WRITE setItemLayout
                   NOTIFY itemLayoutChanged)

public:
    explicit MdNavigationBar(QWidget *parent = nullptr);
    ~MdNavigationBar() override;

    // --- items --------------------------------------------------------------
    void addItem(MdNavigationBarItem *item);
    void insertItem(int index, MdNavigationBarItem *item);
    void removeItem(MdNavigationBarItem *item);
    void clearItems();
    QList<MdNavigationBarItem *> items() const { return m_items; }

    MdNavigationBarItem *itemAt(int index) const;
    int indexOf(const MdNavigationBarItem *item) const;
    int count() const { return int(m_items.size()); }

    // --- selection ----------------------------------------------------------
    /// Index of the selected destination, or -1 when none is. Setting it makes
    /// the selection exclusive: every other item is unselected.
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    // --- variant and arrangement --------------------------------------------
    MdNavigationBarVariant variant() const { return m_variant; }
    void setVariant(MdNavigationBarVariant variant);

    MdNavigationBarArrangement arrangement() const { return m_arrangement; }
    void setArrangement(MdNavigationBarArrangement arrangement);

    /// `Top` (icon above label) or `Start` (icon beside label). The baseline
    /// publishes no horizontal item rows and Compose gives its items no such
    /// option, so `Start` is the flexible family's configuration; the bar
    /// accepts it either way and records the divergence.
    MdNavigationItemIconPosition itemLayout() const { return m_itemLayout; }
    void setItemLayout(MdNavigationItemIconPosition layout);

    // --- tokens -------------------------------------------------------------
    const MdNavigationBarTokens &tokens() const;

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -----------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void itemsChanged();
    void currentIndexChanged(int index);
    void variantChanged(md::MdNavigationBarVariant variant);
    void arrangementChanged(md::MdNavigationBarArrangement arrangement);
    void itemLayoutChanged(md::MdNavigationItemIconPosition layout);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void invalidateTokens();
    /// Push the family and the layout onto one item, and wire its selection
    /// back to the bar. Called for every path that adds or changes an item.
    void applyToItem(MdNavigationBarItem *item);
    void applyToAllItems();
    void placeChildren();

    QList<MdNavigationBarItem *> m_items;
    int m_currentIndex = -1;
    MdNavigationBarVariant m_variant = MdNavigationBarVariant::Baseline;
    MdNavigationBarArrangement m_arrangement = MdNavigationBarArrangement::EqualWeight;
    MdNavigationItemIconPosition m_itemLayout = MdNavigationItemIconPosition::Top;

    mutable MdNavigationBarTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
    /// Guards the re-entrancy of `setCurrentIndex` while it mirrors the
    /// selection onto the items, each of which reports `toggled` back.
    bool m_updatingSelection = false;
};

} // namespace md

#endif // MD_NAVIGATION_BAR_H
