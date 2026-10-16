#include "MdEmptyState.h"

#include "MdButton.h"
#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// Default art-disc edge. Large enough to read as an illustration slot,
/// small enough to sit inside a half-empty list.
constexpr int kDefaultArtSize = 96;
constexpr int kMinArtSize = 32;
constexpr int kMaxArtSize = 256;

/// Material Symbols optical size inside the disc.
constexpr qreal kIconSize = 32.0;

constexpr int kGap = 16;
constexpr int kGapSmall = 8;
constexpr int kGapAction = 24;

} // namespace

MdEmptyState::MdEmptyState(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdEmptyState::MdEmptyState(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
{
    init();
}

MdEmptyState::~MdEmptyState() = default;

void MdEmptyState::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(m_title.isEmpty() ? QStringLiteral("empty state") : m_title);

    m_actionButton = new MdButton(this);
    m_actionButton->setVariant(ButtonVariant::Text);
    m_actionButton->setButtonSize(ButtonSize::XSmall);
    m_actionButton->setObjectName(QStringLiteral("md-empty-state-action"));
    m_actionButton->setVisible(false);
    connect(m_actionButton, &MdButton::clicked, this, &MdEmptyState::actionTriggered);

    MdStyleBase::connectThemeUpdate(this, &MdEmptyState::onThemeChanged);
}

void MdEmptyState::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    setAccessibleName(m_title.isEmpty() ? QStringLiteral("empty state") : m_title);
    updateGeometry();
    update();
    emit titleChanged(m_title);
}

void MdEmptyState::setDescription(const QString &description)
{
    if (m_description == description) {
        return;
    }
    m_description = description;
    updateGeometry();
    update();
    emit descriptionChanged(m_description);
}

void MdEmptyState::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    update();
    emit iconNameChanged(m_iconName);
}

void MdEmptyState::setActionText(const QString &text)
{
    if (m_actionText == text) {
        return;
    }
    m_actionText = text;
    if (m_actionButton) {
        m_actionButton->setText(m_actionText);
        m_actionButton->setVisible(!m_actionText.isEmpty());
    }
    updateGeometry();
    update();
    emit actionTextChanged(m_actionText);
}

void MdEmptyState::setTitleStyle(TypeStyle style)
{
    if (style >= TypeStyle::Count) {
        return;
    }
    if (m_titleStyle == style) {
        return;
    }
    m_titleStyle = style;
    updateGeometry();
    update();
    emit titleStyleChanged(m_titleStyle);
}

void MdEmptyState::setArtSize(int size)
{
    const int clamped = size == 0 ? kDefaultArtSize : qBound(kMinArtSize, size, kMaxArtSize);
    if (m_artSize == clamped) {
        return;
    }
    m_artSize = clamped;
    updateGeometry();
    update();
    emit artSizeChanged(m_artSize);
}

void MdEmptyState::setArtCorner(ShapeCorner corner)
{
    if (m_artCorner == corner) {
        return;
    }
    m_artCorner = corner;
    update();
    emit artCornerChanged(m_artCorner);
}

void MdEmptyState::setArtWidget(QWidget *widget)
{
    if (m_artWidget == widget) {
        return;
    }
    if (m_artWidget) {
        m_artWidget->setParent(nullptr);
        m_artWidget->hide();
    }
    m_artWidget = widget;
    if (m_artWidget) {
        m_artWidget->setParent(this);
        m_artWidget->show();
    }
    updateGeometry();
    update();
    emit artWidgetChanged(m_artWidget);
}

QColor MdEmptyState::resolvedArtBackground() const
{
    return MdTheme::instance().color(ColorRole::SurfaceVariant);
}

QColor MdEmptyState::resolvedArtContent() const
{
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

QSize MdEmptyState::sizeHint() const
{
    const QFont titleFont = MdTypeScale::font(m_titleStyle);
    const QFont bodyFont = MdTypeScale::font(TypeStyle::BodyLarge);
    const QFontMetrics titleMetrics(titleFont);
    const QFontMetrics bodyMetrics(bodyFont);

    int w = 280;
    w = qMax(w, titleMetrics.horizontalAdvance(m_title) + 48);
    w = qMax(w, bodyMetrics.horizontalAdvance(m_description) + 48);
    if (!m_actionText.isEmpty() && m_actionButton) {
        w = qMax(w, m_actionButton->sizeHint().width() + 48);
    }

    int h = m_artSize + kGap;
    h += qRound(MdTypeScale::lineHeight(m_titleStyle));
    if (!m_description.isEmpty()) {
        h += kGapSmall + qRound(MdTypeScale::lineHeight(TypeStyle::BodyLarge)) * 2;
    }
    if (!m_actionText.isEmpty()) {
        h += kGapAction + 32;
    }
    h += 32;
    return QSize(w, h);
}

QSize MdEmptyState::minimumSizeHint() const
{
    return QSize(200, 160);
}

QRectF MdEmptyState::artRect(const QRectF &content) const
{
    const qreal size = m_artSize;
    return QRectF(content.center().x() - size * 0.5, content.top(), size, size);
}

QRectF MdEmptyState::titleRect(const QRectF &content) const
{
    const QRectF art = artRect(content);
    const qreal y = art.bottom() + kGap;
    const qreal h = MdTypeScale::lineHeight(m_titleStyle);
    return QRectF(content.left(), y, content.width(), h);
}

QRectF MdEmptyState::descriptionRect(const QRectF &content) const
{
    if (m_description.isEmpty()) {
        return QRectF();
    }
    const QRectF title = titleRect(content);
    const qreal y = title.bottom() + kGapSmall;
    const qreal h = MdTypeScale::lineHeight(TypeStyle::BodyLarge) * 2;
    return QRectF(content.left(), y, content.width(), h);
}

QRectF MdEmptyState::actionRect(const QRectF &content) const
{
    if (m_actionText.isEmpty() || !m_actionButton) {
        return QRectF();
    }
    const QRectF description = descriptionRect(content);
    const QRectF anchor = description.isValid() && !description.isEmpty()
                              ? description
                              : titleRect(content);
    const qreal y = anchor.bottom() + kGapAction;
    return QRectF(content.left(), y, content.width(), 32);
}

void MdEmptyState::layoutAction()
{
    if (!m_actionButton) {
        return;
    }
    const QRectF content(0, 0, width(), height());
    const QRectF target = actionRect(content);
    if (target.isEmpty() || m_actionText.isEmpty()) {
        m_actionButton->setVisible(false);
        return;
    }
    const QSize hint = m_actionButton->sizeHint();
    const QRect buttonRect(qRound(target.center().x() - hint.width() * 0.5),
                           qRound(target.top() + (target.height() - hint.height()) * 0.5),
                           hint.width(), hint.height());
    m_actionButton->setGeometry(buttonRect);
    m_actionButton->setVisible(true);

    if (m_artWidget) {
        const QRectF art = artRect(content);
        m_artWidget->setGeometry(art.toRect());
    }
}

void MdEmptyState::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF content(0, 0, width(), height());

    // Art disc (or caller illustration).
    const QRectF art = artRect(content);
    if (m_artWidget == nullptr) {
        const QPainterPath path = MdShape::roundedRect(art, m_artCorner);
        painter.fillPath(path, resolvedArtBackground());
        if (!m_iconName.isEmpty()) {
            const QRectF iconBox(art.center().x() - kIconSize * 0.5,
                                 art.center().y() - kIconSize * 0.5, kIconSize, kIconSize);
            MdIcon::paint(&painter, iconBox, m_iconName, resolvedArtContent());
        }
    }

    // Title.
    painter.setFont(MdTypeScale::font(m_titleStyle));
    painter.setPen(MdTheme::instance().color(ColorRole::OnSurface));
    painter.drawText(titleRect(content), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine,
                     m_title);

    // Description (two lines max).
    if (!m_description.isEmpty()) {
        painter.setFont(MdTypeScale::font(TypeStyle::BodyLarge));
        painter.setPen(MdTheme::instance().color(ColorRole::OnSurfaceVariant));
        painter.drawText(descriptionRect(content),
                         Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, m_description);
    }
}

void MdEmptyState::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutAction();
}

void MdEmptyState::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
        layoutAction();
    }
    QWidget::changeEvent(event);
}

void MdEmptyState::onThemeChanged()
{
    update();
}

} // namespace md
