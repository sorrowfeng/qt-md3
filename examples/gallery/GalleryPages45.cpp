// Gallery page 45: showcase - Music player.
//
// A now-playing screen built like an official MD3 media demo: a full-bleed
// painted album-art block, Display/Headline track meta, a progress slider
// with times, a large play FAB among small transport icons, playlist chips
// and an up-next list. Real Md* widgets throughout; the art is a tonal
// gradient painted from theme tokens.

#include "GalleryPages.h"

#include "core/MdMotion.h"
#include "widgets/MdChip.h"
#include "widgets/MdFab.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"
#include "widgets/MdSlider.h"

#include "I18n.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QTimer>
#include <QtCore/QVector>

#include <cmath>

namespace gallery {

namespace {

struct Slot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

constexpr qreal kScreenHeight = 920.0;
constexpr qreal kArtHeight = 300.0;

struct Track
{
    const char *id;
    const char *titleZh;
    const char *titleEn;
    const char *artistZh;
    const char *artistEn;
    const char *timeZh;
    const char *timeEn;
};

const Track kUpNext[] = {
    {"t1", "夜曲", "Nocturne", "古星乐队", "Pale Ensemble", "3:42", "3:42"},
    {"t2", "雨巷", "Rain Alley", "城市回音", "City Echoes", "4:05", "4:05"},
    {"t3", "远线", "Far Line", "北纬组合", "Latitude", "2:58", "2:58"},
};

} // namespace

class ShowcaseMusicPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit ShowcaseMusicPage(QWidget *parent = nullptr);
    ~ShowcaseMusicPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void layoutChildren();
    void placeChildren();
    void animatePlayScaleTo(qreal target);

    md::MdIconButton *iconButton(const QString &id, const QString &icon);
    md::MdFab *transportFab();
    md::MdSlider *progressSlider();
    md::MdChip *chip(const QString &id, const QString &label);
    md::MdList *upNextList();
    md::MdListItem *trackItem(const QString &id);

    QHash<QString, md::MdIconButton *> m_iconButtons;
    md::MdFab *m_playFab = nullptr;
    md::MdSlider *m_slider = nullptr;
    QHash<QString, md::MdChip *> m_chips;
    md::MdList *m_list = nullptr;
    QHash<QString, md::MdListItem *> m_items;
    QVector<Slot> m_slots;
    /// Ids whose interaction state has already been seeded from the scene.
    /// build() re-runs on every measure/paint and must not clobber a click.
    QSet<QString> m_stateSeeded;
    /// Interruptible press scale on the play FAB (0.96 pressed -> 1.0 rest),
    /// driven by the SpatialFast spring. Retargets from the on-screen scale.
    qreal m_playScale = 1.0;
    qreal m_playScaleFrom = 1.0;
    qreal m_playScaleTo = 1.0;
    QElapsedTimer m_playScaleClock;
    QTimer m_playScaleTimer;
};

ShowcaseMusicPage::ShowcaseMusicPage(QWidget *parent)
    : GalleryPage(parent)
{
    // Shared-element feel: the play control scales 0.96 -> 1.0 on the
    // SpatialFast spring. Interrupting a press/release retargets from the
    // on-screen scale (m_playScale), never jumps. Velocity is not carried —
    // MdSpring::valueAt is released-from-rest (see MdMotion.h).
    m_playScaleTimer.setInterval(16);
    m_playScaleTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_playScaleTimer, &QTimer::timeout, this, [this] {
        const md::MdSpring spring = md::MdMotion::spring(md::MotionSpring::SpatialFast);
        const qreal seconds = qreal(m_playScaleClock.elapsed()) / 1000.0;
        const qreal t = qBound<qreal>(0.0, spring.valueAt(seconds), 1.0);
        m_playScale = m_playScaleFrom + (m_playScaleTo - m_playScaleFrom) * t;
        placeChildren();
        if (t >= 0.999 || m_playScaleClock.elapsed()
            >= int(spring.settlingDurationMs())) {
            m_playScale = m_playScaleTo;
            m_playScaleTimer.stop();
            placeChildren();
        }
    });
}

ShowcaseMusicPage::~ShowcaseMusicPage() = default;

QString ShowcaseMusicPage::title() const
{
    return L("示例·音乐", "Showcase·Music");
}

QString ShowcaseMusicPage::slug() const
{
    return QStringLiteral("showcase-music");
}

QString ShowcaseMusicPage::subtitle() const
{
    return L("正在播放：全幅专辑艺术、大号曲名、进度与传输控制、播放列表芯片与待播队列。",
             "Now playing: full-bleed album art, large track title, scrubber and "
             "transport, playlist chips and an up-next queue.");
}

md::MdIconButton *ShowcaseMusicPage::iconButton(const QString &id, const QString &icon)
{
    const auto found = m_iconButtons.constFind(id);
    if (found != m_iconButtons.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdIconButton(icon, this);
    created->setObjectName(id);
    created->hide();
    m_iconButtons.insert(id, created);
    return created;
}

md::MdFab *ShowcaseMusicPage::transportFab()
{
    if (m_playFab != nullptr) {
        return m_playFab;
    }
    // Primary play: the one filled large control, so the screen spends its
    // colour ceremony on the action that matters.
    m_playFab = new md::MdFab(QStringLiteral("play_arrow"), this);
    m_playFab->setObjectName(QStringLiteral("play"));
    m_playFab->setVariant(md::FabVariant::Primary);
    m_playFab->setFabSize(md::FabSize::Large);
    // The control is live: click toggles play/pause so the demo is not dead.
    // MdFab's own press response is the ripple; the icon swap is the shared-
    // element feel the music screen is supposed to show.
    connect(m_playFab, &QAbstractButton::clicked, this, [this] {
        const bool playing = m_playFab->iconName() == QStringLiteral("pause");
        m_playFab->setIconName(playing ? QStringLiteral("play_arrow")
                                       : QStringLiteral("pause"));
    });
    // Interruptible SpatialFast press scale (0.96 -> 1.0), shared-element feel.
    connect(m_playFab, &QAbstractButton::pressed, this, [this] {
        animatePlayScaleTo(0.96);
    });
    connect(m_playFab, &QAbstractButton::released, this, [this] {
        animatePlayScaleTo(1.0);
    });
    m_playFab->hide();
    return m_playFab;
}

void ShowcaseMusicPage::animatePlayScaleTo(qreal target)
{
    if (m_playScaleTimer.isActive() && qFuzzyCompare(m_playScaleTo, target)) {
        return;
    }
    m_playScaleFrom = m_playScale;
    m_playScaleTo = target;
    if (qFuzzyCompare(m_playScaleFrom, m_playScaleTo)) {
        m_playScale = target;
        m_playScaleTimer.stop();
        placeChildren();
        return;
    }
    m_playScaleClock.restart();
    m_playScaleTimer.start();
}

md::MdSlider *ShowcaseMusicPage::progressSlider()
{
    if (m_slider == nullptr) {
        m_slider = new md::MdSlider(this);
        m_slider->setObjectName(QStringLiteral("scrubber"));
        m_slider->setMin(0.0);
        m_slider->setMax(100.0);
        m_slider->setValue(32.0);
        m_slider->hide();
    }
    return m_slider;
}

md::MdChip *ShowcaseMusicPage::chip(const QString &id, const QString &label)
{
    const auto found = m_chips.constFind(id);
    if (found != m_chips.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdChip(label, this);
    created->setObjectName(id);
    created->setVariant(md::MdChipVariant::Filter);
    created->hide();
    m_chips.insert(id, created);
    return created;
}

md::MdList *ShowcaseMusicPage::upNextList()
{
    if (m_list == nullptr) {
        m_list = new md::MdList(this);
        m_list->setSelectionMode(md::MdList::SelectionMode::Single);
    }
    return m_list;
}

md::MdListItem *ShowcaseMusicPage::trackItem(const QString &id)
{
    const auto found = m_items.constFind(id);
    if (found != m_items.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdListItem(this);
    created->setInteractive(true);
    created->setLeadingKind(md::MdListItem::LeadingKind::Avatar);
    m_items.insert(id, created);
    return created;
}

void ShowcaseMusicPage::build(GalleryContext &context)
{
    m_slots.clear();
    QPainter *painter = context.painter();

    context.paragraph(L(
        "拖动滑块可跳转，点主播放键与芯片可交互。专辑艺术是令牌渐变，不是图片。",
        "Drag the scrubber to seek; the play control and chips are live. "
        "Album art is a token gradient, not a bitmap."));

    const QRectF screen = context.band(kScreenHeight);
    const qreal left = screen.left();
    const qreal width = screen.width();
    const qreal centre = left + width / 2.0;

    if (painter != nullptr) {
        paintTonalPanel(*painter, screen, md::ColorRole::SurfaceContainerLow,
                        md::ShapeCorner::ExtraLarge);
    }

    // --- full-bleed album art --------------------------------------------
    // Expressive corner on a hero surface: tertiary -> primary gradient with
    // a music glyph, the imagery stand-in official demos use when photos
    // cannot ship.
    const QRectF art(left + 16.0, screen.top() + 16.0, width - 32.0, kArtHeight);
    if (painter != nullptr) {
        paintGradientArt(*painter, art, md::ColorRole::TertiaryContainer,
                         md::ColorRole::PrimaryContainer, md::ShapeCorner::ExtraLarge,
                         QStringLiteral("music_note"));

        // Small "Hi-Res" pill over the art, a product detail.
        painter->save();
        const QFont pillFont = fontFor(md::TypeStyle::LabelSmall);
        const QFontMetricsF pillMetrics(pillFont);
        const QString pillText = QStringLiteral("Hi-Res Lossless");
        const qreal pillW = pillMetrics.horizontalAdvance(pillText) + 20.0;
        const QRectF pill(art.left() + 16.0, art.top() + 16.0, pillW, 28.0);
        fillRounded(*painter, pill, role(md::ColorRole::SurfaceContainerHighest),
                    md::ShapeCorner::Full);
        painter->setFont(pillFont);
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(pill, Qt::AlignCenter, pillText);
        painter->restore();
    }
    qreal y = art.bottom() + 24.0;

    // --- track meta: Display title + artist -------------------------------
    if (painter != nullptr) {
        painter->save();
        const QFont titleFont = fontFor(md::TypeStyle::DisplaySmall);
        painter->setFont(titleFont);
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(QRectF(left + 28.0, y, width - 56.0, 56.0),
                          Qt::AlignLeft | Qt::AlignVCenter, L("后弦", "Afterglow"));

        const QFont artistFont = fontFor(md::TypeStyle::TitleMedium);
        painter->setFont(artistFont);
        painter->setPen(role(md::ColorRole::OnSurfaceVariant));
        painter->drawText(QRectF(left + 28.0, y + 56.0, width - 56.0, 28.0),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          L("北纬组合 · 远线现场", "Latitude · Far Line Live"));
        painter->restore();
    }
    y += 100.0;

    // --- progress slider + times -----------------------------------------
    md::MdSlider *slider = progressSlider();
    const qreal sliderHeight = slider->sizeHint().height();
    m_slots.append(Slot{slider, QRectF(left + 28.0, y, width - 56.0, sliderHeight)});
    if (painter != nullptr) {
        painter->save();
        const QFont timeFont = fontFor(md::TypeStyle::LabelSmall);
        painter->setFont(timeFont);
        painter->setPen(role(md::ColorRole::OnSurfaceVariant));
        painter->drawText(QRectF(left + 28.0, y + sliderHeight, 80.0, 20.0),
                          Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("1:12"));
        painter->drawText(QRectF(left + width - 28.0 - 80.0, y + sliderHeight, 80.0, 20.0),
                          Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("-2:30"));
        painter->restore();
    }
    y += sliderHeight + 28.0;

    // --- transport: big play FAB + small icons ---------------------------
    struct Transport
    {
        const char *id;
        const char *icon;
        bool primary;
    };
    const Transport transport[] = {
        {"shuffle", "shuffle", false},
        {"prev", "skip_previous", false},
        {"play", "play_arrow", true},
        {"next", "skip_next", false},
        {"repeat", "repeat", false},
    };
    const qreal buttonPitch = 72.0;
    const qreal rowWidth = 5.0 * buttonPitch;
    qreal x = centre - rowWidth / 2.0;
    for (const Transport &t : transport) {
        if (t.primary) {
            md::MdFab *button = transportFab();
            const QSize hint = button->sizeHint();
            m_slots.append(Slot{button,
                                QRectF(x + (buttonPitch - qreal(hint.width())) / 2.0, y,
                                       qreal(hint.width()), qreal(hint.height()))});
        } else {
            md::MdIconButton *button =
                iconButton(QString::fromUtf8(t.id), QString::fromUtf8(t.icon));
            button->setVariant(md::IconButtonVariant::Standard);
            button->setButtonSize(md::ButtonSize::Medium);
            const QSize hint = button->sizeHint();
            m_slots.append(Slot{button,
                                QRectF(x + (buttonPitch - qreal(hint.width())) / 2.0, y,
                                       qreal(hint.width()), qreal(hint.height()))});
        }
        x += buttonPitch;
    }
    y += 100.0;

    // --- playlist chips ---------------------------------------------------
    const qreal chipHeight = 32.0;
    {
        struct ChipEntry
        {
            const char *id;
            const char *zh;
            const char *en;
            bool selected;
        };
        const ChipEntry entries[] = {
            {"chill", "放松", "Chill", true},
            {"focus", "专注", "Focus", false},
            {"sport", "运动", "Workout", false},
            {"fav", "收藏", "Favorites", false},
        };
        qreal chipX = left + 28.0;
        for (const ChipEntry &entry : entries) {
            const QString id = QString::fromUtf8(entry.id);
            md::MdChip *c = chip(id, L(entry.zh, entry.en));
            if (!m_stateSeeded.contains(id)) {
                c->setChecked(entry.selected);
                m_stateSeeded.insert(id);
            }
            const qreal w = qMax<qreal>(c->sizeHint().width(), 72.0);
            m_slots.append(Slot{c, QRectF(chipX, y, w, chipHeight)});
            chipX += w + 8.0;
        }
    }
    y += chipHeight + 28.0;

    // --- up next on a nested panel ---------------------------------------
    if (painter != nullptr) {
        painter->save();
        const QFont headingFont = fontFor(md::TypeStyle::TitleLarge);
        painter->setFont(headingFont);
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(QRectF(left + 28.0, y, width - 56.0, 36.0),
                          Qt::AlignLeft | Qt::AlignVCenter, L("接下来", "Up next"));
        painter->restore();
    }
    y += 44.0;

    md::MdList *list = upNextList();
    if (list->itemCount() == 0) {
        for (const Track &track : kUpNext) {
            list->addItem(trackItem(QString::fromUtf8(track.id)));
        }
    }
    for (const Track &track : kUpNext) {
        md::MdListItem *item = m_items.value(QString::fromUtf8(track.id));
        if (item == nullptr) {
            continue;
        }
        item->setAvatarLabel(QStringLiteral("♪"));
        item->setHeadline(L(track.titleZh, track.titleEn));
        item->setSupporting(L(track.artistZh, track.artistEn));
        item->setTrailingSupporting(L(track.timeZh, track.timeEn));
        item->setVariant(md::MdListVariant::Expressive);
    }
    const qreal listHeight = list->sizeHint().height();
    const QRectF listPanel(left + 20.0, y, width - 40.0, listHeight + 8.0);
    if (painter != nullptr) {
        paintTonalPanel(*painter, listPanel, md::ColorRole::SurfaceContainerLowest,
                        md::ShapeCorner::Large);
    }
    m_slots.append(Slot{list, QRectF(left + 28.0, y + 4.0, width - 56.0, listHeight)});

    context.space(8.0);
}

void ShowcaseMusicPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const Slot &slot : m_slots) {
        if (slot.widget == nullptr) {
            continue;
        }
        QRect target(content.topLeft().toPoint() + slot.rect.topLeft().toPoint(),
                     slot.rect.size().toSize());
        // Shared-element press scale: the play FAB shrinks toward its centre
        // on the SpatialFast spring while the user holds it.
        if (slot.widget == m_playFab && !qFuzzyCompare(m_playScale, 1.0)) {
            const QPointF centre = QRectF(target).center();
            const QSizeF scaled(QSizeF(target.size()) * m_playScale);
            target = QRectF(centre - QPointF(scaled.width(), scaled.height()) / 2.0, scaled)
                         .toAlignedRect();
        }
        if (slot.widget->geometry() != target) {
            slot.widget->setGeometry(target);
        }
        if (!slot.widget->isVisible()) {
            slot.widget->show();
        }
        slot.widget->raise();
    }
}

void ShowcaseMusicPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void ShowcaseMusicPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void ShowcaseMusicPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void ShowcaseMusicPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createShowcaseMusicPage()
{
    return new ShowcaseMusicPage;
}

} // namespace gallery

#include "GalleryPages45.moc"
