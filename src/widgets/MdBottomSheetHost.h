#ifndef MD_BOTTOM_SHEET_HOST_H
#define MD_BOTTOM_SHEET_HOST_H

// MdBottomSheetHost — presentation for MdBottomSheet: the Compose
// `ModalBottomSheet` wrapper half of the family.
//
// Compose splits the family the same way it splits the dialog: the sheet
// surface is "only the visuals and the anchors"; `ModalBottomSheet` owns the
// dialog window, the dimming scrim and the `onDismissRequest` flow. This
// host is that other half on Qt's widget tree:
//
//   * the host is an overlay across its parent that paints the scrim —
//     md.sys.color.scrim, black at 0.32 — and swallows everything below it
//     (modality stops at the host's parent, the recorded child-widget caveat
//     shared with the dialog host);
//   * the sheet sits bottom-centred, its width clamped into the 640 px
//     `SheetMaxWidth` (+ the shadow margins), never wider than the host;
//   * the scrim's alpha animates on the default-effects spring — Compose's
//     `animateFloatAsState(targetValue = isScrimVisible, DefaultEffects)`;
//   * Escape and a scrim click run the onDismissRequest flow, split the way
//     Compose splits them: a scrim click is `animateToDismiss` (always hide),
//     back/Escape is `settleToDismiss` (Expanded collapses to partial when
//     the partial anchor exists, otherwise hides). Hiding settles into
//     `dismissed()` — emitted by the sheet at request time, forwarded here.
//
// The sheet child exists from construction — callers configure its content
// and height through sheet() before showing.

#include "core/MdBottomSheetTokens.h"
#include "core/QtMd3Export.h"
#include "MdBottomSheet.h"

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdBottomSheet;

class QT_MD3_EXPORT MdBottomSheetHost : public QWidget
{
    Q_OBJECT

public:
    /// An overlay across `parent`. A null parent makes a top-level host,
    /// which is rarely what a modal sheet wants.
    explicit MdBottomSheetHost(QWidget *parent = nullptr);
    ~MdBottomSheetHost() override;

    // --- the sheet ----------------------------------------------------------
    /// The hosted sheet. Created eagerly; configure before showing.
    MdBottomSheet *sheet() const { return m_sheet; }

    // --- requests -----------------------------------------------------------
    /// Slide the sheet up over the scrim. No-op while already showing.
    void show();
    /// Run the onDismissRequest flow: the sheet hides (confirmed), the scrim
    /// fades out. `dismissed()` arrives through the sheet.
    void dismiss();

    // --- observable state (for tests and callers) ---------------------------
    bool isShowing() const { return m_sheet->isVisible() || m_phase != Phase::Idle; }
    /// The current scrim alpha — 1 fully dimmed.
    qreal currentScrimAlpha() const { return m_scrimAlpha; }
    /// The scrim transition is running while true.
    bool isTransitioning() const { return m_phase != Phase::Idle; }

signals:
    /// The sheet was asked to go away (Escape, scrim click, drag to hidden).
    /// Emitted at request time, before the slide-down completes.
    void dismissed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class Phase
    {
        Idle,
        Entering,
        Leaving,
    };

    void layoutSheet();
    void startTransition(Phase phase);
    void onTransitionTick();

    MdBottomSheet *m_sheet = nullptr;
    QTimer *m_transitionTimer = nullptr;
    QElapsedTimer m_transitionClock;
    Phase m_phase = Phase::Idle;
    qreal m_scrimAlpha = 0.0;
};

} // namespace md

#endif // MD_BOTTOM_SHEET_HOST_H
