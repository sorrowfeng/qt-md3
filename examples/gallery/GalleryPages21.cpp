// Gallery page 21: Cards — the first Containment family,
// `md.comp.<filled|elevated|outlined>-card.*`.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. The family-specific note:
// a card is a *container* — the sample content (title, supporting text) is
// ordinary child widgets in a layout installed on the card, filling the
// painted container through the card's contents margins.

#include "GalleryPages.h"

#include "widgets/MdCard.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <functional>

namespace gallery {

namespace {

/// One banked card plus the content-local rect the latest build() gave it.
struct CardSlot
{
    md::MdCard *card = nullptr;
    QRectF rect;
};

/// Painted media header used by the composed scene: gradient art + copy.
/// A child widget (not page paint) so the card's own container paints first
/// and the art sits inside it, the way a real media card would.
class SceneMediaContent : public QWidget
{
public:
    SceneMediaContent(md::ColorRole from, md::ColorRole to, const QString &icon,
                      const QString &title, const QString &meta, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_from(from)
        , m_to(to)
        , m_icon(icon)
        , m_title(title)
        , m_meta(meta)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF bounds(rect());
        const QRectF art(bounds.left() + 12.0, bounds.top() + 12.0,
                         bounds.width() - 24.0, 108.0);
        GalleryPage::paintGradientArt(painter, art, m_from, m_to,
                                      md::ShapeCorner::Large, m_icon);
        painter.setFont(GalleryPage::fontFor(md::TypeStyle::TitleMedium));
        painter.setPen(GalleryPage::role(md::ColorRole::OnSurface));
        painter.drawText(QRectF(bounds.left() + 16.0, art.bottom() + 10.0,
                                bounds.width() - 32.0, 24.0),
                         Qt::AlignLeft | Qt::AlignVCenter, m_title);
        painter.setFont(GalleryPage::fontFor(md::TypeStyle::BodySmall));
        painter.setPen(GalleryPage::role(md::ColorRole::OnSurfaceVariant));
        painter.drawText(QRectF(bounds.left() + 16.0, art.bottom() + 36.0,
                                bounds.width() - 32.0, 20.0),
                         Qt::AlignLeft | Qt::AlignVCenter, m_meta);
    }

private:
    md::ColorRole m_from;
    md::ColorRole m_to;
    QString m_icon;
    QString m_title;
    QString m_meta;
};

/// The sample content every card carries: a title and two lines of
/// supporting text, laid out by the card's own layout.
void fillWithSampleContent(md::MdCard *card)
{
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);

    auto *title = new QLabel(L("卡片标题", "Card title"), card);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    auto *body = new QLabel(
        L("支持文本——容器由状态行驱动：悬停抬升，按压起涟漪。",
          "Supporting text — the container is state-row driven: hover lifts, press ripples."),
        card);
    body->setWordWrap(true);
    layout->addWidget(body);
}

} // namespace

class CardPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit CardPage(QWidget *parent = nullptr);
    ~CardPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void layoutChildren();
    void placeChildren();

    /// Banked construction; configuration runs only on first creation.
    md::MdCard *card(const QString &id, md::MdCardVariant variant,
                     const std::function<void(md::MdCard *)> &configure = {});

    QHash<QString, md::MdCard *> m_bank;
    QVector<CardSlot> m_slots;
};

CardPage::CardPage(QWidget *parent)
    : GalleryPage(parent)
{
}

CardPage::~CardPage() = default;

QString CardPage::title() const
{
    return L("Card", "Cards");
}

QString CardPage::slug() const
{
    return QStringLiteral("cards");
}

QString CardPage::subtitle() const
{
    return L(
        "md.comp.filled-card / elevated-card / outlined-card：三个变体共享 "
        "corner-medium 容器与 on-surface 状态层，差别在容器色与海拔阶梯——filled "
        "是 surface-container-highest 静置 level0（悬停 level1、拖拽 level3）；"
        "elevated 是 surface-container-low 静置 level1（悬停 level2、拖拽 "
        "level4）；outlined 是 surface 加 1 px 的 outline-variant 描边（键盘焦点"
        "转 on-surface）。按压不高抬——按下行标注 \"Pressed (ripple)\"，按压响应"
        "就是涟漪。导出发布禁用行（filled → surface-variant、elevated → "
        "surface、outlined 容器不变），绘制时按 Compose 的 0.38 容器 / 0.38 "
        "内容 / 0.12 描边合成。海拔在状态之间以标准缓动 200 ms 过渡，禁用瞬跳。",
        "md.comp.filled-card / elevated-card / outlined-card: the three variants share a "
        "corner-medium container and the on-surface state layer, differing in container "
        "colour and elevation ladder — filled sits on surface-container-highest at level0 "
        "(hover level1, dragged level3); elevated on surface-container-low at level1 (hover "
        "level2, dragged level4); outlined on surface with a 1 px outline-variant stroke "
        "(on-surface under keyboard focus). Press never raises — the \"Pressed (ripple)\" "
        "row repeats the resting elevation, the press response is the ripple. The export "
        "publishes disabled rows (filled → surface-variant, elevated → surface, outlined "
        "container unchanged), composited per Compose at 0.38 container / 0.38 content / "
        "0.12 outline. Elevation animates between states over 200 ms with the standard "
        "easing; disabled snaps.");
}

md::MdCard *CardPage::card(const QString &id, md::MdCardVariant variant,
                           const std::function<void(md::MdCard *)> &configure)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdCard(variant, this);
    fillWithSampleContent(created);
    if (configure) {
        configure(created);
    }
    m_bank.insert(id, created);
    return created;
}

void CardPage::build(GalleryContext &context)
{
    m_slots.clear();
    const qreal contentWidth = context.width();
    constexpr qreal kCardHeight = 148.0;

    // --- composed scene: a media shelf ------------------------------------------
    // A card is a content surface. The product shape is a shelf of media cards
    // with painted art headers, not a stack of empty boxes.
    {
        context.section(L("场景：媒体架", "Scene: media shelf"));
        const qreal sceneHeight = 280.0;
        const QRectF scene = context.band(sceneHeight);
        const qreal gap = 16.0;
        const qreal cardW = (scene.width() - gap) / 2.0;
        const QRectF leftCard(scene.left(), scene.top(), cardW, scene.height() - 12.0);
        const QRectF rightCard(scene.left() + cardW + gap, scene.top(), cardW,
                               scene.height() - 12.0);

        auto sceneCard = [&](const QString &id, md::MdCardVariant variant,
                             md::ColorRole from, md::ColorRole to, const QString &icon,
                             const QString &title, const QString &meta) {
            md::MdCard *c = card(id, variant, [&](md::MdCard *widget) {
                widget->setClickable(true);
                // The bank already installed sample labels via a QVBoxLayout.
                // Clear that layout and drop a painted media header in its place.
                QLayout *existing = widget->layout();
                if (existing != nullptr) {
                    while (existing->count() > 0) {
                        QLayoutItem *item = existing->takeAt(0);
                        if (item->widget() != nullptr) {
                            item->widget()->deleteLater();
                        }
                        delete item;
                    }
                    existing->setContentsMargins(0, 0, 0, 0);
                }
                auto *media = new SceneMediaContent(from, to, icon, title, meta, widget);
                if (existing != nullptr) {
                    existing->addWidget(media);
                }
            });
            return c;
        };

        md::MdCard *left = sceneCard(QStringLiteral("scene-mix"), md::MdCardVariant::Filled,
                                     md::ColorRole::PrimaryContainer,
                                     md::ColorRole::TertiaryContainer,
                                     QStringLiteral("graphic_eq"),
                                     L("通勤歌单", "Commute mix"),
                                     L("42 首 · 2 小时 51 分", "42 songs · 2 hr 51 min"));
        m_slots.append(CardSlot{left, leftCard});

        md::MdCard *right = sceneCard(QStringLiteral("scene-live"), md::MdCardVariant::Elevated,
                                      md::ColorRole::SecondaryContainer,
                                      md::ColorRole::PrimaryContainer,
                                      QStringLiteral("podcasts"),
                                      L("设计访谈", "Design interview"),
                                      L("每周新集 · 38 分钟", "New weekly · 38 min"));
        m_slots.append(CardSlot{right, rightCard});

        context.space(16.0);
    }

    // --- filled -------------------------------------------------------------------
    context.section(L("Filled card", "Filled card"));
    context.paragraph(L(
        "surface-container-highest 静置 level0：三个变体中最重的容器色，视觉"
        "分组靠色调不靠阴影。这张是可点击的——悬停画 on-surface 0.08 状态层并"
        "抬到 level1，按下起 on-surface 涟漪（不高抬），Tab 后画 secondary "
        "焦点环。",
        "surface-container-highest at level0: the heaviest container colour of the three, "
        "grouping by tone rather than shadow. This one is clickable — hover paints the "
        "on-surface 0.08 layer and lifts to level1, press ripples on-surface without "
        "lifting, and Tab draws the secondary focus ring."));
    {
        const qreal width = qMin<qreal>(280.0, contentWidth);
        md::MdCard *filled = card(QStringLiteral("filled"), md::MdCardVariant::Filled,
                                  [](md::MdCard *w) { w->setClickable(true); });
        const QRectF band = context.band(kCardHeight);
        m_slots.append(CardSlot{filled, QRectF(band.topLeft(), QSizeF(width, kCardHeight))});
    }

    // --- elevated -------------------------------------------------------------------
    context.section(L("Elevated card", "Elevated card"));
    context.paragraph(L(
        "surface-container-low 静置 level1：海拔由阴影表达，悬停 level2、拖拽"
        "level4。这张保持非点击——Compose 的非点击重载没有交互源，状态行整表"
        "停在启用的行上，容器即控件矩形、不预留焦点边距。",
        "surface-container-low at level1: elevation expressed by the shadow, hover level2 "
        "and dragged level4. This one stays non-clickable — Compose's non-clickable "
        "overload has no interaction source, so the state table rests on the enabled row "
        "and the container is the widget rect with no focus margin reserved."));
    {
        const qreal width = qMin<qreal>(280.0, contentWidth);
        md::MdCard *elevated = card(QStringLiteral("elevated"), md::MdCardVariant::Elevated);
        const QRectF band = context.band(kCardHeight);
        m_slots.append(CardSlot{elevated, QRectF(band.topLeft(), QSizeF(width, kCardHeight))});
    }

    // --- outlined -------------------------------------------------------------------
    context.section(L("Outlined card", "Outlined card"));
    context.paragraph(L(
        "surface 加 1 px 的 outline-variant 描边：零阴影的轻量分组。键盘焦点把"
        "描边转到 on-surface；禁用行不改容器，只把描边淡到 0.12。",
        "surface with a 1 px outline-variant stroke: the zero-shadow lightweight grouping. "
        "Keyboard focus turns the stroke on-surface; the disabled row keeps the container "
        "and only fades the outline to 0.12."));
    {
        const qreal width = qMin<qreal>(280.0, contentWidth);
        md::MdCard *outlined = card(QStringLiteral("outlined"), md::MdCardVariant::Outlined,
                                    [](md::MdCard *w) { w->setClickable(true); });
        const QRectF band = context.band(kCardHeight);
        m_slots.append(CardSlot{outlined, QRectF(band.topLeft(), QSizeF(width, kCardHeight))});
    }

    // --- two container hues on one screen ----------------------------------------
    // The three published variants pin their container to surface tones, which
    // is a token fact. To keep one screen multi-hue the way the official demos
    // read, two extra surfaces are painted from the theme's secondary-container
    // and tertiary-container roles — same shape, same tokens, different colour
    // families — so purple/blue/red can sit on a single page.
    context.section(L("同屏两族容器色：secondary-container 与 tertiary-container",
                      "Two container hues on one screen: secondary-container and "
                      "tertiary-container"));
    context.paragraph(L(
        "三个官方变体的容器色锁在 surface 色阶上，这是令牌事实。下面两张是同一形状、"
        "同一 corner-medium 的角色色样：左边 secondary-container，右边 tertiary-container。"
        "全部来自 MdTheme::color(ColorRole::…)，换种子时与真实卡片一起变色。",
        "The three published variants pin their containers to the surface ladder — a token "
        "fact. The two samples below share the same shape and corner-medium radius but read "
        "their fills from secondary-container and tertiary-container. Both come from "
        "MdTheme::color(ColorRole::…) and recolour with the real cards on a seed change."));
    {
        QPainter *painter = context.painter();
        const qreal width = qMin<qreal>(280.0, contentWidth);
        const QRectF band = context.band(kCardHeight);
        const QRectF left(band.left(), band.top(), width, kCardHeight);
        const QRectF right(band.left() + width + 16.0, band.top(),
                           qMin<qreal>(width, band.right() - band.left() - width - 16.0),
                           kCardHeight);
        if (painter != nullptr) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            fillRounded(*painter, left, role(md::ColorRole::SecondaryContainer),
                        md::ShapeCorner::Medium);
            fillRounded(*painter, right, role(md::ColorRole::TertiaryContainer),
                        md::ShapeCorner::Medium);
            painter->setFont(fontFor(md::TypeStyle::TitleMedium));
            painter->setPen(role(md::ColorRole::OnSecondaryContainer));
            painter->drawText(left.adjusted(16.0, 16.0, -16.0, -16.0),
                              Qt::AlignLeft | Qt::AlignTop, QStringLiteral("secondary-container"));
            painter->setPen(role(md::ColorRole::OnSecondaryContainer));
            painter->setFont(fontFor(md::TypeStyle::BodySmall));
            painter->drawText(left.adjusted(16.0, 48.0, -16.0, -16.0),
                              Qt::AlignLeft | Qt::AlignTop,
                              L("色调容器样张", "Tonal container sample"));
            painter->setFont(fontFor(md::TypeStyle::TitleMedium));
            painter->setPen(role(md::ColorRole::OnTertiaryContainer));
            painter->drawText(right.adjusted(16.0, 16.0, -16.0, -16.0),
                              Qt::AlignLeft | Qt::AlignTop, QStringLiteral("tertiary-container"));
            painter->setFont(fontFor(md::TypeStyle::BodySmall));
            painter->drawText(right.adjusted(16.0, 48.0, -16.0, -16.0),
                              Qt::AlignLeft | Qt::AlignTop,
                              L("第三色容器样张", "Tertiary container sample"));
            painter->restore();
        }
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts -----------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-{filled,elevated,outlined}-"
        "card.scss (34.0.21) + Compose M3 Card.kt (no web component published)"));
    context.detail(QStringLiteral(
        "export      container corner-medium · state layer on-surface · icon 24 px primary · "
        "focus indicator secondary outer 2 / 3"));
    context.detail(QStringLiteral(
        "ladders     filled L0→hover L1→dragged L3 · elevated L1→hover L2→dragged L4 · "
        "outlined L0→hover L1→dragged L3 · pressed = resting"));
    context.detail(QStringLiteral(
        "disabled    filled surface-variant · elevated surface · outlined unchanged · "
        "composited 0.38 container / 0.38 content / 0.12 outline"));
    context.detail(QStringLiteral(
        "compose     elevation animates (motion scheme; here standard easing 200 ms, snap "
        "when disabled) · clickability splits the state table · drag via setter"));
    context.space(8.0);
    context.detail(L("TestMd3Card 将三张状态行表、禁用合成常量、覆盖解析、布局几何、"
                     "状态优先级、指针与键盘契约、海拔动画与瞬跳逐字段锁定。",
                     "TestMd3Card pins the three state row tables, the disabled compositing "
                     "constants, the override parsing, the layout geometry, the state "
                     "priority, the pointer and keyboard contracts and the elevation "
                     "animation/snap field by field."));
}

void CardPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const CardSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.card->geometry() != target) {
            slot.card->setGeometry(target);
        }
    }
}

void CardPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void CardPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void CardPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void CardPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createCardPage()
{
    return new CardPage;
}

} // namespace gallery

#include "GalleryPages21.moc"
