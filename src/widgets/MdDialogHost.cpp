#include "MdDialogHost.h"

#include "MdDialog.h"

#include "core/MdMotion.h"
#include "core/MdTheme.h"

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

#include <cmath>

namespace md {

namespace {

/// The transition tick granularity (~60 fps, the same cadence the other
/// animation cycles use).
constexpr int kTransitionTickMs = 16;

} // namespace

MdDialogHost::MdDialogHost(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::StrongFocus);
    if (parent) {
        setGeometry(parent->rect());
        parent->installEventFilter(this);
    }

    // The dialog exists from construction — the tooltip host's lesson: a
    // lazy create hands callers a null pointer between construction and the
    // first show.
    m_dialog = new MdDialog(this);
    m_dialog->hide();
    connect(m_dialog, &MdDialog::dismissed, this, &MdDialogHost::dismiss);

    m_transitionTimer = new QTimer(this);
    m_transitionTimer->setInterval(kTransitionTickMs);
    connect(m_transitionTimer, &QTimer::timeout, this, &MdDialogHost::onTransitionTick);
}

MdDialogHost::~MdDialogHost() = default;

void MdDialogHost::show()
{
    if (m_dialog->isVisible() && m_phase == Phase::Idle) {
        return;
    }
    raise();
    layoutDialog();
    m_dialog->show();
    setFocus();
    m_dialog->setFocus();
    m_opacity = 0.0;
    startTransition(Phase::Entering);
}

void MdDialogHost::dismiss()
{
    if (m_phase == Phase::Leaving
        || (!m_dialog->isVisible() && m_phase == Phase::Idle)) {
        return;
    }
    emit dismissed();
    if (parentWidget()) {
        parentWidget()->setFocus();
    }
    startTransition(Phase::Leaving);
}

void MdDialogHost::layoutDialog()
{
    if (!m_dialog) {
        return;
    }
    // The width clamp is Compose's DialogMinWidth / DialogMaxWidth, applied
    // to the dialog *widget* (container + shadow margins) against the host's
    // rect. The dialog is centred in both axes.
    qreal dialogWidth = MdDialogTokens::kMaxWidth + MdDialogTokens::kShadowMargin * 2.0;
    dialogWidth = qMin(dialogWidth, qreal(width()));
    const int dialogHeight = m_dialog->heightForWidth(int(dialogWidth));
    const QRect resting(int(std::round((qreal(width()) - dialogWidth) / 2.0)),
                        int(std::round((qreal(height()) - dialogHeight) / 2.0)),
                        int(std::round(dialogWidth)), dialogHeight);
    if (m_dialog->geometry() != resting) {
        m_dialog->setGeometry(resting);
    }
}

void MdDialogHost::startTransition(Phase phase)
{
    m_phase = phase;
    m_transitionClock.restart();
    m_transitionTimer->start();
    onTransitionTick();
}

void MdDialogHost::onTransitionTick()
{
    if (m_phase == Phase::Idle) {
        m_transitionTimer->stop();
        return;
    }
    const MdSpring opacitySpring = MdMotion::spring(MotionSpring::EffectsFast);
    const qreal seconds = qreal(m_transitionClock.elapsed()) / 1000.0;
    m_opacity = m_phase == Phase::Entering ? opacitySpring.valueAt(seconds)
                                           : 1.0 - opacitySpring.valueAt(seconds);
    m_opacity = qBound(0.0, m_opacity, 1.0);

    const bool settled = seconds * 1000.0 >= opacitySpring.settlingDurationMs();
    if (settled) {
        m_transitionTimer->stop();
        m_opacity = m_phase == Phase::Entering ? 1.0 : 0.0;
        m_phase = Phase::Idle;
    }

    m_dialog->setPaintOpacity(m_opacity);
    update();
    if (settled && m_opacity <= 0.0) {
        m_dialog->hide();
    }
}

void MdDialogHost::paintEvent(QPaintEvent * /*event*/)
{
    if (!m_dialog->isVisible()) {
        return;
    }
    // The scrim: md.sys.color.scrim, black at 0.32, faded by the transition.
    QColor scrim = MdTheme::instance().color(ColorRole::Scrim);
    scrim.setAlphaF(scrim.alphaF() * MdDialogTokens::kScrimOpacity * m_opacity);
    if (scrim.alpha() <= 0) {
        return;
    }
    QPainter painter(this);
    painter.fillRect(rect(), scrim);
}

void MdDialogHost::mousePressEvent(QMouseEvent *event)
{
    // Click on the scrim: the onDismissRequest flow (Compose's
    // dismissOnClickOutside). Clicks inside the dialog never reach the host.
    if (m_dialog->isVisible() && !m_dialog->geometry().contains(event->pos())) {
        dismiss();
        return;
    }
    QWidget::mousePressEvent(event);
}

bool MdDialogHost::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
        layoutDialog();
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace md
