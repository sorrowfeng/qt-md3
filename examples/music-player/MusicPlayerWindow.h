#ifndef MUSIC_PLAYER_WINDOW_H
#define MUSIC_PLAYER_WINDOW_H

// The qt-md3 music-player example shell: MdScaffold + NavigationBar,
// four pages, a persistent mini player, snackbar, menus and dialogs.
// Example code, not library code.

#include "MusicData.h"

#include <QtWidgets/QMainWindow>

namespace md {
class MdIconButton;
class MdMenu;
class MdMenuItem;
class MdMiniPlayer;
class MdNavigationBar;
class MdScaffold;
class MdSnackbarHost;
class MdTopAppBar;
class MdStackedWidget;
} // namespace md

class NowPlayingPage;
class LibraryPage;
class QueuePage;
class SettingsPage;

class MusicPlayerWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MusicPlayerWindow(QWidget *parent = nullptr);

    music::Player *player() { return &m_player; }
    int currentPage() const;
    void setCurrentPage(int index);

    /// Screenshot hook: the four page widgets.
    int pageCount() const { return 4; }
    QWidget *pageWidget(int index) const;
    QString pageSlug(int index) const;

    /// Open the track context menu at a global position.
    void showTrackMenu(const QString &trackId, const QPoint &globalPos);
    void showSnackbar(const QString &message);
    void confirmRemoveFromQueue(int index);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onNavChanged(int index);
    void onCurrentTrackChanged(const QString &trackId);
    void onPlayingChanged(bool playing);
    void onPositionChanged(qreal seconds);
    void onQueueChanged();
    void onThemeAction();
    void onSearchSubmitted(const QString &text);

private:
    void buildChrome();
    void buildPages();
    void layoutMiniPlayer();
    void refreshMiniPlayer();
    void connectPlayer();

    music::Player m_player;

    md::MdScaffold *m_scaffold = nullptr;
    md::MdTopAppBar *m_appBar = nullptr;
    md::MdNavigationBar *m_navBar = nullptr;
    md::MdStackedWidget *m_stack = nullptr;
    md::MdMiniPlayer *m_miniPlayer = nullptr;
    md::MdSnackbarHost *m_snackbar = nullptr;
    md::MdIconButton *m_searchButton = nullptr;
    md::MdIconButton *m_avatarButton = nullptr;
    md::MdMenu *m_avatarMenu = nullptr;
    /// One reused track context menu — a stack temporary destroyed itself
    /// while still shown and leaked native popup windows per right-click.
    md::MdMenu *m_trackMenu = nullptr;
    md::MdMenuItem *m_trackAddToQueue = nullptr;
    md::MdMenuItem *m_trackAddToPlaylist = nullptr;
    md::MdMenuItem *m_trackGoToArtist = nullptr;
    QString m_pendingTrackId;

    NowPlayingPage *m_nowPlaying = nullptr;
    LibraryPage *m_library = nullptr;
    QueuePage *m_queuePage = nullptr;
    SettingsPage *m_settings = nullptr;
};

#endif // MUSIC_PLAYER_WINDOW_H
