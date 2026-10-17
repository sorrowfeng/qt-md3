#include "MdQRCode.h"

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
constexpr int kQuietModules = 4;
constexpr int kMinModule = 2;
constexpr int kMaxModule = 16;
constexpr int kDemoDimension = 21;

/// Tiny non-ISO demo mixer. See the header note: this paints a stable
/// module pattern from the payload; it is not a scannable encoder.
quint32 mix32(quint32 &state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

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
    // Demo matrix only — stable per payload, finder-like corners so the
    // silhouette reads as a code. Real codes come in through setMatrix().
    m_dimension = kDemoDimension;
    m_modules = QList<bool>(m_dimension * m_dimension, false);

    quint32 state = 2166136261u;
    const QByteArray utf8 = m_text.toUtf8();
    for (int i = 0; i < utf8.size(); ++i) {
        state ^= quint32(uchar(utf8.at(i)));
        state *= 16777619u;
    }
    if (state == 0) {
        state = 0x9e3779b9u;
    }

    auto setDark = [this](int x, int y, bool dark) {
        if (x >= 0 && y >= 0 && x < m_dimension && y < m_dimension) {
            m_modules[y * m_dimension + x] = dark;
        }
    };

    // Finder-like eyes at three corners (7x7 ring + 3x3 core).
    auto eye = [&](int ox, int oy) {
        for (int y = 0; y < 7; ++y) {
            for (int x = 0; x < 7; ++x) {
                const bool ring = x == 0 || x == 6 || y == 0 || y == 6;
                const bool core = x >= 2 && x <= 4 && y >= 2 && y <= 4;
                setDark(ox + x, oy + y, ring || core);
            }
        }
    };
    eye(0, 0);
    eye(m_dimension - 7, 0);
    eye(0, m_dimension - 7);

    // Timing-like rows and a seeded fill elsewhere.
    for (int i = 8; i < m_dimension - 8; ++i) {
        setDark(i, 6, i % 2 == 0);
        setDark(6, i, i % 2 == 0);
    }
    for (int y = 0; y < m_dimension; ++y) {
        for (int x = 0; x < m_dimension; ++x) {
            const bool nearEye = (x < 8 && y < 8) || (x >= m_dimension - 8 && y < 8)
                                 || (x < 8 && y >= m_dimension - 8);
            if (nearEye || x == 6 || y == 6) {
                continue;
            }
            setDark(x, y, (mix32(state) & 1u) != 0u);
        }
    }

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
