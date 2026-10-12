// Gallery page 45: showcase - Music player.
//
// A now-playing screen: top app bar, album-art placeholder, track meta,
// a progress slider, transport icon buttons and playlist chips. Same rules
// as the other pages: widgets are banked, build() is re-entrant, and the page
// never styles anything.

#include "GalleryPages.h"

#include "widgets/MdChip.h"
#include "widgets/MdFab.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"
#include "widgets/MdSlider.h"
#include "widgets/MdTopAppBar.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>

#include <cmath>

namespace gallery {

namespace {

struct Slot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

constexpr qreal kScreenHeight = 900.0;
constexpr qreal kArtSize = 280.0;

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

    md::MdTopAppBar *appBar();
    md::MdIconButton *iconButton(const QString &id, const QString &icon);
    md::MdFab *transportFab();
    md::MdSlider *progressSlider();
    md::MdChip *chip(const QString &id, const QString &label);
    md::MdList *upNextList();
    md::MdListItem *trackItem(const QString &id);

    md::MdTopAppBar *m_appBar = nullptr;
    QHash<QString, md::MdIconButton *> m_iconButtons;
    md::MdFab *m_playFab = nullptr;
    md::MdSlider *m_slider = nullptr;
    QHash<QString, md::MdChip *> m_chips;
    md::MdList *m_list = nullptr;
    QHash<QString, md::MdListItem *> m_items;
    QVector<Slot> m_slots;
};

ShowcaseMusicPage::ShowcaseMusicPage(QWidget *parent)
    : GalleryPage(parent)
{
}

ShowcaseMusicPage::~ShowcaseMusicPage() = default;

QString ShowcaseMusicPage::title() const
{
    return L("示例·音乐播放", "Showcase·Music");
}

QString ShowcaseMusicPage::slug() const
{
    return QStringLiteral("showcase-music");
}

QString ShowcaseMusicPage::subtitle() const
{
    return L("正在播放屏幕：专辑艺术占位、曲目信息、进度滑块、传输控制与"
             "待播列表。芯片选择播放列表。全部由已移植的组件拼装，"
             "颜色和字体均来自主题令牌。",
             "A now-playing screen: album-art placeholder, track meta, a progress "
             "slider, transport controls and an up-next list. Playlist chips select "
             "a queue. Assembled from already-ported components; colours and type "
             "come from theme tokens.");
}

md::MdTopAppBar *ShowcaseMusicPage::appBar()
{
    if (m_appBar == nullptr) {
        m_appBar = new md::MdTopAppBar(this);
        m_appBar->hide();
    }
    return m_appBar;
}

md::MdIconButton *ShowcaseMusicPage::iconButton(const QString &id, const QString &icon)
{
    const auto found = m_iconButtons.constFind(id);
    if (found != m_iconButtons.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdIconButton(icon, this);
    created->hide();
    m_iconButtons.insert(id, created);
    return created;
}

md::MdFab *ShowcaseMusicPage::transportFab()
{
    if (m_playFab != nullptr) {
        return m_playFab;
    }
    // The play transport sits on tertiary so the now-playing screen spends
    // all three colour families: tertiary art, tertiary play, primary scrubber.
    m_playFab = new md::MdFab(QStringLiteral("play_arrow"), this);
    m_playFab->setVariant(md::FabVariant::Tertiary);
    m_playFab->hide();
    return m_playFab;
}

md::MdSlider *ShowcaseMusicPage::progressSlider()
{
    if (m_slider == nullptr) {
        m_slider = new md::MdSlider(this);
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

    context.section(L("正在播放", "Now playing"));
    context.paragraph(L("更大的艺术图、更少的文字：典型的 M3 媒体屏幕。"
                        "滑条是真 MdSlider，播放按钮是真 MdIconButton，"
                        "芯片是可选中筛选芯片。",
                        "Bigger art, fewer words: a typical M3 media screen. "
                        "The scrubber is a real MdSlider, the transport buttons "
                        "are real MdIconButtons, and the chips are selectable "
                        "filter chips."));

    const QRectF screen = context.band(kScreenHeight);
    const qreal left = screen.left();
    const qreal width = screen.width();
    const qreal centre = left + width / 2.0;
    qreal y = screen.top();

    if (painter != nullptr) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        fillRounded(*painter, screen, role(md::ColorRole::SurfaceContainerLow),
                    md::ShapeCorner::ExtraLarge);
        painter->restore();
    }

    // --- top app bar ------------------------------------------------------
    md::MdTopAppBar *bar = appBar();
    bar->setTitle(L("正在播放", "Now playing"));
    bar->setVariant(md::MdAppBarVariant::Small);
    bar->setNavigationWidget(iconButton(QStringLiteral("back"), QStringLiteral("arrow_back")));
    if (bar->actionWidgets().isEmpty()) {
        bar->addActionWidget(iconButton(QStringLiteral("more"), QStringLiteral("more_vert")));
    }
    const qreal barHeight = bar->sizeHint().height();
    m_slots.append(Slot{bar, QRectF(left, y, width, barHeight)});
    y += barHeight + 12.0;

    // --- album art (painted placeholder) ---------------------------------
    const QRectF art(centre - kArtSize / 2.0, y, kArtSize, kArtSize);
    if (painter != nullptr) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        // Art placeholder: a tertiary container disc with a music-note glyph.
        fillRounded(*painter, art, role(md::ColorRole::TertiaryContainer),
                    md::ShapeCorner::ExtraLarge);
        md::MdIcon::paint(painter, art.adjusted(48.0, 48.0, -48.0, -48.0),
                          QStringLiteral("music_note"),
                          role(md::ColorRole::OnTertiaryContainer), md::MdIconSet::Auto,
                          md::MdIconFamily::Outlined, md::MdIconStyle{0.0, 400.0, 0.0, 96.0});
        painter->restore();
    }
    y += kArtSize + 20.0;

    // --- track meta (painted) --------------------------------------------
    if (painter != nullptr) {
        painter->save();
        const QFont titleFont = fontFor(md::TypeStyle::HeadlineSmall);
        painter->setFont(titleFont);
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(QRectF(left, y, width, 36.0), Qt::AlignHCenter | Qt::AlignVCenter,
                          L("后弦", "Afterglow"));
        const QFont artistFont = fontFor(md::TypeStyle::BodyLarge);
        painter->setFont(artistFont);
        painter->setPen(role(md::ColorRole::OnSurfaceVariant));
        painter->drawText(QRectF(left, y + 36.0, width, 28.0),
                          Qt::AlignHCenter | Qt::AlignVCenter, L("北纬组合", "Latitude"));
        painter->restore();
    }
    y += 72.0;

    // --- progress slider + times ------------------------------------------
    md::MdSlider *slider = progressSlider();
    const qreal sliderHeight = slider->sizeHint().height();
    m_slots.append(Slot{slider, QRectF(left + 32.0, y, width - 64.0, sliderHeight)});
    if (painter != nullptr) {
        painter->save();
        const QFont timeFont = fontFor(md::TypeStyle::LabelSmall);
        painter->setFont(timeFont);
        painter->setPen(role(md::ColorRole::OnSurfaceVariant));
        painter->drawText(QRectF(left + 32.0, y + sliderHeight, 80.0, 20.0),
                          Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("1:12"));
        painter->drawText(QRectF(left + width - 32.0 - 80.0, y + sliderHeight, 80.0, 20.0),
                          Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("3:42"));
        painter->restore();
    }
    y += sliderHeight + 28.0;

    // --- transport row ----------------------------------------------------
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
    const qreal buttonPitch = 64.0;
    const qreal rowWidth = 5.0 * buttonPitch;
    qreal x = centre - rowWidth / 2.0;
    for (const Transport &t : transport) {
        if (t.primary) {
            // Tertiary FAB: the one filled transport, so the screen shows
            // primary (scrubber) + tertiary (art & play) together.
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
    y += 96.0 + 12.0;

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
        qreal chipX = left + 32.0;
        for (const ChipEntry &entry : entries) {
            md::MdChip *c = chip(QString::fromUtf8(entry.id), L(entry.zh, entry.en));
            c->setChecked(entry.selected);
            const qreal w = qMax<qreal>(c->sizeHint().width(), 72.0);
            m_slots.append(Slot{c, QRectF(chipX, y, w, chipHeight)});
            chipX += w + 8.0;
        }
    }
    y += chipHeight + 20.0;

    // --- up next ----------------------------------------------------------
    if (painter != nullptr) {
        painter->save();
        const QFont headingFont = fontFor(md::TypeStyle::TitleMedium);
        painter->setFont(headingFont);
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(QRectF(left + 32.0, y, width - 64.0, 28.0),
                          Qt::AlignLeft | Qt::AlignVCenter, L("接下来", "Up next"));
        painter->restore();
    }
    y += 36.0;

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
        item->setAvatarLabel(QStringLiteral("-"));
        item->setHeadline(L(track.titleZh, track.titleEn));
        item->setSupporting(L(track.artistZh, track.artistEn));
        item->setTrailingSupporting(L(track.timeZh, track.timeEn));
        item->setVariant(md::MdListVariant::Expressive);
    }
    const qreal listHeight = list->sizeHint().height();
    m_slots.append(Slot{list, QRectF(left + 24.0, y, width - 48.0, listHeight)});
    y += listHeight;

    Q_UNUSED(y);
    context.space(12.0);
    context.detail(L("提示：拖动滑条可改变进度，点击主播放按钮，芯片可切换选择。",
                     "Tip: drag the scrubber to seek, click the primary transport "
                     "button, and toggle the playlist chips."));
}

void ShowcaseMusicPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const Slot &slot : m_slots) {
        if (slot.widget == nullptr) {
            continue;
        }
        const QRect target(content.topLeft().toPoint() + slot.rect.topLeft().toPoint(),
                           slot.rect.size().toSize());
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
