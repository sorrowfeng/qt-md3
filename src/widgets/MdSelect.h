#ifndef MD_SELECT_H
#define MD_SELECT_H

// MdSelect — Stage 2 Qt extension (Appendix D, P0), the exposed dropdown menu.
//
// MD3's "exposed dropdown menu" is the M3 answer to a combo box: a text
// field that *shows* the current choice and opens a menu of options on
// activation. `md3.material.io` documents it as a Text field + Menu
// composition, not a native combo — which is exactly what this widget is:
//
//   * the **anchor** is an `MdTextField` (filled or outlined) with the
//     label float, active indicator and supporting text of the text-field
//     export (34.0.21). It is read-only: the user does not type the value,
//     they pick it from the menu;
//   * the **trailing icon** is `arrow_drop_down` (the same indicator
//     MdSplitButton uses), 24 px, on-surface-variant / error;
//   * the **popup** is an `MdMenu` on the classic menu token set — surface-
//     container at level 2, 48 px items, the selected item's
//     `secondary-container` side, the open scale+alpha spring pair;
//   * the **leading icon** is the optional 24 px text-field leading slot.
//
// Variants, states and tokens all come from the Stage 1 text-field and menu
// exports; this widget only composes them. Keyboard follows the combo
// convention inside those semantics: Enter / Space / Alt+Down / F4 open the
// menu, Escape closes it, Up/Down move the selection when closed and the
// highlight when open.
//
// There is no QSS. The field is an `MdTextField`, the popup an `MdMenu`.

#include "core/MdTextFieldTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QLabel;

namespace md {

class MdMenu;
class MdMenuItem;
class MdTextField;

class QT_MD3_EXPORT MdSelect : public QWidget
{
    Q_OBJECT

    /// The field chrome: `md.comp.filled-text-field` or `outlined-text-field`.
    Q_PROPERTY(md::MdTextFieldVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    /// Floating label above the input line.
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText NOTIFY labelTextChanged)
    /// Body-small supporting line under the container.
    Q_PROPERTY(QString supportingText READ supportingText WRITE setSupportingText NOTIFY
                   supportingTextChanged)
    /// Shown when no item is selected.
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY
                   placeholderTextChanged)
    /// The error overlay (indicator / label / supporting text / trailing icon).
    Q_PROPERTY(bool error READ hasError WRITE setError NOTIFY errorChanged)
    /// The 24 px leading icon slot.
    Q_PROPERTY(QString leadingIconName READ leadingIconName WRITE setLeadingIconName NOTIFY
                   leadingIconNameChanged)
    /// Zero-based selection; -1 when nothing is selected.
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    /// The selected item's text, empty when nothing is selected.
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged)
    /// Number of items.
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit MdSelect(QWidget *parent = nullptr);
    ~MdSelect() override;

    // --- field appearance -------------------------------------------------------
    MdTextFieldVariant variant() const;
    void setVariant(MdTextFieldVariant variant);

    QString labelText() const;
    void setLabelText(const QString &label);

    QString supportingText() const;
    void setSupportingText(const QString &text);

    QString placeholderText() const;
    void setPlaceholderText(const QString &text);

    bool hasError() const;
    void setError(bool error);

    QString leadingIconName() const;
    void setLeadingIconName(const QString &iconName);

    // --- model --------------------------------------------------------------------
    int count() const;
    int currentIndex() const;
    void setCurrentIndex(int index);
    QString currentText() const;

    /// Appends one entry and returns its index.
    int addItem(const QString &text);
    void addItems(const QStringList &texts);
    void insertItem(int index, const QString &text);
    void removeItem(int index);
    void clear();
    QString itemText(int index) const;

    // --- popup -------------------------------------------------------------------
    /// Opens the menu under the field. Re-entrant calls close first.
    void showPopup();
    void hidePopup();
    bool isPopupVisible() const;

    // --- geometry ----------------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void variantChanged(md::MdTextFieldVariant variant);
    void labelTextChanged(const QString &label);
    void supportingTextChanged(const QString &text);
    void placeholderTextChanged(const QString &text);
    void errorChanged(bool error);
    void leadingIconNameChanged(const QString &iconName);
    void currentIndexChanged(int index);
    void currentTextChanged(const QString &text);
    void countChanged(int count);
    /// Emitted when the *user* picks an item (not on programmatic setCurrentIndex).
    void activated(int index);
    void textActivated(const QString &text);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onMenuClosed();
    void onMenuItemClicked();

private:
    void init();
    void rebuildMenu();
    void applyCurrentText();
    void setIndexFromUser(int index);

    MdTextField *m_field = nullptr;
    MdMenu *m_menu = nullptr;
    QList<MdMenuItem *> m_menuItems;
    QStringList m_items;
    int m_currentIndex = -1;
    bool m_popupOpen = false;
    /// True when `m_items` changed and the menu rows need a rebuild.
    bool m_menuDirty = true;
    bool m_rebuilding = false;
};

} // namespace md

#endif // MD_SELECT_H
