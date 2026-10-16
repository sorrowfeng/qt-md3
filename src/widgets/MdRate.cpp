#include "MdRate.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// Cell edge. md.comp.icon-button medium container is 40 px on the default
/// track; 48 px is the touch target the icon-button host reserves around it
/// (40 + the focus-indicator margin), so a rate cell matches a real button.
constexpr qreal kCell = 48.0;
/// Glyph optical size inside the cell — `md.comp.icon.size`, 24 px.
constexpr qreal kGlyph = 24.0;

QString glyphName(MdRateGlyph glyph, bool filled)
{
    switch (glyph) {
    case MdRateGlyph::Heart:
        return filled ? QStringLiteral("favorite") : QStringLiteral("favorite_border");
    case MdRateGlyph::Star:
    case MdRateGlyph::Count:
        break;
    }
    return filled ? QStringLiteral("star") : QStringLiteral("star_border");
}

} // namespace

MdRate::MdRate(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdRate::~MdRate() = default;

void MdRate::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("rating"));
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);

    MdStyleBase::connectThemeUpdate(this, &MdRate::onThemeChanged);
}

qreal MdRate::cellSize()
{
    return kCell;
}

void MdRate::setValue(qreal value)
{
    const qreal clamped = qBound(0.0, value, qreal(m_max));
    if (qFuzzyCompare(m_value, clamped)) {
        return;
    }
    m_value = clamped;
    update();
    emit valueChanged(m_value);
}

void MdRate::setMax(int max)
{
    const int clamped = qMax(1, max);
    if (m_max == clamped) {
        return;
    }
    m_max = clamped;
    if (m_value > qreal(m_max)) {
        m_value = qreal(m_max);
        emit valueChanged(m_value);
    }
    if (m_hoverValue > qreal(m_max)) {
        m_hoverValue = qreal(m_max);
    }
    updateGeometry();
    update();
    emit maxChanged(m_max);
}

void MdRate::setReadOnly(bool readOnly)
{
    if (m_readOnly == readOnly) {
        return;
    }
    m_readOnly = readOnly;
    if (m_readOnly) {
        m_hoverValue = -1.0;
    }
    update();
    emit readOnlyChanged(m_readOnly);
}

void MdRate::setHalfSteps(bool halfSteps)
{
    if (m_halfSteps == halfSteps) {
        return;
    }
    m_halfSteps = halfSteps;
    emit halfStepsChanged(m_halfSteps);
}

void MdRate::setGlyph(MdRateGlyph glyph)
{
    if (glyph >= MdRateGlyph::Count) {
        return;
    }
    if (m_glyph == glyph) {
        return;
    }
    m_glyph = glyph;
    update();
    emit glyphChanged(m_glyph);
}

QColor MdRate::resolvedSelectedColor() const
{
    // md.sys.color.primary — the icon-button selected / filled reading.
    return MdTheme::instance().color(ColorRole::Primary);
}

QColor MdRate::resolvedUnselectedColor() const
{
    // md.sys.color.on-surface-variant — the unselected icon-button content.
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

QColor MdRate::resolvedStateLayerContent() const
{
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QSize MdRate::sizeHint() const
{
    return QSize(qRound(kCell * m_max), qRound(kCell));
}

QSize MdRate::minimumSizeHint() const
{
    return QSize(qRound(kCell), qRound(kCell));
}

qreal MdRate::cellValueAt(const QPointF &position) const
{
    if (position.x() < 0.0 || position.y() < 0.0
        || position.y() > height()) {
        return -1.0;
    }
    const qreal raw = position.x() / kCell;
    if (raw < 0.0 || raw >= qreal(m_max)) {
        // Allow the trailing edge to resolve to max.
        if (position.x() >= kCell * m_max && position.x() < width() + 1.0) {
            return qreal(m_max);
        }
        return -1.0;
    }
    const int cell = int(raw);
    const qreal fraction = raw - qreal(cell);
    if (m_halfSteps && fraction < 0.5) {
        return qreal(cell) + 0.5;
    }
    return qreal(cell) + 1.0;
}

void MdRate::commit(qreal value)
{
    if (m_readOnly) {
        return;
    }
    setValue(value);
    emit rated(m_value);
}

void MdRate::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal paintValue = m_hovered && !m_readOnly && m_hoverValue >= 0.0
                                 ? m_hoverValue
                                 : m_value;
    const QColor selected = resolvedSelectedColor();
    const QColor unselected = resolvedUnselectedColor();

    for (int i = 0; i < m_max; ++i) {
        const QRectF cell(i * kCell, 0.0, kCell, kCell);
        const QRectF glyphRect(cell.center().x() - kGlyph / 2.0,
                               cell.center().y() - kGlyph / 2.0,
                               kGlyph, kGlyph);

        // Hover state layer on the hovered cell (icon-button hover 0.08).
        if (m_hovered && !m_readOnly && m_hoverValue > qreal(i)
            && m_hoverValue <= qreal(i + 1)) {
            const QColor layer =
                MdStateLayer::overlay(resolvedStateLayerContent(),
                                      StateLayerKind::Hover);
            const QList<qreal> radii =
                MdShape::resolvedRadii(ShapeCorner::Full, cell.size());
            painter.fillPath(MdShape::roundedRect(cell, radii), layer);
        }

        const qreal fill = qBound(0.0, paintValue - qreal(i), 1.0);
        if (fill >= 1.0) {
            MdIcon::paint(&painter, glyphRect, glyphName(m_glyph, true), selected);
        } else if (fill <= 0.0) {
            MdIcon::paint(&painter, glyphRect, glyphName(m_glyph, false), unselected);
        } else {
            // Half (or partial) fill: outline underneath, filled glyph
            // clipped to the left `fill` fraction of the cell.
            MdIcon::paint(&painter, glyphRect, glyphName(m_glyph, false), unselected);
            painter.save();
            QRectF clip = glyphRect;
            clip.setWidth(glyphRect.width() * fill);
            painter.setClipRect(clip, Qt::IntersectClip);
            MdIcon::paint(&painter, glyphRect, glyphName(m_glyph, true), selected);
            painter.restore();
        }
    }

    // :focus-visible ring — keyboard focus only.
    if (hasFocus() && m_focusIsKeyboard) {
        const QRectF bounds = QRectF(rect());
        MdFocusRingSpec spec;
        spec.shape = ShapeCorner::ExtraLarge;
        spec.inward = true;
        const QColor ring = MdTheme::instance().color(ColorRole::Secondary);
        MdFocusRing::paint(&painter, bounds, QList<qreal>(), ring, spec);
    }
}

void MdRate::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    if (m_readOnly) {
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const qreal hit = cellValueAt(mousePosition(event));
    if (hit >= 0.0) {
        commit(hit);
    }
}

void MdRate::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    if (m_readOnly) {
        return;
    }
    const qreal hit = cellValueAt(mousePosition(event));
    if (hit != m_hoverValue) {
        m_hoverValue = hit;
        update();
    }
}

void MdRate::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_hoverValue != -1.0) {
        m_hoverValue = -1.0;
    }
    if (m_hovered) {
        m_hovered = false;
    }
    update();
}

void MdRate::enterEvent(md::MdEnterEvent *event)
{
    QWidget::enterEvent(event);
    m_hovered = true;
    update();
}

void MdRate::keyPressEvent(QKeyEvent *event)
{
    if (!m_readOnly) {
        const qreal step = m_halfSteps ? 0.5 : 1.0;
        switch (event->key()) {
        case Qt::Key_Right:
        case Qt::Key_Up:
            commit(m_value + step);
            return;
        case Qt::Key_Left:
        case Qt::Key_Down:
            commit(m_value - step);
            return;
        case Qt::Key_Home:
            commit(0.0);
            return;
        case Qt::Key_End:
            commit(qreal(m_max));
            return;
        default:
            break;
        }
        if (event->key() >= Qt::Key_0 && event->key() <= Qt::Key_9) {
            commit(qreal(event->key() - Qt::Key_0));
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void MdRate::focusInEvent(QFocusEvent *event)
{
    m_focusIsKeyboard = event->reason() == Qt::TabFocusReason
                        || event->reason() == Qt::BacktabFocusReason
                        || event->reason() == Qt::ShortcutFocusReason;
    QWidget::focusInEvent(event);
    update();
}

void MdRate::focusOutEvent(QFocusEvent *event)
{
    m_focusIsKeyboard = false;
    QWidget::focusOutEvent(event);
    update();
}

void MdRate::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::EnabledChange
        || event->type() == QEvent::LayoutDirectionChange) {
        update();
    }
}

void MdRate::onThemeChanged()
{
    update();
}

} // namespace md
