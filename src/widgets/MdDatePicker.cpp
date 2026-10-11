#include "MdDatePicker.h"

#include "styles/MdDatePickerStyle.h"

#include "widgets/MdMenu.h"

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

void MdDatePicker::setVariant(MdDatePickerVariant variant)
{
    if (variant == m_variant) {
        return;
    }
    m_variant = variant;
    // Docked has no year face — the year is a menu button.
    if (variant == MdDatePickerVariant::Docked && m_face == MdDatePickerFace::Years) {
        setFace(MdDatePickerFace::Calendar);
    }
    updateGeometry();
    update();
    emit variantChanged(m_variant);
}

// --- geometry ---------------------------------------------------------------

QRectF MdDatePicker::headerRect() const
{
    if (m_variant == MdDatePickerVariant::Docked) {
        const MdDatePickerDockedTokens &tokens = datePickerDockedTokens();
        return QRectF(0.0, 0.0, tokens.containerWidth, tokens.headerHeight);
    }
    const MdDatePickerTokens &tokens = datePickerTokens();
    return QRectF(8.0, 8.0, tokens.containerWidth - 16.0, tokens.headerHeight);
}

QRectF MdDatePicker::dateCellRect(int row, int col) const
{
    if (m_variant == MdDatePickerVariant::Docked) {
        const MdDatePickerDockedTokens &tokens = datePickerDockedTokens();
        // The export publishes no grid gap row: the 48 px cell is the slot
        // and the 40 px state layer is the pill centred inside it.
        const qreal cellW = tokens.dateCellWidth;
        const qreal cellH = tokens.dateCellHeight;
        const qreal gridTop = headerRect().bottom() + 32.0; // weekday row band
        const qreal gridWidth = 7.0 * cellW;
        const qreal left = (tokens.containerWidth - gridWidth) / 2.0;
        return QRectF(left + col * cellW, gridTop + row * cellH, cellW, cellH);
    }
    const MdDatePickerTokens &tokens = datePickerTokens();
    const qreal gridTop = headerRect().bottom() + 24.0 + 24.0; // weekday row + month subhead
    const qreal cell = tokens.dateCellSize;
    const qreal gap = 4.0;
    const qreal gridWidth = 7.0 * cell + 6.0 * gap;
    const qreal left = 8.0 + (tokens.containerWidth - 16.0 - gridWidth) / 2.0;
    return QRectF(left + col * (cell + gap), gridTop + row * (cell + gap), cell, cell);
}

QRectF MdDatePicker::dateStateLayerRect(int row, int col) const
{
    const QRectF cell = dateCellRect(row, col);
    if (m_variant == MdDatePickerVariant::Docked) {
        const MdDatePickerDockedTokens &tokens = datePickerDockedTokens();
        const qreal w = tokens.dateStateLayerWidth;
        const qreal h = tokens.dateStateLayerHeight;
        return QRectF(cell.left() + (cell.width() - w) / 2.0,
                      cell.top() + (cell.height() - h) / 2.0, w, h);
    }
    return cell;
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

QRectF MdDatePicker::monthMenuButtonRect() const
{
    const MdDatePickerDockedTokens &tokens = datePickerDockedTokens();
    // Two 40 px pills in the 64 px header, 12 px apart, 16 px side insets.
    const qreal y = (tokens.headerHeight - tokens.menuButtonHeight) / 2.0;
    return QRectF(16.0, y, 140.0, tokens.menuButtonHeight);
}

QRectF MdDatePicker::yearMenuButtonRect() const
{
    const MdDatePickerDockedTokens &tokens = datePickerDockedTokens();
    const qreal y = (tokens.headerHeight - tokens.menuButtonHeight) / 2.0;
    return QRectF(16.0 + 140.0 + 12.0, y, 100.0, tokens.menuButtonHeight);
}

QSize MdDatePicker::sizeHint() const
{
    if (m_variant == MdDatePickerVariant::Docked) {
        const MdDatePickerDockedTokens &tokens = datePickerDockedTokens();
        return QSize(int(tokens.containerWidth), int(tokens.containerHeight));
    }
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

const MdDatePickerDockedTokens &MdDatePicker::datePickerDockedTokens() const
{
    if (m_dockedTokensDirty) {
        m_dockedTokens = MdDatePickerDockedTokens::resolve();
        m_dockedTokensDirty = false;
    }
    return m_dockedTokens;
}

void MdDatePicker::setDatePickerDockedTokens(const MdDatePickerDockedTokens &tokens)
{
    m_dockedTokens = tokens;
    m_dockedTokensDirty = false;
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
    const QPointF pos = mousePosition(event);

    // Docked: the header's two menu buttons. The dropdown reuses the classic
    // `MdMenu` surface (recorded divergence — the docked `menu.list-item.*`
    // rows have no dedicated widget).
    if (m_variant == MdDatePickerVariant::Docked) {
        if (monthMenuButtonRect().contains(pos)) {
            auto *menu = new MdMenu(nullptr);
            menu->setAttribute(Qt::WA_DeleteOnClose);
            for (int month = 1; month <= 12; ++month) {
                const QString name =
                    QDate(2000, month, 1).toString(QStringLiteral("MMMM"));
                MdMenuItem *item = menu->addItem(name);
                item->setSelected(month == m_displayed.month());
                connect(item, &QAbstractButton::clicked, menu, [this, month, menu] {
                    setDisplayedMonth(QDate(m_displayed.year(), month, 1));
                    menu->close();
                });
            }
            menu->popup(QRect(mapToGlobal(monthMenuButtonRect().topLeft().toPoint()),
                              monthMenuButtonRect().size().toSize()));
            return;
        }
        if (yearMenuButtonRect().contains(pos)) {
            auto *menu = new MdMenu(nullptr);
            menu->setAttribute(Qt::WA_DeleteOnClose);
            const int baseYear = m_displayed.year();
            for (int offset = -3; offset <= 3; ++offset) {
                const int year = baseYear + offset;
                MdMenuItem *item = menu->addItem(QString::number(year));
                item->setSelected(year == m_displayed.year());
                connect(item, &QAbstractButton::clicked, menu, [this, year, menu] {
                    setDisplayedMonth(QDate(year, m_displayed.month(), 1));
                    menu->close();
                });
            }
            menu->popup(QRect(mapToGlobal(yearMenuButtonRect().topLeft().toPoint()),
                              yearMenuButtonRect().size().toSize()));
            return;
        }
    }

    // The header's supporting text switches to the year face (modal only —
    // docked has no year face).
    if (m_variant == MdDatePickerVariant::Modal && m_face == MdDatePickerFace::Calendar
        && headerRect().adjusted(0.0, 40.0, 0.0, 0.0).contains(pos)) {
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

void MdDatePicker::enterEvent(MdEnterEvent *event)
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
