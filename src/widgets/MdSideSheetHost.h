#ifndef MD_SIDE_SHEET_HOST_H
#define MD_SIDE_SHEET_HOST_H

// MdSideSheetHost — presentation for MdSideSheet: the SideSheetDialog half
// of the family.
//
// MDC-Android splits the family the same way the bottom sheet splits it: the
// sheet surface ("only the visuals and the anchors") lives in
// `SideSheetBehavior`; `SideSheetDialog` owns the dialog window, the dimming
// scrim and the cancel flow. This host is that other half on Qt's widget
// tree:
//
//   * the host is an overlay across its parent that paints the scrim —
//     md.sys.color.scrim, black at 0.32 — and swallows everything below it
//     (modality stops at the host's parent, the recorded child-widget caveat
//     shared with the dialog and bottom-sheet hosts);
//   * the sheet sits docked against the host's edge, full-height, at the
//     token's 256 px container width (+ the shadow margins), never wider
//     than the host;
//   * the scrim's alpha animates on the default-effects spring — the same
//     transcription the bottom-sheet host makes of Compose's
//     `animateFloatAsState` (MDC's dialog dims with the window scrim);
//   * Escape and a scrim click run the cancel flow: the sheet hides
//     (confirmed), the scrim fades out, `dismissed()` arrives through the
//     sheet. MDC's `SideSheetDialog` cancels when the behavior reports
//     HIDDEN;
//   * the modal sheet starts Expanded on show — `SideSheetDialog`'s
//     `getStateOnStart()` returns STATE_EXPANDED, with the enter animation
//     carrying it in.
//
// The sheet child exists from construction — callers configure its content
// and edge through sheet() before showing.

#include "core/MdSideSheetTokens.h"
#include "core/QtMd3Export.h"
#include "MdSideSheet.h"

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdSideSheet;

class QT_MD3_EXPORT MdSideSheetHost : public QWidget
{
    Q_OBJECT

public:
    /// An overlay across `parent`. A null parent makes a top-level host,
    /// which is rarely what a modal sheet wants.
    explicit MdSideSheetHost(QWidget *parent = nullptr);
    ~MdSideSheetHost() override;

    // --- the sheet ----------------------------------------------------------
    /// The hosted sheet. Created eagerly; configure before showing.
    MdSideSheet *sheet() const { return m_sheet; }

    // --- requests -----------------------------------------------------------
    /// Slide the sheet in over the scrim. No-op while already showing.
    void show();
    /// Run the cancel flow: the sheet hides (confirmed), the scrim fades
    /// out. `dismissed()` arrives through the sheet.
    void dismiss();

    // --- observable state (for tests and callers) ---------------------------
    bool isShowing() const { return m_sheet->isVisible() || m_phase != Phase::Idle; }
    /// The current scrim alpha — 1 fully dimmed.
    qreal currentScrimAlpha() const { return m_scrimAlpha; }
    /// The scrim transition is running while true.
    bool isTransitioning() const { return m_phase != Phase::Idle; }

signals:
    /// The sheet was asked to go away (Escape, scrim click, drag to hidden).
    /// Emitted at request time, before the slide-out completes.
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

    MdSideSheet *m_sheet = nullptr;
    QTimer *m_transitionTimer = nullptr;
    QElapsedTimer m_transitionClock;
    Phase m_phase = Phase::Idle;
    qreal m_scrimAlpha = 0.0;
};

} // namespace md

#endif // MD_SIDE_SHEET_HOST_H
