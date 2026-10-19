#include "MusicPlayerWindow.h"

#include "MusicPages.h"

#include "core/MdDesign.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdMenu.h"
#include "widgets/MdMessageBox.h"
#include "widgets/MdMiniPlayer.h"
#include "widgets/MdNavigationBar.h"
#include "widgets/MdNavigationBarItem.h"
#include "widgets/MdScaffold.h"
#include "widgets/MdSnackbarHost.h"
#include "widgets/MdStackedWidget.h"
#include "widgets/MdTopAppBar.h"

#include <QtGui/QResizeEvent>
#include <QtWidgets/QVBoxLayout>

using namespace md;

MusicPlayerWindow::MusicPlayerWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("qt-md3 Music Player"));
    resize(420, 780);

    buildChrome();
    buildPages();
    connectPlayer();
    refreshMiniPlayer();

    // Seed a default queue so the player is usable immediately.
    m_player.setQueue(m_player.playlistTrackIds(QStringLiteral("p2")));
    if (!m_player.queue().isEmpty()) {
        m_player.playTrack(m_player.queue().first());
        m_player.pause();
    }
}

void MusicPlayerWindow::buildChrome()
{
    m_scaffold = new MdScaffold(this);
    setCentralWidget(m_scaffold);

    m_appBar = new MdTopAppBar(QStringLiteral("Music"), m_scaffold);
    m_appBar->setAlignment(MdAppBarAlignment::Leading);

    m_searchButton = new MdIconButton(QStringLiteral("search"), m_appBar);
    m_searchButton->setAccessibleName(QStringLiteral("Search"));
    m_searchButton->setToolTip(QStringLiteral("Search"));
    m_avatarButton = new MdIconButton(QStringLiteral("account_circle"), m_appBar);
    m_avatarButton->setAccessibleName(QStringLiteral("Account"));
    m_avatarButton->setToolTip(QStringLiteral("Account"));

    m_appBar->addActionWidget(m_searchButton);
    m_appBar->addActionWidget(m_avatarButton);

    m_avatarMenu = new MdMenu(m_avatarButton);
    MdMenuItem *themeItem = new MdMenuItem(QStringLiteral("Toggle light / dark"), m_avatarMenu);
    themeItem->setLeadingIconName(QStringLiteral("dark_mode"));
    MdMenuItem *settingsItem = new MdMenuItem(QStringLiteral("Settings"), m_avatarMenu);
    settingsItem->setLeadingIconName(QStringLiteral("settings"));
    MdMenuItem *aboutItem = new MdMenuItem(QStringLiteral("About"), m_avatarMenu);
    aboutItem->setLeadingIconName(QStringLiteral("info"));
    m_avatarMenu->addItem(themeItem);
    m_avatarMenu->addItem(settingsItem);
    m_avatarMenu->addItem(aboutItem);

    connect(m_avatarButton, &QAbstractButton::clicked, this, [this](bool) {
        m_avatarMenu->popup(QRect(m_avatarButton->mapToGlobal(QPoint(0, m_avatarButton->height())),
                                  QSize(m_avatarButton->width(), 0)));
    });
    connect(themeItem, &QAbstractButton::clicked, this, &MusicPlayerWindow::onThemeAction);
    connect(settingsItem, &QAbstractButton::clicked, this, [this](bool) {
        setCurrentPage(3);
    });
    connect(aboutItem, &QAbstractButton::clicked, this, [this](bool) {
        MdMessageBox::information(this, QStringLiteral("qt-md3 Music Player"),
                                  QStringLiteral("A demo application built with the qt-md3 "
                                                  "Material Design 3 widget library."));
    });
    connect(m_searchButton, &QAbstractButton::clicked, this, [this](bool) {
        setCurrentPage(1);
    });

    m_navBar = new MdNavigationBar(m_scaffold);
    m_navBar->addItem(new MdNavigationBarItem(QStringLiteral("Now Playing"),
                                              QStringLiteral("graphic_eq"), m_navBar));
    m_navBar->addItem(new MdNavigationBarItem(QStringLiteral("Library"),
                                              QStringLiteral("library_music"), m_navBar));
    m_navBar->addItem(new MdNavigationBarItem(QStringLiteral("Queue"),
                                              QStringLiteral("queue_music"), m_navBar));
    m_navBar->addItem(new MdNavigationBarItem(QStringLiteral("Settings"),
                                              QStringLiteral("settings"), m_navBar));
    connect(m_navBar, &MdNavigationBar::currentIndexChanged, this,
            &MusicPlayerWindow::onNavChanged);

    m_scaffold->setAppBar(m_appBar);
    m_scaffold->setNavigation(m_navBar, MdNavPlacement::Bottom);

    m_snackbar = new MdSnackbarHost(this);

    m_miniPlayer = new MdMiniPlayer(this);
    m_miniPlayer->setVisible(false);
    connect(m_miniPlayer, &MdMiniPlayer::playClicked, this, [this]() {
        m_player.togglePlay();
    });
    connect(m_miniPlayer, &MdMiniPlayer::nextClicked, this, [this]() {
        m_player.next();
    });
    connect(m_miniPlayer, &MdMiniPlayer::seekRequested, this, [this](qreal v) {
        m_player.seek(v);
    });
}

void MusicPlayerWindow::buildPages()
{
    m_stack = new MdStackedWidget(m_scaffold);
    m_nowPlaying = new NowPlayingPage(this, m_stack);
    m_library = new LibraryPage(this, m_stack);
    m_queuePage = new QueuePage(this, m_stack);
    m_settings = new SettingsPage(this, m_stack);
    m_stack->addWidget(m_nowPlaying);
    m_stack->addWidget(m_library);
    m_stack->addWidget(m_queuePage);
    m_stack->addWidget(m_settings);
    m_scaffold->setBody(m_stack);
}

void MusicPlayerWindow::connectPlayer()
{
    connect(&m_player, &music::Player::currentTrackChanged, this,
            &MusicPlayerWindow::onCurrentTrackChanged);
    connect(&m_player, &music::Player::playingChanged, this,
            &MusicPlayerWindow::onPlayingChanged);
    connect(&m_player, &music::Player::positionChanged, this,
            &MusicPlayerWindow::onPositionChanged);
    connect(&m_player, &music::Player::queueChanged, this, &MusicPlayerWindow::onQueueChanged);
    connect(&m_player, &music::Player::finished, this, [this]() {
        showSnackbar(QStringLiteral("Playback finished"));
    });
}

int MusicPlayerWindow::currentPage() const
{
    return m_stack ? m_stack->currentIndex() : 0;
}

void MusicPlayerWindow::setCurrentPage(int index)
{
    if (!m_stack || index < 0 || index >= m_stack->count()) {
        return;
    }
    m_stack->setCurrentIndex(index);
    if (m_navBar && m_navBar->currentIndex() != index) {
        m_navBar->setCurrentIndex(index);
    }
    // Mini player hides on the Now Playing page (the full transport is there).
    if (m_miniPlayer) {
        m_miniPlayer->setVisible(index != 0);
    }
    layoutMiniPlayer();
    if (index == 1 && m_library) {
        m_library->refresh();
    } else if (index == 2 && m_queuePage) {
        m_queuePage->refresh();
    } else if (index == 3 && m_settings) {
        m_settings->refresh();
    } else if (index == 0 && m_nowPlaying) {
        m_nowPlaying->refresh();
    }
}

QWidget *MusicPlayerWindow::pageWidget(int index) const
{
    switch (index) {
    case 0:
        return m_nowPlaying;
    case 1:
        return m_library;
    case 2:
        return m_queuePage;
    case 3:
        return m_settings;
    default:
        return nullptr;
    }
}

QString MusicPlayerWindow::pageSlug(int index) const
{
    switch (index) {
    case 0:
        return QStringLiteral("now-playing");
    case 1:
        return QStringLiteral("library");
    case 2:
        return QStringLiteral("queue");
    case 3:
        return QStringLiteral("settings");
    default:
        return QStringLiteral("page");
    }
}

void MusicPlayerWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    layoutMiniPlayer();
    if (m_snackbar) {
        m_snackbar->setGeometry(rect());
    }
}

void MusicPlayerWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    layoutMiniPlayer();
    if (m_snackbar) {
        m_snackbar->setGeometry(rect());
    }
}

void MusicPlayerWindow::onNavChanged(int index)
{
    setCurrentPage(index);
}

void MusicPlayerWindow::onCurrentTrackChanged(const QString &trackId)
{
    Q_UNUSED(trackId);
    refreshMiniPlayer();
    if (m_nowPlaying) {
        m_nowPlaying->refresh();
    }
    if (m_queuePage) {
        m_queuePage->refresh();
    }
}

void MusicPlayerWindow::onPlayingChanged(bool playing)
{
    Q_UNUSED(playing);
    refreshMiniPlayer();
    if (m_nowPlaying) {
        m_nowPlaying->refresh();
    }
}

void MusicPlayerWindow::onPositionChanged(qreal seconds)
{
    Q_UNUSED(seconds);
    if (m_miniPlayer && m_miniPlayer->isVisible()) {
        m_miniPlayer->setValue(m_player.position());
    }
    if (m_nowPlaying && m_stack && m_stack->currentIndex() == 0) {
        m_nowPlaying->refresh();
    }
}

void MusicPlayerWindow::onQueueChanged()
{
    if (m_queuePage) {
        m_queuePage->refresh();
    }
}

void MusicPlayerWindow::showTrackMenu(const QString &trackId, const QPoint &globalPos)
{
    const music::Track track = m_player.track(trackId);
    MdMenu menu(this);
    MdMenuItem *addToQueue = new MdMenuItem(QStringLiteral("Add to queue"), &menu);
    addToQueue->setLeadingIconName(QStringLiteral("playlist_add"));
    MdMenuItem *addToPlaylist = new MdMenuItem(QStringLiteral("Add to playlist"), &menu);
    addToPlaylist->setLeadingIconName(QStringLiteral("library_add"));
    MdMenuItem *goToArtist = new MdMenuItem(QStringLiteral("Go to artist"), &menu);
    goToArtist->setLeadingIconName(QStringLiteral("person"));
    menu.addItem(addToQueue);
    menu.addItem(addToPlaylist);
    menu.addItem(goToArtist);

    connect(addToQueue, &QAbstractButton::clicked, this, [this, trackId](bool) {
        m_player.addToQueue(trackId);
        showSnackbar(QStringLiteral("Added to queue"));
    });
    connect(addToPlaylist, &QAbstractButton::clicked, this, [this](bool) {
        showSnackbar(QStringLiteral("Added to playlist"));
    });
    connect(goToArtist, &QAbstractButton::clicked, this, [this](bool) {
        setCurrentPage(1);
    });

    menu.popup(QRect(globalPos, QSize(180, 0)));
}

void MusicPlayerWindow::confirmRemoveFromQueue(int index)
{
    if (index < 0 || index >= m_player.queue().size()) {
        return;
    }
    const music::Track track = m_player.track(m_player.queue().at(index));
    const QString name = track.title.isEmpty() ? QStringLiteral("this track") : track.title;
    const int result = MdMessageBox::question(
        this, QStringLiteral("Remove from queue?"),
        QStringLiteral("Remove “%1” from the queue?").arg(name),
        MdMessageBoxStandardButton::Yes | MdMessageBoxStandardButton::No);
    if (result == int(MdMessageBoxStandardButton::Yes)) {
        m_player.removeFromQueue(index);
        showSnackbar(QStringLiteral("Removed from queue"));
    }
}

void MusicPlayerWindow::showSnackbar(const QString &message)
{
    if (m_snackbar) {
        m_snackbar->showSnackbar(message);
    }
}

void MusicPlayerWindow::onThemeAction()
{
    MdTheme::instance().toggleThemeMode();
    showSnackbar(MdTheme::instance().themeMode() == ThemeMode::Dark
                     ? QStringLiteral("Dark theme")
                     : QStringLiteral("Light theme"));
}

void MusicPlayerWindow::onSearchSubmitted(const QString &text)
{
    Q_UNUSED(text);
    setCurrentPage(1);
}

void MusicPlayerWindow::layoutMiniPlayer()
{
    if (!m_miniPlayer) {
        return;
    }
    // Sit just above the bottom navigation bar, inset 8 px.
    const int navHeight = m_navBar ? m_navBar->height() : 80;
    const int miniHeight = 72;
    const int y = height() - navHeight - miniHeight - 8;
    if (m_miniPlayer->isVisible()) {
        m_miniPlayer->setGeometry(8, y, width() - 16, miniHeight);
        m_miniPlayer->raise();
        // Keep page content clear of the floating mini player: the stacked
        // widget honours contentsMargins and re-places its pages.
        if (m_stack) {
            m_stack->setContentsMargins(0, 0, 0, miniHeight + 8);
        }
    } else if (m_stack) {
        m_stack->setContentsMargins(0, 0, 0, 0);
    }
}

void MusicPlayerWindow::refreshMiniPlayer()
{
    if (!m_miniPlayer) {
        return;
    }
    const music::Track track = m_player.currentTrack();
    m_miniPlayer->setTitle(track.title.isEmpty() ? QStringLiteral("Nothing playing") : track.title);
    m_miniPlayer->setArtist(track.artist);
    m_miniPlayer->setThumbnail(track.id.isEmpty()
                                   ? QPixmap()
                                   : music::albumArt(track.id, 96));
    m_miniPlayer->setMax(qreal(qMax(1, track.durationSec)));
    m_miniPlayer->setValue(m_player.position());
    m_miniPlayer->setPlaying(m_player.isPlaying());
}
