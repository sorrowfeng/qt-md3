#ifndef MD_TIME_PICKER_H
#define MD_TIME_PICKER_H

// MdTimePicker — the MD3 time picker, `md.comp.time-picker.*` (34.0.21).
//
// The picker face: a headline row, the two time selectors (hours / minutes)
// with the `:` separator, the AM/PM period selector, and the 256 px clock dial
// whose 48 px handle follows the pointer and snaps to the nearest slot.
//
// Two modes drive the dial (`MdTimePickerFace`): the hours face snaps to 1..12
// (or 0..23 in `is24h` mode) and the minutes face snaps to 5-minute slots.
// Clicking a time selector switches the face; the period selector flips the
// AM/PM half. The value is one `QTime`.
//
// The container is surface-container-high at level 3 with corner-extra-large —
// the picker paints its own face; the surrounding dialog is `MdDialog`'s.

#include "core/MdTimePickerTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QTime>
#include <QtWidgets/QWidget>

namespace md {

class QT_MD3_EXPORT MdTimePicker : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QTime time READ time WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(bool is24h READ is24h WRITE set24h NOTIFY is24hChanged)
    Q_PROPERTY(md::MdTimePickerFace face READ face WRITE setFace NOTIFY faceChanged)
    Q_PROPERTY(md::MdTimePeriod period READ period WRITE setPeriod NOTIFY periodChanged)

public:
    explicit MdTimePicker(QWidget *parent = nullptr);
    ~MdTimePicker() override;

    QTime time() const { return m_time; }
    void setTime(const QTime &time);

    bool is24h() const { return m_24h; }
    void set24h(bool is24h);

    MdTimePickerFace face() const { return m_face; }
    void setFace(MdTimePickerFace face);

    MdTimePeriod period() const;
    void setPeriod(MdTimePeriod period);

    // --- geometry -----------------------------------------------------------
    /// The dial's circle in widget coordinates.
    QRectF dialRect() const;
    /// The handle's 48 px circle at the dial's current angle.
    QRectF selectorHandleRect() const;
    /// The hours / minutes selector rect (`start` = hours).
    QRectF timeSelectorRect(bool start) const;
    /// The AM/PM period selector rect (`am` = the upper/left half).
    QRectF periodSelectorRect(bool am) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- interaction state -----------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    const MdTimePickerTokens &timePickerTokens() const;
    void setTimePickerTokens(const MdTimePickerTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

signals:
    void timeChanged(const QTime &time);
    void is24hChanged(bool is24h);
    void faceChanged(md::MdTimePickerFace face);
    void periodChanged(md::MdTimePeriod period);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void applyDialPosition(const QPointF &pos);
    void setHour(int hour);
    void setMinute(int minute);
    qreal handleAngle() const;

    QTime m_time = QTime(12, 0);
    bool m_24h = false;
    MdTimePickerFace m_face = MdTimePickerFace::Hours;
    bool m_dragging = false;

    bool m_hovered = false;
    bool m_focusIsKeyboard = false;

    mutable MdTimePickerTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_TIME_PICKER_H
