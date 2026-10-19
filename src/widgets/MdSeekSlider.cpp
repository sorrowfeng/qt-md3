#include "MdSeekSlider.h"

#include "MdSlider.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QResizeEvent>

namespace md {

namespace {

/// Preview chip padding (plain-tooltip body-small, 8 px horizontal).
constexpr int kChipPadX = 8;
constexpr int kChipPadY = 4;
/// Clearance above the track so the chip never sits on the handle.
constexpr int kChipClearance = 4;

} // namespace

MdSeekSlider::MdSeekSlider(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdSeekSlider::~MdSeekSlider() = default;

void MdSeekSlider::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("seek"));
    setMouseTracking(true);

    m_slider = new MdSlider(this);
    m_slider->setAccessibleName(QStringLiteral("seek position"));
    m_slider->installEventFilter(this);

    connect(m_slider, &MdSlider::valueChanged, this, &MdSeekSlider::valueChanged);
    connect(m_slider, &MdSlider::minChanged, this, &MdSeekSlider::minChanged);
    connect(m_slider, &MdSlider::maxChanged, this, &MdSeekSlider::maxChanged);
    connect(m_slider, &MdSlider::stepChanged, this, &MdSeekSlider::stepChanged);
    connect(m_slider, &MdSlider::sliderPressed, this, &MdSeekSlider::sliderPressed);
    connect(m_slider, &MdSlider::sliderReleased, this, &MdSeekSlider::sliderReleased);
    connect(m_slider, &MdSlider::sliderMoved, this, &MdSeekSlider::sliderMoved);

    layoutSlider();
}

qreal MdSeekSlider::value() const
{
    return m_slider ? m_slider->value() : 0.0;
}

void MdSeekSlider::setValue(qreal value)
{
    if (m_slider) {
        m_slider->setValue(value);
    }
}

qreal MdSeekSlider::min() const
{
    return m_slider ? m_slider->min() : 0.0;
}

void MdSeekSlider::setMin(qreal min)
{
    if (m_slider) {
        m_slider->setMin(min);
    }
}

qreal MdSeekSlider::max() const
{
    return m_slider ? m_slider->max() : 100.0;
}

void MdSeekSlider::setMax(qreal max)
{
    if (m_slider) {
        m_slider->setMax(max);
    }
}

qreal MdSeekSlider::step() const
{
    return m_slider ? m_slider->step() : 0.0;
}

void MdSeekSlider::setStep(qreal step)
{
    if (m_slider) {
        m_slider->setStep(step);
    }
}

void MdSeekSlider::setPreviewEnabled(bool enabled)
{
    if (m_previewEnabled == enabled) {
        return;
    }
    m_previewEnabled = enabled;
    if (!m_previewEnabled) {
        hidePreview();
    }
    update();
    emit previewEnabledChanged(m_previewEnabled);
}

bool MdSeekSlider::isLabeled() const
{
    return m_slider ? m_slider->isLabeled() : false;
}

void MdSeekSlider::setLabeled(bool labeled)
{
    if (m_slider) {
        m_slider->setLabeled(labeled);
    }
    emit labeledChanged(labeled);
}

void MdSeekSlider::setSecondsPerUnit(qreal seconds)
{
    if (qFuzzyCompare(m_secondsPerUnit, seconds)) {
        return;
    }
    m_secondsPerUnit = seconds;
    emit secondsPerUnitChanged(m_secondsPerUnit);
}

QString MdSeekSlider::formatTime(qreal seconds)
{
    if (seconds < 0 || !qIsFinite(seconds)) {
        seconds = 0;
    }
    const int total = int(seconds + 0.5);
    const int h = total / 3600;
    const int m = (total % 3600) / 60;
    const int s = total % 60;
    if (h > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(h)
            .arg(m, 2, 10, QLatin1Char('0'))
            .arg(s, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2").arg(m).arg(s, 2, 10, QLatin1Char('0'));
}

QRect MdSeekSlider::previewRect() const
{
    return m_previewVisible ? m_previewRect : QRect();
}

QSize MdSeekSlider::sizeHint() const
{
    const QSize inner = m_slider ? m_slider->sizeHint() : QSize(200, 48);
    return QSize(inner.width(), inner.height() + 20);
}

QSize MdSeekSlider::minimumSizeHint() const
{
    const QSize inner = m_slider ? m_slider->minimumSizeHint() : QSize(80, 48);
    return QSize(inner.width(), inner.height() + 20);
}

void MdSeekSlider::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutSlider();
}

void MdSeekSlider::paintEvent(QPaintEvent *)
{
    if (!m_previewVisible || m_previewRect.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QColor bg = MdTheme::instance().color(ColorRole::InverseSurface);
    const QColor fg = MdTheme::instance().color(ColorRole::InverseOnSurface);
    // `md.comp.plain-tooltip.container.shape` = corner extra-small.
    const qreal radius = 4.0;

    QPainterPath path;
    path.addRoundedRect(QRectF(m_previewRect), radius, radius);
    painter.fillPath(path, bg);

    const QString label = formatTime(m_previewValue * m_secondsPerUnit);
    painter.setPen(fg);
    painter.setFont(MdTypeScale::font(TypeStyle::BodySmall, TypeEmphasis::Baseline,
                                      MdTheme::instance().scriptCategory()));
    painter.drawText(m_previewRect, Qt::AlignCenter, label);
}

bool MdSeekSlider::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_slider) {
        switch (event->type()) {
        case QEvent::MouseMove: {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            updatePreviewFromPos(mousePosition(mouse));
            break;
        }
        case QEvent::Leave:
            hidePreview();
            break;
        case QEvent::MouseButtonPress: {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            updatePreviewFromPos(mousePosition(mouse));
            break;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void MdSeekSlider::layoutSlider()
{
    if (!m_slider) {
        return;
    }
    // Reserve 20 px above the slider for the preview chip.
    m_slider->setGeometry(0, 20, width(), qMax(0, height() - 20));
}

void MdSeekSlider::updatePreviewFromPos(const QPointF &pos)
{
    if (!m_previewEnabled || !m_slider) {
        return;
    }
    const QRectF track = m_slider->trackRect();
    if (track.width() <= 0) {
        return;
    }
    const qreal t = qBound(0.0, (pos.x() - track.left()) / track.width(), 1.0);
    const qreal lo = m_slider->min();
    const qreal hi = m_slider->max();
    const qreal raw = lo + t * (hi - lo);
    const qreal step = m_slider->step();
    const qreal snapped = step > 0 ? lo + qRound((raw - lo) / step) * step : raw;

    m_previewValue = snapped;
    m_previewVisible = true;

    const QString label = formatTime(snapped * m_secondsPerUnit);
    const QFont font = MdTypeScale::font(TypeStyle::BodySmall, TypeEmphasis::Baseline,
                                         MdTheme::instance().scriptCategory());
    const QFontMetrics fm(font);
    const int w = fm.horizontalAdvance(label) + 2 * kChipPadX;
    const int h = fm.height() + 2 * kChipPadY;
    const int cx = int(pos.x()) - w / 2;
    const int cy = 20 - h - kChipClearance;
    m_previewRect = QRect(qBound(0, cx, qMax(0, width() - w)), qMax(0, cy), w, h);

    emit previewChanged(snapped);
    update();
}

void MdSeekSlider::hidePreview()
{
    if (!m_previewVisible) {
        return;
    }
    m_previewVisible = false;
    m_previewRect = QRect();
    update();
}

} // namespace md
