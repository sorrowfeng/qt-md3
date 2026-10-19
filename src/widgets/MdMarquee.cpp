#include "MdMarquee.h"

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

namespace md {

namespace {

constexpr int kTickMs = 16;
constexpr int kMinSpeed = 1;
constexpr int kMaxSpeed = 400;

} // namespace

MdMarquee::MdMarquee(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdMarquee::MdMarquee(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
{
    init();
}

MdMarquee::~MdMarquee()
{
    stopTimer();
}

void MdMarquee::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(m_text.isEmpty() ? QStringLiteral("marquee") : m_text);

    m_timer.setInterval(kTickMs);
    connect(&m_timer, &QTimer::timeout, this, &MdMarquee::onTick);

    MdStyleBase::connectThemeUpdate(this, &MdMarquee::onThemeChanged);
}

void MdMarquee::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    setAccessibleName(m_text.isEmpty() ? QStringLiteral("marquee") : m_text);
    m_offset = 0.0;
    recomputeScrolling();
    updateGeometry();
    update();
    emit textChanged(m_text);
}

void MdMarquee::setTypeStyle(TypeStyle style)
{
    if (style >= TypeStyle::Count || m_typeStyle == style) {
        return;
    }
    m_typeStyle = style;
    recomputeScrolling();
    updateGeometry();
    update();
    emit typeStyleChanged(m_typeStyle);
}

void MdMarquee::setEmphasis(TypeEmphasis emphasis)
{
    if (emphasis >= TypeEmphasis::Count || m_emphasis == emphasis) {
        return;
    }
    m_emphasis = emphasis;
    recomputeScrolling();
    updateGeometry();
    update();
    emit emphasisChanged(m_emphasis);
}

void MdMarquee::setSpeed(qreal pxPerSecond)
{
    const qreal clamped = qBound(qreal(kMinSpeed), pxPerSecond, qreal(kMaxSpeed));
    if (qFuzzyCompare(m_speed, clamped)) {
        return;
    }
    m_speed = clamped;
    emit speedChanged(m_speed);
}

void MdMarquee::setGap(int gap)
{
    const int clamped = qMax(0, gap);
    if (m_gap == clamped) {
        return;
    }
    m_gap = clamped;
    recomputeScrolling();
    update();
    emit gapChanged(m_gap);
}

void MdMarquee::setTextColor(const QColor &color)
{
    if (m_textColor == color) {
        return;
    }
    m_textColor = color;
    update();
    emit textColorChanged(m_textColor);
}

QSize MdMarquee::sizeHint() const
{
    const QFont font = resolvedFont();
    const QFontMetrics fm(font);
    const int h = fm.height() + 4;
    return QSize(qMax(80, fm.horizontalAdvance(m_text) / 4), h);
}

QSize MdMarquee::minimumSizeHint() const
{
    const QFontMetrics fm(resolvedFont());
    return QSize(40, fm.height() + 4);
}

void MdMarquee::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setFont(resolvedFont());
    painter.setPen(m_textColor.isValid() ? m_textColor
                                         : MdTheme::instance().color(ColorRole::OnSurface));

    const QFontMetrics fm(painter.font());
    const int textWidth = fm.horizontalAdvance(m_text);
    const int y = (height() + fm.ascent() - fm.descent()) / 2;

    if (m_text.isEmpty()) {
        return;
    }

    if (!m_scrolling || textWidth <= width()) {
        painter.drawText(0, y, m_text);
        return;
    }

    // Continuous marquee: draw the text, then a gap, then the same text
    // again so the wrap-around is seamless. The offset scrolls left and
    // wraps at one full cycle.
    const qreal cycle = qreal(textWidth + m_gap);
    painter.setClipRect(rect());
    qreal x = -m_offset;
    while (x < width()) {
        painter.drawText(int(x), y, m_text);
        x += cycle;
    }
}

void MdMarquee::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    recomputeScrolling();
}

void MdMarquee::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    recomputeScrolling();
    startTimer();
}

void MdMarquee::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    stopTimer();
}

void MdMarquee::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange) {
        recomputeScrolling();
        updateGeometry();
        update();
    }
}

void MdMarquee::onTick()
{
    if (!m_scrolling) {
        stopTimer();
        return;
    }
    const QFontMetrics fm(resolvedFont());
    const qreal cycle = qreal(fm.horizontalAdvance(m_text) + m_gap);
    if (cycle <= 0) {
        return;
    }
    m_offset += m_speed * (kTickMs / 1000.0);
    if (m_offset >= cycle) {
        m_offset -= cycle;
    }
    update();
}

void MdMarquee::onThemeChanged()
{
    update();
}

void MdMarquee::recomputeScrolling()
{
    const bool was = m_scrolling;
    const QFontMetrics fm(resolvedFont());
    m_scrolling = !m_text.isEmpty() && isVisible() && fm.horizontalAdvance(m_text) > width();
    if (m_scrolling != was) {
        emit scrollingChanged(m_scrolling);
    }
    if (m_scrolling) {
        startTimer();
    } else {
        stopTimer();
        m_offset = 0.0;
    }
}

void MdMarquee::startTimer()
{
    if (m_scrolling && !m_timer.isActive() && isVisible()) {
        m_timer.start();
    }
}

void MdMarquee::stopTimer()
{
    m_timer.stop();
}

QFont MdMarquee::resolvedFont() const
{
    return MdTypeScale::font(m_typeStyle, m_emphasis, MdTheme::instance().scriptCategory());
}

} // namespace md
