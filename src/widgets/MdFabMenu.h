#ifndef MD_FAB_MENU_H
#define MD_FAB_MENU_H

// MdFabMenu — MD3 FAB menus (an M3 Expressive component; material-web is in
// maintenance mode and does not implement it).
//
//   variant  md.comp.fab-menu.primary | secondary | tertiary — each group is
//            published twice: the close button takes the pure colour, the
//            list items the container colour
//
// Anatomy, from the spec page: an anchor FAB (a normal MdFab), a 56 px close
// button that "shares the top trailing corner as an anchor" with the FAB —
// i.e. appears in the same place — and up to six list items below it. The
// menu "animates from the top trailing edge of the FAB"; on the web it
// "inherits its states and specs from the baseline menu".
//
// Behaviour here: tapping the anchor FAB hides it and reveals the close
// button in its place, then the items stagger in top-down (fade + downward
// settle). Tapping the close button (or calling collapse()) reverses it.
// The reveal uses the Compose M3 Expressive `SpatialDefault` spring with a
// 40 ms per-item stagger — the token export publishes no motion rows for
// this family, which is recorded in docs/porting-todo.md, so the spring
// choice is a sourced-from-Compose convention and not a token fact.
//
// The container's sizeHint is always the *expanded* size, so a layout that
// reserved room for the open menu keeps its geometry while the menu animates.

#include "core/MdFabMenuTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QPointF>
#include <QtCore/QVariantAnimation>
#include <QtCore/QVector>
#include <QtWidgets/QWidget>

namespace md {

class MdFab;
class MdFabMenuItem;

class QT_MD3_EXPORT MdFabMenu : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(md::FabMenuVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(QString anchorIconName READ anchorIconName WRITE setAnchorIconName NOTIFY
                   anchorIconNameChanged)

public:
    explicit MdFabMenu(QWidget *parent = nullptr);
    /// Convenience: seeds the anchor FAB's icon.
    MdFabMenu(const QString &anchorIconName, QWidget *parent = nullptr);
    ~MdFabMenu() override;

    // --- configuration -------------------------------------------------------
    FabMenuVariant variant() const { return m_variant; }
    void setVariant(FabMenuVariant variant);

    /// The anchor FAB's icon (`"add"`, `"edit"`, …). The close button's icon
    /// is not configurable — the spec's close glyph.
    QString anchorIconName() const;
    void setAnchorIconName(const QString &iconName);

    /// The anchor FAB (owned; variant tracks the menu's).
    MdFab *anchorFab() const { return m_anchor; }

    /// The close button (owned; appears in the anchor's place when expanded).
    MdFabMenuItem *closeButton() const { return m_closeButton; }

    // --- items ----------------------------------------------------------------
    /// Adds one list item; the menu shows at most the spec's six.
    void addItem(const QString &iconName, const QString &label);
    int itemCount() const;
    /// The item at `index`, or nullptr.
    MdFabMenuItem *itemAt(int index) const;
    void clearItems();

    // --- expand / collapse ------------------------------------------------------
    bool isExpanded() const { return m_expanded; }
    /// Setting this animates.
    void setExpanded(bool expanded);
    /// True while the open/close animation is running.
    bool isAnimating() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void variantChanged(md::FabMenuVariant variant);
    void expandedChanged(bool expanded);
    void anchorIconNameChanged(const QString &iconName);
    /// A list item was activated; `index` is its position, `label` its text.
    void itemActivated(int index, const QString &label);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onAnchorClicked();
    void onCloseClicked();
    void onRevealChanged(const QVariant &value);

private:
    void init();
    void relayout();
    /// The expanded height: close button + gaps + items.
    qreal expandedContentHeight() const;
    qreal menuWidth() const;

    FabMenuVariant m_variant = FabMenuVariant::Primary;
    bool m_expanded = false;
    MdFab *m_anchor = nullptr;
    MdFabMenuItem *m_closeButton = nullptr;
    QVector<MdFabMenuItem *> m_items;
    QVariantAnimation m_revealAnimation;
    /// Current animation progress, 0 = collapsed, 1 = expanded.
    qreal m_reveal = 0.0;
};

} // namespace md

#endif // MD_FAB_MENU_H
