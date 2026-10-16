#ifndef MD_TREE_SELECT_H
#define MD_TREE_SELECT_H

// MdTreeSelect — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "List 的展开折叠 + Menu 弹层" — List tree
// rows plus a Menu select popup.
//
// MdTreeSelect is the tree reading of the exposed dropdown menu MdSelect
// already ports: a read-only text field that shows the current choice and
// opens a popup of *tree rows* on activation.
//
//   field    an MdTextField (filled / outlined) showing the selected node's
//            text, with the trailing `arrow_drop_down` indicator
//   popup    a Qt::Popup window hosting an MdTree on `surface-container`
//            (the menu's surface, elevation level 2)
//   select   clicking a leaf (or pressing Enter on one) commits and closes;
//            expanding / collapsing a branch never closes
//
// Keyboard follows the combo convention inside those semantics: Enter /
// Space / Alt+Down / F4 open the popup, Escape closes it, Up/Down move the
// selection when closed and walk the tree rows when open.
//
// There is no QSS. The field is an MdTextField, the rows are MdTree's.

#include "core/MdQtCompat.h"
#include "core/MdTextFieldTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include "MdTree.h"

#include <QtWidgets/QWidget>

namespace md {

class MdTextField;
class MdTree;

class QT_MD3_EXPORT MdTreeSelect : public QWidget
{
    Q_OBJECT

    /// The field chrome: filled or outlined text field.
    Q_PROPERTY(md::MdTextFieldVariant variant READ variant WRITE setVariant
                   NOTIFY variantChanged)
    /// Floating label above the input line.
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText NOTIFY labelTextChanged)
    /// Body-small supporting line under the container.
    Q_PROPERTY(QString supportingText READ supportingText WRITE setSupportingText
                   NOTIFY supportingTextChanged)
    /// Shown when nothing is selected.
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText
                   NOTIFY placeholderTextChanged)
    /// Selected node text, empty when nothing is selected.
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged)

public:
    explicit MdTreeSelect(QWidget *parent = nullptr);
    ~MdTreeSelect() override;

    // --- field appearance -------------------------------------------------------
    MdTextFieldVariant variant() const;
    void setVariant(MdTextFieldVariant variant);

    QString labelText() const;
    void setLabelText(const QString &label);

    QString supportingText() const;
    void setSupportingText(const QString &text);

    QString placeholderText() const;
    void setPlaceholderText(const QString &text);

    QString currentText() const;
    MdTreePath currentPath() const;

    // --- tree model (delegates to the owned MdTree) ------------------------------
    MdTree *tree() const { return m_tree; }

    void addTopLevelItem(MdTreeItem *item);
    void clear();

    void setSelectedPath(const MdTreePath &path);
    void clearSelection();

    // --- popup -------------------------------------------------------------------
    void showPopup();
    void hidePopup();
    bool isPopupVisible() const;

    // --- geometry ----------------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

signals:
    void variantChanged(md::MdTextFieldVariant variant);
    void labelTextChanged(const QString &label);
    void supportingTextChanged(const QString &text);
    void placeholderTextChanged(const QString &text);
    void currentTextChanged(const QString &text);
    void currentPathChanged(const md::MdTreePath &path);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onFieldClicked();
    void onTreeActivated(const md::MdTreePath &path);
    void onPopupClosed();

private:
    void init();
    void relayout();
    void syncField();

    MdTextField *m_field = nullptr;
    MdTree *m_tree = nullptr;
    QWidget *m_popup = nullptr;
    bool m_popupOpen = false;
    MdTreePath m_currentPath;
};

} // namespace md

#endif // MD_TREE_SELECT_H
