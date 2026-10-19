#include "MdMiniPlayer.h"

#include "MdIconButton.h"
#include "MdSeekSlider.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QResizeEvent>

namespace md {

namespace {

/// Filled-card container (`md.comp.filled-card.container.color`).
constexpr int kThumbSize = 48;
constexpr int kPad = 12;
constexpr int kGap = 12;
constexpr int kButtonSize = 40;
constexpr int kProgressHeight = 24;

} // namespace

MdMiniPlayer::MdMiniPlayer(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdMiniPlayer::~MdMiniPlayer() = default;

void MdMiniPlayer::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("mini player"));
    setMinimumHeight(72);

    m_playButton = new MdIconButton(QStringLiteral("play_arrow"), this);
    m_playButton->setVariant(IconButtonVariant::Standard);
    m_playButton->setButtonSize(ButtonSize::Medium);
    m_playButton->setAccessibleName(QStringLiteral("Play"));

    m_nextButton = new MdIconButton(QStringLiteral("skip_next"), this);
    m_nextButton->setVariant(IconButtonVariant::Standard);
    m_nextButton->setButtonSize(ButtonSize::Medium);
    m_nextButton->setAccessibleName(QStringLiteral("Next track"));

    m_progress = new MdSeekSlider(this);
    m_progress->setAccessibleName(QStringLiteral("Playback position"));
    m_progress->setPreviewEnabled(true);

    connect(m_playButton, &QAbstractButton::clicked, this, [this](bool) {
        setPlaying(!m_playing);
        emit playClicked();
    });
    connect(m_nextButton, &QAbstractButton::clicked, this, &MdMiniPlayer::nextClicked);
    connect(m_progress, &MdSeekSlider::sliderMoved, this, &MdMiniPlayer::seekRequested);
    connect(m_progress, &MdSeekSlider::valueChanged, this, &MdMiniPlayer::valueChanged);

    MdStyleBase::connectThemeUpdate(this, &MdMiniPlayer::onThemeChanged);
    layoutParts();
}

void MdMiniPlayer::onThemeChanged()
{
    update();
}

void MdMiniPlayer::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    setAccessibleName(m_title.isEmpty() ? QStringLiteral("mini player") : m_title);
    update();
    emit titleChanged(m_title);
}

void MdMiniPlayer::setArtist(const QString &artist)
{
    if (m_artist == artist) {
        return;
    }
    m_artist = artist;
    update();
    emit artistChanged(m_artist);
}

void MdMiniPlayer::setThumbnail(const QPixmap &pixmap)
{
    m_thumbnail = pixmap;
    update();
    emit thumbnailChanged(m_thumbnail);
}

qreal MdMiniPlayer::value() const
{
    return m_progress ? m_progress->value() : 0.0;
}

void MdMiniPlayer::setValue(qreal value)
{
    if (m_progress) {
        m_progress->setValue(value);
    }
}

qreal MdMiniPlayer::max() const
{
    return m_progress ? m_progress->max() : 100.0;
}

void MdMiniPlayer::setMax(qreal max)
{
    if (m_progress) {
        m_progress->setMax(max);
    }
    emit maxChanged(max);
}

void MdMiniPlayer::setPlaying(bool playing)
{
    if (m_playing == playing) {
        return;
    }
    m_playing = playing;
    refreshPlayIcon();
    emit playingChanged(m_playing);
}

void MdMiniPlayer::setProgressVisible(bool visible)
{
    if (m_progressVisible == visible) {
        return;
    }
    m_progressVisible = visible;
    if (m_progress) {
        m_progress->setVisible(m_progressVisible);
    }
    layoutParts();
    emit progressVisibleChanged(m_progressVisible);
}

QSize MdMiniPlayer::sizeHint() const
{
    return QSize(320, 80);
}

QSize MdMiniPlayer::minimumSizeHint() const
{
    return QSize(240, 72);
}

void MdMiniPlayer::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdTheme &theme = MdTheme::instance();
    // `md.comp.filled-card.container.color` = surface-variant.
    const QColor container = theme.color(ColorRole::SurfaceVariant);
    const QColor onContainer = theme.color(ColorRole::OnSurfaceVariant);
    const QColor onSurface = theme.color(ColorRole::OnSurface);

    const QRectF bounds = rect();
    const QPainterPath path = MdShape::roundedRect(bounds, ShapeCorner::Medium);
    painter.fillPath(path, container);

    // Thumbnail disc. Keep it inside the body band above the seek slider so
    // the track never runs under the art.
    const int bodyHeight = m_progressVisible ? height() - kProgressHeight : height();
    const QRectF thumbRect(kPad, (bodyHeight - kThumbSize) / 2.0, kThumbSize, kThumbSize);
    if (!m_thumbnail.isNull()) {
        QPainterPath clip = MdShape::roundedRect(thumbRect, ShapeCorner::Small);
        painter.save();
        painter.setClipPath(clip);
        painter.drawPixmap(thumbRect.toRect(), m_thumbnail);
        painter.restore();
    } else {
        painter.fillPath(MdShape::roundedRect(thumbRect, ShapeCorner::Small), onContainer);
    }

    // Title / artist stack. When the seek slider is visible it owns the bottom
    // kProgressHeight of the widget; centre the text in the band above it so
    // the artist line can never slide under the track.
    const int textLeft = kPad + kThumbSize + kGap;
    const int textRight = width() - kPad - 2 * kButtonSize - 2 * kGap;
    if (textRight > textLeft) {
        const QString title = m_title.isEmpty() ? QStringLiteral("Nothing playing") : m_title;
        const QString artist = m_artist;

        QFont titleFont = MdTypeScale::font(TypeStyle::TitleSmall, TypeEmphasis::Baseline,
                                            theme.scriptCategory());
        QFont artistFont = MdTypeScale::font(TypeStyle::BodySmall, TypeEmphasis::Baseline,
                                             theme.scriptCategory());
        QFontMetrics titleFm(titleFont);
        QFontMetrics artistFm(artistFont);

        const int textHeight = m_progressVisible ? height() - kProgressHeight : height();
        const QRectF textRect(textLeft, 0, textRight - textLeft, textHeight);
        const int lines = artist.isEmpty() ? 1 : 2;
        const int totalH = titleFm.height() + (lines > 1 ? artistFm.height() : 0);
        int y = (qMax(0, textHeight - totalH)) / 2 + titleFm.ascent();

        painter.setFont(titleFont);
        painter.setPen(onSurface);
        painter.drawText(QRectF(textRect.left(), y - titleFm.ascent(), textRect.width(),
                                titleFm.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, titleFm.elidedText(
                             title, Qt::ElideRight, int(textRect.width())));
        if (!artist.isEmpty()) {
            y += titleFm.height();
            painter.setFont(artistFont);
            painter.setPen(onContainer);
            painter.drawText(QRectF(textRect.left(), y - artistFm.ascent(), textRect.width(),
                                    artistFm.height()),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             artistFm.elidedText(artist, Qt::ElideRight, int(textRect.width())));
        }
    }
}

void MdMiniPlayer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutParts();
}

void MdMiniPlayer::layoutParts()
{
    if (!m_playButton || !m_nextButton || !m_progress) {
        return;
    }
    // Buttons and the painted thumb/text share the body band above the seek
    // slider; the slider owns the bottom kProgressHeight when visible.
    const int bodyHeight = m_progressVisible ? height() - kProgressHeight : height();
    const int y = (qMax(kButtonSize, bodyHeight) - kButtonSize) / 2;
    int x = width() - kPad - kButtonSize;
    m_nextButton->setGeometry(x, y, kButtonSize, kButtonSize);
    x -= kButtonSize + 4;
    m_playButton->setGeometry(x, y, kButtonSize, kButtonSize);

    if (m_progressVisible) {
        const int py = height() - kProgressHeight;
        m_progress->setGeometry(kPad, py, qMax(0, width() - 2 * kPad), kProgressHeight);
    }
}

void MdMiniPlayer::refreshPlayIcon()
{
    if (!m_playButton) {
        return;
    }
    m_playButton->setIconName(m_playing ? QStringLiteral("pause") : QStringLiteral("play_arrow"));
    m_playButton->setAccessibleName(m_playing ? QStringLiteral("Pause") : QStringLiteral("Play"));
}

} // namespace md
