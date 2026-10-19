#include "MusicPages.h"

#include "MusicPlayerWindow.h"

#include "core/MdCardTokens.h"
#include "core/MdChipTokens.h"
#include "core/MdFont.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdButton.h"
#include "widgets/MdCard.h"
#include "widgets/MdChip.h"
#include "widgets/MdColorPicker.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"
#include "widgets/MdMarquee.h"
#include "widgets/MdReorderList.h"
#include "widgets/MdSearchBar.h"
#include "widgets/MdSeekSlider.h"
#include "widgets/MdSelect.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSwitch.h"
#include "widgets/MdVolumeSlider.h"

#include <QtGui/QContextMenuEvent>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

using namespace md;

namespace {

/// Painted album-art tile using a deterministic gradient (no image files).
class AlbumArtWidget : public QWidget
{
public:
    explicit AlbumArtWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(200, 200);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setAccessibleName(QStringLiteral("album art"));
        setProperty("artSeed", QStringLiteral("default"));
    }

    void setSeed(const QString &seed)
    {
        setProperty("artSeed", seed);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QString seed = property("artSeed").toString();
        const QPixmap art = music::albumArt(seed.isEmpty() ? QStringLiteral("default") : seed,
                                            qMin(width(), height()));
        const int side = qMin(width(), height());
        const QRect target((width() - side) / 2, (height() - side) / 2, side, side);
        painter.setClipRect(target);
        painter.drawPixmap(target, art);

        // Soft vignette using the theme's scrim so the art sits on the surface.
        painter.setClipping(false);
        painter.setOpacity(0.15);
        painter.fillRect(rect(), MdTheme::instance().color(ColorRole::Scrim));
    }
};

QLabel *makeLabel(const QString &text, TypeStyle style, ColorRole role)
{
    QLabel *label = new QLabel(text);
    label->setFont(MdTypeScale::font(style, TypeEmphasis::Baseline,
                                     MdTheme::instance().scriptCategory()));
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, MdTheme::instance().color(role));
    label->setPalette(pal);
    return label;
}

void restyleLabel(QLabel *label, TypeStyle style, ColorRole role)
{
    if (!label) {
        return;
    }
    label->setFont(MdTypeScale::font(style, TypeEmphasis::Baseline,
                                     MdTheme::instance().scriptCategory()));
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, MdTheme::instance().color(role));
    label->setPalette(pal);
}

} // namespace

// ===========================================================================
// NowPlayingPage
// ===========================================================================

NowPlayingPage::NowPlayingPage(MusicPlayerWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    setObjectName(QStringLiteral("now-playing"));
    setAccessibleName(QStringLiteral("Now Playing"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 16, 24, 16);
    root->setSpacing(12);

    m_art = new AlbumArtWidget(this);
    m_art->setMinimumHeight(220);
    root->addWidget(m_art, 3);

    m_title = new MdMarquee(QStringLiteral("Nothing playing"), this);
    m_title->setTypeStyle(TypeStyle::HeadlineSmall);
    m_title->setAccessibleName(QStringLiteral("Track title"));
    root->addWidget(m_title);

    m_artist = makeLabel(QStringLiteral(""), TypeStyle::TitleMedium, ColorRole::OnSurfaceVariant);
    m_artist->setAccessibleName(QStringLiteral("Artist"));
    root->addWidget(m_artist);

    m_seek = new MdSeekSlider(this);
    m_seek->setAccessibleName(QStringLiteral("Playback position"));
    m_seek->setMax(243.0);
    root->addWidget(m_seek);

    m_timeRow = new QWidget(this);
    auto *times = new QHBoxLayout(m_timeRow);
    times->setContentsMargins(0, 0, 0, 0);
    auto *elapsed = makeLabel(QStringLiteral("0:00"), TypeStyle::LabelSmall,
                              ColorRole::OnSurfaceVariant);
    elapsed->setObjectName(QStringLiteral("elapsed"));
    auto *total = makeLabel(QStringLiteral("0:00"), TypeStyle::LabelSmall,
                            ColorRole::OnSurfaceVariant);
    total->setObjectName(QStringLiteral("total"));
    times->addWidget(elapsed);
    times->addStretch(1);
    times->addWidget(total);
    root->addWidget(m_timeRow);

    auto *transport = new QWidget(this);
    auto *row = new QHBoxLayout(transport);
    row->setContentsMargins(0, 4, 0, 4);
    row->setSpacing(8);
    row->addStretch(1);

    m_shuffle = new MdIconButton(QStringLiteral("shuffle"), this);
    m_shuffle->setVariant(IconButtonVariant::Standard);
    m_shuffle->setCheckable(true);
    m_shuffle->setAccessibleName(QStringLiteral("Shuffle"));
    m_prev = new MdIconButton(QStringLiteral("skip_previous"), this);
    m_prev->setVariant(IconButtonVariant::Standard);
    m_prev->setButtonSize(ButtonSize::Large);
    m_prev->setAccessibleName(QStringLiteral("Previous track"));
    m_play = new MdIconButton(QStringLiteral("play_arrow"), this);
    m_play->setVariant(IconButtonVariant::Filled);
    m_play->setButtonSize(ButtonSize::Large);
    m_play->setAccessibleName(QStringLiteral("Play"));
    m_next = new MdIconButton(QStringLiteral("skip_next"), this);
    m_next->setVariant(IconButtonVariant::Standard);
    m_next->setButtonSize(ButtonSize::Large);
    m_next->setAccessibleName(QStringLiteral("Next track"));
    m_repeat = new MdIconButton(QStringLiteral("repeat"), this);
    m_repeat->setVariant(IconButtonVariant::Standard);
    m_repeat->setCheckable(true);
    m_repeat->setAccessibleName(QStringLiteral("Repeat"));

    row->addWidget(m_shuffle);
    row->addWidget(m_prev);
    row->addWidget(m_play);
    row->addWidget(m_next);
    row->addWidget(m_repeat);
    row->addStretch(1);
    root->addWidget(transport);

    auto *volumeRow = new QWidget(this);
    auto *vol = new QHBoxLayout(volumeRow);
    vol->setContentsMargins(0, 0, 0, 0);
    m_volume = new MdVolumeSlider(this);
    m_volume->setAccessibleName(QStringLiteral("Volume"));
    vol->addWidget(m_volume, 1);
    m_quality = new MdChip(QStringLiteral("High"), this);
    m_quality->setVariant(MdChipVariant::Assist);
    m_quality->setAccessibleName(QStringLiteral("Audio quality"));
    vol->addWidget(m_quality);
    root->addWidget(volumeRow);

    connect(m_play, &QAbstractButton::clicked, this, [this](bool) {
        m_window->player()->togglePlay();
    });
    connect(m_prev, &QAbstractButton::clicked, this, [this](bool) {
        m_window->player()->previous();
    });
    connect(m_next, &QAbstractButton::clicked, this, [this](bool) {
        m_window->player()->next();
    });
    connect(m_shuffle, &QAbstractButton::toggled, this, [this](bool on) {
        m_window->player()->setShuffle(on);
    });
    connect(m_repeat, &QAbstractButton::toggled, this, [this](bool on) {
        m_window->player()->setRepeatMode(on ? 1 : 0);
    });
    connect(m_seek, &MdSeekSlider::sliderMoved, this, &NowPlayingPage::onSeek);
    connect(m_volume, &MdVolumeSlider::valueChanged, this, &NowPlayingPage::onVolume);
    connect(m_volume, &MdVolumeSlider::muteToggled, this, [this](bool muted) {
        m_window->player()->setMuted(muted);
    });

    // Tooltips on the transport icons.
    m_play->setToolTip(QStringLiteral("Play / pause"));
    m_prev->setToolTip(QStringLiteral("Previous"));
    m_next->setToolTip(QStringLiteral("Next"));
    m_shuffle->setToolTip(QStringLiteral("Shuffle"));
    m_repeat->setToolTip(QStringLiteral("Repeat"));

    MdStyleBase::connectThemeUpdate(this, &NowPlayingPage::refresh);
    refresh();
}

void NowPlayingPage::refresh()
{
    const music::Track track = m_window->player()->currentTrack();
    const QString title = track.title.isEmpty() ? QStringLiteral("Nothing playing") : track.title;
    m_title->setText(title);
    static_cast<QLabel *>(m_artist)->setText(track.artist.isEmpty()
                                                 ? QStringLiteral("Pick a track from the library")
                                                 : QStringLiteral("%1 — %2").arg(track.artist,
                                                                                  track.album));
    restyleLabel(static_cast<QLabel *>(m_artist), TypeStyle::TitleMedium,
                 ColorRole::OnSurfaceVariant);

    if (m_art) {
        m_art->setProperty("artSeed", track.id.isEmpty() ? QStringLiteral("empty") : track.id);
        m_art->update();
    }

    m_seek->setMax(qreal(qMax(1, track.durationSec)));
    m_seek->setValue(m_window->player()->position());
    m_volume->setValue(m_window->player()->volume());
    m_volume->setMuted(m_window->player()->isMuted());

    const QString quality = track.quality.isEmpty() ? QStringLiteral("High") : track.quality;
    m_quality->setText(quality);

    const bool playing = m_window->player()->isPlaying();
    m_play->setIconName(playing ? QStringLiteral("pause") : QStringLiteral("play_arrow"));
    m_play->setAccessibleName(playing ? QStringLiteral("Pause") : QStringLiteral("Play"));
    m_shuffle->setChecked(m_window->player()->shuffle());
    m_repeat->setChecked(m_window->player()->repeatMode() != 0);

    // Time labels.
    const QList<QLabel *> labels = m_timeRow->findChildren<QLabel *>();
    if (labels.size() >= 2) {
        labels.at(0)->setText(MdSeekSlider::formatTime(m_window->player()->position()));
        labels.at(1)->setText(MdSeekSlider::formatTime(qreal(qMax(1, track.durationSec))));
    }
}

void NowPlayingPage::onSeek(qreal seconds)
{
    m_window->player()->seek(seconds);
}

void NowPlayingPage::onVolume(qreal value)
{
    m_window->player()->setVolume(value);
}

void NowPlayingPage::layoutContents()
{
    // Layout is owned by the QVBoxLayout; nothing to do here.
}

// ===========================================================================
// LibraryPage
// ===========================================================================

LibraryPage::LibraryPage(MusicPlayerWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    setObjectName(QStringLiteral("library"));
    setAccessibleName(QStringLiteral("Library"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 12);
    root->setSpacing(12);

    m_search = new MdSearchBar(this);
    m_search->setPlaceholderText(QStringLiteral("Search albums, artists, playlists"));
    m_search->setAccessibleName(QStringLiteral("Search library"));
    root->addWidget(m_search);

    m_filterRow = new QWidget(this);
    auto *filters = new QHBoxLayout(m_filterRow);
    filters->setContentsMargins(0, 0, 0, 0);
    filters->setSpacing(8);
    m_filterAll = new MdChip(QStringLiteral("All"), this);
    m_filterAll->setVariant(MdChipVariant::Filter);
    m_filterAll->setChecked(true);
    m_filterAlbums = new MdChip(QStringLiteral("Albums"), this);
    m_filterAlbums->setVariant(MdChipVariant::Filter);
    m_filterPlaylists = new MdChip(QStringLiteral("Playlists"), this);
    m_filterPlaylists->setVariant(MdChipVariant::Filter);
    m_filterLossless = new MdChip(QStringLiteral("Lossless"), this);
    m_filterLossless->setVariant(MdChipVariant::Filter);
    filters->addWidget(m_filterAll);
    filters->addWidget(m_filterAlbums);
    filters->addWidget(m_filterPlaylists);
    filters->addWidget(m_filterLossless);
    filters->addStretch(1);
    root->addWidget(m_filterRow);

    m_featured = new MdCard(this);
    m_featured->setVariant(MdCardVariant::Filled);
    m_featured->setClickable(true);
    m_featured->setMinimumHeight(96);
    m_featured->setAccessibleName(QStringLiteral("Featured playlist"));
    auto *featLayout = new QVBoxLayout(m_featured);
    featLayout->setContentsMargins(16, 12, 16, 12);
    auto *featTitle = makeLabel(QStringLiteral("Featured — Night Drive"), TypeStyle::TitleLarge,
                                ColorRole::OnSurface);
    auto *featSub = makeLabel(QStringLiteral("5 tracks · synthwave and electronic"),
                              TypeStyle::BodyMedium, ColorRole::OnSurfaceVariant);
    featLayout->addWidget(featTitle);
    featLayout->addWidget(featSub);
    root->addWidget(m_featured);

    m_list = new MdList(this);
    m_list->setSelectionMode(MdList::SelectionMode::Single);
    root->addWidget(m_list, 1);

    connect(m_search, &QLineEdit::textChanged, this, &LibraryPage::applyFilter);
    connect(m_filterAll, &QAbstractButton::toggled, this, &LibraryPage::onFilterChipToggled);
    connect(m_filterAlbums, &QAbstractButton::toggled, this, &LibraryPage::onFilterChipToggled);
    connect(m_filterPlaylists, &QAbstractButton::toggled, this, &LibraryPage::onFilterChipToggled);
    connect(m_filterLossless, &QAbstractButton::toggled, this, &LibraryPage::onFilterChipToggled);
    connect(m_list, &MdList::itemActivated, this, &LibraryPage::onPlaylistActivated);
    connect(m_featured, &MdCard::clicked, this, [this]() {
        m_window->player()->playPlaylist(QStringLiteral("p2"));
    });

    MdStyleBase::connectThemeUpdate(this, &LibraryPage::refresh);
    rebuildList(QString());
}

void LibraryPage::refresh()
{
    rebuildList(m_filter);
}

void LibraryPage::applyFilter(const QString &text)
{
    m_filter = text;
    rebuildList(text);
}

void LibraryPage::onPlaylistActivated(MdListItem *item)
{
    if (!item) {
        return;
    }
    const QString id = item->property("playlistId").toString();
    if (!id.isEmpty()) {
        m_window->player()->playPlaylist(id);
    }
}

void LibraryPage::onFilterChipToggled()
{
    // Exclusive-ish filter row: the last checked chip wins.
    const QList<MdChip *> chips{m_filterAll, m_filterAlbums, m_filterPlaylists, m_filterLossless};
    MdChip *source = qobject_cast<MdChip *>(sender());
    for (MdChip *chip : chips) {
        if (chip && chip != source && chip->isChecked()) {
            QSignalBlocker blocker(chip);
            chip->setChecked(false);
        }
    }
    if (source && !source->isChecked()) {
        QSignalBlocker blocker(m_filterAll);
        m_filterAll->setChecked(true);
    }
    rebuildList(m_filter);
}

void LibraryPage::contextMenuEvent(QContextMenuEvent *event)
{
    // Right-click a playlist / album row to open the track context menu.
    const QPoint listPos = m_list->mapFromGlobal(event->globalPos());
    const QList<MdListItem *> items = m_list->items();
    for (MdListItem *item : items) {
        if (item->geometry().contains(listPos)) {
            QString trackId = item->property("playlistId").toString();
            if (trackId.startsWith(QLatin1String("album:"))) {
                const QString album = trackId.mid(6);
                for (const music::Track &t : m_window->player()->catalogue()) {
                    if (t.album == album) {
                        trackId = t.id;
                        break;
                    }
                }
            } else {
                const QList<QString> ids = m_window->player()->playlistTrackIds(trackId);
                trackId = ids.isEmpty() ? QString() : ids.first();
            }
            if (!trackId.isEmpty()) {
                m_window->showTrackMenu(trackId, event->globalPos());
            }
            return;
        }
    }
}

void LibraryPage::rebuildList(const QString &filter)
{
    // Clear existing items.
    const QList<MdListItem *> old = m_list->items();
    for (MdListItem *item : old) {
        m_list->removeItem(item);
        item->deleteLater();
    }

    const QString needle = filter.trimmed().toLower();
    const bool losslessOnly = m_filterLossless && m_filterLossless->isChecked();
    const bool albumsOnly = m_filterAlbums && m_filterAlbums->isChecked();
    const bool playlistsOnly = m_filterPlaylists && m_filterPlaylists->isChecked();

    // Playlists first (Library / Playlists screen).
    if (!albumsOnly) {
        for (const music::Playlist &p : m_window->player()->playlists()) {
            if (playlistsOnly && p.id.isEmpty()) {
                continue;
            }
            if (!needle.isEmpty()
                && !p.name.toLower().contains(needle)
                && !p.description.toLower().contains(needle)) {
                continue;
            }
            auto *item = new MdListItem(p.name);
            item->setInteractive(true);
            item->setSupporting(p.description);
            item->setTrailingSupporting(QStringLiteral("%1 tracks").arg(p.trackIds.size()));
            item->setLeadingIcon(QStringLiteral("queue_music"));
            item->setProperty("playlistId", p.id);
            m_list->addItem(item);
        }
    }

    // Album entries derived from the catalogue.
    if (!playlistsOnly) {
        QHash<QString, music::Track> albumFirst;
        for (const music::Track &t : m_window->player()->catalogue()) {
            if (losslessOnly && t.quality != QLatin1String("Lossless")) {
                continue;
            }
            if (!albumFirst.contains(t.album)) {
                albumFirst.insert(t.album, t);
            }
        }
        for (auto it = albumFirst.constBegin(); it != albumFirst.constEnd(); ++it) {
            const music::Track &t = it.value();
            if (!needle.isEmpty()
                && !t.album.toLower().contains(needle)
                && !t.artist.toLower().contains(needle)) {
                continue;
            }
            auto *item = new MdListItem(t.album);
            item->setInteractive(true);
            item->setSupporting(t.artist);
            item->setTrailingSupporting(t.quality);
            item->setLeadingIcon(QStringLiteral("album"));
            item->setProperty("playlistId", QStringLiteral("album:%1").arg(t.album));
            // Album activation plays every track of that album.
            m_list->addItem(item);
        }
    }
}

// ===========================================================================
// QueuePage
// ===========================================================================

QueuePage::QueuePage(MusicPlayerWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    setObjectName(QStringLiteral("queue"));
    setAccessibleName(QStringLiteral("Queue"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 12);
    root->setSpacing(12);

    auto *header = makeLabel(QStringLiteral("Up next"), TypeStyle::HeadlineSmall, ColorRole::OnSurface);
    root->addWidget(header);

    m_toolRow = new QWidget(this);
    auto *tools = new QHBoxLayout(m_toolRow);
    tools->setContentsMargins(0, 0, 0, 0);
    tools->setSpacing(8);
    m_moveUp = new MdIconButton(QStringLiteral("arrow_upward"), this);
    m_moveUp->setAccessibleName(QStringLiteral("Move up"));
    m_moveUp->setToolTip(QStringLiteral("Move up"));
    m_moveDown = new MdIconButton(QStringLiteral("arrow_downward"), this);
    m_moveDown->setAccessibleName(QStringLiteral("Move down"));
    m_moveDown->setToolTip(QStringLiteral("Move down"));
    m_remove = new MdIconButton(QStringLiteral("close"), this);
    m_remove->setAccessibleName(QStringLiteral("Remove from queue"));
    m_remove->setToolTip(QStringLiteral("Remove"));
    m_clearButton = new MdButton(QStringLiteral("Clear queue"), this);
    m_clearButton->setVariant(ButtonVariant::Text);
    tools->addWidget(m_moveUp);
    tools->addWidget(m_moveDown);
    tools->addWidget(m_remove);
    tools->addStretch(1);
    tools->addWidget(m_clearButton);
    root->addWidget(m_toolRow);

    m_list = new MdReorderList(this);
    m_list->setSelectionMode(MdList::SelectionMode::Single);
    m_list->setDragEnabled(true);
    m_list->setMoveButtonsVisible(false);
    root->addWidget(m_list, 1);

    connect(m_moveUp, &QAbstractButton::clicked, m_list, &MdReorderList::moveUp);
    connect(m_moveDown, &QAbstractButton::clicked, m_list, &MdReorderList::moveDown);
    connect(m_remove, &QAbstractButton::clicked, this, &QueuePage::onRemoveClicked);
    connect(m_clearButton, &QAbstractButton::clicked, this, [this](bool) {
        m_window->player()->clearQueue();
    });
    connect(m_list, &MdReorderList::itemMoved, this, &QueuePage::onItemMoved);

    MdStyleBase::connectThemeUpdate(this, &QueuePage::refresh);
    rebuildList();
}

void QueuePage::refresh()
{
    rebuildList();
}

void QueuePage::onItemMoved(int from, int to)
{
    m_window->player()->moveInQueue(from, to);
}

void QueuePage::onRemoveClicked()
{
    const QList<MdListItem *> selected = m_list->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const int index = m_list->items().indexOf(selected.first());
    if (index >= 0) {
        m_window->confirmRemoveFromQueue(index);
    }
}

void QueuePage::contextMenuEvent(QContextMenuEvent *event)
{
    const QPoint listPos = m_list->mapFromGlobal(event->globalPos());
    const QList<MdListItem *> items = m_list->items();
    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i)->geometry().contains(listPos)) {
            const QList<QString> queue = m_window->player()->queue();
            if (i < queue.size()) {
                m_window->showTrackMenu(queue.at(i), event->globalPos());
            }
            return;
        }
    }
}

void QueuePage::rebuildList()
{
    const QList<MdListItem *> old = m_list->items();
    for (MdListItem *item : old) {
        m_list->removeItem(item);
        item->deleteLater();
    }

    const QList<QString> queue = m_window->player()->queue();
    const QString currentId = m_window->player()->currentTrackId();
    for (int i = 0; i < queue.size(); ++i) {
        const music::Track t = m_window->player()->track(queue.at(i));
        auto *item = new MdListItem(t.title.isEmpty() ? queue.at(i) : t.title);
        item->setInteractive(true);
        item->setSupporting(t.artist);
        item->setTrailingSupporting(MdSeekSlider::formatTime(qreal(t.durationSec)));
        item->setLeadingIcon(QStringLiteral("music_note"));
        if (t.id == currentId) {
            item->setSelected(true);
            item->setLeadingIcon(QStringLiteral("graphic_eq"));
        }
        m_list->addItem(item);
    }
}

// ===========================================================================
// SettingsPage
// ===========================================================================

SettingsPage::SettingsPage(MusicPlayerWindow *window, QWidget *parent)
    : QWidget(parent)
    , m_window(window)
{
    setObjectName(QStringLiteral("settings"));
    setAccessibleName(QStringLiteral("Settings"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 16);
    root->setSpacing(16);

    auto *appearance = makeLabel(QStringLiteral("Appearance"), TypeStyle::TitleMedium,
                                 ColorRole::OnSurface);
    root->addWidget(appearance);

    m_themeSelect = new MdSelect(this);
    m_themeSelect->setLabelText(QStringLiteral("Theme"));
    m_themeSelect->setAccessibleName(QStringLiteral("Theme mode"));
    m_themeSelect->addItem(QStringLiteral("Light"));
    m_themeSelect->addItem(QStringLiteral("Dark"));
    m_themeSelect->addItem(QStringLiteral("System"));
    root->addWidget(m_themeSelect);

    m_seedRow = new QWidget(this);
    auto *seedLayout = new QVBoxLayout(m_seedRow);
    seedLayout->setContentsMargins(0, 0, 0, 0);
    seedLayout->addWidget(makeLabel(QStringLiteral("Seed colour"), TypeStyle::LabelLarge,
                                    ColorRole::OnSurfaceVariant));
    m_seedPicker = new MdColorPicker(this);
    m_seedPicker->setAccessibleName(QStringLiteral("Seed colour"));
    seedLayout->addWidget(m_seedPicker);
    root->addWidget(m_seedRow);

    m_highContrast = new MdSwitch(QStringLiteral("High contrast"), this);
    m_highContrast->setAccessibleName(QStringLiteral("High contrast"));
    root->addWidget(m_highContrast);

    auto *audio = makeLabel(QStringLiteral("Audio"), TypeStyle::TitleMedium, ColorRole::OnSurface);
    root->addWidget(audio);

    m_qualitySelect = new MdSelect(this);
    m_qualitySelect->setLabelText(QStringLiteral("Streaming quality"));
    m_qualitySelect->setAccessibleName(QStringLiteral("Audio quality"));
    m_qualitySelect->addItem(QStringLiteral("Low — 96 kbps"));
    m_qualitySelect->addItem(QStringLiteral("Standard — 160 kbps"));
    m_qualitySelect->addItem(QStringLiteral("High — 320 kbps"));
    m_qualitySelect->addItem(QStringLiteral("Lossless"));
    m_qualitySelect->setCurrentIndex(2);
    root->addWidget(m_qualitySelect);

    auto *crossLabel = makeLabel(QStringLiteral("Crossfade"), TypeStyle::LabelLarge,
                                 ColorRole::OnSurfaceVariant);
    root->addWidget(crossLabel);
    m_crossfade = new MdSlider(this);
    m_crossfade->setMin(0.0);
    m_crossfade->setMax(12.0);
    m_crossfade->setValue(3.0);
    m_crossfade->setStep(1.0);
    m_crossfade->setLabeled(true);
    m_crossfade->setAccessibleName(QStringLiteral("Crossfade seconds"));
    root->addWidget(m_crossfade);

    m_normalize = new MdSwitch(QStringLiteral("Volume normalisation"), this);
    m_normalize->setChecked(true);
    m_normalize->setAccessibleName(QStringLiteral("Volume normalisation"));
    root->addWidget(m_normalize);

    m_gapless = new MdSwitch(QStringLiteral("Gapless playback"), this);
    m_gapless->setChecked(true);
    m_gapless->setAccessibleName(QStringLiteral("Gapless playback"));
    root->addWidget(m_gapless);

    root->addStretch(1);

    connect(m_themeSelect, &MdSelect::currentIndexChanged, this, &SettingsPage::onThemeChanged);
    connect(m_seedPicker, &MdColorPicker::colorChanged, this, &SettingsPage::onSeedChanged);
    connect(m_highContrast, &QAbstractButton::toggled, this, &SettingsPage::onHighContrast);
    connect(m_qualitySelect, &MdSelect::currentIndexChanged, this, &SettingsPage::onQualityChanged);
    connect(m_crossfade, &MdSlider::valueChanged, this, &SettingsPage::onCrossfade);
    connect(m_normalize, &QAbstractButton::toggled, this, &SettingsPage::onNormalize);
    connect(m_gapless, &QAbstractButton::toggled, this, &SettingsPage::onGapless);

    MdStyleBase::connectThemeUpdate(this, &SettingsPage::refresh);
    refresh();
}

void SettingsPage::refresh()
{
    const MdTheme &theme = MdTheme::instance();
    const int index = theme.themeMode() == ThemeMode::Dark
                          ? 1
                          : (theme.themeModePreference() == ThemeMode::System ? 2 : 0);
    if (m_themeSelect->currentIndex() != index) {
        QSignalBlocker blocker(m_themeSelect);
        m_themeSelect->setCurrentIndex(index);
    }
    if (m_seedPicker->color() != theme.seedColor()) {
        QSignalBlocker blocker(m_seedPicker);
        m_seedPicker->setColor(theme.seedColor());
    }
    QSignalBlocker contrastBlocker(m_highContrast);
    m_highContrast->setChecked(theme.isHighContrast());
}

void SettingsPage::onThemeChanged(int index)
{
    MdTheme &theme = MdTheme::instance();
    if (index == 0) {
        theme.setThemeMode(ThemeMode::Light);
    } else if (index == 1) {
        theme.setThemeMode(ThemeMode::Dark);
    } else {
        theme.setThemeMode(ThemeMode::System);
    }
}

void SettingsPage::onSeedChanged(const QColor &color)
{
    if (color.isValid()) {
        MdTheme::instance().setSeedColor(color);
    }
}

void SettingsPage::onHighContrast(bool on)
{
    MdTheme::instance().setHighContrast(on);
}

void SettingsPage::onQualityChanged(int index)
{
    Q_UNUSED(index);
}

void SettingsPage::onCrossfade(qreal value)
{
    Q_UNUSED(value);
}

void SettingsPage::onNormalize(bool on)
{
    Q_UNUSED(on);
}

void SettingsPage::onGapless(bool on)
{
    Q_UNUSED(on);
}
