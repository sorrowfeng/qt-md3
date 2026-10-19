#ifndef MD_MARQUEE_H
#define MD_MARQUEE_H

// MdMarquee — Stage 2 Qt extension: a scrolling text label.
//
// MD3 has no marquee family. The control is the standard "long title that
// must stay readable" element every media / list surface needs. It paints one
// line of text in a type-scale style; when the text overflows the widget it
// scrolls continuously, otherwise it draws statically (start-aligned).
//
// Tokens (nothing invented):
//   text colour  md.sys.color.on-surface (or caller override)
//   type style   the type scale (default TitleLarge)
//   gap          the spacing scale's 24 px step between repetitions
//
// No QSS. Not focusable: a marquee is a display slot.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QTimer>
#include <QtGui/QColor>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

class QT_MD3_EXPORT MdMarquee : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(md::TypeStyle typeStyle READ typeStyle WRITE setTypeStyle NOTIFY typeStyleChanged)
    Q_PROPERTY(md::TypeEmphasis emphasis READ emphasis WRITE setEmphasis NOTIFY emphasisChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(int gap READ gap WRITE setGap NOTIFY gapChanged)
    Q_PROPERTY(bool scrolling READ isScrolling NOTIFY scrollingChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)

public:
    explicit MdMarquee(QWidget *parent = nullptr);
    explicit MdMarquee(const QString &text, QWidget *parent = nullptr);
    ~MdMarquee() override;

    QString text() const { return m_text; }
    void setText(const QString &text);

    TypeStyle typeStyle() const { return m_typeStyle; }
    void setTypeStyle(TypeStyle style);

    TypeEmphasis emphasis() const { return m_emphasis; }
    void setEmphasis(TypeEmphasis emphasis);

    /// Scroll speed in logical px per second. Must be > 0.
    qreal speed() const { return m_speed; }
    void setSpeed(qreal pxPerSecond);

    /// Gap between the end of one repetition and the start of the next.
    int gap() const { return m_gap; }
    void setGap(int gap);

    /// True while the text is wider than the widget and the timer is running.
    bool isScrolling() const { return m_scrolling; }

    /// Text colour override. Invalid means `md.sys.color.on-surface`.
    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor &color);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void textChanged(const QString &text);
    void typeStyleChanged(md::TypeStyle style);
    void emphasisChanged(md::TypeEmphasis emphasis);
    void speedChanged(qreal speed);
    void gapChanged(int gap);
    void scrollingChanged(bool scrolling);
    void textColorChanged(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onTick();
    void onThemeChanged();

private:
    void init();
    void recomputeScrolling();
    void startTimer();
    void stopTimer();
    QFont resolvedFont() const;

    QString m_text;
    TypeStyle m_typeStyle = TypeStyle::TitleLarge;
    TypeEmphasis m_emphasis = TypeEmphasis::Baseline;
    qreal m_speed = 40.0;
    int m_gap = 24;
    bool m_scrolling = false;
    QColor m_textColor;
    qreal m_offset = 0.0;
    QTimer m_timer;
};

} // namespace md

#endif // MD_MARQUEE_H
