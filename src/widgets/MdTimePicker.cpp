#include "MdTimePicker.h"

#include "styles/MdTimePickerStyle.h"

#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtMath>

namespace md {

MdTimePicker::MdTimePicker(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

MdTimePicker::~MdTimePicker() = default;

void MdTimePicker::setTime(const QTime &time)
{
    if (!time.isValid() || time == m_time) {
        return;
    }
    m_time = time;
    emit timeChanged(time);
    update();
}

void MdTimePicker::set24h(bool is24h)
{
    if (is24h == m_24h) {
        return;
    }
    m_24h = is24h;
    emit is24hChanged(is24h);
    update();
}

void MdTimePicker::setFace(MdTimePickerFace face)
{
    if (face == m_face) {
        return;
    }
    m_face = face;
    emit faceChanged(face);
    update();
}

MdTimePeriod MdTimePicker::period() const
{
    return m_time.hour() >= 12 ? MdTimePeriod::Pm : MdTimePeriod::Am;
}

void MdTimePicker::setPeriod(MdTimePeriod period)
{
    if (period == this->period()) {
        return;
    }
    int hour = m_time.hour();
    if (period == MdTimePeriod::Pm && hour < 12) {
        hour += 12;
    } else if (period == MdTimePeriod::Am && hour >= 12) {
        hour -= 12;
    }
    setTime(QTime(hour, m_time.minute()));
    emit periodChanged(period);
}

// --- geometry -------------------------------------------------------------

QRectF MdTimePicker::dialRect() const
{
    const MdTimePickerTokens &tokens = timePickerTokens();
    const qreal size = tokens.dialSize;
    return QRectF((rect().width() - size) / 2.0, rect().height() - size - 16.0, size, size);
}

QRectF MdTimePicker::selectorHandleRect() const
{
    const MdTimePickerTokens &tokens = timePickerTokens();
    const QRectF dial = dialRect();
    const qreal angle = handleAngle();
    const qreal radius = dial.width() / 2.0 - tokens.selectorHandleSize / 2.0 - 8.0;
    const QPointF centre = dial.center() + QPointF(qCos(angle) * radius, qSin(angle) * radius);
    const qreal handle = tokens.selectorHandleSize;
    return QRectF(centre.x() - handle / 2.0, centre.y() - handle / 2.0, handle, handle);
}

QRectF MdTimePicker::timeSelectorRect(bool start) const
{
    const MdTimePickerTokens &tokens = timePickerTokens();
    // Centre the *whole* row — hours, gap, minutes, and (in 12h mode) the
    // AM/PM column — so the period pill cannot run past the dialog edge.
    const qreal period = m_24h ? 0.0 : (24.0 + tokens.periodVerticalWidth);
    const qreal total = m_24h ? tokens.timeSelector24hWidth
                              : 2.0 * tokens.timeSelectorWidth + 24.0 + period;
    const qreal left = (rect().width() - total) / 2.0;
    // Below the "Select time" headline (label-medium at y+12, 20 tall).
    const qreal y = 44.0;
    if (m_24h) {
        return QRectF(left, y, tokens.timeSelector24hWidth, tokens.timeSelectorHeight);
    }
    return start ? QRectF(left, y, tokens.timeSelectorWidth, tokens.timeSelectorHeight)
                 : QRectF(left + tokens.timeSelectorWidth + 24.0, y, tokens.timeSelectorWidth,
                          tokens.timeSelectorHeight);
}

QRectF MdTimePicker::periodSelectorRect(bool am) const
{
    const MdTimePickerTokens &tokens = timePickerTokens();
    // [hours 24 gap] [minutes 24 gap] [period] — the period sits past the
    // minutes field, not just past the hours field (the first revision added
    // only the gaps and parked AM/PM on top of the minute digits).
    //
    // The export's *vertical* slot is 52×80 (AM above PM). The first
    // revision sliced the horizontal 216×38 in half and stacked that, which
    // needed 348 px of row and overflowed the dialog.
    const QRectF minutes = timeSelectorRect(false);
    const qreal x = minutes.right() + 24.0;
    const qreal w = tokens.periodVerticalWidth;
    const qreal h = tokens.periodVerticalHeight / 2.0;
    return am ? QRectF(x, minutes.top(), w, h)
              : QRectF(x, minutes.top() + h, w, h);
}

QSize MdTimePicker::sizeHint() const
{
    const MdTimePickerTokens &tokens = timePickerTokens();
    return QSize(int(tokens.dialSize + 32.0),
                 int(12.0 + 20.0 + 12.0 + tokens.timeSelectorHeight + 16.0 + tokens.dialSize
                     + 16.0));
}

QSize MdTimePicker::minimumSizeHint() const
{
    return sizeHint();
}

// --- tokens -------------------------------------------------------------------

const MdTimePickerTokens &MdTimePicker::timePickerTokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdTimePickerTokens::resolve();
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdTimePicker::setTimePickerTokens(const MdTimePickerTokens &tokens)
{
    m_tokens = tokens;
    m_tokensDirty = false;
    update();
}

// --- dial maths ---------------------------------------------------------------

qreal MdTimePicker::handleAngle() const
{
    // 12 o'clock is -90°; the dial runs clockwise.
    if (m_face == MdTimePickerFace::Hours) {
        const int hour = m_24h ? m_time.hour() : (m_time.hour() % 12 == 0 ? 12 : m_time.hour() % 12);
        return qDegreesToRadians(-90.0 + 360.0 * hour / (m_24h ? 24 : 12));
    }
    return qDegreesToRadians(-90.0 + 360.0 * m_time.minute() / 60.0);
}

void MdTimePicker::setHour(int hour)
{
    const int minute = m_time.minute();
    if (m_24h) {
        setTime(QTime(qBound(0, hour, 23), minute));
    } else {
        const bool pm = period() == MdTimePeriod::Pm;
        int h = hour % 12;
        if (pm) {
            h += 12;
        }
        setTime(QTime(h, minute));
    }
}

void MdTimePicker::setMinute(int minute)
{
    setTime(QTime(m_time.hour(), qBound(0, minute, 59)));
}

void MdTimePicker::applyDialPosition(const QPointF &pos)
{
    const QRectF dial = dialRect();
    const QPointF delta = pos - dial.center();
    qreal angle = qAtan2(delta.y(), delta.x()) + qDegreesToRadians(90.0);
    if (angle < 0.0) {
        angle += 2.0 * M_PI;
    }
    const qreal turns = angle / (2.0 * M_PI);

    if (m_face == MdTimePickerFace::Hours) {
        const int slotCount = m_24h ? 24 : 12;
        int hour = int(turns * slotCount + 0.5) % slotCount;
        if (!m_24h && hour == 0) {
            hour = 12;
        }
        setHour(hour);
    } else {
        // The minutes face snaps to 5-minute slots (the M3 contract).
        const int slot = int(turns * 12.0 + 0.5) % 12;
        setMinute(slot * 5);
    }
}

// --- events ---------------------------------------------------------------------

void MdTimePicker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    MdTimePickerStyle::paintTimePicker(painter, *this, timePickerTokens());
}

void MdTimePicker::mousePressEvent(QMouseEvent *event)
{
    if (isEffectivelyDisabled()) {
        return;
    }
    const QPointF pos = event->position();

    // Mode switching: the time selectors and the period selector take clicks.
    if (timeSelectorRect(true).contains(pos) && m_face != MdTimePickerFace::Hours) {
        setFace(MdTimePickerFace::Hours);
        return;
    }
    if (!m_24h && timeSelectorRect(false).contains(pos) && m_face != MdTimePickerFace::Minutes) {
        setFace(MdTimePickerFace::Minutes);
        return;
    }
    if (!m_24h && periodSelectorRect(true).contains(pos)) {
        setPeriod(MdTimePeriod::Am);
        return;
    }
    if (!m_24h && periodSelectorRect(false).contains(pos)) {
        setPeriod(MdTimePeriod::Pm);
        return;
    }

    if (dialRect().contains(pos)) {
        m_dragging = true;
        applyDialPosition(pos);
    }
}

void MdTimePicker::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging || isEffectivelyDisabled()) {
        return;
    }
    applyDialPosition(event->position());
}

void MdTimePicker::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    m_dragging = false;
}

void MdTimePicker::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    m_hovered = true;
    update();
}

void MdTimePicker::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdTimePicker::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    m_focusIsKeyboard = event->reason() != Qt::MouseFocusReason
        && event->reason() != Qt::PopupFocusReason;
    update();
}

void MdTimePicker::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    m_focusIsKeyboard = false;
    update();
}

void MdTimePicker::keyPressEvent(QKeyEvent *event)
{
    if (isEffectivelyDisabled()) {
        QWidget::keyPressEvent(event);
        return;
    }
    const bool hours = m_face == MdTimePickerFace::Hours;
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Down:
        if (hours) {
            setHour(m_time.hour() - 1);
        } else {
            setMinute(m_time.minute() - 5);
        }
        break;
    case Qt::Key_Right:
    case Qt::Key_Up:
        if (hours) {
            setHour(m_time.hour() + 1);
        } else {
            setMinute(m_time.minute() + 5);
        }
        break;
    case Qt::Key_Tab:
        setFace(hours ? MdTimePickerFace::Minutes : MdTimePickerFace::Hours);
        break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    update();
}

} // namespace md
