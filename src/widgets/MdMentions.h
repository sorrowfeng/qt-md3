#ifndef MD_MENTIONS_H
#define MD_MENTIONS_H

// MdMentions — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "Text field + Menu 弹层" — type an
// activation character, pick a person / topic from a Menu popup.
//
// MD3 has no mentions family. The control is the exposed-dropdown reading
// of a Text field, one character later:
//
//   field   a real MdTextField (filled / outlined — the Stage 1 tokens)
//   popup   a real MdMenu (md.comp.menu.*) anchored under the field,
//           items are MdMenuItem rows (48 px classic)
//   trigger `@` by default; typing it (or later characters after it)
//           filters the mention list, Enter / click commits a mention
//           as `@Name` into the text
//
// Escape closes the popup and keeps the typed text. Up / Down walk the
// suggestion rows. The widget is the exposed-dropdown composition MdSelect
// and MdAutoComplete already establish, so the popup semantics cannot drift.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QKeyEvent;

namespace md {

class MdMenu;
class MdMenuItem;
class MdTextField;

class QT_MD3_EXPORT MdMentions : public QWidget
{
    Q_OBJECT

    /// Current text of the field.
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// Field label (MdTextField `labelText`).
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText NOTIFY labelTextChanged)
    /// Field placeholder.
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText
                   NOTIFY placeholderTextChanged)
    /// Activation character. Default `@`.
    Q_PROPERTY(QChar trigger READ trigger WRITE setTrigger NOTIFY triggerChanged)
    /// Candidate names offered after the trigger.
    Q_PROPERTY(QStringList items READ items WRITE setItems NOTIFY itemsChanged)
    /// True while the suggestion Menu is open.
    Q_PROPERTY(bool popupVisible READ isPopupVisible NOTIFY popupVisibleChanged)
    /// Max suggestion rows shown (default 6).
    Q_PROPERTY(int maxVisible READ maxVisible WRITE setMaxVisible NOTIFY maxVisibleChanged)

public:
    explicit MdMentions(QWidget *parent = nullptr);
    ~MdMentions() override;

    QString text() const;
    void setText(const QString &text);

    QString labelText() const;
    void setLabelText(const QString &label);

    QString placeholderText() const;
    void setPlaceholderText(const QString &placeholder);

    QChar trigger() const { return m_trigger; }
    void setTrigger(QChar trigger);

    QStringList items() const { return m_items; }
    void setItems(const QStringList &items);

    bool isPopupVisible() const { return m_popupVisible; }
    void setPopupVisible(bool visible);

    int maxVisible() const { return m_maxVisible; }
    void setMaxVisible(int count);

    /// The query currently typed after the trigger, empty when idle.
    QString currentQuery() const;
    /// The suggestion list after `currentQuery()` filtering.
    QStringList filteredItems() const;

    /// The underlying Stage 1 field (owned; read-only for tests).
    MdTextField *field() const { return m_field; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    /// Insert `name` at the pending trigger and close the popup.
    void commit(const QString &name);
    void hidePopup();

signals:
    void textChanged(const QString &text);
    void labelTextChanged(const QString &label);
    void placeholderTextChanged(const QString &placeholder);
    void triggerChanged(QChar trigger);
    void itemsChanged(const QStringList &items);
    void popupVisibleChanged(bool visible);
    void maxVisibleChanged(int count);
    /// A suggestion was chosen; `name` has no trigger prefix.
    void mentioned(const QString &name);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onFieldTextChanged(const QString &text);
    void onThemeChanged();
    void onItemTriggered();

private:
    void init();
    void rebuildPopup();
    void showPopup();
    void anchorPopup();
    void moveHighlight(int delta);

    MdTextField *m_field = nullptr;
    MdMenu *m_menu = nullptr;
    QList<MdMenuItem *> m_menuItems;
    QChar m_trigger = QLatin1Char('@');
    QStringList m_items;
    QStringList m_filtered;
    bool m_popupVisible = false;
    int m_maxVisible = 6;
    int m_highlight = 0;
};

} // namespace md

#endif // MD_MENTIONS_H
