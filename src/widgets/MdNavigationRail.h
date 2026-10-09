#ifndef MD_NAVIGATION_RAIL_H
#define MD_NAVIGATION_RAIL_H

// MdNavigationRail — the MD3 navigation rail, both published families.
//
// "Navigation rails provide access to primary destinations in apps when using
// tablet or desktop screens" (m3.material.io/components/navigation-rail). A
// column of three to seven destinations, optionally headed by a FAB or a menu
// icon.
//
// ## The variant is the family, the state is the width
//
// `variant()` selects between the baseline family (`navigation-rail`, 80 px,
// no expanded rows — `setExpanded(true)` is refused) and the flexible one
// (`nav-rail-collapsed` / `-expanded`, 96 collapsed, 220–360 expanded). The
// flexible family's two widths are a **state** of one rail, not two variants:
// `setExpanded()` animates the container's width on a spatial spring (the
// modal rail takes the fast one, as Compose does) and flips the items between
// their `Top` (collapsed pill, no label) and `Start` (expanded pill with
// label) arrangements — the same switch Compose's wide rail makes by handing
// its shared `NavigationItem` a different style set.
//
// ## The items are the bar's items
//
// `MdNavigationBarItem` is the shared expressive item; the rail pushes its own
// family's rows into each one (see `MdNavigationRailTokens`'s `item` field).
// The selection model is the bar's: the rail owns `currentIndex`, the items
// mirror it, and the row is a group of Tab stops the rail itself is not part
// of.
//
// The header is a foreign widget (`setHeader`) the rail positions but the
// caller owns the content of — a `MdFab`, typically.

#include "core/MdNavigationRailTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdNavigationBarItem;
class MdNavigationRailStyle;

class QT_MD3_EXPORT MdNavigationRail : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged)
    Q_PROPERTY(md::MdNavigationRailVariant variant READ variant WRITE setVariant NOTIFY
                   variantChanged)
    Q_PROPERTY(md::MdNavigationRailArrangement arrangement READ arrangement WRITE setArrangement
                   NOTIFY arrangementChanged)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(bool modal READ isModal WRITE setModal NOTIFY modalChanged)

public:
    explicit MdNavigationRail(QWidget *parent = nullptr);
    ~MdNavigationRail() override;

    // --- header -------------------------------------------------------------
    /// The widget above the items. The rail takes it as a child and places it,
    /// but its content is the caller's — a `MdFab`, a menu icon.
    void setHeader(QWidget *header);
    QWidget *header() const { return m_header; }

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
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    // --- variant and state --------------------------------------------------
    MdNavigationRailVariant variant() const { return m_variant; }
    void setVariant(MdNavigationRailVariant variant);

    MdNavigationRailArrangement arrangement() const { return m_arrangement; }
    void setArrangement(MdNavigationRailArrangement arrangement);

    /// The flexible family's second width. Refused — with the state left as it
    /// was — on the baseline family, which publishes no expanded rows.
    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool expanded);

    /// The modal rows: the drawer's replacement arriving over a scrim. Also
    /// switches the width spring to the fast scheme, as Compose does.
    bool isModal() const { return m_modal; }
    void setModal(bool modal);

    // --- tokens -------------------------------------------------------------
    const MdNavigationRailTokens &tokens() const;

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -----------------------------------------------------------
    /// The width the container is at right now — animated between the two
    /// states, so a caller reading it mid-flight sees the spring's frame.
    qreal currentWidth() const { return m_currentWidth; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void itemsChanged();
    void currentIndexChanged(int index);
    void variantChanged(md::MdNavigationRailVariant variant);
    void arrangementChanged(md::MdNavigationRailArrangement arrangement);
    void expandedChanged(bool expanded);
    void modalChanged(bool modal);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onWidthTick();

private:
    void init();
    void invalidateTokens();
    void applyToItem(MdNavigationBarItem *item);
    void applyToAllItems();
    void placeChildren();
    /// The width the container wants right now: the state's own width for the
    /// baseline rail, the content-driven expanded target or the collapsed one
    /// for the flexible rail.
    qreal desiredWidth() const;
    void restartWidthAnimation();

    QWidget *m_header = nullptr;
    QList<MdNavigationBarItem *> m_items;
    int m_currentIndex = -1;
    MdNavigationRailVariant m_variant = MdNavigationRailVariant::Baseline;
    MdNavigationRailArrangement m_arrangement = MdNavigationRailArrangement::Top;
    bool m_expanded = false;
    bool m_modal = false;

    qreal m_currentWidth = 0.0;
    qreal m_widthFrom = 0.0;
    qreal m_widthTo = 0.0;
    QElapsedTimer m_widthClock;
    QTimer *m_widthTimer = nullptr;

    mutable MdNavigationRailTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
    bool m_updatingSelection = false;
};

} // namespace md

#endif // MD_NAVIGATION_RAIL_H
