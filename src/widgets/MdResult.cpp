#include "MdResult.h"

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

constexpr int kDefaultArtSize = 96;
constexpr int kMinArtSize = 32;
constexpr int kMaxArtSize = 256;
constexpr qreal kIconSize = 32.0;

constexpr int kGap = 16;
constexpr int kGapSmall = 8;
constexpr int kGapAction = 24;

} // namespace

MdResult::MdResult(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdResult::MdResult(MdResultStatus status, const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_status(status)
    , m_title(title)
{
    init();
}

MdResult::~MdResult() = default;

void MdResult::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("result"));

    m_actionButton = new MdButton(this);
    m_actionButton->setVariant(ButtonVariant::Text);
    m_actionButton->setButtonSize(ButtonSize::XSmall);
    m_actionButton->setObjectName(QStringLiteral("md-result-action"));
    m_actionButton->setVisible(false);
    connect(m_actionButton, &MdButton::clicked, this, &MdResult::actionTriggered);

    MdStyleBase::connectThemeUpdate(this, &MdResult::onThemeChanged);
}

void MdResult::setStatus(MdResultStatus status)
{
    if (status >= MdResultStatus::Count) {
        return;
    }
    if (m_status == status) {
        return;
    }
    m_status = status;
    update();
    emit statusChanged(m_status);
}

void MdResult::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateGeometry();
    update();
    emit titleChanged(m_title);
}

void MdResult::setSubtitle(const QString &subtitle)
{
    if (m_subtitle == subtitle) {
        return;
    }
    m_subtitle = subtitle;
    updateGeometry();
    update();
    emit subtitleChanged(m_subtitle);
}

void MdResult::setExtraText(const QString &text)
{
    if (m_extraText == text) {
        return;
    }
    m_extraText = text;
    updateGeometry();
    update();
    emit extraTextChanged(m_extraText);
}

void MdResult::setIconName(const QString &name)
{
    if (m_iconName == name) {
        return;
    }
    m_iconName = name;
    update();
    emit iconNameChanged(m_iconName);
}

void MdResult::setActionText(const QString &text)
{
    if (m_actionText == text) {
        return;
    }
    m_actionText = text;
    if (m_actionButton) {
        m_actionButton->setText(m_actionText);
    }
    layoutAction();
    updateGeometry();
    update();
    emit actionTextChanged(m_actionText);
}

void MdResult::setActionFilled(bool filled)
{
    if (m_actionFilled == filled) {
        return;
    }
    m_actionFilled = filled;
    syncActionVariant();
    update();
    emit actionFilledChanged(m_actionFilled);
}

void MdResult::setTitleStyle(TypeStyle style)
{
    if (m_titleStyle == style) {
        return;
    }
    m_titleStyle = style;
    updateGeometry();
    update();
    emit titleStyleChanged(m_titleStyle);
}

void MdResult::setArtSize(int size)
{
    const int clamped = size <= 0 ? kDefaultArtSize : qBound(kMinArtSize, size, kMaxArtSize);
    if (m_artSize == clamped) {
        return;
    }
    m_artSize = clamped;
    updateGeometry();
    update();
    emit artSizeChanged(m_artSize);
}

QColor MdResult::resolvedStatusColor() const
{
    const MdTheme &theme = MdTheme::instance();
    switch (m_status) {
    case MdResultStatus::Success:
    case MdResultStatus::Info:
    case MdResultStatus::Custom:
        return theme.color(ColorRole::Primary);
    case MdResultStatus::Warning:
        return theme.color(ColorRole::Tertiary);
    case MdResultStatus::Error:
        return theme.color(ColorRole::Error);
    case MdResultStatus::Count:
        break;
    }
    return theme.color(ColorRole::Primary);
}

QColor MdResult::resolvedArtBackground() const
{
    const MdTheme &theme = MdTheme::instance();
    switch (m_status) {
    case MdResultStatus::Success:
    case MdResultStatus::Info:
    case MdResultStatus::Custom:
        return theme.color(ColorRole::PrimaryContainer);
    case MdResultStatus::Warning:
        return theme.color(ColorRole::TertiaryContainer);
    case MdResultStatus::Error:
        return theme.color(ColorRole::ErrorContainer);
    case MdResultStatus::Count:
        break;
    }
    return theme.color(ColorRole::PrimaryContainer);
}

QColor MdResult::resolvedArtContent() const
{
    const MdTheme &theme = MdTheme::instance();
    switch (m_status) {
    case MdResultStatus::Success:
    case MdResultStatus::Info:
    case MdResultStatus::Custom:
        return theme.color(ColorRole::OnPrimaryContainer);
    case MdResultStatus::Warning:
        return theme.color(ColorRole::OnTertiaryContainer);
    case MdResultStatus::Error:
        return theme.color(ColorRole::OnErrorContainer);
    case MdResultStatus::Count:
        break;
    }
    return theme.color(ColorRole::OnPrimaryContainer);
}

QString MdResult::defaultIconName() const
{
    switch (m_status) {
    case MdResultStatus::Success:
        return QStringLiteral("check_circle");
    case MdResultStatus::Info:
        return QStringLiteral("info");
    case MdResultStatus::Warning:
        return QStringLiteral("warning");
    case MdResultStatus::Error:
        return QStringLiteral("error");
    case MdResultStatus::Custom:
    case MdResultStatus::Count:
        break;
    }
    return QString();
}

QSize MdResult::sizeHint() const
{
    const QRectF content(0, 0, 360, 0);
    qreal height = m_artSize + kGap;
    height += QFontMetrics(MdTypeScale::font(m_titleStyle)).height() + kGap;
    if (!m_subtitle.isEmpty()) {
        height += QFontMetrics(MdTypeScale::font(TypeStyle::BodyLarge)).height() * 2 + kGap;
    }
    if (!m_extraText.isEmpty()) {
        height += QFontMetrics(MdTypeScale::font(TypeStyle::BodySmall)).height() * 2 + kGapSmall;
    }
    if (!m_actionText.isEmpty()) {
        height += 32 + kGapAction;
    }
    return QSize(360, int(height + 24));
}

QSize MdResult::minimumSizeHint() const
{
    return QSize(200, 160);
}

QRectF MdResult::artRect(const QRectF &content) const
{
    const qreal x = content.center().x() - m_artSize * 0.5;
    return QRectF(x, content.top(), m_artSize, m_artSize);
}

QRectF MdResult::titleRect(const QRectF &content) const
{
    const qreal y = artRect(content).bottom() + kGap;
    const qreal h = QFontMetrics(MdTypeScale::font(m_titleStyle)).height();
    return QRectF(content.left(), y, content.width(), h);
}

QRectF MdResult::subtitleRect(const QRectF &content) const
{
    const QRectF anchor = m_title.isEmpty() ? artRect(content) : titleRect(content);
    const qreal y = anchor.bottom() + kGap;
    const qreal h = QFontMetrics(MdTypeScale::font(TypeStyle::BodyLarge)).height() * 2;
    return QRectF(content.left(), y, content.width(), h);
}

QRectF MdResult::extraRect(const QRectF &content) const
{
    const QRectF anchor = m_subtitle.isEmpty()
                              ? (m_title.isEmpty() ? artRect(content) : titleRect(content))
                              : subtitleRect(content);
    const qreal y = anchor.bottom() + kGapSmall;
    const qreal h = QFontMetrics(MdTypeScale::font(TypeStyle::BodySmall)).height() * 2;
    return QRectF(content.left(), y, content.width(), h);
}

QRectF MdResult::actionRect(const QRectF &content) const
{
    QRectF anchor;
    if (!m_extraText.isEmpty()) {
        anchor = extraRect(content);
    } else if (!m_subtitle.isEmpty()) {
        anchor = subtitleRect(content);
    } else if (!m_title.isEmpty()) {
        anchor = titleRect(content);
    } else {
        anchor = artRect(content);
    }
    const qreal y = anchor.bottom() + kGapAction;
    return QRectF(content.left(), y, content.width(), 32);
}

void MdResult::layoutAction()
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
}

void MdResult::syncActionVariant()
{
    if (!m_actionButton) {
        return;
    }
    m_actionButton->setVariant(m_actionFilled ? ButtonVariant::Filled : ButtonVariant::Text);
}

void MdResult::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF content(0, 0, width(), height());

    const QRectF art = artRect(content);
    const QPainterPath path = MdShape::roundedRect(art, ShapeCorner::Full);
    painter.fillPath(path, resolvedArtBackground());

    const QString icon = m_iconName.isEmpty() ? defaultIconName() : m_iconName;
    if (!icon.isEmpty()) {
        const QRectF iconBox(art.center().x() - kIconSize * 0.5,
                             art.center().y() - kIconSize * 0.5, kIconSize, kIconSize);
        MdIcon::paint(&painter, iconBox, icon, resolvedArtContent());
    }

    painter.setFont(MdTypeScale::font(m_titleStyle));
    painter.setPen(MdTheme::instance().color(ColorRole::OnSurface));
    painter.drawText(titleRect(content), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine,
                     m_title);

    if (!m_subtitle.isEmpty()) {
        painter.setFont(MdTypeScale::font(TypeStyle::BodyLarge));
        painter.setPen(MdTheme::instance().color(ColorRole::OnSurfaceVariant));
        painter.drawText(subtitleRect(content), Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                         m_subtitle);
    }

    if (!m_extraText.isEmpty()) {
        painter.setFont(MdTypeScale::font(TypeStyle::BodySmall));
        painter.setPen(MdTheme::instance().color(ColorRole::OnSurfaceVariant));
        painter.drawText(extraRect(content), Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                         m_extraText);
    }
}

void MdResult::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutAction();
}

void MdResult::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
        layoutAction();
    }
    QWidget::changeEvent(event);
}

void MdResult::onThemeChanged()
{
    update();
}

} // namespace md
