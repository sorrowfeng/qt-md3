#ifndef MD_DATE_PICKER_H
#define MD_DATE_PICKER_H

// MdDatePicker — the MD3 date picker, `md.comp.date-picker-modal.*` (34.0.21).
//
// The picker face: a header row (the selected date's headline plus the
// supporting text that switches to the year list), the weekday row, the
// month subhead and the calendar grid of 40 px date cells. Two faces
// (`MdDatePickerFace`): the calendar grid and the year list.
//
// Selection is one `QDate`. The selected cell fills with primary / on-primary;
// today carries the 1 px primary outline; the range form fills the in-range
// days with secondary-container / on-secondary-container.
//
// The container is surface-container-high at level 3 with corner-extra-large —
// the picker paints its own face; the surrounding dialog is `MdDialog`'s.

#include "core/MdDatePickerTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QDate>
#include <QtWidgets/QWidget>

namespace md {

class QT_MD3_EXPORT MdDatePicker : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QDate selectedDate READ selectedDate WRITE setSelectedDate NOTIFY
                   selectedDateChanged)
    Q_PROPERTY(QDate displayedMonth READ displayedMonth WRITE setDisplayedMonth NOTIFY
                   displayedMonthChanged)
    Q_PROPERTY(bool range READ isRange WRITE setRange NOTIFY rangeChanged)
    Q_PROPERTY(QDate rangeStart READ rangeStart WRITE setRangeStart NOTIFY rangeStartChanged)
    Q_PROPERTY(QDate rangeEnd READ rangeEnd WRITE setRangeEnd NOTIFY rangeEndChanged)
    Q_PROPERTY(md::MdDatePickerFace face READ face WRITE setFace NOTIFY faceChanged)

public:
    explicit MdDatePicker(QWidget *parent = nullptr);
    ~MdDatePicker() override;

    QDate selectedDate() const { return m_selected; }
    void setSelectedDate(const QDate &date);

    /// The month the grid displays (the day part is ignored).
    QDate displayedMonth() const { return m_displayed; }
    void setDisplayedMonth(const QDate &month);

    bool isRange() const { return m_range; }
    void setRange(bool range);

    QDate rangeStart() const { return m_rangeStart; }
    void setRangeStart(const QDate &date);

    QDate rangeEnd() const { return m_rangeEnd; }
    void setRangeEnd(const QDate &date);

    MdDatePickerFace face() const { return m_face; }
    void setFace(MdDatePickerFace face);

    // --- geometry -----------------------------------------------------------
    /// The header's rect (the headline + supporting text row).
    QRectF headerRect() const;
    /// The 40 px date cell for one grid slot (`row` 0..5, `col` 0..6).
    QRectF dateCellRect(int row, int col) const;
    /// The year cell rect in the year list (`index` 0-based).
    QRectF yearCellRect(int index) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /// The date shown in one grid slot, or an invalid date outside the month.
    QDate dateAtCell(int row, int col) const;
    int weeksInMonth() const;

    // --- interaction state -------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    const MdDatePickerTokens &datePickerTokens() const;
    void setDatePickerTokens(const MdDatePickerTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

signals:
    void selectedDateChanged(const QDate &date);
    void displayedMonthChanged(const QDate &month);
    void rangeChanged(bool range);
    void rangeStartChanged(const QDate &date);
    void rangeEndChanged(const QDate &date);
    void faceChanged(md::MdDatePickerFace face);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:

    QDate m_selected;
    QDate m_displayed = QDate::currentDate();
    bool m_range = false;
    QDate m_rangeStart;
    QDate m_rangeEnd;
    MdDatePickerFace m_face = MdDatePickerFace::Calendar;

    bool m_hovered = false;
    bool m_focusIsKeyboard = false;

    mutable MdDatePickerTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_DATE_PICKER_H
