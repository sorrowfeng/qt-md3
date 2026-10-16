#ifndef MD_NOTIFICATION_H
#define MD_NOTIFICATION_H

// MdNotification — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "Snackbar 的持久化变体" — the persistent
// variant of the Snackbar family.
//
// Compose's snackbar has two halves: the visual (`Snackbar`) and the host
// (`SnackbarHostState`) that owns queueing and the 4000 / 10000 / Indefinite
// durations. A *desktop notification* is the Indefinite case made first-class:
// a surface that stays until the user dismisses it, with the dismiss action
// the snackbar export already publishes (and recommends whenever the
// snackbar does not self-dismiss).
//
// MdNotification is that persistent reading of the same token set
// (`md.comp.snackbar.*`, export 34.0.21):
//
//   container  inverse-surface, elevation 3, extra-small corner
//   message    body-medium, inverse-on-surface
//   action     label-large, inverse-primary (optional)
//   dismiss    24 px close, inverse-on-surface (on by default)
//
// Unlike MdSnackbarHost there is no queue and no timer — the widget is the
// notification itself, placed inline or overlaid by the caller. `dismiss()`
// (or the close icon) emits `dismissed()`; `actionClicked()` is the same
// signal MdSnackbar exposes. The visuals are a real MdSnackbar child, so the
// two families can never drift apart.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdSnackbarTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

namespace md {

class MdSnackbar;

class QT_MD3_EXPORT MdNotification : public QWidget
{
    Q_OBJECT

    /// The supporting text (body-medium, inverse-on-surface).
    Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY messageChanged)
    /// The optional action label (label-large, inverse-primary).
    Q_PROPERTY(QString actionLabel READ actionLabel WRITE setActionLabel
                   NOTIFY actionLabelChanged)
    /// Show the 24 px close icon. Defaults to true — a persistent
    /// notification must be dismissible.
    Q_PROPERTY(bool hasDismissAction READ hasDismissAction WRITE setHasDismissAction
                   NOTIFY hasDismissActionChanged)
    /// Put the action on its own line below the message.
    Q_PROPERTY(bool actionOnNewLine READ isActionOnNewLine WRITE setActionOnNewLine
                   NOTIFY actionOnNewLineChanged)

public:
    explicit MdNotification(QWidget *parent = nullptr);
    explicit MdNotification(const QString &message, QWidget *parent = nullptr);
    ~MdNotification() override;

    QString message() const;
    void setMessage(const QString &message);

    QString actionLabel() const;
    void setActionLabel(const QString &actionLabel);

    bool hasDismissAction() const;
    void setHasDismissAction(bool show);

    bool isActionOnNewLine() const;
    void setActionOnNewLine(bool onNewLine);

    /// The underlying snackbar visual (owned; read-only for tests).
    MdSnackbar *snackbar() const { return m_snackbar; }

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

public slots:
    /// Hide the notification and emit `dismissed()`. Idempotent.
    void dismiss();

signals:
    void messageChanged(const QString &message);
    void actionLabelChanged(const QString &actionLabel);
    void hasDismissActionChanged(bool show);
    void actionOnNewLineChanged(bool onNewLine);
    void actionClicked();
    void dismissed();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onSnackbarAction();
    void onSnackbarDismiss();

private:
    void init();
    void relayout();

    MdSnackbar *m_snackbar = nullptr;
    bool m_dismissed = false;
};

} // namespace md

#endif // MD_NOTIFICATION_H
