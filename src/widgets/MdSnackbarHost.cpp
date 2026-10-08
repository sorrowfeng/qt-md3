#include "MdSnackbarHost.h"

#include "MdSnackbar.h"

#include "core/MdMotion.h"

#include <QtCore/QTimer>
#include <QtWidgets/QWidget>

#include <cmath>
#include <QtCore/QEvent>
#include <QtWidgets/QWidget>

namespace md {

namespace {

/// The transition tick granularity (~60 fps, the same cadence the other
/// animation cycles use).
constexpr int kTransitionTickMs = 16;

/// The scale value Compose's FadeInFadeOutWithScale starts from.
constexpr qreal kEnterScaleStart = 0.8;

} // namespace

MdSnackbarHost::MdSnackbarHost(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    if (parent) {
        parent->installEventFilter(this);
    }
    m_durationTimer = new QTimer(this);
    m_durationTimer->setSingleShot(true);
    connect(m_durationTimer, &QTimer::timeout, this, &MdSnackbarHost::onTimeout);
    m_transitionTimer = new QTimer(this);
    m_transitionTimer->setInterval(kTransitionTickMs);
    connect(m_transitionTimer, &QTimer::timeout, this, &MdSnackbarHost::onTransitionTick);
}

MdSnackbarHost::~MdSnackbarHost() = default;

void MdSnackbarHost::showSnackbar(const QString &message,
                                  const QString &actionLabel,
                                  MdSnackbarDuration duration)
{
    // Compose de-dupes only the current snackbar; queued duplicates are this
    // port's own queue back-pressure, recorded in docs/porting-todo.md.
    if (m_current && m_current->message() == message
        && m_current->actionLabel() == actionLabel) {
        return;
    }
    m_queue.append({message, actionLabel, duration});
    if (!m_current) {
        showNext();
    }
}

void MdSnackbarHost::dismissCurrent()
{
    if (!m_current) {
        return;
    }
    emit dismissed();
    startTransition(Phase::Leaving);
}

bool MdSnackbarHost::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize) {
        setGeometry(parentWidget()->rect());
        layoutCurrent();
    }
    return QWidget::eventFilter(watched, event);
}

void MdSnackbarHost::onTimeout()
{
    // The duration ran out without user interaction.
    startTransition(Phase::Leaving);
}

void MdSnackbarHost::showNext()
{
    if (m_queue.isEmpty()) {
        m_current = nullptr;
        emit allCleared();
        return;
    }
    const Request request = m_queue.takeFirst();
    m_current = new MdSnackbar(request.message, this);
    m_current->setActionLabel(request.actionLabel);
    connect(m_current, &MdSnackbar::actionClicked, this, [this] {
        emit actionClicked();
        startTransition(Phase::Leaving);
    });
    connect(m_current, &MdSnackbar::dismissClicked, this, [this] {
        emit dismissed();
        startTransition(Phase::Leaving);
    });
    layoutCurrent();
    m_current->show();
    raise();

    const qint64 ms = MdSnackbarTokens::durationMs(request.duration,
                                                   m_current->hasAction());
    if (ms >= 0) {
        m_durationTimer->start(int(ms));
    }

    m_opacity = 0.0;
    m_scale = kEnterScaleStart;
    startTransition(Phase::Entering);
}

void MdSnackbarHost::layoutCurrent()
{
    if (!m_current) {
        return;
    }
    // The widget rect carries the shadow margin around the container, so the
    // 600 px cap applies to the container: widget = container + 2 margins.
    const qreal available = width() - MdSnackbarTokens::kHostMargin * 2.0;
    const qreal maxWidgetWidth = MdSnackbarTokens::kContainerMaxWidth
        + MdSnackbarTokens::kShadowMargin * 2.0;
    const qreal widgetWidth = qMin(available, maxWidgetWidth);
    const int widgetHeight = m_current->heightForWidth(int(widgetWidth));
    const qreal x = (width() - widgetWidth) / 2.0;
    const qreal y = height() - widgetHeight - MdSnackbarTokens::kHostMargin;
    m_restingGeometry =
        QRect(int(x), int(y), int(std::ceil(widgetWidth)), widgetHeight);
    m_current->setGeometry(m_restingGeometry);
}

void MdSnackbarHost::startTransition(Phase phase)
{
    if (phase == Phase::Leaving) {
        m_durationTimer->stop();
    }
    m_phase = phase;
    m_transitionClock.restart();
    m_transitionTimer->start();
    onTransitionTick();
}

void MdSnackbarHost::onTransitionTick()
{
    if (m_phase == Phase::Idle) {
        m_transitionTimer->stop();
        return;
    }
    const MdSpring opacitySpring = MdMotion::spring(MotionSpring::EffectsFast);
    const MdSpring scaleSpring = MdMotion::spring(MotionSpring::SpatialFast);
    const qreal seconds = qreal(m_transitionClock.elapsed()) / 1000.0;
    const qreal settled = qMax(opacitySpring.settlingDurationMs(),
                               scaleSpring.settlingDurationMs());
    qreal opacity;
    qreal scale;
    if (m_phase == Phase::Entering) {
        opacity = opacitySpring.valueAt(seconds);
        scale = kEnterScaleStart
            + (1.0 - kEnterScaleStart) * scaleSpring.valueAt(seconds);
    } else {
        opacity = 1.0 - opacitySpring.valueAt(seconds);
        scale = 1.0 - (1.0 - kEnterScaleStart) * scaleSpring.valueAt(seconds);
    }
    m_opacity = qBound(0.0, opacity, 1.0);
    m_scale = qBound<qreal>(kEnterScaleStart, scale, 1.0);

    if (seconds * 1000.0 >= settled) {
        m_transitionTimer->stop();
        m_opacity = m_phase == Phase::Entering ? 1.0 : 0.0;
        m_scale = m_phase == Phase::Entering ? 1.0 : kEnterScaleStart;
        if (m_phase == Phase::Leaving) {
            finishCurrent();
        }
        m_phase = Phase::Idle;
    }

    if (m_current) {
        // A child widget has no window opacity; the snackbar's paintOpacity
        // is applied inside the style, and the scale animates the geometry
        // around the resting rect.
        m_current->setPaintOpacity(m_opacity);
        const QPointF center = m_restingGeometry.center();
        QRect scaled = m_restingGeometry;
        scaled.setSize(QSize(int(std::round(m_restingGeometry.width() * m_scale)),
                             int(std::round(m_restingGeometry.height() * m_scale))));
        scaled.moveCenter(center.toPoint());
        m_current->setGeometry(scaled);
    }
}

void MdSnackbarHost::finishCurrent()
{
    if (!m_current) {
        return;
    }
    m_current->deleteLater();
    m_current = nullptr;
    showNext();
}

} // namespace md
