#ifndef MUSIC_DATA_H
#define MUSIC_DATA_H

// In-memory playlist model and the simulated playback engine for the
// qt-md3 music-player example. Example code, not library code.

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtGui/QPixmap>

namespace music {

struct Track
{
    QString id;
    QString title;
    QString artist;
    QString album;
    int durationSec = 180;
    QString quality = QStringLiteral("High");
};

struct Playlist
{
    QString id;
    QString name;
    QString description;
    QList<QString> trackIds;
};

/// Fake catalogue: 16 tracks across 6 albums / playlists.
QList<Track> buildCatalogue();
QList<Playlist> buildPlaylists();

/// Simulated player. A QTimer advances `position` while `playing`.
class Player : public QObject
{
    Q_OBJECT
public:
    explicit Player(QObject *parent = nullptr);

    const QList<Track> &catalogue() const { return m_catalogue; }
    const QList<Playlist> &playlists() const { return m_playlists; }

    /// Current queue of track ids (playback order).
    QList<QString> queue() const { return m_queue; }
    void setQueue(const QList<QString> &ids);
    void addToQueue(const QString &trackId);
    void removeFromQueue(int index);
    void moveInQueue(int from, int to);
    void clearQueue();

    /// Currently playing track; empty id when nothing is loaded.
    QString currentTrackId() const { return m_currentId; }
    Track currentTrack() const;

    bool isPlaying() const { return m_playing; }
    qreal position() const { return m_position; }
    qreal duration() const;
    qreal volume() const { return m_volume; }
    bool isMuted() const { return m_muted; }
    bool shuffle() const { return m_shuffle; }
    int repeatMode() const { return m_repeatMode; } // 0 off, 1 all, 2 one

    const QList<QString> &libraryQueue() const { return m_libraryQueue; }
    void setLibraryQueue(const QList<QString> &ids) { m_libraryQueue = ids; }

    /// Look up a track by id.
    Track track(const QString &id) const;
    /// All track ids in catalogue order.
    QList<QString> allTrackIds() const;
    /// Tracks belonging to a playlist.
    QList<QString> playlistTrackIds(const QString &playlistId) const;

public slots:
    void play();
    void pause();
    void togglePlay();
    void stop();
    void next();
    void previous();
    void seek(qreal seconds);
    void setVolume(qreal volume);
    void setMuted(bool muted);
    void setShuffle(bool shuffle);
    void setRepeatMode(int mode);
    void playTrack(const QString &trackId);
    void playPlaylist(const QString &playlistId);

signals:
    void playingChanged(bool playing);
    void positionChanged(qreal seconds);
    void currentTrackChanged(const QString &trackId);
    void queueChanged();
    void volumeChanged(qreal volume);
    void mutedChanged(bool muted);
    void shuffleChanged(bool shuffle);
    void repeatModeChanged(int mode);
    void finished();

private slots:
    void onTick();

private:
    void startTimer();
    void stopTimer();
    void advance();

    QList<Track> m_catalogue;
    QList<Playlist> m_playlists;
    QList<QString> m_queue;
    QList<QString> m_libraryQueue;
    QString m_currentId;
    bool m_playing = false;
    qreal m_position = 0.0;
    qreal m_volume = 80.0;
    bool m_muted = false;
    bool m_shuffle = false;
    int m_repeatMode = 0;
    QTimer m_timer;
};

/// Deterministic "album art" gradient pixmap (no image files).
QPixmap albumArt(const QString &seed, int size = 256);

} // namespace music

#endif // MUSIC_DATA_H
