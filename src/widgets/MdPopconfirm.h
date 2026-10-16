#ifndef MD_POPCONFIRM_H
#define MD_POPCONFIRM_H

// MdPopconfirm — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "用 MD3 Menu / Dialog 语义收敛，不另造浮层样式".
// A popconfirm is a *popover with actions*: the same `surface-container`
// Menu surface, but the body is the Dialog's supporting-text + action row.
// It asks a short question before a destructive commit and lives next to its
// trigger rather than centring on the window.
//
//   surface    `surface-container` @ level 2 (Menu container)
//   question   `body-medium` (Dialog supporting-text)
//   actions    text buttons, trailing-aligned (Dialog action row)
//   motion     fast spatial + fast effects (the Menu pair)
//   keyboard   Esc cancels, Enter confirms the default button
//
// There is no QSS.

#include "MdPopover.h"

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

class MdButton;

class QT_MD3_EXPORT MdPopconfirm : public MdPopover
{
    Q_OBJECT

    /// Question shown in the body (Dialog supporting-text).
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// Label of the confirm action. Defaults to a localized "OK".
    Q_PROPERTY(QString confirmText READ confirmText WRITE setConfirmText NOTIFY confirmTextChanged)
    /// Label of the cancel action. Defaults to a localized "Cancel".
    Q_PROPERTY(QString cancelText READ cancelText WRITE setCancelText NOTIFY cancelTextChanged)

public:
    explicit MdPopconfirm(QWidget *parent = nullptr);
    ~MdPopconfirm() override;

    QString text() const { return m_text; }
    void setText(const QString &text);

    QString confirmText() const { return m_confirmText; }
    void setConfirmText(const QString &text);

    QString cancelText() const { return m_cancelText; }
    void setCancelText(const QString &text);

    /// The confirm / cancel buttons (owned by the popconfirm).
    MdButton *confirmButton() const { return m_confirm; }
    MdButton *cancelButton() const { return m_cancel; }

public slots:
    /// Reveal the popconfirm under `anchorRect` (global coordinates).
    void open(const QRect &anchorRect);

signals:
    void textChanged(const QString &text);
    void confirmTextChanged(const QString &text);
    void cancelTextChanged(const QString &text);
    /// The user committed.
    void confirmed();
    /// The user cancelled (button, Esc, or outside click).
    void cancelled();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onConfirmClicked();
    void onCancelClicked();
    void onClosed();

private:
    void init();
    void rebuildBody();

    QString m_text;
    QString m_confirmText;
    QString m_cancelText;
    MdButton *m_confirm = nullptr;
    MdButton *m_cancel = nullptr;
};

} // namespace md

#endif // MD_POPCONFIRM_H
