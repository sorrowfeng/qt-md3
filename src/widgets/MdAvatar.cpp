#include "MdAvatar.h"

#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// The list-item leading avatar is 40 px; the standalone control defaults to
/// the same edge so a list row and a free-standing avatar line up.
constexpr int kDefaultSize = 40;
constexpr int kMinSize = 16;
constexpr int kMaxSize = 256;

/// Material Symbols optical size for the icon slot — `md.comp.icon.size`.
constexpr qreal kIconSize = 24.0;

} // namespace

MdAvatar::MdAvatar(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdAvatar::MdAvatar(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
{
    init();
}

MdAvatar::~MdAvatar() = default;

void MdAvatar::init()
{
    // Display slot only: no focus, no state layer, no ripple (see header).
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAccessibleName(m_text.isEmpty() ? QStringLiteral("avatar") : m_text);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    MdStyleBase::connectThemeUpdate(this, &MdAvatar::onThemeChanged);
}

QString MdAvatar::initialsFrom(const QString &name)
{
    const QStringList words =
        name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.isEmpty()) {
        return QString();
    }
    if (words.size() == 1) {
        return words.first().left(1).toUpper();
    }
    // First and last word, one character each — the common initials rule.
    QString result = words.first().left(1);
    result += words.last().left(1);
    return result.toUpper();
}

void MdAvatar::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    setAccessibleName(m_text.isEmpty() ? QStringLiteral("avatar") : m_text);
    update();
    emit textChanged(m_text);
}

void MdAvatar::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    update();
    emit iconNameChanged(m_iconName);
}

void MdAvatar::setCorner(ShapeCorner corner)
{
    if (m_corner == corner) {
        return;
    }
    m_corner = corner;
    update();
    emit cornerChanged(m_corner);
}

void MdAvatar::setAvatarSize(int size)
{
    const int clamped = qBound(kMinSize, size, kMaxSize);
    if (m_avatarSize == clamped) {
        return;
    }
    m_avatarSize = clamped;
    updateGeometry();
    update();
    emit avatarSizeChanged(m_avatarSize);
}

void MdAvatar::setBackgroundColor(const QColor &color)
{
    if (m_backgroundOverride == color) {
        return;
    }
    m_backgroundOverride = color;
    update();
    emit backgroundColorChanged(m_backgroundOverride);
}

void MdAvatar::setContentColor(const QColor &color)
{
    if (m_contentOverride == color) {
        return;
    }
    m_contentOverride = color;
    update();
    emit contentColorChanged(m_contentOverride);
}

QColor MdAvatar::resolvedBackgroundColor() const
{
    if (m_backgroundOverride.isValid()) {
        return m_backgroundOverride;
    }
    // md.sys.color.surface-variant — the appendix D anchor.
    return MdTheme::instance().color(ColorRole::SurfaceVariant);
}

QColor MdAvatar::resolvedContentColor() const
{
    if (m_contentOverride.isValid()) {
        return m_contentOverride;
    }
    // md.sys.color.on-surface-variant.
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

QSize MdAvatar::sizeHint() const
{
    return QSize(m_avatarSize, m_avatarSize);
}

QSize MdAvatar::minimumSizeHint() const
{
    return QSize(kMinSize, kMinSize);
}

void MdAvatar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect());
    const QList<qreal> radii = MdShape::resolvedRadii(m_corner, box.size());
    const QPainterPath shape = MdShape::roundedRect(box, radii);

    painter.fillPath(shape, resolvedBackgroundColor());

    const QColor content = resolvedContentColor();
    if (!m_iconName.isEmpty()) {
        const qreal side = qMin(kIconSize, box.width() * 0.6);
        const QRectF iconRect(QPointF(box.center().x() - side / 2.0, box.center().y() - side / 2.0),
                              QSizeF(side, side));
        MdIcon::paint(&painter, iconRect, m_iconName, content);
    } else if (!m_text.isEmpty()) {
        QFont font = MdTypeScale::font(TypeStyle::LabelLarge);
        // Slightly larger glyphs for bigger discs; still a type-scale font
        // (weight / family / tracking), only the pixel size scales with the
        // avatar edge the way Compose scales list leading text.
        const qreal scale = box.height() / qreal(kDefaultSize);
        if (scale > 1.0) {
            font.setPixelSize(qRound(font.pixelSize() * qMin(scale, 1.6)));
        }
        painter.setFont(font);
        painter.setPen(content);
        painter.drawText(box, Qt::AlignCenter, m_text);
    }
}

void MdAvatar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::EnabledChange) {
        update();
    }
}

void MdAvatar::onThemeChanged()
{
    update();
}

} // namespace md
