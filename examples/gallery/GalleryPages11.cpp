// Gallery page 11: Segmented buttons — the last Actions family.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. The family-specific note:
// one published set only (40 px, outlined, secondary-container selection) —
// the segments overlap by the outline width so the shared edge is a single
// 1 px stroke, and the check scales in on a reserved icon slot.

#include "GalleryPages.h"

#include "widgets/MdSegmentedButton.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace gallery {

namespace {

/// One banked segmented button plus the content-local rect the latest build() gave it.
struct SegmentedSlot
{
    md::MdSegmentedButton *button = nullptr;
    QRectF rect;
};

/// Gap between neighbouring segmented buttons in a row.
constexpr qreal kRowGap = 32.0;
/// Gap between stacked rows inside one section.
constexpr qreal kStackGap = 20.0;

} // namespace

class SegmentedButtonPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit SegmentedButtonPage(QWidget *parent = nullptr);
    ~SegmentedButtonPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void layFlow(GalleryContext &context, const QVector<md::MdSegmentedButton *> &buttons);
    void layoutChildren();
    void placeChildren();

    md::MdSegmentedButton *segmented(const QString &id, const QStringList &segments);
    template <typename Configure>
    md::MdSegmentedButton *segmented(const QString &id, const QStringList &segments,
                                     Configure configure)
    {
        const bool created = !m_bank.contains(id);
        md::MdSegmentedButton *widget = segmented(id, segments);
        if (created) {
            configure(widget);
        }
        return widget;
    }

    QHash<QString, md::MdSegmentedButton *> m_bank;
    QVector<SegmentedSlot> m_slots;
};

SegmentedButtonPage::SegmentedButtonPage(QWidget *parent)
    : GalleryPage(parent)
{
}

SegmentedButtonPage::~SegmentedButtonPage() = default;

QString SegmentedButtonPage::title() const
{
    return L("分段按钮", "Segmented buttons");
}

QString SegmentedButtonPage::slug() const
{
    return QStringLiteral("Segmented buttons");
}

QString SegmentedButtonPage::subtitle() const
{
    return L("一行共用一条 1 px 描边的分段选择器：相邻两段正好重叠一个描边宽度，"
             "重叠的那条边就是分隔线。选中段填 secondary-container，对勾在预留的"
             "图标槽里缩放登场——文字从不动。",
             "One row sharing a single 1 px outline: neighbours overlap by exactly the stroke "
             "width, and that shared edge is the divider. A selected segment fills with "
             "secondary-container and the check scales in on a reserved icon slot — the label "
             "never moves.");
}

md::MdSegmentedButton *SegmentedButtonPage::segmented(const QString &id,
                                                      const QStringList &segments)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSegmentedButton(segments, this);
    created->show();
    m_bank.insert(id, created);
    return created;
}

void SegmentedButtonPage::layFlow(GalleryContext &context,
                                  const QVector<md::MdSegmentedButton *> &buttons)
{
    const qreal available = context.width();

    QVector<md::MdSegmentedButton *> row;
    qreal rowWidth = 0.0;
    qreal rowHeight = 0.0;

    const auto flush = [&] {
        if (row.isEmpty()) {
            return;
        }
        const QRectF band = context.band(rowHeight);
        qreal x = band.left();
        for (md::MdSegmentedButton *button : row) {
            const QSize hint = button->sizeHint();
            m_slots.append(SegmentedSlot{
                button,
                QRectF(x, band.top() + (rowHeight - hint.height()) / 2.0, hint.width(),
                       hint.height())});
            x += hint.width() + kRowGap;
        }
        context.space(kStackGap);
        row.clear();
        rowWidth = 0.0;
        rowHeight = 0.0;
    };

    for (md::MdSegmentedButton *button : buttons) {
        const QSize hint = button->sizeHint();
        const qreal needed =
            row.isEmpty() ? qreal(hint.width()) : rowWidth + kRowGap + hint.width();
        if (!row.isEmpty() && needed > available) {
            flush();
        }
        if (!row.isEmpty()) {
            rowWidth += kRowGap;
        }
        row.append(button);
        rowWidth += hint.width();
        rowHeight = qMax(rowHeight, qreal(hint.height()));
    }
    flush();
}

void SegmentedButtonPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- single choice ----------------------------------------------------------
    context.section(L("单选 —— check 缩放入场", "Single choice — the check scales in"));
    context.paragraph(L(
        "md.comp.outlined-segmented-button：40 px 高、label-large、描边 1 px、corner-full。"
        "图标槽（18 px + 8 px 间距）永远预留，所以选中时对勾缩放登场、文字一个像素都不动。"
        "图标槽是 Compose 的度量策略——导出没有这一行。",
        "md.comp.outlined-segmented-button: 40 px tall, label-large, 1 px outline, corner-full. "
        "The icon slot (18 px + 8 px spacing) is always reserved, so the check scales in on "
        "selection without moving the label by a pixel. The slot is the Compose measure "
        "policy — the export publishes no such row."));
    {
        auto *days = segmented(QStringLiteral("days"),
                               {L("日", "Day"), L("周", "Week"), L("月", "Month")});
        days->setChecked(1, true);
        auto *views = segmented(QStringLiteral("views"),
                                {L("列表", "List"), L("网格", "Grid"), L("详情", "Detail")});
        views->setChecked(0, true);
        layFlow(context, {days, views});
    }

    // --- with icons ---------------------------------------------------------------
    context.section(L("带图标的段", "Segments with icons"));
    context.paragraph(L(
        "段可以带自己的图标：未选中时显示自定义图标，选中时对勾与它交叉淡入淡出——"
        "两条动画都跑 spring-fast-spatial。",
        "A segment can carry its own icon: the custom icon shows while unselected and "
        "crossfades with the check on selection — both animations run on spring-fast-spatial."));
    {
        auto *kinds = segmented(QStringLiteral("kinds"),
                                {L("列表", "List"), L("文字", "Text"), L("网格", "Grid")});
        kinds->setLeadingIcons({QStringLiteral("list"), QString(),
                                QStringLiteral("dashboard")});
        layFlow(context, {kinds});
    }

    // --- multi choice ---------------------------------------------------------------
    context.section(L("多选", "Multi choice"));
    context.paragraph(L(
        "singleChoice 关掉之后每段是独立的开关——几何、颜色、对勾动画完全不变，"
        "只有语义不同。这是 Compose 的 MultiChoiceSegmentedButtonRow。",
        "Turn singleChoice off and every segment becomes an independent toggle — the "
        "geometry, colours and check animation are identical; only the semantics change. "
        "This is Compose's MultiChoiceSegmentedButtonRow."));
    {
        auto *filters = segmented(QStringLiteral("filters"),
                                  {L("粗体", "Bold"), L("斜体", "Italic"), L("下划线", "Underline")});
        filters->setSingleChoice(false);
        filters->setChecked(0, true);
        filters->setChecked(2, true);
        layFlow(context, {filters});
    }

    // --- disabled -------------------------------------------------------------------
    context.section(L("禁用", "Disabled"));
    context.paragraph(L(
        "禁用行只有透明度：内容 on-surface @ 0.38、描边 @ 0.12。选中的段保留"
        "secondary-container 容器填充——导出没有 disabled-container 行，Compose 的"
        "disabledActive 也用 SelectedContainerColor。",
        "The disabled rows are opacities only: content on-surface @ 0.38, outline @ 0.12. A "
        "disabled selected segment keeps its secondary-container fill — the export publishes "
        "no disabled-container row, and Compose's disabledActive uses SelectedContainerColor "
        "too."));
    {
        auto *off = segmented(QStringLiteral("off"),
                              {L("日", "Day"), L("周", "Week"), L("月", "Month")});
        off->setChecked(1, true);
        off->setSoftDisabled(true);
        layFlow(context, {off});
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts ------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-outlined-segmented-button.scss "
        "+ androidx SegmentedButton.kt"));
    context.detail(QStringLiteral(
        "one set   40 px · label-large · outline 1 px · corner-full · icon 18 · no size scale, "
        "no colour variants"));
    context.detail(QStringLiteral(
        "selected  secondary-container / on-secondary-container (every interactive state) · "
        "unselected on-surface / outline"));
    context.detail(QStringLiteral(
        "shape     first segment rounds inline-start, last rounds inline-end, middle = "
        "rectangle; neighbours overlap by the outline width — the shared edge is the divider "
        "(compose spacedBy(-BorderWidth))"));
    context.detail(QStringLiteral(
        "quirk     pressed-state-layer-opacity is the FOCUS opacity in the export (as "
        "published); motion rows absent — the check runs spring-fast-spatial (compose "
        "FastSpatial)"));
    context.space(8.0);
    context.detail(L("TestMd3SegmentedButton 将这套行、itemShape 角规则、重叠几何与每段的"
                     "涟漪 / 焦点 / 禁用规则逐字段锁定。",
                     "TestMd3SegmentedButton pins this set, the itemShape corner rule, the "
                     "overlap geometry and the per-segment ripple / focus / disabled rules "
                     "field by field."));
}

void SegmentedButtonPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const SegmentedSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.button->geometry() != target) {
            slot.button->setGeometry(target);
        }
    }
}

void SegmentedButtonPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void SegmentedButtonPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void SegmentedButtonPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void SegmentedButtonPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createSegmentedButtonPage()
{
    return new SegmentedButtonPage;
}

} // namespace gallery

#include "GalleryPages11.moc"
