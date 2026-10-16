#include "MdStatusBar.h"

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QWindow>

namespace md {

namespace {

/// QStatusBar's classic grip sits in a trailing square of the bar height.
constexpr int kGripMinWidth = 16;

/// Three diagonal ticks, the classic size-grip motif (no stock icon: the
/// motif is pure geometry and stays available without the symbol font).
constexpr int kGripTicks = 3;

} // namespace

MdStatusBar::MdStatusBar(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdStatusBar::~MdStatusBar() = default;

void MdStatusBar::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("status bar"));

    MdStyleBase::connectThemeUpdate(this, &MdStatusBar::onThemeChanged);
}

void MdStatusBar::setLeftMessage(const QString &message)
{
    if (m_leftMessage == message) {
        return;
    }
    m_leftMessage = message;
    update();
    emit leftMessageChanged(m_leftMessage);
}

void MdStatusBar::setRightMessage(const QString &message)
{
    if (m_rightMessage == message) {
        return;
    }
    m_rightMessage = message;
    update();
    emit rightMessageChanged(m_rightMessage);
}

void MdStatusBar::setSizeGripVisible(bool visible)
{
    if (m_sizeGripVisible == visible) {
        return;
    }
    m_sizeGripVisible = visible;
    update();
    emit sizeGripVisibleChanged(m_sizeGripVisible);
}

void MdStatusBar::setBarHeight(int height)
{
    const int clamped = height < 0 ? 0 : height;
    if (m_barHeight == clamped) {
        return;
    }
    m_barHeight = clamped;
    updateGeometry();
    update();
    emit barHeightChanged(m_barHeight);
}

void MdStatusBar::setBackgroundColor(const QColor &color)
{
    if (m_backgroundOverride == color) {
        return;
    }
    m_backgroundOverride = color;
    update();
    emit backgroundColorChanged(m_backgroundOverride);
}

QColor MdStatusBar::resolvedBackgroundColor() const
{
    return m_backgroundOverride.isValid() ? m_backgroundOverride
                                          : MdTheme::instance().color(ColorRole::SurfaceContainer);
}

QColor MdStatusBar::resolvedLeftColor() const
{
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QColor MdStatusBar::resolvedRightColor() const
{
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

QRect MdStatusBar::sizeGripRect() const
{
    if (!m_sizeGripVisible) {
        return QRect();
    }
    const int h = height();
    const int w = qMax(kGripMinWidth, h);
    if (width() < w) {
        return QRect();
    }
    return QRect(width() - w, 0, w, h);
}

QSize MdStatusBar::sizeHint() const
{
    const QFont font = MdTypeScale::font(TypeStyle::LabelLarge);
    const QFontMetrics metrics(font);
    const int h = m_barHeight > 0 ? m_barHeight
                                  : qRound(MdTypeScale::lineHeight(TypeStyle::LabelLarge)) + 12;
    int w = metrics.horizontalAdvance(m_leftMessage) + metrics.horizontalAdvance(m_rightMessage) + 32;
    if (m_sizeGripVisible) {
        w += qMax(kGripMinWidth, h);
    }
    return QSize(qMax(w, 120), h);
}

QSize MdStatusBar::minimumSizeHint() const
{
    const QSize hint = sizeHint();
    return QSize(qMin(hint.width(), 64), hint.height());
}

void MdStatusBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), resolvedBackgroundColor());
    paintMessages(&painter);
    if (m_sizeGripVisible) {
        paintSizeGrip(&painter);
    }
}

void MdStatusBar::paintMessages(QPainter *painter) const
{
    const QRect grip = sizeGripRect();
    const int rightEdge = grip.isValid() ? grip.left() : width();

    const QFont leftFont = MdTypeScale::font(TypeStyle::LabelLarge);
    QFont leftFontWork = leftFont;
    painter->setFont(leftFontWork);
    painter->setPen(resolvedLeftColor());
    if (!m_leftMessage.isEmpty()) {
        const QRect leftRect(12, 0, rightEdge - 24, height());
        painter->drawText(leftRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                          m_leftMessage);
    }

    if (!m_rightMessage.isEmpty()) {
        QFont rightFont = MdTypeScale::font(TypeStyle::BodyMedium);
        painter->setFont(rightFont);
        painter->setPen(resolvedRightColor());
        const QRect rightRect(12, 0, rightEdge - 24, height());
        painter->drawText(rightRect, Qt::AlignVCenter | Qt::AlignRight | Qt::TextSingleLine,
                          m_rightMessage);
    }
}

void MdStatusBar::paintSizeGrip(QPainter *painter) const
{
    const QRect grip = sizeGripRect();
    if (grip.isEmpty()) {
        return;
    }
    painter->save();
    painter->setClipRect(grip);
    painter->setPen(QPen(MdTheme::instance().color(ColorRole::OutlineVariant), 1.0));
    // Diagonal ticks in the trailing-bottom corner, the classic grip motif.
    const qreal step = 3.0;
    const QPointF origin(grip.right() - 4.0, grip.bottom() - 4.0);
    for (int i = 0; i < kGripTicks; ++i) {
        const QPointF a(origin.x() - i * step, origin.y());
        const QPointF b(origin.x(), origin.y() - i * step);
        painter->drawLine(a, b);
    }
    painter->restore();
}

void MdStatusBar::mousePressEvent(QMouseEvent *event)
{
    if (m_sizeGripVisible && sizeGripRect().contains(event->pos())) {
        m_gripDragging = true;
        m_gripLastPos = event->pos();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MdStatusBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_gripDragging) {
        const QPoint delta = event->pos() - m_gripLastPos;
        m_gripLastPos = event->pos();
        emit sizeGripDragged(delta);
        if (QWidget *top = window()) {
            if (top->isWindow()) {
                top->resize(top->width() + delta.x(), top->height() - delta.y());
            }
        }
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void MdStatusBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_gripDragging) {
        m_gripDragging = false;
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void MdStatusBar::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
    }
    QWidget::changeEvent(event);
}

void MdStatusBar::onThemeChanged()
{
    update();
}

} // namespace md
