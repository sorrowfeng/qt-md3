#ifndef MUSIC_PAGES_H
#define MUSIC_PAGES_H

// The four music-player pages. Example code, not library code.

#include "MusicData.h"

#include <QtWidgets/QWidget>

namespace md {
class MdAvatar;
class MdButton;
class MdCard;
class MdChip;
class MdIconButton;
class MdList;
class MdListItem;
class MdReorderList;
class MdSearchBar;
class MdSeekSlider;
class MdSelect;
class MdSlider;
class MdSwitch;
class MdMarquee;
class MdVolumeSlider;
class MdColorPicker;
} // namespace md

class MusicPlayerWindow;

// ---------------------------------------------------------------------------

class NowPlayingPage : public QWidget
{
    Q_OBJECT
public:
    explicit NowPlayingPage(MusicPlayerWindow *window, QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onSeek(qreal seconds);
    void onVolume(qreal value);

private:
    void layoutContents();

    MusicPlayerWindow *m_window = nullptr;
    QWidget *m_art = nullptr;
    md::MdMarquee *m_title = nullptr;
    QWidget *m_artist = nullptr;
    md::MdSeekSlider *m_seek = nullptr;
    QWidget *m_timeRow = nullptr;
    md::MdIconButton *m_prev = nullptr;
    md::MdIconButton *m_play = nullptr;
    md::MdIconButton *m_next = nullptr;
    md::MdIconButton *m_shuffle = nullptr;
    md::MdIconButton *m_repeat = nullptr;
    md::MdVolumeSlider *m_volume = nullptr;
    md::MdChip *m_quality = nullptr;
};

// ---------------------------------------------------------------------------

class LibraryPage : public QWidget
{
    Q_OBJECT
public:
    explicit LibraryPage(MusicPlayerWindow *window, QWidget *parent = nullptr);

    /// The single library search field — reused by the app-bar search action.
    md::MdSearchBar *searchBar() const { return m_search; }

public slots:
    void refresh();
    void applyFilter(const QString &text);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onPlaylistActivated(md::MdListItem *item);
    void onFilterChipToggled();

private:
    void rebuildList(const QString &filter);

    MusicPlayerWindow *m_window = nullptr;
    md::MdSearchBar *m_search = nullptr;
    QWidget *m_filterRow = nullptr;
    md::MdChip *m_filterAll = nullptr;
    md::MdChip *m_filterAlbums = nullptr;
    md::MdChip *m_filterPlaylists = nullptr;
    md::MdChip *m_filterLossless = nullptr;
    md::MdList *m_list = nullptr;
    md::MdCard *m_featured = nullptr;
    QString m_filter;
};

// ---------------------------------------------------------------------------

class QueuePage : public QWidget
{
    Q_OBJECT
public:
    explicit QueuePage(MusicPlayerWindow *window, QWidget *parent = nullptr);

public slots:
    void refresh();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void onItemMoved(int from, int to);
    void onRemoveClicked();

private:
    void rebuildList();

    MusicPlayerWindow *m_window = nullptr;
    md::MdReorderList *m_list = nullptr;
    md::MdIconButton *m_moveUp = nullptr;
    md::MdIconButton *m_moveDown = nullptr;
    md::MdIconButton *m_remove = nullptr;
    md::MdButton *m_clearButton = nullptr;
    QWidget *m_toolRow = nullptr;
};

// ---------------------------------------------------------------------------

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(MusicPlayerWindow *window, QWidget *parent = nullptr);

public slots:
    void refresh();

private slots:
    void onThemeChanged(int index);
    void onSeedChanged(const QColor &color);
    void onHighContrast(bool on);
    void onQualityChanged(int index);
    void onCrossfade(qreal value);
    void onNormalize(bool on);
    void onGapless(bool on);

private:
    MusicPlayerWindow *m_window = nullptr;
    md::MdSelect *m_themeSelect = nullptr;
    md::MdColorPicker *m_seedPicker = nullptr;
    md::MdSwitch *m_highContrast = nullptr;
    md::MdSelect *m_qualitySelect = nullptr;
    md::MdSlider *m_crossfade = nullptr;
    md::MdSwitch *m_normalize = nullptr;
    md::MdSwitch *m_gapless = nullptr;
    QWidget *m_seedRow = nullptr;
};

#endif // MUSIC_PAGES_H
