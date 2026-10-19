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

/// Expressive enter scale start (Compose dialog scale-in floor).
constexpr qreal kEnterScaleStart = 0.8;

} // namespace

MdDialogHost::MdDialogHost(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    // Idle, the host is a transparent overlay: a lazily-created host must
    // never steal clicks from the page underneath. Scrim clicks only apply
    // while the dialog is actually showing.
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
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
    // The scrim is live: take pointer events so outside clicks can dismiss.
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    // Map the overlay first so the scrim first paint and the dialog first
    // paint land in the same frame.
    if (!isVisible()) {
        QWidget::show();
    }
    raise();
    // Geometry before visibility: the dialog must never be seen at a
    // placeholder rect and then jump to its resting centred rect.
    layoutDialog();
    // Opacity/scale start at their closed values before the first paint.
    m_opacity = 0.0;
    m_scale = kEnterScaleStart;
    m_dialog->setPaintOpacity(0.0);
    m_dialog->setPaintScale(kEnterScaleStart);
    m_dialog->setChildrenPaintedByDialog(true);
    m_dialog->show();
    // The dialog (and its action children) must sit above the scrim. One
    // raise is enough; a second one is a redundant restack.
    m_dialog->raise();
    setFocus();
    m_dialog->setFocus();
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
    // Spec s7: the enter/exit is a spring scale + opacity pair. Opacity
    // rides the effects-fast spring, scale the spatial-fast spring.
    const MdSpring opacitySpring = MdMotion::spring(MotionSpring::EffectsFast);
    const MdSpring scaleSpring = MdMotion::spring(MotionSpring::SpatialFast);
    const qreal seconds = qreal(m_transitionClock.elapsed()) / 1000.0;
    const qreal settledMs = qMax(opacitySpring.settlingDurationMs(),
                                 scaleSpring.settlingDurationMs());
    qreal opacity;
    qreal scale;
    if (m_phase == Phase::Entering) {
        opacity = opacitySpring.valueAt(seconds);
        scale = kEnterScaleStart + (1.0 - kEnterScaleStart) * scaleSpring.valueAt(seconds);
    } else {
        opacity = 1.0 - opacitySpring.valueAt(seconds);
        scale = 1.0 - (1.0 - kEnterScaleStart) * scaleSpring.valueAt(seconds);
    }
    m_opacity = qBound(0.0, opacity, 1.0);
    m_scale = qBound<qreal>(kEnterScaleStart, scale, 1.0);

    const bool settled = seconds * 1000.0 >= settledMs;
    if (settled) {
        m_transitionTimer->stop();
        m_opacity = m_phase == Phase::Entering ? 1.0 : 0.0;
        m_scale = m_phase == Phase::Entering ? 1.0 : kEnterScaleStart;
        m_phase = Phase::Idle;
    }

    m_dialog->setPaintOpacity(m_opacity);
    m_dialog->setPaintScale(m_scale);
    if (settled) {
        m_dialog->setChildrenPaintedByDialog(false);
    }
    update();
    if (settled && m_opacity <= 0.0) {
        m_dialog->hide();
        // Idle again: go transparent so the page underneath keeps its clicks.
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
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
    // dismissOnClickOutside). Clicks that land on the dialog — or on any of
    // its children (the action buttons) — must never dismiss.
    if (m_dialog->isVisible()) {
        const QPoint local = event->pos();
        QWidget *hit = childAt(local);
        if (hit == m_dialog || (hit && m_dialog->isAncestorOf(hit))) {
            event->accept();
            return;
        }
        if (m_dialog->geometry().contains(local)) {
            event->accept();
            return;
        }
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
