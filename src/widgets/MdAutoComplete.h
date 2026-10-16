#ifndef MD_AUTOCOMPLETE_H
#define MD_AUTOCOMPLETE_H

// MdAutoComplete — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "Text field + exposed dropdown" — TextField +
// Menu. QCompleter-like suggestions offered in an `MdMenu` popup.
//
//   field     `MdTextField` (the caller's variant), the query
//   menu      `MdMenu` popup (surface-container, corner-extra-small) listing
//             the matching suggestions — the same exposed-dropdown reading
//             `MdSelect` already uses
//   filter    case-insensitive substring match by default; `startsWith` and
//             `custom` are available
//   keyboard  Down/Up walk the suggestion list, Enter commits the highlighted
//             one, Esc closes without committing
//
// The popup is a `Qt::Popup` top-level (like MdSelect's menu) so it is not
// parented into the layout. There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QKeyEvent;
class QResizeEvent;

namespace md {

class MdMenu;
class MdMenuItem;
class MdTextField;

/// How the suggestion list is filtered against the current text.
enum class MdAutoCompleteFilter {
    /// Case-insensitive substring match (default).
    Contains,
    /// Case-insensitive prefix match.
    StartsWith,
    /// The host filters via `filterRequested` and calls `setFilteredItems`.
    Custom,
    Count,
};

class QT_MD3_EXPORT MdAutoComplete : public QWidget
{
    Q_OBJECT

    /// The query text (mirrors the field).
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// Field label (MdTextField).
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText NOTIFY labelTextChanged)
    /// Field placeholder (MdTextField).
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText
                   NOTIFY placeholderTextChanged)
    /// How suggestions are filtered against the query.
    Q_PROPERTY(md::MdAutoCompleteFilter filter READ filter WRITE setFilter NOTIFY filterChanged)
    /// Maximum number of visible suggestions. 0 = unlimited.
    Q_PROPERTY(int maxVisible READ maxVisible WRITE setMaxVisible NOTIFY maxVisibleChanged)
    /// True while the suggestion popup is open.
    Q_PROPERTY(bool popupVisible READ isPopupVisible NOTIFY popupVisibleChanged)

public:
    explicit MdAutoComplete(QWidget *parent = nullptr);
    ~MdAutoComplete() override;

    QString text() const;
    void setText(const QString &text);

    QString labelText() const;
    void setLabelText(const QString &label);

    QString placeholderText() const;
    void setPlaceholderText(const QString &placeholder);

    MdAutoCompleteFilter filter() const { return m_filter; }
    void setFilter(MdAutoCompleteFilter filter);

    int maxVisible() const { return m_maxVisible; }
    void setMaxVisible(int maxVisible);

    bool isPopupVisible() const { return m_popupOpen; }

    /// The full candidate list. `setItems` re-filters against the query.
    void setItems(const QStringList &items);
    void addItem(const QString &text);
    void clearItems();
    QStringList items() const { return m_items; }

    /// The suggestion list currently shown (after filtering).
    QStringList filteredItems() const { return m_filtered; }

    /// Publish the result of a Custom filter. Rebuilds the popup.
    void setFilteredItems(const QStringList &items);

    /// The inner field (for hosts that need to tweak MdTextField directly).
    MdTextField *field() const { return m_field; }

public slots:
    void showPopup();
    void hidePopup();

signals:
    void textChanged(const QString &text);
    void labelTextChanged(const QString &label);
    void placeholderTextChanged(const QString &placeholder);
    void filterChanged(md::MdAutoCompleteFilter filter);
    void maxVisibleChanged(int maxVisible);
    void popupVisibleChanged(bool visible);
    /// A suggestion was committed (click or Enter).
    void activated(const QString &text);
    /// Emitted when `filter` is Custom, with the current query and the base list.
    void filterRequested(const QString &query, const QStringList &items);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onFieldTextChanged(const QString &text);
    void onMenuClosed();
    void onMenuItemClicked();
    void onThemeChanged();

private:
    void init();
    void rebuildMenu();
    void recomputeFiltered();
    void commit(const QString &text);
    void moveHighlight(int delta);

    MdTextField *m_field = nullptr;
    MdMenu *m_menu = nullptr;
    QList<MdMenuItem *> m_menuItems;

    QStringList m_items;
    QStringList m_filtered;
    MdAutoCompleteFilter m_filter = MdAutoCompleteFilter::Contains;
    int m_maxVisible = 8;
    int m_highlight = -1;
    bool m_popupOpen = false;
    bool m_rebuilding = false;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdAutoCompleteFilter)

#endif // MD_AUTOCOMPLETE_H
