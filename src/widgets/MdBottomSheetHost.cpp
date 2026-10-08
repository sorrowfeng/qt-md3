#include "MdBottomSheetHost.h"

#include "MdBottomSheet.h"

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

MdBottomSheetHost::MdBottomSheetHost(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::StrongFocus);
    if (parent) {
        setGeometry(parent->rect());
        parent->installEventFilter(this);
    }

    // The sheet exists from construction — the dialog host's lesson: a lazy
    // create hands callers a null pointer between construction and the first
    // show.
    m_sheet = new MdBottomSheet(MdSheetKind::Modal, this);
    m_sheet->setState(MdSheetState::Hidden);
    m_sheet->hide();
    // The host is the modal presentation: it owns the sheet's geometry.
    m_sheet->setGeometryManaged(true);
    connect(m_sheet, &MdBottomSheet::dismissed, this, [this] {
        emit dismissed();
        if (m_phase == Phase::Idle) {
            startTransition(Phase::Leaving);
        }
    });

    m_transitionTimer = new QTimer(this);
    m_transitionTimer->setInterval(kTransitionTickMs);
    connect(m_transitionTimer, &QTimer::timeout, this, &MdBottomSheetHost::onTransitionTick);

    // Idle hosts are hidden: an overlay that stays visible across its parent
    // would swallow every click below it while painting nothing.
    hide();
}

MdBottomSheetHost::~MdBottomSheetHost() = default;

void MdBottomSheetHost::show()
{
    if (m_sheet->isVisible() && m_phase == Phase::Idle) {
        return;
    }
    raise();
    QWidget::show();
    layoutSheet();
    m_sheet->showSheet();
    setFocus();
    m_scrimAlpha = 0.0;
    startTransition(Phase::Entering);
}

void MdBottomSheetHost::dismiss()
{
    // The animateToDismiss flow: the sheet's confirm veto runs first; a
    // confirmed hide settles into dismissed() which fades the scrim.
    if (m_phase == Phase::Leaving
        || (!m_sheet->isVisible() && m_phase == Phase::Idle)) {
        return;
    }
    m_sheet->hideSheet();
    if (!m_sheet->isAnimating() && !m_sheet->isVisible()) {
        // Vetoed or already hidden: settle the scrim directly.
        startTransition(Phase::Leaving);
    }
}

void MdBottomSheetHost::layoutSheet()
{
    if (!m_sheet) {
        return;
    }
    // The width clamp is Compose's SheetMaxWidth, applied to the sheet
    // widget (container + side/top shadow margins) against the host's rect.
    // Bottom-centred: the x centring is the sheet's own applyOffset, the
    // width is set here.
    const int sheetWidth = qMin(int(MdBottomSheetTokens::kSheetMaxWidth
                                    + MdBottomSheetTokens::kShadowMargin * 2.0),
                                width());
    if (m_sheet->width() != sheetWidth) {
        m_sheet->resize(sheetWidth, m_sheet->height());
    }
}

void MdBottomSheetHost::startTransition(Phase phase)
{
    m_phase = phase;
    m_transitionClock.restart();
    m_transitionTimer->start();
    onTransitionTick();
}

void MdBottomSheetHost::onTransitionTick()
{
    if (m_phase == Phase::Idle) {
        m_transitionTimer->stop();
        return;
    }
    // The scrim alpha rides the default-effects spring — Compose's
    // `animateFloatAsState` on MotionSchemeKeyTokens.DefaultEffects.
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

void MdBottomSheetHost::paintEvent(QPaintEvent * /*event*/)
{
    if (m_scrimAlpha <= 0.0) {
        return;
    }
    // The scrim: md.sys.color.scrim, black at 0.32, faded by the transition.
    QColor scrim = MdTheme::instance().color(ColorRole::Scrim);
    scrim.setAlphaF(scrim.alphaF() * MdBottomSheetTokens::kScrimOpacity * m_scrimAlpha);
    if (scrim.alpha() <= 0) {
        return;
    }
    QPainter painter(this);
    painter.fillRect(rect(), scrim);
}

void MdBottomSheetHost::mousePressEvent(QMouseEvent *event)
{
    // Click on the scrim: the animateToDismiss flow (Compose's
    // dismissOnClickOutside). Clicks inside the sheet never reach the host.
    if (m_sheet->isVisible() && !m_sheet->geometry().contains(event->pos())) {
        dismiss();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MdBottomSheetHost::keyPressEvent(QKeyEvent *event)
{
    // Back/Escape is the settleToDismiss flow: Expanded collapses to partial
    // when the partial anchor exists, otherwise the sheet hides.
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        const bool partialExists = MdBottomSheetStyle::hasAnchor(
            MdSheetKind::Modal, MdSheetState::PartiallyExpanded, qreal(height()),
            m_sheet->sheetHeight(), m_sheet->peekHeight(), m_sheet->skipHiddenState(),
            m_sheet->skipPartiallyExpanded());
        if (m_sheet->state() == MdSheetState::Expanded && partialExists) {
            m_sheet->partialExpand();
        } else {
            dismiss();
        }
        return;
    }
    QWidget::keyPressEvent(event);
}

bool MdBottomSheetHost::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
        layoutSheet();
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace md
