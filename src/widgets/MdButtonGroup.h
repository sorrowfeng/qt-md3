#ifndef MD_BUTTON_GROUP_H
#define MD_BUTTON_GROUP_H

// MdButtonGroup — MD3 Expressive button groups.
//
// A button group is an *invisible container*. The design spec is unusually
// blunt about it:
//
//   "Button groups are invisible containers that add padding between buttons
//    and modify button shape. They don't contain any buttons by default."
//   "Button groups have no color properties."
//
// so this widget paints nothing at all. It lays its items out, keeps their
// spacing to the published `between-space`, gives each item the corner it
// should show towards its neighbours, and owns the selection model. Everything
// visible is the buttons inside it.
//
// The two forms differ in exactly one behaviour — what happens to the
// neighbours:
//
//   Standard   pressing an item grows it and pushes the items beside it.
//              `md.comp.button-group.standard.<size>.pressed.item.width.
//              multiplier` is 15%, animated on spring-fast-spatial.
//   Connected  pressing an item changes only that item's shape. The rest of
//              the group does not move.
//
// Why a QWidget rather than a container of MdButtons: it *is* a container of
// MdButtons. Each item is a real MdButton, so ripple, state layer, focus ring,
// keyboard activation, the QAction integration and the accessibility role all
// come for free and stay identical to a standalone button. Re-implementing that
// per item is a large surface to get subtly wrong, and the group needs none of
// it.
//
// The one consequence worth knowing about: an MdButton reserves a transparent
// margin around its container for the outward focus indicator (see MdButton),
// and Qt clips a child to its own rectangle. Two items whose *containers* are
// `between-space` apart therefore have widget rects that *overlap* — by
// `2 * focusRingInset - between-space`. `MdButtonGroupStyle::layoutFor` does
// that arithmetic; the group only consumes the result. A focused item's
// indicator can be partly covered by its neighbour's transparent margin, which
// is the visible cost of getting the containers the spec's distance apart and
// is noted in docs/porting-todo.md.

#include "core/MdButtonGroupTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdButton;

class QT_MD3_EXPORT MdButtonGroup : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(md::ButtonGroupVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(md::ButtonSize groupSize READ groupSize WRITE setGroupSize NOTIFY groupSizeChanged)
    Q_PROPERTY(md::ButtonGroupOrientation orientation READ orientation WRITE setOrientation
                   NOTIFY orientationChanged)
    Q_PROPERTY(md::ButtonGroupSelection selectionMode READ selectionMode WRITE setSelectionMode
                   NOTIFY selectionModeChanged)
    Q_PROPERTY(md::ButtonShape groupShape READ groupShape WRITE setGroupShape NOTIFY groupShapeChanged)
    Q_PROPERTY(md::ButtonVariant itemVariant READ itemVariant WRITE setItemVariant
                   NOTIFY itemVariantChanged)
    Q_PROPERTY(md::ButtonVariant selectedItemVariant READ selectedItemVariant
                   WRITE setSelectedItemVariant NOTIFY selectedItemVariantChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    explicit MdButtonGroup(QWidget *parent = nullptr);
    ~MdButtonGroup() override;

    // --- form -------------------------------------------------------------
    ButtonGroupVariant variant() const { return m_variant; }
    void setVariant(ButtonGroupVariant variant);

    /// The size every item is set to, which is also the size the group's own
    /// `between-space` and `container.height` tokens are read for. M3: "Works
    /// with all button sizes XS, S, M, L, and XL."
    ButtonSize groupSize() const { return m_size; }
    void setGroupSize(ButtonSize size);

    ButtonGroupOrientation orientation() const { return m_orientation; }
    void setOrientation(ButtonGroupOrientation orientation);

    /// The group's default shape. A *selected* item gets the other one — "when
    /// a toggle button is selected in a standard button group, its shape should
    /// change between square and round."
    ButtonShape groupShape() const { return m_shape; }
    void setGroupShape(ButtonShape shape);

    // --- selection --------------------------------------------------------
    ButtonGroupSelection selectionMode() const { return m_selection; }
    void setSelectionMode(ButtonGroupSelection selection);

    /// The colour style an unselected item uses, and the one a selected item
    /// uses.
    ///
    /// These are *the group's* choice, not a token: the spec says a group "can
    /// use the default button or toggle button color styles, like filled,
    /// tonal, and outlined", so the style is a caller decision and the group
    /// only applies it. Defaults are filled for an unselected item and tonal
    /// for a selected one, which is the pairing the spec's own figures show.
    ButtonVariant itemVariant() const { return m_itemVariant; }
    void setItemVariant(ButtonVariant variant);

    ButtonVariant selectedItemVariant() const { return m_selectedItemVariant; }
    void setSelectedItemVariant(ButtonVariant variant);

    /// Index of the selected item, or -1 when nothing is selected.
    ///
    /// For `Single` and `Required` this mirrors the selection. For `Multiple`
    /// it is the item that was selected most recently, which is what a caller
    /// driving a toolbar usually wants to know.
    int currentIndex() const { return m_currentIndex; }
    /// Select `index` and nothing else. -1 clears the selection.
    void setCurrentIndex(int index);

    QList<int> selectedIndexes() const { return m_selected; }
    bool isSelected(int index) const;
    void setSelected(int index, bool selected);
    void clearSelection();

    // --- items ------------------------------------------------------------
    int count() const { return m_items.size(); }
    /// The item at `index`, or nullptr.
    MdButton *itemAt(int index) const;
    /// The item's index, or -1 when it is not in this group.
    int indexOf(const MdButton *item) const;
    /// Every item, in order. (Copied through iterators: Qt 5 keeps
    /// QList and QVector as distinct types, Qt 6 aliases them.)
    QList<MdButton *> items() const
    {
        return QList<MdButton *>(m_items.cbegin(), m_items.cend());
    }

    /// Create and append an item.
    ///
    /// `text` may be empty for an icon-only item — which is the usual case for
    /// a button group, since it exists mostly to hold icon buttons. An
    /// icon-only item is given an accessible name derived from `icon` unless
    /// `accessibleName` says otherwise, because an icon-only control with no
    /// name is unusable with a screen reader.
    MdButton *addItem(const QString &text = QString(),
                      const QString &icon = QString(),
                      const QString &accessibleName = QString());
    MdButton *insertItem(int index,
                         const QString &text = QString(),
                         const QString &icon = QString(),
                         const QString &accessibleName = QString());
    /// Append a button the caller already owns; the group takes parenting.
    void addButton(MdButton *button);
    void removeItem(int index);
    void clear();

    // --- tokens -----------------------------------------------------------
    /// The resolved `md.comp.button-group.*` set for this group.
    const MdButtonGroupTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this group alone. Falls back to
    /// MdComponentTokens::global() for anything not set here.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry ---------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /// Where each item sits, in group coordinates. Sized to the group's own
    /// count, so index i is item i.
    QVector<QRectF> itemRects() const;

    /// The gap the group is currently keeping between two neighbouring
    /// *containers*, after overrides.
    qreal betweenSpace() const { return tokens().betweenSpace; }

signals:
    void variantChanged(md::ButtonGroupVariant variant);
    void groupSizeChanged(md::ButtonSize size);
    void orientationChanged(md::ButtonGroupOrientation orientation);
    void selectionModeChanged(md::ButtonGroupSelection selection);
    void groupShapeChanged(md::ButtonShape shape);
    void itemVariantChanged(md::ButtonVariant variant);
    void selectedItemVariantChanged(md::ButtonVariant variant);
    void currentIndexChanged(int index);
    void itemClicked(int index);
    void itemSelected(int index);
    void itemDeselected(int index);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onGrowthTick();

private:
    void init();
    void invalidateTokens();
    /// Re-apply variant / size / shape / corner radii to every item.
    void applyItemAppearance();
    /// Move every item to its rect.
    void layoutItems();
    /// Refresh geometry, appearance and layout together.
    void refresh();
    void connectItem(MdButton *item);
    void handleItemClicked(MdButton *item);
    void handleItemPressed(MdButton *item);
    void handleItemReleased(MdButton *item);
    /// Make `index` the only selected item, or clear the selection when it is
    /// -1. Emits `currentIndexChanged` at most once, so an exclusive selection
    /// moving from one item to the next reads as one change rather than as a
    /// collapse to -1 and back.
    void selectOnly(int index);
    /// Add or remove one item without touching the others.
    void applySelection(int index, bool selected);
    /// Force the selection back inside what `selectionMode()` allows.
    void reconcileSelection();
    /// Index of the item that currently precedes/follows `index` along the
    /// group's main axis, or -1.
    int neighbourIndex(int index, int delta) const;
    /// Animate the pressed item's growth towards `multiplier`.
    void animateGrowthTo(qreal multiplier);
    /// Per-item press growth, all zeros but the pressed item.
    QVector<qreal> growthVector() const;

    ButtonGroupVariant m_variant = ButtonGroupVariant::Standard;
    ButtonSize m_size = ButtonSize::Small;
    ButtonGroupOrientation m_orientation = ButtonGroupOrientation::Horizontal;
    ButtonGroupSelection m_selection = ButtonGroupSelection::Single;
    ButtonShape m_shape = ButtonShape::Round;
    ButtonVariant m_itemVariant = ButtonVariant::Filled;
    ButtonVariant m_selectedItemVariant = ButtonVariant::Tonal;

    QVector<MdButton *> m_items;
    /// Sorted, unique.
    QList<int> m_selected;
    int m_currentIndex = -1;

    /// Press growth, as a multiplier of the pressed item's natural extent.
    /// 0 at rest, `tokens().pressedWidthMultiplier` at full growth.
    int m_pressedIndex = -1;
    qreal m_growth = 0.0;
    qreal m_growthFrom = 0.0;
    qreal m_growthTo = 0.0;
    QElapsedTimer m_growthClock;
    QTimer *m_growthTimer = nullptr;

    mutable MdButtonGroupTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_BUTTON_GROUP_H
