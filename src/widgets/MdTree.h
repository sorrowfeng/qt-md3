#ifndef MD_TREE_H
#define MD_TREE_H

// MdTree — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "List 的展开折叠 + Menu 弹层" — the List
// family's expand / collapse rows, as a hierarchy.
//
// MD3 has no tree family. The control is the standard disclosure tree built
// from the list's row language:
//
//   row        a List one-line item (56 px floor), headline in body-large
//   indent     16 px per depth level (`leading-space`), so the disclosure
//              gutter lines up across siblings
//   chevron    24 px `chevron_right` (collapsed) / `expand_less` (expanded)
//              in the leading gutter, the list's disclosure indicator
//   states     hover 0.08 / focus 0.12 / press 0.12 state layers on
//              `on-surface`, selected rows on `secondary-container`
//   motion     the expand / collapse of a child row uses the FastSpatial
//              spring (shared with MdCollapse)
//
// The model is deliberately not a QAbstractItemModel: the other Stage 2
// controls expose a simple value API (MdTable's string grid, MdUpload's file
// list), and a tree of `MdTreeItem` nodes matches that. Paths are lists of
// child indices (`{0, 2, 1}` = root 0 → child 2 → child 1).
//
// Keyboard: Up/Down walk visible rows, Left collapses or moves to the parent,
// Right expands or moves to the first child, Home/End jump, Enter/Space
// activates. Tab-reachable; the focus ring is `:focus-visible` only.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtWidgets/QWidget>

class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

namespace md {

class MdFocusRingController;
class MdRippleController;

/// A path from the root: one child index per level. Empty = the root level.
using MdTreePath = QVector<int>;

/// One node of the tree. Owned by its parent node (or by MdTree at the root).
class QT_MD3_EXPORT MdTreeItem
{
public:
    MdTreeItem() = default;
    explicit MdTreeItem(const QString &text);

    QString text() const { return m_text; }
    void setText(const QString &text);

    QString supportingText() const { return m_supporting; }
    void setSupportingText(const QString &text);

    QString iconName() const { return m_iconName; }
    void setIconName(const QString &name);

    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool expanded);

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);

    /// Child nodes. The tree takes ownership through the parent link.
    const QList<MdTreeItem *> &children() const { return m_children; }
    int childCount() const { return m_children.size(); }
    MdTreeItem *childAt(int index) const;
    void appendChild(MdTreeItem *child);
    void insertChild(int index, MdTreeItem *child);
    void removeChild(MdTreeItem *child);
    void clearChildren();

    MdTreeItem *parentItem() const { return m_parent; }

private:
    friend class MdTree;
    QString m_text;
    QString m_supporting;
    QString m_iconName;
    bool m_expanded = false;
    bool m_enabled = true;
    QList<MdTreeItem *> m_children;
    MdTreeItem *m_parent = nullptr;
};

class QT_MD3_EXPORT MdTree : public QWidget
{
    Q_OBJECT

    /// Indent per depth level, in logical px. Default 16 (list leading-space).
    Q_PROPERTY(int indent READ indent WRITE setIndent NOTIFY indentChanged)
    /// Row height floor. Default 56 (list one-line).
    Q_PROPERTY(int rowHeight READ rowHeight WRITE setRowHeight NOTIFY rowHeightChanged)
    /// Wrap Up/Down navigation at the ends (material-web's wrapNavigation).
    Q_PROPERTY(bool wrapNavigation READ wrapsNavigation WRITE setWrapNavigation
                   NOTIFY wrapNavigationChanged)

public:
    explicit MdTree(QWidget *parent = nullptr);
    ~MdTree() override;

    // --- model ---------------------------------------------------------------
    /// Append a root item. The tree takes ownership.
    void addTopLevelItem(MdTreeItem *item);
    void insertTopLevelItem(int index, MdTreeItem *item);
    void removeTopLevelItem(MdTreeItem *item);
    void clear();
    QList<MdTreeItem *> topLevelItems() const { return m_roots; }
    int topLevelItemCount() const { return m_roots.size(); }

    /// The node at `path`, or nullptr when the path is stale.
    MdTreeItem *itemAt(const MdTreePath &path) const;
    /// The path of `item`, or an empty path when it is not in this tree.
    MdTreePath pathOf(const MdTreeItem *item) const;

    // --- selection -------------------------------------------------------------
    MdTreePath selectedPath() const { return m_selectedPath; }
    void setSelectedPath(const MdTreePath &path);
    MdTreeItem *selectedItem() const { return itemAt(m_selectedPath); }
    void clearSelection();

    // --- expansion ---------------------------------------------------------------
    bool isExpanded(const MdTreePath &path) const;
    void setExpanded(const MdTreePath &path, bool expanded);
    void expandAll();
    void collapseAll();

    // --- metrics ------------------------------------------------------------------
    int indent() const { return m_indent; }
    void setIndent(int indent);

    int rowHeight() const { return m_rowHeight; }
    void setRowHeight(int height);

    bool wrapsNavigation() const { return m_wrap; }
    void setWrapNavigation(bool wrap);

    /// Visible rows, flattened depth-first in display order.
    struct Row
    {
        MdTreeItem *item = nullptr;
        MdTreePath path;
        int depth = 0;
        bool hasChildren = false;
    };
    QList<Row> visibleRows() const;
    int visibleRowCount() const { return visibleRows().size(); }

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void indentChanged(int indent);
    void rowHeightChanged(int height);
    void wrapNavigationChanged(bool wrap);
    void selectedPathChanged(const md::MdTreePath &path);
    /// A row was activated (click or Enter/Space).
    void itemActivated(const md::MdTreePath &path);
    /// A row's expansion state changed.
    void itemExpanded(const md::MdTreePath &path, bool expanded);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void collectRows(const MdTreeItem *item, const MdTreePath &base, int depth,
                     QList<Row> *out) const;
    void setHoveredRow(int row);
    int rowAt(const QPointF &position) const;
    QRectF chevronRectForRow(int row) const;
    void moveSelection(int delta);
    void activateRow(int row);
    void toggleRow(int row);

    QList<MdTreeItem *> m_roots;
    MdTreePath m_selectedPath;
    int m_indent = 16;
    int m_rowHeight = 56;
    bool m_wrap = true;

    int m_hoveredRow = -1;
    int m_pressedRow = -1;
    int m_focusRow = 0;
    bool m_focusIsKeyboard = false;

    MdFocusRingController *m_focusRing = nullptr;
    MdRippleController *m_ripple = nullptr;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdTreePath)

#endif // MD_TREE_H
