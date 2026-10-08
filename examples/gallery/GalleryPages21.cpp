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
