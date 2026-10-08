#include "MdSideSheetHost.h"

#include "MdSideSheet.h"

#include "core/MdMotion.h"
#include "core/MdTheme.h"

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

#include <cmath>

namespace md {

namespace {

/// The transition tick granularity (~60 fps, the same cadence the other
/// animation cycles use).
constexpr int kTransitionTickMs = 16;

} // namespace

MdSideSheetHost::MdSideSheetHost(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::StrongFocus);
    if (parent) {
        setGeometry(parent->rect());
        parent->installEventFilter(this);
    }

    // The sheet exists from construction — the bottom-sheet host's lesson: a
    // lazy create hands callers a null pointer between construction and the
    // first show.
    m_sheet = new MdSideSheet(MdSideSheetKind::Modal, MdSideSheetEdge::Right, this);
    m_sheet->setState(MdSideSheetState::Hidden);
    m_sheet->hide();
    // The host is the modal presentation: it owns the sheet's geometry.
    m_sheet->setGeometryManaged(true);
    connect(m_sheet, &MdSideSheet::dismissed, this, [this] {
        emit dismissed();
        if (m_phase == Phase::Idle) {
            startTransition(Phase::Leaving);
        }
    });

    m_transitionTimer = new QTimer(this);
    m_transitionTimer->setInterval(kTransitionTickMs);
    connect(m_transitionTimer, &QTimer::timeout, this, &MdSideSheetHost::onTransitionTick);

    // Idle hosts are hidden: an overlay that stays visible across its parent
    // would swallow every click below it while painting nothing.
    hide();
}

MdSideSheetHost::~MdSideSheetHost() = default;

void MdSideSheetHost::show()
{
    if (m_sheet->isVisible() && m_phase == Phase::Idle) {
        return;
    }
    raise();
    QWidget::show();
    layoutSheet();
    // `SideSheetDialog.getStateOnStart()` — the modal side sheet starts
    // Expanded, the enter animation carrying it in.
    m_sheet->expandSheet();
    setFocus();
    m_scrimAlpha = 0.0;
    startTransition(Phase::Entering);
}

void MdSideSheetHost::dismiss()
{
    // The cancel flow: the sheet's confirm veto runs first; a confirmed
    // hide settles into dismissed() which fades the scrim.
    if (m_phase == Phase::Leaving || (!m_sheet->isVisible() && m_phase == Phase::Idle)) {
        return;
    }
    m_sheet->hideSheet();
    if (!m_sheet->isAnimating() && !m_sheet->isVisible()) {
        // Vetoed or already hidden: settle the scrim directly.
        startTransition(Phase::Leaving);
    }
}

void MdSideSheetHost::layoutSheet()
{
    if (!m_sheet) {
        return;
    }
    // The width is the token's 256 px container width (+ the shadow margin
    // on the inner side), clamped into the host's rect. The height is the
    // host's full height (`container.height` = 100%).
    const int sheetWidth = qMin(int(MdSideSheetTokens::resolve().containerWidth
                                    + MdSideSheetTokens::kShadowMargin),
                                width());
    if (m_sheet->width() != sheetWidth) {
        m_sheet->resize(sheetWidth, height());
    }
}

void MdSideSheetHost::startTransition(Phase phase)
{
    m_phase = phase;
    m_transitionClock.restart();
    m_transitionTimer->start();
    onTransitionTick();
}

void MdSideSheetHost::onTransitionTick()
{
    if (m_phase == Phase::Idle) {
        m_transitionTimer->stop();
        return;
    }
    // The scrim alpha rides the default-effects spring — the same
    // transcription the bottom-sheet host makes of Compose's
    // `animateFloatAsState` on DefaultEffects.
    const MdSpring scrimSpring = MdMotion::spring(MotionSpring::EffectsDefault);
    const qreal seconds = qreal(m_transitionClock.elapsed()) / 1000.0;
    m_scrimAlpha = m_phase == Phase::Entering ? scrimSpring.valueAt(seconds)
                                              : 1.0 - scrimSpring.valueAt(seconds);
    m_scrimAlpha = qBound(0.0, m_scrimAlpha, 1.0);

    const bool settled = seconds * 1000.0 >= scrimSpring.settlingDurationMs();
    if (settled) {
        m_transitionTimer->stop();
        m_scrimAlpha = m_phase == Phase::Entering ? 1.0 : 0.0;
        m_phase = Phase::Idle;
        if (m_scrimAlpha <= 0.0) {
            hide();
        }
    }
    update();
}

void MdSideSheetHost::paintEvent(QPaintEvent * /*event*/)
{
    if (m_scrimAlpha <= 0.0) {
        return;
    }
    // The scrim: md.sys.color.scrim, black at 0.32, faded by the transition.
    QColor scrim = MdTheme::instance().color(ColorRole::Scrim);
    scrim.setAlphaF(scrim.alphaF() * MdSideSheetTokens::kScrimOpacity * m_scrimAlpha);
    if (scrim.alpha() <= 0) {
        return;
    }
    QPainter painter(this);
    painter.fillRect(rect(), scrim);
}

void MdSideSheetHost::mousePressEvent(QMouseEvent *event)
{
    // Click on the scrim: the cancel flow (MDC's dialog cancels on touch
    // outside). Clicks inside the sheet never reach the host.
    if (m_sheet->isVisible() && !m_sheet->geometry().contains(event->pos())) {
        dismiss();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MdSideSheetHost::keyPressEvent(QKeyEvent *event)
{
    // Escape runs the cancel flow — the dialog's back handling.
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        dismiss();
        return;
    }
    QWidget::keyPressEvent(event);
}

bool MdSideSheetHost::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
        layoutSheet();
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace md
