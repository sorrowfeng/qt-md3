#include "MdQRCode.h"

#include "core/MdQrEncoder.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtCore/QHash>

namespace md {

namespace {

/// Quiet zone in modules, per the QR family's own scanning requirement.
/// ISO/IEC 18004:2015 9.1 (light margin, 4 modules).
constexpr int kQuietModules = 4;
constexpr int kMinModule = 2;
constexpr int kMaxModule = 16;
constexpr int kDemoDimension = 21;

} // namespace

MdQRCode::MdQRCode(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdQRCode::MdQRCode(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
{
    init();
    regenerate();
}

MdQRCode::~MdQRCode() = default;

void MdQRCode::init()
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("QR code"));
    MdStyleBase::connectThemeUpdate(this, &MdQRCode::onThemeChanged);
}

void MdQRCode::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    regenerate();
    emit textChanged(m_text);
}

void MdQRCode::setModuleSize(int size)
{
    const int clamped = qBound(kMinModule, size, kMaxModule);
    if (m_moduleSize == clamped) {
        return;
    }
    m_moduleSize = clamped;
    updateGeometry();
    update();
    emit moduleSizeChanged(m_moduleSize);
}

void MdQRCode::setQuietZone(bool on)
{
    if (m_quietZone == on) {
        return;
    }
    m_quietZone = on;
    updateGeometry();
    update();
    emit quietZoneChanged(m_quietZone);
}


void MdQRCode::setEcLevel(MdQrEcLevel level)
{
    if (level == m_ecLevel || level == MdQrEcLevel::Count) {
        return;
    }
    m_ecLevel = level;
    emit ecLevelChanged(m_ecLevel);
    regenerate();
}

void MdQRCode::setMatrix(int dimension, const QList<bool> &modules)
{
    if (dimension <= 0 || modules.size() != dimension * dimension) {
        return;
    }
    m_dimension = dimension;
    m_modules = modules;
    updateGeometry();
    update();
    emit matrixChanged(m_dimension);
}

void MdQRCode::clearMatrix()
{
    if (m_dimension == 0 && m_modules.isEmpty()) {
        return;
    }
    m_dimension = 0;
    m_modules.clear();
    updateGeometry();
    update();
    emit matrixChanged(0);
}

bool MdQRCode::moduleAt(int x, int y) const
{
    if (x < 0 || y < 0 || x >= m_dimension || y >= m_dimension) {
        return false;
    }
    return m_modules.at(y * m_dimension + x);
}

QColor MdQRCode::resolvedModuleColor() const
{
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QColor MdQRCode::resolvedSurfaceColor() const
{
    return MdTheme::instance().color(ColorRole::SurfaceContainer);
}

QRectF MdQRCode::codeRect() const
{
    const int quiet = m_quietZone ? kQuietModules : 0;
    const int total = m_dimension + quiet * 2;
    const qreal side = total * m_moduleSize;
    return QRectF((width() - side) * 0.5, (height() - side) * 0.5, side, side);
}

QSize MdQRCode::sizeHint() const
{
    const int quiet = m_quietZone ? kQuietModules : 0;
    const int total = (m_dimension > 0 ? m_dimension : kDemoDimension) + quiet * 2;
    const int side = total * m_moduleSize;
    return QSize(side + 16, side + 16);
}

QSize MdQRCode::minimumSizeHint() const
{
    return QSize(32, 32);
}

void MdQRCode::regenerate()
{
    // ISO/IEC 18004 byte-mode encode via MdQrEncoder (versions 1-10, EC L/M/Q/H).
    // A payload that does not fit leaves the previous matrix untouched so the
    // widget never paints a half-true code.
    const MdQrEncodeResult result = MdQrEncoder::encodeText(m_text, m_ecLevel);
    if (!result.ok || result.dimension <= 0) {
        return;
    }
    m_dimension = result.dimension;
    m_modules = result.modules;

    updateGeometry();
    update();
    emit matrixChanged(m_dimension);
}

void MdQRCode::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    const QRectF bounds(0, 0, width(), height());
    const QPainterPath path = MdShape::roundedRect(bounds, ShapeCorner::Medium);
    painter.fillPath(path, resolvedSurfaceColor());

    if (m_dimension <= 0 || m_modules.isEmpty()) {
        return;
    }

    const QRectF code = codeRect();
    const int quiet = m_quietZone ? kQuietModules : 0;
    painter.setPen(Qt::NoPen);
    painter.setBrush(resolvedModuleColor());
    for (int y = 0; y < m_dimension; ++y) {
        for (int x = 0; x < m_dimension; ++x) {
            if (!m_modules.at(y * m_dimension + x)) {
                continue;
            }
            const QRectF cell(code.left() + (x + quiet) * m_moduleSize,
                              code.top() + (y + quiet) * m_moduleSize, m_moduleSize,
                              m_moduleSize);
            painter.fillRect(cell, resolvedModuleColor());
        }
    }
}

void MdQRCode::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
    }
    QWidget::changeEvent(event);
}

void MdQRCode::onThemeChanged()
{
    update();
}

} // namespace md
