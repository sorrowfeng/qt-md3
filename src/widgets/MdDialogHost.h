#ifndef MD_DIALOG_HOST_H
#define MD_DIALOG_HOST_H

// MdDialogHost — presentation for MdDialog: the Compose `BasicAlertDialog`
// half of the family.
//
// Compose splits the family the same way it splits the tooltip and the
// snackbar: the dialog content is "only the visuals"; `BasicAlertDialog`
// owns the platform window, the dimming scrim, the width bounds and the
// `onDismissRequest` flow. This host is that other half on Qt's widget tree:
//
//   * the host is an overlay across its parent that paints the scrim —
//     md.sys.color.scrim, black at 0.32 — and swallows everything below it
//     (a dialog is modal within the host's parent, which is as far as a
//     child-widget port can honestly go; see the divergence note);
//   * the dialog sits centred, its width clamped into [280, 560] (the
//     Compose `DialogMinWidth` / `DialogMaxWidth` bounds), never wider than
//     the host;
//   * Escape (the dialog's own key handling) and a click on the scrim both
//     run the `onDismissRequest` flow: `dismissed()` is emitted at request
//     time and the dialog fades out — FadeInFadeOut on the effects-fast
//     spring. Compose's dialogs fade only, no scale.
//
// The dialog child exists from construction — callers configure its content
// through dialog() before showing.

#include "core/MdDialogTokens.h"
#include "core/QtMd3Export.h"
#include "MdDialog.h"

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdDialog;

class QT_MD3_EXPORT MdDialogHost : public QWidget
{
    Q_OBJECT

public:
    /// An overlay across `parent`. A null parent makes a top-level host,
    /// which is rarely what a dialog wants.
    explicit MdDialogHost(QWidget *parent = nullptr);
    ~MdDialogHost() override;

    // --- the dialog ---------------------------------------------------------
    /// The hosted dialog. Created eagerly; configure before showing.
    MdDialog *dialog() const { return m_dialog; }

    // --- requests -----------------------------------------------------------
    /// Fade the dialog in over the scrim. No-op while already showing.
    void show();
    /// Run the onDismissRequest flow: emit `dismissed()`, fade out.
    void dismiss();

    // --- observable state (for tests and callers) ---------------------------
    bool isShowing() const { return m_phase != Phase::Idle || m_dialog->isVisible(); }
    /// The current enter/exit fade value — 1 fully shown.
    qreal currentOpacity() const { return m_opacity; }
    /// The enter/exit transition is running while true.
    bool isTransitioning() const { return m_phase != Phase::Idle; }

signals:
    /// The dialog was asked to go away (Escape, scrim click, dismiss()).
    /// Emitted at request time, before the fade-out completes.
    void dismissed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class Phase
    {
        Idle,
        Entering,
        Leaving,
    };

    void layoutDialog();
    void startTransition(Phase phase);
    void onTransitionTick();

    MdDialog *m_dialog = nullptr;
    QTimer *m_transitionTimer = nullptr;
    QElapsedTimer m_transitionClock;
    Phase m_phase = Phase::Idle;
    qreal m_opacity = 0.0;
};

} // namespace md

#endif // MD_DIALOG_HOST_H
