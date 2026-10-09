#ifndef MD_NAVIGATION_DRAWER_H
#define MD_NAVIGATION_DRAWER_H

// MdNavigationDrawer — the MD3 navigation drawer, as published.
//
// "Navigation drawers provide access to destinations in your app" — a
// start-anchored sheet of full-width destination rows under an optional
// headline. The spec ships one token family; the Expressive replacement (the
// expanded navigation rail) is `MdNavigationRail`'s expanded state, and this
// widget ports the drawer as published.
//
// ## Variants and what the host owns
//
// `variant()` picks between the file's two container row groups — `Standard`
// (`surface` at level0, docked) and `Modal` (`surface-container-low` at
// level1, arriving over a scrim). Dismissible vs permanent is placement and
// gesture behaviour the host owns, not token rows. Two rows are carried and
// not painted, on the library's established grounds:
//
//   * **the scrim** — a modal drawer's scrim covers the *parent*, which a
//     child widget cannot paint. `scrimColor()` / `scrimOpacity()` expose
//     the carried rows for a host overlay;
//   * **the modal elevation** — level1 falls outside a child sheet's rect.
//
// ## The item placement rule
//
// The drawer's items are placed by **direct geometry**, not through
// `MdChildBox::resizedGeometryOn`: the pill is the item, and its width is
// the container minus the behaviour's 2 x 12 `itemPadding` — there is
// nothing to centre. The height is the item's own 56.

#include "core/MdNavigationDrawerTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtWidgets/QWidget>

namespace md {

class MdNavigationDrawerItem;
class MdNavigationDrawerStyle;

class QT_MD3_EXPORT MdNavigationDrawer : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged)
    Q_PROPERTY(md::MdNavigationDrawerVariant variant READ variant WRITE setVariant NOTIFY
                   variantChanged)
    Q_PROPERTY(QString headline READ headline WRITE setHeadline NOTIFY headlineChanged)
    Q_PROPERTY(bool dividerVisible READ isDividerVisible WRITE setDividerVisible NOTIFY
                   dividerVisibleChanged)

public:
    explicit MdNavigationDrawer(QWidget *parent = nullptr);
    ~MdNavigationDrawer() override;

    // --- items --------------------------------------------------------------
    void addItem(MdNavigationDrawerItem *item);
    void insertItem(int index, MdNavigationDrawerItem *item);
    void removeItem(MdNavigationDrawerItem *item);
    void clearItems();
    QList<MdNavigationDrawerItem *> items() const { return m_items; }

    MdNavigationDrawerItem *itemAt(int index) const;
    int indexOf(const MdNavigationDrawerItem *item) const;
    int count() const { return int(m_items.size()); }

    // --- selection ------------------------------------------------------------
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    // --- variant and chrome -----------------------------------------------------
    MdNavigationDrawerVariant variant() const { return m_variant; }
    void setVariant(MdNavigationDrawerVariant variant);

    /// The sheet's headline — `title-small` in `on-surface-variant`.
    QString headline() const { return m_headline; }
    void setHeadline(const QString &headline);

    /// The `divider-color` line under the headline. Off by default; the spec's
    /// anatomy shows it as optional.
    bool isDividerVisible() const { return m_dividerVisible; }
    void setDividerVisible(bool visible);

    // --- the carried scrim (see the header note) ----------------------------------
    ColorRole scrimColor() const;
    qreal scrimOpacity() const;

    // --- tokens ---------------------------------------------------------------
    const MdNavigationDrawerTokens &tokens() const;

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -----------------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void itemsChanged();
    void currentIndexChanged(int index);
    void variantChanged(md::MdNavigationDrawerVariant variant);
    void headlineChanged(const QString &headline);
    void dividerVisibleChanged(bool visible);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void invalidateTokens();
    void applyToItem(MdNavigationDrawerItem *item);
    void applyToAllItems();
    void placeChildren();

    QList<MdNavigationDrawerItem *> m_items;
    int m_currentIndex = -1;
    MdNavigationDrawerVariant m_variant = MdNavigationDrawerVariant::Standard;
    QString m_headline;
    bool m_dividerVisible = false;

    mutable MdNavigationDrawerTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
    bool m_updatingSelection = false;
};

} // namespace md

#endif // MD_NAVIGATION_DRAWER_H
