#ifndef MD_INPUT_DIALOG_H
#define MD_INPUT_DIALOG_H

// MdInputDialog — Stage 2 Qt extension (Appendix D, P0).
//
// Semantic anchor (appendix D): "Dialog + Text field + Buttons".
//
// The QInputDialog replacement. Chrome is the same `md.comp.dialog.*` set
// MdMessageBox paints (surface-container-high, level 3, corner extra-large);
// the body is one filled `MdTextField`; the actions are real `MdButton`
// children (Filled confirm / Text dismiss), end-aligned with 8 px spacing.
//
//   container  surface-container-high, elevation level 3, corner extra-large
//   headline   headline-small, on-surface
//   field      filled MdTextField (label + value)
//   actions    label-large MdButton pair
//
// Escape rejects; Enter accepts. `exec()` runs a local loop like
// MdMessageBox.
//
// There is no QSS.

#include "core/MdDialogTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QEventLoop;
class QKeyEvent;
class QPaintEvent;

namespace md {

class MdButton;
class MdTextField;

/// What kind of value the dialog collects. Affects keyboard filtering only;
/// the chrome is identical in every mode.
enum class MdInputDialogMode {
    /// Free-form text.
    TextInput,
    /// Integer value.
    IntInput,
    /// Floating-point value.
    DoubleInput,
    /// Password text (echo mode password).
    PasswordInput,
    Count,
};

class QT_MD3_EXPORT MdInputDialog : public QWidget
{
    Q_OBJECT

    /// Dialog headline (headline-small).
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// Field label text.
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText NOTIFY labelTextChanged)
    /// Current field value as text.
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// Field placeholder text.
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY
                   placeholderTextChanged)
    /// Input mode (filters characters and echo mode).
    Q_PROPERTY(md::MdInputDialogMode mode READ mode WRITE setMode NOTIFY modeChanged)
    /// Confirm button caption.
    Q_PROPERTY(QString acceptText READ acceptText WRITE setAcceptText NOTIFY acceptTextChanged)
    /// Dismiss button caption.
    Q_PROPERTY(QString rejectText READ rejectText WRITE setRejectText NOTIFY rejectTextChanged)

public:
    explicit MdInputDialog(QWidget *parent = nullptr);
    MdInputDialog(const QString &title, const QString &label, const QString &text = QString(),
                  QWidget *parent = nullptr);
    ~MdInputDialog() override;

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString labelText() const { return m_label; }
    void setLabelText(const QString &label);

    QString text() const;
    void setText(const QString &text);

    QString placeholderText() const;
    void setPlaceholderText(const QString &placeholder);

    MdInputDialogMode mode() const { return m_mode; }
    void setMode(MdInputDialogMode mode);

    QString acceptText() const { return m_acceptText; }
    void setAcceptText(const QString &text);

    QString rejectText() const { return m_rejectText; }
    void setRejectText(const QString &text);

    /// Numeric helpers. Meaningful in IntInput / DoubleInput modes.
    int intValue() const;
    void setIntValue(int value);
    double doubleValue() const;
    void setDoubleValue(double value);

    /// The field and the two action buttons (always created).
    MdTextField *field() const { return m_field; }
    MdButton *acceptButton() const { return m_acceptButton; }
    MdButton *rejectButton() const { return m_rejectButton; }

    /// Show and run a local loop until the dialog closes. Returns true when
    /// the user accepted.
    bool exec();

    /// Painted container in widget coordinates.
    QRectF containerRect() const;

    QSize sizeHint() const override;
    int heightForWidth(int width) const override;

    // --- static conveniences -------------------------------------------------
    static bool getText(QWidget *parent, const QString &title, const QString &label,
                        QString *text, bool *ok = nullptr);
    static bool getInt(QWidget *parent, const QString &title, const QString &label,
                       int *value, bool *ok = nullptr);

signals:
    void titleChanged(const QString &title);
    void labelTextChanged(const QString &label);
    void textChanged(const QString &text);
    void placeholderTextChanged(const QString &placeholder);
    void modeChanged(md::MdInputDialogMode mode);
    void acceptTextChanged(const QString &text);
    void rejectTextChanged(const QString &text);
    /// The user accepted. Carries the current text.
    void accepted(const QString &text);
    /// The user dismissed (Escape / reject button).
    void rejected();
    /// The dialog closed. `result` is true when accepted.
    void finished(bool result);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onAcceptClicked();
    void onRejectClicked();
    void onThemeChanged();

private:
    void init();
    void layoutChildren();
    void applyResult(bool ok);

    MdDialogTokens m_tokens;
    QString m_title;
    QString m_label;
    QString m_acceptText;
    QString m_rejectText;
    MdInputDialogMode m_mode = MdInputDialogMode::TextInput;
    MdTextField *m_field = nullptr;
    MdButton *m_acceptButton = nullptr;
    MdButton *m_rejectButton = nullptr;
    bool m_finished = false;
    QEventLoop *m_loop = nullptr;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdInputDialogMode)

#endif // MD_INPUT_DIALOG_H
