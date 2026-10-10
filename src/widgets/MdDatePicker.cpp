#include "MdDatePicker.h"

#include "styles/MdDatePickerStyle.h"

#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

namespace md {

MdDatePicker::MdDatePicker(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_selected = QDate::currentDate();
}

MdDatePicker::~MdDatePicker() = default;

void MdDatePicker::setSelectedDate(const QDate &date)
{
    if (!date.isValid() || date == m_selected) {
        return;
    }
    m_selected = date;
    emit selectedDateChanged(date);
    update();
}

void MdDatePicker::setDisplayedMonth(const QDate &month)
{
    if (!month.isValid() || month == m_displayed) {
        return;
    }
    m_displayed = month;
    emit displayedMonthChanged(month);
    update();
}

void MdDatePicker::setRange(bool range)
{
    if (range == m_range) {
        return;
    }
    m_range = range;
    emit rangeChanged(range);
    update();
}

void MdDatePicker::setRangeStart(const QDate &date)
{
    if (date == m_rangeStart) {
        return;
    }
    m_rangeStart = date;
    emit rangeStartChanged(date);
    update();
}

void MdDatePicker::setRangeEnd(const QDate &date)
{
    if (date == m_rangeEnd) {
        return;
    }
    m_rangeEnd = date;
    emit rangeEndChanged(date);
    update();
}

void MdDatePicker::setFace(MdDatePickerFace face)
{
    if (face == m_face) {
        return;
    }
    m_face = face;
    emit faceChanged(face);
    update();
}

// --- geometry ---------------------------------------------------------------

QRectF MdDatePicker::headerRect() const
{
    const MdDatePickerTokens &tokens = datePickerTokens();
    return QRectF(8.0, 8.0, tokens.containerWidth - 16.0, tokens.headerHeight);
}

QRectF MdDatePicker::dateCellRect(int row, int col) const
{
    const MdDatePickerTokens &tokens = datePickerTokens();
    const qreal gridTop = headerRect().bottom() + 24.0 + 24.0; // weekday row + month subhead
    const qreal cell = tokens.dateCellSize;
    const qreal gap = 4.0;
    const qreal gridWidth = 7.0 * cell + 6.0 * gap;
    const qreal left = 8.0 + (tokens.containerWidth - 16.0 - gridWidth) / 2.0;
    return QRectF(left + col * (cell + gap), gridTop + row * (cell + gap), cell, cell);
}

QRectF MdDatePicker::yearCellRect(int index) const
{
    const MdDatePickerTokens &tokens = datePickerTokens();
    const int row = index / 3;
    const int col = index % 3;
    const qreal top = headerRect().bottom() + 16.0;
    return QRectF(8.0 + col * (tokens.yearWidth + 8.0),
                  top + row * (tokens.yearHeight + 8.0), tokens.yearWidth, tokens.yearHeight);
}

QSize MdDatePicker::sizeHint() const
{
    const MdDatePickerTokens &tokens = datePickerTokens();
    return QSize(int(tokens.containerWidth), int(tokens.containerHeight));
}

QSize MdDatePicker::minimumSizeHint() const
{
    return sizeHint();
}

// --- tokens -------------------------------------------------------------------

const MdDatePickerTokens &MdDatePicker::datePickerTokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdDatePickerTokens::resolve();
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdDatePicker::setDatePickerTokens(const MdDatePickerTokens &tokens)
{
    m_tokens = tokens;
    m_tokensDirty = false;
    update();
}

// --- grid maths -----------------------------------------------------------------

QDate MdDatePicker::dateAtCell(int row, int col) const
{
    const QDate first(m_displayed.year(), m_displayed.month(), 1);
    // Monday-first grid (the M3 weekday row starts on Monday).
    const int lead = (first.dayOfWeek() + 6) % 7; // Mon=0 .. Sun=6
    const int day = row * 7 + col - lead + 1;
    if (day < 1 || day > m_displayed.daysInMonth()) {
        return QDate();
    }
    return QDate(m_displayed.year(), m_displayed.month(), day);
}

int MdDatePicker::weeksInMonth() const
{
    const QDate first(m_displayed.year(), m_displayed.month(), 1);
    const int lead = (first.dayOfWeek() + 6) % 7;
    return (lead + m_displayed.daysInMonth() + 6) / 7;
}

// --- events -----------------------------------------------------------------------

void MdDatePicker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    MdDatePickerStyle::paintDatePicker(painter, *this, datePickerTokens());
}

void MdDatePicker::mousePressEvent(QMouseEvent *event)
{
    if (isEffectivelyDisabled()) {
        return;
    }
    const QPointF pos = event->position();

    // The header's supporting text switches to the year face.
    if (m_face == MdDatePickerFace::Calendar && headerRect().adjusted(0.0, 40.0, 0.0, 0.0).contains(pos)) {
        setFace(MdDatePickerFace::Years);
        return;
    }
    if (m_face == MdDatePickerFace::Years) {
        for (int i = 0; i < 12; ++i) {
            if (yearCellRect(i).contains(pos)) {
                const int year = m_displayed.year() - 6 + i;
                setDisplayedMonth(QDate(year, m_displayed.month(), 1));
                setFace(MdDatePickerFace::Calendar);
                return;
            }
        }
        return;
    }

    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            const QRectF cell = dateCellRect(row, col);
            if (!cell.contains(pos)) {
                continue;
            }
            const QDate date = dateAtCell(row, col);
            if (!date.isValid()) {
                return;
            }
            if (m_range) {
                if (!m_rangeStart.isValid() || m_rangeEnd.isValid()) {
                    setRangeStart(date);
                    setRangeEnd(QDate());
                } else if (date < m_rangeStart) {
                    setRangeEnd(m_rangeStart);
                    setRangeStart(date);
                } else {
                    setRangeEnd(date);
                }
            }
            setSelectedDate(date);
            return;
        }
    }
}

void MdDatePicker::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    m_hovered = true;
    update();
}

void MdDatePicker::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdDatePicker::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    m_focusIsKeyboard = event->reason() != Qt::MouseFocusReason
        && event->reason() != Qt::PopupFocusReason;
    update();
}

void MdDatePicker::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    m_focusIsKeyboard = false;
    update();
}

void MdDatePicker::keyPressEvent(QKeyEvent *event)
{
    if (isEffectivelyDisabled()) {
        QWidget::keyPressEvent(event);
        return;
    }
    switch (event->key()) {
    case Qt::Key_Left:
        setSelectedDate(m_selected.addDays(-1));
        break;
    case Qt::Key_Right:
        setSelectedDate(m_selected.addDays(1));
        break;
    case Qt::Key_Up:
        setSelectedDate(m_selected.addDays(-7));
        break;
    case Qt::Key_Down:
        setSelectedDate(m_selected.addDays(7));
        break;
    case Qt::Key_PageUp:
        setDisplayedMonth(m_displayed.addMonths(-1));
        break;
    case Qt::Key_PageDown:
        setDisplayedMonth(m_displayed.addMonths(1));
        break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    update();
}

} // namespace md
