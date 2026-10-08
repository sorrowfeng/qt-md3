#ifndef MD_DIALOG_H
#define MD_DIALOG_H

// MdDialog — MD3 dialogs: the basic dialog surface.
//
// The `md.comp.dialog.*` family, one published set. material-web publishes
// no dialog *web component* (token export only — the same situation as the
// card), so the behaviour source is Compose M3's AlertDialog.kt /
// BasicAlertDialog: a Surface carrying four slots in a padded column —
//
//     icon (24 px, centred, bottom 16)      — optional
//     title (headline-small, bottom 16)     — optional
//     text (body-medium, bottom 24)         — optional
//     actions (end-aligned FlowRow, 8 px)   — optional
//
// How the slots map onto Qt:
//
//   * the title and the supporting text are painted by the style (the
//     snackbar idiom — the dialog owns their strings and wraps the text);
//   * the icon slot is a plain `QWidget*` handed to setIconWidget — Compose's
//     icon slot is any composable, so any widget goes (an MdIcon, an
//     MdIconButton, a plain QLabel);
//   * the action slots are real `MdButton` children (Text variant) handed to
//     setConfirmButton / setDismissButton. Compose documents that its action
//     slots are TextButtons which use their own colours — and the export's
//     action rows (label-large in primary at the standard opacities) ARE the
//     text button's tokens, so embedding gets ripple, state layers and
//     :focus-visible for free instead of re-drawing them. This is the
//     recorded divergence from the snackbar's self-drawn action, whose
//     inverse-primary colour no MdButton variant could express.
//
// The dialog itself is non-interactive: the container publishes no state
// rows. Escape emits `dismissed()` — the host turns that (and outside
// clicks) into the Compose `onDismissRequest` flow.
//
// The widget rect is the container grown by the shadow margin (the snackbar
// idiom — the level-3 shadow needs headroom a child widget cannot paint
// into). The button and icon children are positioned over the container.

#include "core/MdDialogTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"
#include "styles/MdDialogStyle.h"

#include <QtCore/QRectF>
#include <QtWidgets/QWidget>

class QKeyEvent;

namespace md {

class MdButton;
class MdDialogStyle;

class QT_MD3_EXPORT MdDialog : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    explicit MdDialog(QWidget *parent = nullptr);
    ~MdDialog() override;

    // --- content ------------------------------------------------------------
    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString text() const { return m_text; }
    void setText(const QString &text);

    /// The icon slot: any widget, reparented into the dialog and positioned
    /// centred above the title. Passing nullptr clears the slot.
    void setIconWidget(QWidget *icon);
    QWidget *iconWidget() const { return m_icon; }

    /// The action slots: real MdButton children, reparented and laid out
    /// end-aligned (confirm rightmost when one row fits; wrapped, the
    /// confirm takes the first row). Text variant recommended — the export's
    /// action rows are the text button's own tokens.
    void setConfirmButton(MdButton *button);
    void setDismissButton(MdButton *button);
    MdButton *confirmButton() const { return m_confirmButton; }
    MdButton *dismissButton() const { return m_dismissButton; }

    // --- tokens -------------------------------------------------------------
    /// The resolved `md.comp.dialog.*` set, after the application-wide and
    /// per-instance `md.comp.*` overrides.
    const MdDialogTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this dialog alone.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -----------------------------------------------------------
    /// The painted container, in widget coordinates — the widget rect minus
    /// the shadow margin.
    QRectF containerRect() const;

    /// The layout computed for the current width and content: the geometry
    /// the icon and button children are positioned into.
    MdDialogStyle::Layout currentLayout() const;

    // QWidget
    QSize sizeHint() const override;
    int heightForWidth(int width) const override;

    /// The host's enter/exit fade, applied inside the style (a child widget
    /// has no window opacity — the snackbar idiom).
    qreal paintOpacity() const { return m_paintOpacity; }
    void setPaintOpacity(qreal opacity);

signals:
    void titleChanged(const QString &title);
    void textChanged(const QString &text);
    /// Escape was pressed — the host's onDismissRequest flow.
    void dismissed();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void invalidateTokens();
    void syncChildGeometry();

    QString m_title;
    QString m_text;
    QWidget *m_icon = nullptr;
    MdButton *m_confirmButton = nullptr;
    MdButton *m_dismissButton = nullptr;

    mutable MdDialogTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    qreal m_paintOpacity = 1.0;

    friend class MdDialogStyle;
};

} // namespace md

#endif // MD_DIALOG_H
