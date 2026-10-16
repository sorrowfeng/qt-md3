#ifndef MD_MESSAGE_BOX_H
#define MD_MESSAGE_BOX_H

// MdMessageBox — Stage 2 Qt extension (Appendix D, P0), the QMessageBox
// replacement.
//
// Semantic anchor (appendix D): "Dialog + Buttons + semantic colour (error
// uses the error colour role)".
//
// Why this is not a thin wrapper over MdDialog: MdDialog publishes exactly
// two action slots (confirm / dismiss), which cannot express the three-button
// sets a message box needs (Yes / No / Cancel). The chrome is therefore taken
// from the *same token set* MdDialog paints — `md.comp.dialog.*`, export
// 34.0.21 — and the action row is the spec's end-aligned FlowRow with as
// many `MdButton` children as the box needs:
//
//   container  surface-container-high, elevation level 3, corner extra-large
//   headline   headline-small, on-surface
//   body       body-medium, on-surface-variant
//   icon       24 px, tinted by the variant's semantic colour
//   actions    label-large MdButton children (Filled for affirmative,
//              Text for dismissive), 8 px spacing, end-aligned, wrapping
//
// The semantic layer is the icon colour only: Information / Question tint
// with `primary`, Warning with `tertiary`, Error with `error`. The container
// stays on the dialog token set in every variant.
//
// API follows QMessageBox where the semantics match (title, text, standard
// buttons, exec() -> result code) so the replacement is mechanical.
//
// There is no QSS.

#include "core/MdDialogTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QFlags>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtWidgets/QWidget>

class QEventLoop;
class QKeyEvent;
class QPaintEvent;

namespace md {

class MdButton;

/// Which semantic colour the icon (and only the icon) is tinted with.
enum class MdMessageBoxVariant {
    /// No icon, neutral dialog.
    None,
    /// `primary` icon — an informative note.
    Information,
    /// `tertiary` icon — a warning that does not block progress.
    Warning,
    /// `error` icon — a failure that needs acknowledgement.
    Error,
    /// `primary` icon — a question that needs a decision.
    Question,
    Count,
};

/// The QMessageBox-standard result codes. Values are stable so a result can
/// be stored as int.
enum class MdMessageBoxStandardButton {
    None = 0,
    Ok = 0x01,
    Cancel = 0x02,
    Yes = 0x04,
    No = 0x08,
    Retry = 0x10,
    Close = 0x20,
    Count = 0x40,
};
Q_DECLARE_FLAGS(MdMessageBoxStandardButtons, MdMessageBoxStandardButton)
Q_DECLARE_OPERATORS_FOR_FLAGS(MdMessageBoxStandardButtons)

/// What a custom button does when clicked — decides its result code and
/// whether it is styled as the affirmative (Filled) or dismissive (Text)
/// action.
enum class MdMessageBoxButtonRole {
    Invalid,
    /// Closes the box with an accepting result. Filled button.
    AcceptRole,
    /// Closes the box with a rejecting result. Text button.
    RejectRole,
    /// "Yes" — affirmative. Filled button.
    YesRole,
    /// "No" — dismissive. Text button.
    NoRole,
    /// "Retry" — affirmative. Filled button.
    RetryRole,
    /// "Close" — dismissive. Text button.
    CloseRole,
    /// Does not close the box by itself (the handler decides). Text button.
    ActionRole,
    Count,
};

class QT_MD3_EXPORT MdMessageBox : public QWidget
{
    Q_OBJECT

    /// The semantic colouring of the icon slot.
    Q_PROPERTY(md::MdMessageBoxVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    /// The dialog headline (headline-small, on-surface).
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// The dialog supporting text (body-medium, on-surface-variant).
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// Optional second body line, appended under `text` with a paragraph
    /// break. Empty means only `text` is shown.
    Q_PROPERTY(QString informativeText READ informativeText WRITE setInformativeText NOTIFY
                   informativeTextChanged)
    /// Material Symbols glyph drawn in the icon slot. Empty means the
    /// variant's default (info / warning / error).
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    /// The result of the last finished dialog (a MdMessageBoxStandardButton
    /// value, or 0 when dismissed).
    Q_PROPERTY(int result READ result NOTIFY finished)

public:
    explicit MdMessageBox(QWidget *parent = nullptr);
    /// Convenience constructor: title + body + variant.
    MdMessageBox(MdMessageBoxVariant variant,
                 const QString &title,
                 const QString &text,
                 QWidget *parent = nullptr);
    ~MdMessageBox() override;

    /// The result of the last finished dialog, or 0.
    int result() const { return m_result; }

    // --- content ------------------------------------------------------------
    MdMessageBoxVariant variant() const { return m_variant; }
    void setVariant(MdMessageBoxVariant variant);

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString text() const { return m_text; }
    void setText(const QString &text);

    QString informativeText() const { return m_informativeText; }
    void setInformativeText(const QString &text);

    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    /// Replace the icon slot with a custom widget (any QWidget). Passing
    /// nullptr restores the variant's default glyph.
    void setIconWidget(QWidget *icon);

    /// Colour the icon is painted with, derived from the variant (or
    /// `primary` when the variant is None and a glyph is shown).
    QColor semanticColor() const;

    // --- buttons ------------------------------------------------------------
    /// Add a custom button. `role` decides the button variant (Filled for
    /// Accept / Yes / Retry, Text otherwise) and the result code when the
    /// button closes the box. The box takes ownership.
    MdButton *addButton(const QString &text, MdMessageBoxButtonRole role);
    /// Remove and delete every button.
    void clearButtons();
    QList<MdButton *> buttons() const;

    /// Install one of the standard sets. Replaces existing buttons.
    void setStandardButtons(MdMessageBoxStandardButtons buttons);
    MdMessageBoxStandardButtons standardButtons() const { return m_standardButtons; }

    /// The button that owns the default (Enter) and escape (Escape) roles.
    void setDefaultButton(MdButton *button);
    MdButton *defaultButton() const { return m_defaultButton; }
    void setEscapeButton(MdButton *button);
    MdButton *escapeButton() const { return m_escapeButton; }

    // --- presentation -------------------------------------------------------
    /// Show the box and run a local event loop until it closes. Returns the
    /// standard-button value of the clicked button, or 0 when dismissed via
    /// Escape / reject.
    int exec();

    /// The standard-button value of `button`, or 0 for a non-standard one.
    int standardButtonValue(const MdButton *button) const;

    /// The painted container, in widget coordinates.
    QRectF containerRect() const;

    // --- static conveniences -------------------------------------------------
    static int information(QWidget *parent,
                           const QString &title,
                           const QString &text,
                           MdMessageBoxStandardButtons buttons = MdMessageBoxStandardButton::Ok);
    static int warning(QWidget *parent,
                       const QString &title,
                       const QString &text,
                       MdMessageBoxStandardButtons buttons = MdMessageBoxStandardButton::Ok);
    static int critical(QWidget *parent,
                        const QString &title,
                        const QString &text,
                        MdMessageBoxStandardButtons buttons = MdMessageBoxStandardButton::Ok);
    static int question(QWidget *parent,
                        const QString &title,
                        const QString &text,
                        MdMessageBoxStandardButtons buttons = MdMessageBoxStandardButtons());

    // QWidget
    QSize sizeHint() const override;
    int heightForWidth(int width) const override;

signals:
    void variantChanged(md::MdMessageBoxVariant variant);
    void titleChanged(const QString &title);
    void textChanged(const QString &text);
    void informativeTextChanged(const QString &text);
    void iconNameChanged(const QString &iconName);
    /// A button was clicked. `which` is the standard-button value (0 for a
    /// custom button).
    void buttonClicked(int which);
    /// The box closed. `result` is the standard-button value, or 0.
    void finished(int result);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onButtonClicked();
    void onThemeChanged();

private:
    void init();
    void layoutChildren();
    void updateIcon();
    void applyResult(int result);
    /// Title + text + informative, as the body string.
    QString bodyText() const;
    /// Action-row geometry for the current width. End-aligned, wrapping.
    QList<QList<QRectF>> actionRows(const QRectF &content, const QList<qreal> &widths) const;

    MdDialogTokens m_tokens;
    MdMessageBoxVariant m_variant = MdMessageBoxVariant::None;
    QString m_title;
    QString m_text;
    QString m_informativeText;
    QString m_iconName;
    QList<MdButton *> m_ownedButtons;
    QList<qreal> m_buttonWidths;
    MdMessageBoxStandardButtons m_standardButtons = MdMessageBoxStandardButton::None;
    MdButton *m_defaultButton = nullptr;
    MdButton *m_escapeButton = nullptr;
    int m_result = 0;
    bool m_finished = false;
    QEventLoop *m_loop = nullptr;
    QWidget *m_customIcon = nullptr;
    QWidget *m_iconWidget = nullptr;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdMessageBoxVariant)

#endif // MD_MESSAGE_BOX_H
