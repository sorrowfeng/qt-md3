#include "MdSplitter.h"

#include "core/MdMotion.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtCore/QEasingCurve>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QSplitterHandle>

namespace md {

namespace {

/// Divider token thickness — always 1 px, device-pixel aligned.
constexpr qreal kHairline = 1.0;

/// Hover-bar thickness (the grab pill).
constexpr qreal kBarThickness = 3.0;

/// Reveal ramp step. The handle fades in over a short-2 style interval
/// without needing a full animation timeline on every splitter.
constexpr int kRevealTickMs = 16;
constexpr qreal kRevealStep = 0.20;

} // namespace

/// The handle: paints the resting hairline and, on hover/drag, the rounded
/// grab bar. QSplitter positions it; we only decorate it.
class MdSplitterHandle : public QSplitterHandle
{
public:
    explicit MdSplitterHandle(Qt::Orientation orientation, MdSplitter *splitter)
        : QSplitterHandle(orientation, splitter)
        , m_splitter(splitter)
    {
        setAttribute(Qt::WA_OpaquePaintEvent, false);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        if (!m_splitter) {
            return;
        }
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QColor line = m_splitter->resolvedDividerColor();
        const QRectF r = QRectF(rect());
        const bool horizontal = (orientation() == Qt::Horizontal);

        // Resting hairline, centred on the handle box and snapped to the
        // device grid so 1 px stays a crisp single pixel.
        painter.setPen(Qt::NoPen);
        painter.setBrush(line);
        if (horizontal) {
            const qreal x = qRound(r.center().x() - kHairline * 0.5);
            painter.drawRect(QRectF(x, r.top(), kHairline, r.height()));
        } else {
            const qreal y = qRound(r.center().y() - kHairline * 0.5);
            painter.drawRect(QRectF(r.left(), y, r.width(), kHairline));
        }

        // Hover / drag grab bar.
        const qreal reveal = m_splitter->handleReveal();
        if (reveal <= 0.0) {
            return;
        }
        QColor bar = m_splitter->resolvedHandleColor();
        bar.setAlphaF(bar.alphaF() * reveal);
        painter.setBrush(bar);

        const qreal length = qMin<qreal>(m_splitter->handleBarLength(),
                                        horizontal ? r.height() : r.width());
        const qreal thickness = kBarThickness;
        if (horizontal) {
            const qreal x = r.center().x() - thickness * 0.5;
            const qreal y = r.center().y() - length * 0.5;
            const QRectF barRect(x, y, thickness, length);
            const qreal radius = thickness * 0.5;
            painter.drawRoundedRect(barRect, radius, radius);
        } else {
            const qreal y = r.center().y() - thickness * 0.5;
            const qreal x = r.center().x() - length * 0.5;
            const QRectF barRect(x, y, length, thickness);
            const qreal radius = thickness * 0.5;
            painter.drawRoundedRect(barRect, radius, radius);
        }
    }

    void enterEvent(md::MdEnterEvent *event) override
    {
        if (m_splitter) {
            m_splitter->m_handleActive = true;
            m_splitter->tickHandleReveal();
        }
        QSplitterHandle::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override
    {
        if (m_splitter) {
            m_splitter->m_handleActive = false;
            m_splitter->tickHandleReveal();
        }
        QSplitterHandle::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (m_splitter) {
            m_splitter->m_handleActive = true;
            m_splitter->tickHandleReveal();
        }
        QSplitterHandle::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        QSplitterHandle::mouseReleaseEvent(event);
        if (m_splitter && !underMouse()) {
            m_splitter->m_handleActive = false;
            m_splitter->tickHandleReveal();
        }
    }

private:
    MdSplitter *m_splitter = nullptr;
};

MdSplitter::MdSplitter(QWidget *parent)
    : QSplitter(parent)
{
    init();
}

MdSplitter::MdSplitter(Qt::Orientation orientation, QWidget *parent)
    : QSplitter(orientation, parent)
{
    init();
}

MdSplitter::~MdSplitter() = default;

void MdSplitter::init()
{
    setChildrenCollapsible(false);
    setHandleWidth(m_handleWidth);
    setFocusPolicy(Qt::NoFocus);
    MdStyleBase::connectThemeUpdate(this, &MdSplitter::onThemeChanged);
}

void MdSplitter::setOrientation(Qt::Orientation orientation)
{
    if (QSplitter::orientation() == orientation) {
        return;
    }
    QSplitter::setOrientation(orientation);
    emit orientationChanged(orientation);
    update();
}

void MdSplitter::setHandleWidth(int width)
{
    const int clamped = qBound(4, width, 32);
    if (m_handleWidth == clamped) {
        return;
    }
    m_handleWidth = clamped;
    QSplitter::setHandleWidth(m_handleWidth);
    update();
    emit handleWidthChanged(m_handleWidth);
}

void MdSplitter::setHandleBarLength(int length)
{
    const int clamped = qBound(8, length, 128);
    if (m_handleBarLength == clamped) {
        return;
    }
    m_handleBarLength = clamped;
    update();
    emit handleBarLengthChanged(m_handleBarLength);
}

void MdSplitter::setDividerColor(const QColor &color)
{
    if (m_dividerColor == color) {
        return;
    }
    m_dividerColor = color;
    update();
    emit dividerColorChanged(m_dividerColor);
}

QColor MdSplitter::resolvedDividerColor() const
{
    return m_dividerColor.isValid() ? m_dividerColor
                                    : MdTheme::instance().color(ColorRole::OutlineVariant);
}

QColor MdSplitter::resolvedHandleColor() const
{
    return MdTheme::instance().color(ColorRole::Outline);
}

QSplitterHandle *MdSplitter::createHandle()
{
    return new MdSplitterHandle(orientation(), this);
}

void MdSplitter::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LayoutDirectionChange) {
        update();
    }
    QSplitter::changeEvent(event);
}

void MdSplitter::onThemeChanged()
{
    update();
}

void MdSplitter::setHandleReveal(qreal reveal)
{
    const qreal clamped = qBound(0.0, reveal, 1.0);
    if (qFuzzyCompare(m_handleReveal, clamped)) {
        return;
    }
    m_handleReveal = clamped;
    update();
    emit handleRevealChanged(m_handleReveal);
}

void MdSplitter::tickHandleReveal()
{
    // Short ramp toward the active/inactive target. One-shot timers avoid a
    // permanent animation clock on every splitter in a layout.
    const qreal target = m_handleActive ? 1.0 : 0.0;
    if (qFuzzyCompare(m_handleReveal, target)) {
        return;
    }
    auto *timer = findChild<QTimer *>(QStringLiteral("md-splitter-reveal"),
                                     Qt::FindDirectChildrenOnly);
    if (timer == nullptr) {
        timer = new QTimer(this);
        timer->setObjectName(QStringLiteral("md-splitter-reveal"));
        timer->setInterval(kRevealTickMs);
        connect(timer, &QTimer::timeout, this, [this, timer]() {
            const qreal goal = m_handleActive ? 1.0 : 0.0;
            if (m_handleActive) {
                setHandleReveal(qMin(goal, m_handleReveal + kRevealStep));
            } else {
                setHandleReveal(qMax(goal, m_handleReveal - kRevealStep));
            }
            if (qFuzzyCompare(m_handleReveal, goal)) {
                timer->stop();
            }
        });
    }
    if (!timer->isActive()) {
        timer->start();
    }
}

} // namespace md
