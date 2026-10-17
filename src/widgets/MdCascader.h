#ifndef MD_CASCADER_H
#define MD_CASCADER_H

// MdCascader — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "Cascading menu 横向展开".
//
// The multi-column cascade: a read-only `MdTextField` trigger that opens a
// horizontal strip of `MdMenu`-sized panels, one per level of the option
// tree. Selecting a branch advances to the next column; selecting a leaf
// commits the path.
//
//   trigger   the exposed-dropdown reading (MdSelect's field)
//   panels    surface-container, corner-extra-small, list one-line rows
//             (56 px), secondary-container selection — the menu/list pair
//             the Stage 1 exports already publish
//
// The option model is a value tree (`MdCascaderNode`), matching the other
// Stage 2 value APIs (MdTree / MdTransfer / MdTimeline). There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtWidgets/QWidget>

class QKeyEvent;

namespace md {

class MdTextField;

/// One node of the cascader tree. Owned by its parent (or by MdCascader).
class QT_MD3_EXPORT MdCascaderNode
{
public:
    MdCascaderNode() = default;
    explicit MdCascaderNode(const QString &label);
    ~MdCascaderNode();

    QString label() const { return m_label; }
    void setLabel(const QString &label);

    bool isLeaf() const { return m_children.isEmpty(); }
    const QList<MdCascaderNode *> &children() const { return m_children; }
    MdCascaderNode *childAt(int index) const;
    void appendChild(MdCascaderNode *child);
    void clearChildren();

    MdCascaderNode *parent() const { return m_parent; }

private:
    QString m_label;
    QList<MdCascaderNode *> m_children;
    MdCascaderNode *m_parent = nullptr;

    friend class MdCascader;
};

class QT_MD3_EXPORT MdCascader : public QWidget
{
    Q_OBJECT

    /// The committed path, as labels (leaf only). Empty before a commit.
    Q_PROPERTY(QStringList currentPath READ currentPath NOTIFY currentPathChanged)
    /// Field label text.
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText NOTIFY labelTextChanged)
    /// Placeholder shown when no path is committed.
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY
                   placeholderTextChanged)
    /// Whether the popup is open.
    Q_PROPERTY(bool popupVisible READ isPopupVisible NOTIFY popupVisibleChanged)

public:
    explicit MdCascader(QWidget *parent = nullptr);
    ~MdCascader() override;

    QStringList currentPath() const { return m_currentPath; }

    QString labelText() const;
    void setLabelText(const QString &label);

    QString placeholderText() const;
    void setPlaceholderText(const QString &placeholder);

    bool isPopupVisible() const { return m_popupVisible; }

    /// Tree model. Nodes are owned by the cascader.
    void addTopLevelNode(MdCascaderNode *node);
    void clearNodes();
    QList<MdCascaderNode *> topLevelNodes() const { return m_roots; }

    /// Re-open the cascade at the given path of labels (partial paths allowed).
    void setCurrentPath(const QStringList &path);

public slots:
    void showPopup();
    void hidePopup();

signals:
    void currentPathChanged(const QStringList &path);
    void labelTextChanged(const QString &label);
    void placeholderTextChanged(const QString &placeholder);
    void popupVisibleChanged(bool visible);
    /// A leaf was chosen. Carries the full label path.
    void pathSelected(const QStringList &path);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onThemeChanged();
    void onFieldClicked();
    void onPopupClosed();

private:
    void init();
    void rebuildPanels();
    void closePopup();
    void commitPath(const QStringList &path);
    void relayout();

    MdTextField *m_field = nullptr;
    QWidget *m_popup = nullptr;
    QList<MdCascaderNode *> m_roots;
    QStringList m_currentPath;
    QStringList m_openPath;
    bool m_popupVisible = false;
    QList<QWidget *> m_panels;
    QList<int> m_highlight;
};

} // namespace md

#endif // MD_CASCADER_H
