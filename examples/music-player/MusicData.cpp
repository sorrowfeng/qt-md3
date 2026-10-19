#include "MusicData.h"

#include <QtCore/QHash>
#include <QtCore/QRandomGenerator>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtMath>

namespace music {

namespace {

Track makeTrack(const QString &id, const QString &title, const QString &artist,
                const QString &album, int duration, const QString &quality)
{
    Track t;
    t.id = id;
    t.title = title;
    t.artist = artist;
    t.album = album;
    t.durationSec = duration;
    t.quality = quality;
    return t;
}

} // namespace

QList<Track> buildCatalogue()
{
    return {
        makeTrack(QStringLiteral("t1"), QStringLiteral("Midnight City"), QStringLiteral("M83"),
                  QStringLiteral("Hurry Up, We're Dreaming"), 243, QStringLiteral("Lossless")),
        makeTrack(QStringLiteral("t2"), QStringLiteral("Wait"), QStringLiteral("M83"),
                  QStringLiteral("Hurry Up, We're Dreaming"), 356, QStringLiteral("Lossless")),
        makeTrack(QStringLiteral("t3"), QStringLiteral("Intro"), QStringLiteral("The xx"),
                  QStringLiteral("xx"), 127, QStringLiteral("High")),
        makeTrack(QStringLiteral("t4"), QStringLiteral("Crystalised"), QStringLiteral("The xx"),
                  QStringLiteral("xx"), 201, QStringLiteral("High")),
        makeTrack(QStringLiteral("t5"), QStringLiteral("Teardrop"), QStringLiteral("Massive Attack"),
                  QStringLiteral("Mezzanine"), 330, QStringLiteral("Lossless")),
        makeTrack(QStringLiteral("t6"), QStringLiteral("Angel"), QStringLiteral("Massive Attack"),
                  QStringLiteral("Mezzanine"), 380, QStringLiteral("Lossless")),
        makeTrack(QStringLiteral("t7"), QStringLiteral("Everything In Its Right Place"),
                  QStringLiteral("Radiohead"), QStringLiteral("Kid A"), 251, QStringLiteral("High")),
        makeTrack(QStringLiteral("t8"), QStringLiteral("How to Disappear Completely"),
                  QStringLiteral("Radiohead"), QStringLiteral("Kid A"), 356, QStringLiteral("High")),
        makeTrack(QStringLiteral("t9"), QStringLiteral("Nightcall"), QStringLiteral("Kavinsky"),
                  QStringLiteral("OutRun"), 258, QStringLiteral("Standard")),
        makeTrack(QStringLiteral("t10"), QStringLiteral("A Real Hero"), QStringLiteral("College"),
                  QStringLiteral("OutRun"), 267, QStringLiteral("Standard")),
        makeTrack(QStringLiteral("t11"), QStringLiteral("Resonance"), QStringLiteral("HOME"),
                  QStringLiteral("Odyssey"), 212, QStringLiteral("High")),
        makeTrack(QStringLiteral("t12"), QStringLiteral("Odyssey"), QStringLiteral("HOME"),
                  QStringLiteral("Odyssey"), 198, QStringLiteral("High")),
        makeTrack(QStringLiteral("t13"), QStringLiteral("Digital Love"), QStringLiteral("Daft Punk"),
                  QStringLiteral("Discovery"), 301, QStringLiteral("Lossless")),
        makeTrack(QStringLiteral("t14"), QStringLiteral("Something About Us"),
                  QStringLiteral("Daft Punk"), QStringLiteral("Discovery"), 232,
                  QStringLiteral("Lossless")),
        makeTrack(QStringLiteral("t15"), QStringLiteral("Weightless"), QStringLiteral("Marconi Union"),
                  QStringLiteral("Ambient Works"), 480, QStringLiteral("Standard")),
        makeTrack(QStringLiteral("t16"), QStringLiteral("An Ending (Ascent)"),
                  QStringLiteral("Brian Eno"), QStringLiteral("Apollo"), 265,
                  QStringLiteral("Lossless")),
    };
}

QList<Playlist> buildPlaylists()
{
    return {
        {QStringLiteral("p1"), QStringLiteral("Focus Flow"),
         QStringLiteral("Ambient and downtempo for deep work"),
         {QStringLiteral("t15"), QStringLiteral("t16"), QStringLiteral("t5"), QStringLiteral("t7")}},
        {QStringLiteral("p2"), QStringLiteral("Night Drive"),
         QStringLiteral("Synthwave and electronic night rides"),
         {QStringLiteral("t1"), QStringLiteral("t9"), QStringLiteral("t10"), QStringLiteral("t11"),
          QStringLiteral("t12")}},
        {QStringLiteral("p3"), QStringLiteral("Chill Classics"),
         QStringLiteral("Warm, familiar, unhurried"),
         {QStringLiteral("t3"), QStringLiteral("t4"), QStringLiteral("t13"), QStringLiteral("t14")}},
        {QStringLiteral("p4"), QStringLiteral("Deep Cuts"),
         QStringLiteral("Album tracks worth a second listen"),
         {QStringLiteral("t2"), QStringLiteral("t6"), QStringLiteral("t8"), QStringLiteral("t16")}},
        {QStringLiteral("p5"), QStringLiteral("New This Week"),
         QStringLiteral("Fresh additions to the library"),
         {QStringLiteral("t11"), QStringLiteral("t13"), QStringLiteral("t1"), QStringLiteral("t9")}},
        {QStringLiteral("p6"), QStringLiteral("All Tracks"),
         QStringLiteral("The full catalogue"),
         {QStringLiteral("t1"), QStringLiteral("t2"), QStringLiteral("t3"), QStringLiteral("t4"),
          QStringLiteral("t5"), QStringLiteral("t6"), QStringLiteral("t7"), QStringLiteral("t8"),
          QStringLiteral("t9"), QStringLiteral("t10"), QStringLiteral("t11"),
          QStringLiteral("t12"), QStringLiteral("t13"), QStringLiteral("t14"),
          QStringLiteral("t15"), QStringLiteral("t16")}},
    };
}

Player::Player(QObject *parent)
    : QObject(parent)
{
    m_catalogue = buildCatalogue();
    m_playlists = buildPlaylists();
    m_timer.setInterval(250);
    connect(&m_timer, &QTimer::timeout, this, &Player::onTick);
}

Track Player::currentTrack() const
{
    return track(m_currentId);
}

qreal Player::duration() const
{
    return qreal(track(m_currentId).durationSec);
}

Track Player::track(const QString &id) const
{
    for (const Track &t : m_catalogue) {
        if (t.id == id) {
            return t;
        }
    }
    return Track();
}

QList<QString> Player::allTrackIds() const
{
    QList<QString> ids;
    ids.reserve(m_catalogue.size());
    for (const Track &t : m_catalogue) {
        ids.append(t.id);
    }
    return ids;
}

QList<QString> Player::playlistTrackIds(const QString &playlistId) const
{
    for (const Playlist &p : m_playlists) {
        if (p.id == playlistId) {
            return p.trackIds;
        }
    }
    return {};
}

void Player::setQueue(const QList<QString> &ids)
{
    m_queue = ids;
    if (!m_queue.isEmpty() && m_currentId.isEmpty()) {
        m_currentId = m_queue.first();
        m_position = 0.0;
        emit currentTrackChanged(m_currentId);
    }
    emit queueChanged();
}

void Player::addToQueue(const QString &trackId)
{
    m_queue.append(trackId);
    if (m_currentId.isEmpty()) {
        m_currentId = trackId;
        m_position = 0.0;
        emit currentTrackChanged(m_currentId);
    }
    emit queueChanged();
}

void Player::removeFromQueue(int index)
{
    if (index < 0 || index >= m_queue.size()) {
        return;
    }
    const QString removed = m_queue.takeAt(index);
    if (removed == m_currentId) {
        if (m_queue.isEmpty()) {
            m_currentId.clear();
            m_position = 0.0;
            stop();
            emit currentTrackChanged(m_currentId);
        } else {
            m_currentId = m_queue.at(qMin(index, m_queue.size() - 1));
            m_position = 0.0;
            emit currentTrackChanged(m_currentId);
        }
    }
    emit queueChanged();
}

void Player::moveInQueue(int from, int to)
{
    if (from < 0 || from >= m_queue.size() || to < 0 || to >= m_queue.size() || from == to) {
        return;
    }
    m_queue.move(from, to);
    emit queueChanged();
}

void Player::clearQueue()
{
    m_queue.clear();
    m_currentId.clear();
    m_position = 0.0;
    stop();
    emit currentTrackChanged(m_currentId);
    emit queueChanged();
}

void Player::play()
{
    if (m_currentId.isEmpty() && !m_queue.isEmpty()) {
        m_currentId = m_queue.first();
        emit currentTrackChanged(m_currentId);
    }
    if (m_currentId.isEmpty()) {
        return;
    }
    if (!m_playing) {
        m_playing = true;
        startTimer();
        emit playingChanged(true);
    }
}

void Player::pause()
{
    if (m_playing) {
        m_playing = false;
        stopTimer();
        emit playingChanged(false);
    }
}

void Player::togglePlay()
{
    if (m_playing) {
        pause();
    } else {
        play();
    }
}

void Player::stop()
{
    pause();
    m_position = 0.0;
    emit positionChanged(0.0);
}

void Player::next()
{
    if (m_queue.isEmpty()) {
        return;
    }
    int index = m_queue.indexOf(m_currentId);
    if (m_shuffle && m_queue.size() > 1) {
        int nextIndex = index;
        while (nextIndex == index) {
            nextIndex = QRandomGenerator::global()->bounded(m_queue.size());
        }
        index = nextIndex;
    } else {
        index = (index + 1) % m_queue.size();
    }
    m_currentId = m_queue.at(index);
    m_position = 0.0;
    emit currentTrackChanged(m_currentId);
    emit positionChanged(0.0);
    if (m_playing) {
        startTimer();
    }
}

void Player::previous()
{
    if (m_queue.isEmpty()) {
        return;
    }
    if (m_position > 3.0) {
        seek(0.0);
        return;
    }
    int index = m_queue.indexOf(m_currentId);
    index = (index - 1 + m_queue.size()) % m_queue.size();
    m_currentId = m_queue.at(index);
    m_position = 0.0;
    emit currentTrackChanged(m_currentId);
    emit positionChanged(0.0);
    if (m_playing) {
        startTimer();
    }
}

void Player::seek(qreal seconds)
{
    const qreal max = duration();
    m_position = qBound(0.0, seconds, max);
    emit positionChanged(m_position);
}

void Player::setVolume(qreal volume)
{
    const qreal clamped = qBound(0.0, volume, 100.0);
    if (qFuzzyCompare(m_volume, clamped)) {
        return;
    }
    m_volume = clamped;
    emit volumeChanged(m_volume);
}

void Player::setMuted(bool muted)
{
    if (m_muted == muted) {
        return;
    }
    m_muted = muted;
    emit mutedChanged(m_muted);
}

void Player::setShuffle(bool shuffle)
{
    if (m_shuffle == shuffle) {
        return;
    }
    m_shuffle = shuffle;
    emit shuffleChanged(m_shuffle);
}

void Player::setRepeatMode(int mode)
{
    const int clamped = qBound(0, mode, 2);
    if (m_repeatMode == clamped) {
        return;
    }
    m_repeatMode = clamped;
    emit repeatModeChanged(m_repeatMode);
}

void Player::playTrack(const QString &trackId)
{
    if (!m_queue.contains(trackId)) {
        m_queue.append(trackId);
        emit queueChanged();
    }
    m_currentId = trackId;
    m_position = 0.0;
    emit currentTrackChanged(m_currentId);
    emit positionChanged(0.0);
    play();
}

void Player::playPlaylist(const QString &playlistId)
{
    const QList<QString> ids = playlistTrackIds(playlistId);
    if (ids.isEmpty()) {
        return;
    }
    m_queue = ids;
    emit queueChanged();
    m_currentId = m_queue.first();
    m_position = 0.0;
    emit currentTrackChanged(m_currentId);
    emit positionChanged(0.0);
    play();
}

void Player::onTick()
{
    if (!m_playing) {
        return;
    }
    const qreal max = duration();
    m_position += 0.25;
    if (m_position >= max) {
        if (m_repeatMode == 2) {
            m_position = 0.0;
            emit positionChanged(m_position);
            return;
        }
        if (m_repeatMode == 1 || m_queue.indexOf(m_currentId) < m_queue.size() - 1) {
            advance();
            return;
        }
        m_position = max;
        pause();
        emit finished();
        return;
    }
    emit positionChanged(m_position);
}

void Player::startTimer()
{
    m_timer.start();
}

void Player::stopTimer()
{
    m_timer.stop();
}

void Player::advance()
{
    next();
}

QPixmap albumArt(const QString &seed, int size)
{
    // Deterministic two-stop gradient from the seed string — no image files.
    uint h = 0;
    for (const QChar &c : seed) {
        h = h * 31 + uint(c.unicode());
    }
    const qreal hue1 = qreal(h % 360) / 360.0;
    const qreal hue2 = qreal((h * 7 + 120) % 360) / 360.0;
    const QColor c1 = QColor::fromHsvF(hue1, 0.55, 0.75);
    const QColor c2 = QColor::fromHsvF(hue2, 0.65, 0.35);

    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QLinearGradient gradient(0, 0, size, size);
    gradient.setColorAt(0.0, c1);
    gradient.setColorAt(1.0, c2);
    painter.fillRect(pix.rect(), gradient);

    // Soft geometric overlay for a cover-art feel.
    painter.setOpacity(0.18);
    painter.setBrush(Qt::white);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(size * 0.7, size * 0.3), size * 0.35, size * 0.35);
    painter.drawEllipse(QPointF(size * 0.25, size * 0.75), size * 0.28, size * 0.28);
    painter.setOpacity(1.0);
    return pix;
}

} // namespace music
