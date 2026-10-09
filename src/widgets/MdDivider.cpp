#include "MdDivider.h"

#include "styles/MdDividerStyle.h"

#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QEvent>

namespace md {

MdDivider::MdDivider(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdDivider::MdDivider(Qt::Orientation orientation, QWidget *parent)
    : QWidget(parent)
    , m_orientation(orientation)
{
    init();
}

MdDivider::~MdDivider() = default;

void MdDivider::init()
{
    // A divider is not interactive — the export publishes no state rows at
    // all. No focus, and clicks pass straight through the hairline.
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    setAccessibleName(QStringLiteral("divider"));
    updateSizePolicy();

    MdStyleBase::connectThemeUpdate(this, &MdDivider::onThemeChanged);

    // Create and register the shared style as soon as the first divider
    // exists, so a divider is never momentarily painted by the platform
    // style.
    MdDividerStyle::shared();
}

void MdDivider::updateSizePolicy()
{
    // Compose: fillMaxWidth().height(thickness) for the horizontal divider,
    // fillMaxHeight().width(thickness) for the vertical one [compose]. The
    // long axis expands; the cross axis is fixed to the hairline.
    if (m_orientation == Qt::Horizontal) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    } else {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
}

void MdDivider::setOrientation(Qt::Orientation orientation)
{
    if (m_orientation == orientation) {
        return;
    }
    m_orientation = orientation;
    updateSizePolicy();
    updateGeometry();
    update();
    emit orientationChanged(m_orientation);
}

void MdDivider::setInsetMode(InsetMode mode)
{
    if (m_insetMode == mode) {
        return;
    }
    m_insetMode = mode;
    update();
    emit insetModeChanged(m_insetMode);
}

qreal MdDivider::thickness() const
{
    if (m_thicknessOverride > 0.0) {
        return m_thicknessOverride;
    }
    return tokens().thickness;
}

void MdDivider::setThickness(qreal thicknessPx)
{
    const qreal clamped = qMax<qreal>(thicknessPx, 0.0);
    if (m_thicknessOverride == clamped) {
        return;
    }
    m_thicknessOverride = clamped;
    updateGeometry();
    update();
    emit thicknessChanged(thickness());
}

void MdDivider::setCustomColor(const QColor &color)
{
    if (m_customColor == color) {
        return;
    }
    m_customColor = color;
    update();
    emit customColorChanged(m_customColor);
}

QColor MdDivider::effectiveColor() const
{
    if (m_customColor.isValid()) {
        return m_customColor;
    }
    return MdTheme::instance().color(tokens().color);
}

const MdDividerTokens &MdDivider::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdDividerTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

QSize MdDivider::sizeHint() const
{
    // The long axis has no intrinsic length (it is filled by the layout);
    // the cross axis is exactly the hairline. The size policies make the
    // long axis expand.
    const int cross = int(std::ceil(thickness()));
    return QSize(cross, cross);
}

QSize MdDivider::minimumSizeHint() const
{
    return sizeHint();
}

void MdDivider::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    switch (event->type()) {
    case QEvent::EnabledChange:
        // A divider paints identically enabled or disabled — there is no
        // disabled row.
        update();
        break;
    case QEvent::LayoutDirectionChange:
        // Start/end insets are logical inline edges; RTL mirrors them.
        update();
        break;
    default:
        break;
    }
}

void MdDivider::onThemeChanged()
{
    invalidateTokens();
    update();
}

} // namespace md
