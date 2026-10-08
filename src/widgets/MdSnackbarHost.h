#ifndef MD_SNACKBAR_HOST_H
#define MD_SNACKBAR_HOST_H

// MdSnackbarHost — placement, queueing and durations for MdSnackbar.
//
// Compose splits the family in two: `Snackbar` is "only the visuals", and
// `SnackbarHostState.showSnackbar` owns everything else — the queue, the
// 4000/10000/Indefinite durations and the rule that a snackbar with an action
// must not self-dismiss. MdSnackbar is the visuals; this host is that other
// half, ported onto Qt's widget tree:
//
//   * the host is a transparent overlay across its parent — clicks pass
//     through everywhere except the snackbar itself;
//   * the snackbar sits bottom-centre with the host's 12 px margin, never
//     wider than the container max width (600 px);
//   * durations are Compose's: Short 4000 / Long 10000 / Indefinite never,
//     and `Auto` resolves caller-side — an action pins Indefinite;
//   * a new request while one shows is queued; `showSnackbar` on an
//     already-queued duplicate message is dropped (Compose SnackbarHostState
//     de-dupes only the *current* one, so a queue cap is this port's own
//     back-pressure — see the open question in docs/porting-todo.md);
//   * the enter/exit transition is Compose's FadeInFadeOutWithScale: opacity
//     on the effects-fast spring, scale 0.8→1 on the spatial-fast spring.
//
// The host forwards the current snackbar's signals before it animates out,
// so an application reacts at click time, not after the fade.

#include "core/MdSnackbarTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdSnackbar;

class QT_MD3_EXPORT MdSnackbarHost : public QWidget
{
    Q_OBJECT

public:
    /// An overlay across `parent`. A null parent makes a top-level host,
    /// which is rarely what a snackbar wants.
    explicit MdSnackbarHost(QWidget *parent = nullptr);
    ~MdSnackbarHost() override;

    // --- requests -----------------------------------------------------------
    /// Queue a snackbar. `duration` Auto resolves to Indefinite when an
    /// action label is given (an actionable snackbar must not self-dismiss),
    /// Short otherwise.
    void showSnackbar(const QString &message,
                      const QString &actionLabel = QString(),
                      MdSnackbarDuration duration = MdSnackbarDuration::Auto);

    /// Animate the current snackbar out; queued ones follow.
    void dismissCurrent();

    // --- observable state (for tests and callers) ---------------------------
    bool isShowing() const { return m_current != nullptr; }
    int queueLength() const { return m_queue.size(); }
    MdSnackbar *current() const { return m_current; }

    /// The current enter/exit transition values — 1/1 fully shown.
    qreal currentOpacity() const { return m_opacity; }
    qreal currentScale() const { return m_scale; }

    /// The enter/exit transition is running while true.
    bool isTransitioning() const { return m_phase != Phase::Idle; }

signals:
    /// The current snackbar's action was activated.
    void actionClicked();
    /// The current snackbar's dismiss was activated (or dismissCurrent()).
    void dismissed();
    /// Nothing shows and the queue is empty.
    void allCleared();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onTimeout();

private:
    struct Request
    {
        QString message;
        QString actionLabel;
        MdSnackbarDuration duration = MdSnackbarDuration::Auto;
    };

    enum class Phase
    {
        Idle,
        Entering,
        Leaving,
    };

    void layoutCurrent();
    void showNext();
    void startTransition(Phase phase);
    void finishCurrent();
    void onTransitionTick();

    QList<Request> m_queue;
    MdSnackbar *m_current = nullptr;
    QRect m_restingGeometry;
    QTimer *m_durationTimer = nullptr;
    QTimer *m_transitionTimer = nullptr;
    QElapsedTimer m_transitionClock;
    Phase m_phase = Phase::Idle;
    qreal m_opacity = 1.0;
    qreal m_scale = 1.0;
};

} // namespace md

#endif // MD_SNACKBAR_HOST_H
