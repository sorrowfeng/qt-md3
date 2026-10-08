// Gallery page 12: Badges — the first Communication family.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. The family-specific note:
// one published set with two forms decided by content — the 6 px dot and the
// minimum-16 px pill — and no interaction at all: the export publishes no
// state rows, so badges take no focus, paint no state layer and pass clicks
// straight through to the content they sit on.

#include "GalleryPages.h"

#include "widgets/MdBadge.h"
#include "widgets/MdBadgedBox.h"
#include "widgets/MdIconButton.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace gallery {

namespace {

/// One banked widget plus the content-local rect the latest build() gave it.
struct BadgeSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

constexpr qreal kRowGap = 40.0;
constexpr qreal kStackGap = 20.0;

} // namespace

class BadgePage : public GalleryPage
{
    Q_OBJECT

public:
    explicit BadgePage(QWidget *parent = nullptr);
    ~BadgePage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void layFlow(GalleryContext &context, const QVector<QWidget *> &widgets);
    void layoutChildren();
    void placeChildren();

    md::MdBadgedBox *badged(const QString &id, const QString &iconName);
    md::MdBadge *badge(const QString &id, const QString &text);

    QHash<QString, md::MdBadgedBox *> m_boxes;
    QHash<QString, md::MdBadge *> m_badges;
    QVector<BadgeSlot> m_slots;
};

BadgePage::BadgePage(QWidget *parent)
    : GalleryPage(parent)
{
}

BadgePage::~BadgePage() = default;

QString BadgePage::title() const
{
    return L("徽章", "Badges");
}

QString BadgePage::slug() const
{
    return QStringLiteral("badges");
}

QString BadgePage::subtitle() const
{
    return L("md.comp.badge：一套令牌、两种形态，由内容决定——无内容是 6 px 红点，"
             "带内容是最小 16 px 的胶囊。徽章不参与交互：导出没有发布任何状态行，"
             "它不接收焦点、不画状态层，点击会直接穿透到身下的内容。",
             "md.comp.badge: one token set, two forms decided by content — a 6 px dot with no "
             "content, a minimum-16 px pill with it. A badge is not interactive: the export "
             "publishes no state rows, so it takes no focus, paints no state layer, and clicks "
             "pass straight through to the content underneath.");
}

md::MdBadgedBox *BadgePage::badged(const QString &id, const QString &iconName)
{
    const auto found = m_boxes.constFind(id);
    if (found != m_boxes.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdBadgedBox(this);
    created->setContentWidget(new md::MdIconButton(iconName));
    created->show();
    m_boxes.insert(id, created);
    return created;
}

md::MdBadge *BadgePage::badge(const QString &id, const QString &text)
{
    const auto found = m_badges.constFind(id);
    if (found != m_badges.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdBadge(text, this);
    created->show();
    m_badges.insert(id, created);
    return created;
}

void BadgePage::layFlow(GalleryContext &context, const QVector<QWidget *> &widgets)
{
    const qreal available = context.width();

    QVector<QWidget *> row;
    qreal rowWidth = 0.0;
    qreal rowHeight = 0.0;

    const auto flush = [&] {
        if (row.isEmpty()) {
            return;
        }
        const QRectF band = context.band(rowHeight);
        qreal x = band.left();
        for (QWidget *widget : row) {
            const QSize hint = widget->sizeHint();
            m_slots.append(BadgeSlot{
                widget,
                QRectF(x, band.top() + (rowHeight - hint.height()) / 2.0, hint.width(),
                       hint.height())});
            x += hint.width() + kRowGap;
        }
        context.space(kStackGap);
        row.clear();
        rowWidth = 0.0;
        rowHeight = 0.0;
    };

    for (QWidget *widget : widgets) {
        const QSize hint = widget->sizeHint();
        const qreal needed =
            row.isEmpty() ? qreal(hint.width()) : rowWidth + kRowGap + hint.width();
        if (!row.isEmpty() && needed > available) {
            flush();
        }
        if (!row.isEmpty()) {
            rowWidth += kRowGap;
        }
        row.append(widget);
        rowWidth += hint.width();
        rowHeight = qMax(rowHeight, qreal(hint.height()));
    }
    flush();
}

void BadgePage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- dots ---------------------------------------------------------------------
    context.section(L("红点 —— 无内容的形态", "Dots — the content-free form"));
    context.paragraph(L(
        "md.comp.badge：6 px 圆点，error 色，corner-full。锚定在内容的右上角——"
        "红点的尾边贴着内容的尾边、顶边贴着内容的顶边（Compose 偏移 6/6，"
        "material-web 未实现该组件）。红点不拦截任何点击。",
        "md.comp.badge: a 6 px circle, error, corner-full. Anchored in the content's top-end "
        "corner — the dot's end edge sits on the content's end edge and its top edge on the "
        "content's top edge (Compose offsets 6/6; material-web does not implement the "
        "component). The dot never intercepts a click."));
    {
        auto *mail = badged(QStringLiteral("mail"), QStringLiteral("mail"));
        auto *alerts = badged(QStringLiteral("alerts"), QStringLiteral("notifications"));
        auto *cart = badged(QStringLiteral("cart"), QStringLiteral("shopping_cart"));
        layFlow(context, {mail, alerts, cart});
    }

    // --- counts --------------------------------------------------------------------
    context.section(L("数量徽章 —— 胶囊形态", "Counts — the pill form"));
    context.paragraph(L(
        "设置文本就切换到 large.*：最小 16 px、label-small（11 px / 行高 16 / "
        "字重 500）、on-error 文字、左右各 4 px 内边距（Compose 的度量）。锚定"
        "偏移随之变为 12/14——16 px 胶囊会探出锚点 4 px 横向、2 px 纵向；Compose "
        "允许徽章悬越周围，而 Qt 子控件会被父级裁剪，所以 MdBadgedBox 把悬越"
        "量预留进自己的几何里，内容保持原尺寸。",
        "Setting text switches to large.*: minimum 16 px, label-small (11 px / 16 line height / "
        "weight 500), on-error text, 4 px side padding (the Compose metric). The anchor offsets "
        "become 12/14 — a 16 px pill overhangs its anchor by 4 px horizontally and 2 px on top. "
        "Compose lets the badge overlap the surroundings; a Qt child would be clipped, so "
        "MdBadgedBox reserves the overhang in its own geometry and the content keeps its size."));
    {
        auto *one = badged(QStringLiteral("one"), QStringLiteral("chat"));
        one->badge()->setText(QStringLiteral("1"));
        auto *nine = badged(QStringLiteral("nine"), QStringLiteral("notifications"));
        nine->badge()->setText(QStringLiteral("9"));
        auto *many = badged(QStringLiteral("many"), QStringLiteral("mail"));
        many->badge()->setText(QStringLiteral("99+"));
        layFlow(context, {one, nine, many});
    }

    // --- standalone -----------------------------------------------------------------
    context.section(L("独立徽章", "Standalone badges"));
    context.paragraph(L(
        "MdBadge 本身也是独立组件——红点、短计数、文字标签都可以直接放进布局。"
        "无障碍上徽章会朗读自己的文本；红点没有内容，至少以名字 \"badge\" 出现。",
        "MdBadge is a component on its own — a dot, a short count or a text label can go "
        "straight into a layout. The text is the badge's accessible name; the content-free dot "
        "at least presents as \"badge\"."));
    {
        auto *dot = badge(QStringLiteral("dot"), QString());
        auto *two = badge(QStringLiteral("two"), QStringLiteral("2"));
        auto *plus = badge(QStringLiteral("plus"), QStringLiteral("99+"));
        auto *label = badge(QStringLiteral("label"), QStringLiteral("New"));
        layFlow(context, {dot, two, plus, label});
    }

    Q_ASSERT(m_slots.size() == m_boxes.size() + m_badges.size());

    // --- token facts -----------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-badge.scss + androidx Badge.kt "
        "(material-web does not implement the component)"));
    context.detail(QStringLiteral(
        "one set   dot 6 px · large min 16 px · both corner-full · error container / on-error "
        "label (label-small 11/16/0.5/500) · no variants"));
    context.detail(QStringLiteral(
        "anchor    dot 6/6 [compose] · content 12/14 [compose] · pill side padding 4 px "
        "[compose] — every behaviour row is a Compose fact"));
    context.detail(QStringLiteral(
        "states    none published — a badge takes no focus, paints no state layer, no ripple, "
        "no focus ring, and lets clicks pass through (WA_TransparentForMouseEvents)"));
    context.detail(QStringLiteral(
        "quirk     Qt clips children to their parent, so MdBadgedBox reserves the pill overhang "
        "(width-12 / height-14) instead of Compose's free overlap; the painted result matches"));
    context.space(8.0);
    context.detail(L("TestMd3Badge 将这套行、两种形态的几何、锚定偏移、预留悬越与"
                     "非交互契约逐字段锁定。",
                     "TestMd3Badge pins this set, the geometry of both forms, the anchoring "
                     "offsets, the reserved overhang and the non-interactivity contract field "
                     "by field."));
}

void BadgePage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const BadgeSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.widget->geometry() != target) {
            slot.widget->setGeometry(target);
        }
    }
}

void BadgePage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void BadgePage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void BadgePage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void BadgePage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createBadgePage()
{
    return new BadgePage;
}

} // namespace gallery

#include "GalleryPages12.moc"
