// Gallery page 35: Chips — the four published families at export version
// 34.0.21 (`md.comp.assist-chip.*`, `md.comp.filter-chip.*`,
// `md.comp.input-chip.*`, `md.comp.suggestion-chip.*`).
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Every pill on this page is a
// live MdChip — the pressed state-layer swap, the selection colours, the
// elevation ladder and the focus ring are all the real component.
//
// Three things this page is explicit about because they are easy to misread:
//
//   * the **pressed state-layer swap** — a filter chip's unselected press
//     ripples `on-secondary-container` (the colour the chip is about to earn)
//     while a selected press ripples `on-surface-variant`; input chips do not
//     swap;
//   * **no colour animation** — a selection's container/label change is
//     instant, exactly as Compose's `SelectableChipColors` resolve by state;
//   * **drag is a programmatic state** — the dragged row (level 4) is shown,
//     not driven.

#include "GalleryPages.h"

#include "core/MdChipTokens.h"
#include "widgets/MdChip.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

namespace gallery {

namespace {

struct ChipSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class ChipPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit ChipPage(QWidget *parent = nullptr);
    ~ChipPage() override;

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

    md::MdChip *chip(const QString &id);

    QHash<QString, md::MdChip *> m_chips;
    QVector<ChipSlot> m_slots;
};

ChipPage::ChipPage(QWidget *parent)
    : GalleryPage(parent)
{
}

ChipPage::~ChipPage() = default;

QString ChipPage::title() const
{
    return L("卡片筹码", "Chips");
}

QString ChipPage::slug() const
{
    return QStringLiteral("chips");
}

QString ChipPage::subtitle() const
{
    return L(
        "筹码：32dp 高、corner-small 的胶囊，四个家族同在这一个导出里——assist（辅助"
        "动作）、filter（可勾选）、input（可移除的输入）、suggestion（建议）。家族共用"
        "一张骨架表，分歧全在状态层与内容行：filter 的按下涟漪**跨选中换色**（未选中按下"
        "是 on-secondary-container——它即将赢得的颜色），input 不换、按在本色里。"
        "选中没有颜色动画：Compose 的 SelectableChipColors 按状态直接解析，容器/文字色"
        "**即刻落位**。Expressive 的圆角形变（medium→full→small）是上游 opt-in 的重载，"
        "经典 corner-small 赢，形变记录在 porting-todo。",
        "Chips: 32dp tall, corner-small pills, four families in this one export — assist "
        "(an action), filter (checkable), input (a removable entry) and suggestion (a "
        "recommendation). The families share one skeleton table; the divergence lives in the "
        "state layers and the content rows: the filter chip's press ripple **swaps across "
        "selection** (an unselected press ripples on-secondary-container — the colour the "
        "chip is about to earn), input does not swap. Selection has no colour animation: "
        "Compose's SelectableChipColors resolve by state, so the container/label land "
        "**instantly**. The Expressive corner morph (medium→full→small) is an upstream "
        "opt-in overload; the classic corner-small wins and the morph is recorded in "
        "porting-todo.");
}

md::MdChip *ChipPage::chip(const QString &id)
{
    const auto found = m_chips.constFind(id);
    if (found != m_chips.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdChip(this);
    created->hide();
    m_chips.insert(id, created);
    return created;
}

void ChipPage::build(GalleryContext &context)
{
    m_slots.clear();

    // Chips hug their content — never stretch a chip across the band.
    auto addRow = [&](const QVector<QPair<QString, md::MdChip *>> &chips) {
        if (chips.isEmpty()) {
            return;
        }
        const qreal pitch = 16.0;
        qreal total = 0.0;
        for (const auto &entry : chips) {
            total += entry.second->sizeHint().width();
        }
        total += pitch * qreal(chips.size() - 1);
        const qreal bandHeight = chips.first().second->sizeHint().height();
        const QRectF band = context.band(bandHeight);
        qreal x = band.left() + qMax<qreal>(0.0, (band.width() - total) / 2.0);
        for (const auto &entry : chips) {
            const qreal w = entry.second->sizeHint().width();
            m_slots.append(ChipSlot{entry.second, QRectF(x, band.top(), w, bandHeight)});
            x += w + pitch;
        }
    };

    // --- the four families ------------------------------------------------------
    context.section(L("四个家族：assist · filter · input · suggestion",
                      "The four families: assist · filter · input · suggestion"));
    context.paragraph(L(
        "assist 的文字 on-surface、前导图标 primary；suggestion 整体调暗为 "
        "on-surface-variant（图标仍是 primary）；filter 未选中同 suggestion 的灰、但"
        "可勾选；input 是可移除的输入项、尾部是动作图标。filter/input 在前导图标旁把"
        "间距收紧到 4px（CompactHorizontalSpacing），assist/suggestion 全程 8px。",
        "Assist labels on-surface with a primary leading icon; suggestion mutes the whole "
        "row to on-surface-variant (the icon still primary); filter is unselected-grey but "
        "checkable; input is a removable entry with an action trailing icon. Filter and "
        "input tighten the gap beside a leading icon to 4px (CompactHorizontalSpacing); "
        "assist and suggestion keep 8px everywhere."));
    {
        auto *assist = chip(QStringLiteral("assist"));
        assist->setText(L("辅助", "Assist"));
        assist->setIconName(QStringLiteral("add"));
        auto *filter = chip(QStringLiteral("filter"));
        filter->setVariant(md::MdChipVariant::Filter);
        filter->setText(L("过滤", "Filter"));
        filter->setIconName(QStringLiteral("mail"));
        auto *input = chip(QStringLiteral("input"));
        input->setVariant(md::MdChipVariant::Input);
        input->setText(L("输入", "Input"));
        input->setIconName(QStringLiteral("person"));
        input->setTrailingIconName(QStringLiteral("close"));
        auto *suggestion = chip(QStringLiteral("suggestion"));
        suggestion->setVariant(md::MdChipVariant::Suggestion);
        suggestion->setText(L("建议", "Suggestion"));
        suggestion->setIconName(QStringLiteral("star"));
        addRow({{QStringLiteral("a"), assist},
                {QStringLiteral("b"), filter},
                {QStringLiteral("c"), input},
                {QStringLiteral("d"), suggestion}});
        context.detail(L("assist · filter · input · suggestion（flat）",
                         "assist · filter · input · suggestion (flat)"));
        context.space(16.0);
    }

    // --- elevated -----------------------------------------------------------------
    context.section(L("elevated：低容器 + 阶梯阴影",
                      "Elevated: the low container + the elevation ladder"));
    context.paragraph(L(
        "elevated 家族容器 surface-container-low，阶梯 1/2/1/1/4/0（resting/hover/"
        "focus/press/drag/disabled）。flat 没有阴影，唯一的例外是**选中的 filter**——"
        "它悬停时升到 level 1。",
        "The elevated families fill surface-container-low and run the 1/2/1/1/4/0 ladder "
        "(resting/hover/focus/press/drag/disabled). Flat chips carry no shadow — the one "
        "exception is a **selected filter**, which rises to level 1 on hover."));
    {
        auto *assist = chip(QStringLiteral("assist-elevated"));
        assist->setKind(md::MdChipKind::Elevated);
        assist->setText(L("辅助", "Assist"));
        auto *filter = chip(QStringLiteral("filter-elevated"));
        filter->setVariant(md::MdChipVariant::Filter);
        filter->setKind(md::MdChipKind::Elevated);
        filter->setText(L("过滤", "Filter"));
        auto *suggestion = chip(QStringLiteral("suggestion-elevated"));
        suggestion->setVariant(md::MdChipVariant::Suggestion);
        suggestion->setKind(md::MdChipKind::Elevated);
        suggestion->setText(L("建议", "Suggestion"));
        addRow({{QStringLiteral("a"), assist},
                {QStringLiteral("b"), filter},
                {QStringLiteral("c"), suggestion}});
        context.detail(L("assist · filter · suggestion（elevated）",
                         "assist · filter · suggestion (elevated)"));
        context.space(16.0);
    }

    // --- selection ---------------------------------------------------------------
    context.section(L("选中：即刻落位的 secondary-container",
                      "Selection: secondary-container, landing instantly"));
    context.paragraph(L(
        "选中的 filter/input 填 secondary-container、文字与图标 on-secondary-container、"
        "描边消失。没有颜色动画——试试点下面那颗 filter，容器的颜色是**这一帧**就换过去"
        "的（会动的只有图标的出现/消失，那是 Compose 的 AnimatingChipContent 存在性"
        "动画，不是颜色动画）。",
        "A selected filter/input fills secondary-container with on-secondary-container "
        "content and no outline. There is no colour animation — click the filter below and "
        "the container lands **this frame** (the only thing that animates is the icons "
        "appearing, which is Compose's AnimatingChipContent presence animation, not a "
        "colour one)."));
    {
        auto *filter = chip(QStringLiteral("filter-selected"));
        filter->setVariant(md::MdChipVariant::Filter);
        filter->setText(L("已选", "Selected"));
        filter->setIconName(QStringLiteral("check"));
        filter->setSelected(true);
        auto *filterPlain = chip(QStringLiteral("filter-unselected"));
        filterPlain->setVariant(md::MdChipVariant::Filter);
        filterPlain->setText(L("未选", "Unselected"));
        auto *input = chip(QStringLiteral("input-selected"));
        input->setVariant(md::MdChipVariant::Input);
        input->setText(L("已选", "Selected"));
        input->setSelected(true);
        addRow({{QStringLiteral("a"), filter},
                {QStringLiteral("b"), filterPlain},
                {QStringLiteral("c"), input}});
        context.detail(L("filter 选中 · filter 未选 · input 选中",
                         "filter selected · filter unselected · input selected"));
        context.space(16.0);
    }

    // --- avatar -----------------------------------------------------------------
    context.section(L("input 的 24dp 头像槽",
                      "The input family's 24dp avatar slot"));
    context.paragraph(L(
        "avatar 尺寸 24dp、corner-full，出现时压倒前导图标槽。上游它是**照片槽**——导出"
        "只给尺寸与禁用淡出（0.38），不给颜色；Qt 侧没有图片槽，这里以图标圆渲染，分歧"
        "记录在 porting-todo。",
        "The avatar is 24dp, corner-full, and displaces the leading icon slot when present. "
        "Upstream it is a **photo slot** — the export sizes it and fades it when disabled "
        "(0.38) but publishes no colour; Qt has no image slot here, so it renders as an "
        "icon circle, a divergence recorded in porting-todo."));
    {
        auto *input = chip(QStringLiteral("input-avatar"));
        input->setVariant(md::MdChipVariant::Input);
        input->setText(L("张三", "Alex"));
        input->setAvatarIconName(QStringLiteral("person"));
        input->setTrailingIconName(QStringLiteral("close"));
        addRow({{QStringLiteral("a"), input}});
        context.detail(L("input + 24dp corner-full 头像 + 移除动作",
                         "input + a 24dp corner-full avatar + the remove action"));
        context.space(16.0);
    }

    // --- disabled and dragged -------------------------------------------------------
    context.section(L("禁用与拖拽：0.12 与 level 4",
                      "Disabled and dragged: 0.12 and level 4"));
    context.paragraph(L(
        "禁用的选中容器淡到 on-surface @0.12，未选中的 flat 容器变透明、描边淡到 0.12，"
        "内容整体 0.38。dragged 行（level 4 阴影 + 0.16 状态层 + dragged 色表）由 "
        "setDragged() 程序化驱动——这里没有拖拽控制器，与 Compose 的 InteractionSource "
        "发出 Drag 的方式一致。",
        "A disabled selected container fades to on-surface @0.12; an unselected flat one "
        "turns transparent with its outline at 0.12 and the content at 0.38. The dragged "
        "rows (level 4 shadow + 0.16 state layer + the dragged tables) are driven "
        "programmatically by setDragged() — there is no drag controller here, matching how "
        "Compose's InteractionSource would emit Drag."));
    {
        auto *disabledSelected = chip(QStringLiteral("filter-disabled-selected"));
        disabledSelected->setVariant(md::MdChipVariant::Filter);
        disabledSelected->setText(L("禁用已选", "Disabled selected"));
        disabledSelected->setSelected(true);
        disabledSelected->setEnabled(false);
        auto *disabledPlain = chip(QStringLiteral("assist-disabled"));
        disabledPlain->setText(L("禁用", "Disabled"));
        disabledPlain->setEnabled(false);
        auto *dragged = chip(QStringLiteral("input-dragged"));
        dragged->setVariant(md::MdChipVariant::Input);
        dragged->setText(L("拖拽中", "Dragged"));
        dragged->setIconName(QStringLiteral("person"));
        dragged->setDragged(true);
        addRow({{QStringLiteral("a"), disabledSelected},
                {QStringLiteral("b"), disabledPlain},
                {QStringLiteral("c"), dragged}});
        context.detail(L("禁用（选中/未选） · dragged（level 4）",
                         "disabled (selected/unselected) · dragged (level 4)"));
        context.space(16.0);
    }

    // --- the live pair ------------------------------------------------------------
    context.section(L("活的：点它们，看涟漪换色",
                      "Live: click them and watch the ripple swap"));
    context.paragraph(L(
        "filter 的按下涟漪是特例所在：**未选中按下涟漪 on-secondary-container**——芯片"
        "即将赢得的颜色；选中按下换 on-surface-variant。input 不换，两侧都按在本色里。"
        "键盘聚焦（Tab）画 secondary 的焦点环，外偏移 2dp、3dp 粗，鼠标点击不画。",
        "The filter press ripple is where the special case lives: an **unselected press "
        "ripples on-secondary-container** — the colour the chip is about to earn — and a "
        "selected press ripples on-surface-variant. Input does not swap: both sides press "
        "in the colour they have. A keyboard focus (Tab) draws the secondary ring, a 2dp "
        "outer offset and 3dp thick; a mouse focus draws none."));
    {
        auto *filter = chip(QStringLiteral("live-filter"));
        filter->setVariant(md::MdChipVariant::Filter);
        filter->setText(L("过滤", "Filter"));
        auto *input = chip(QStringLiteral("live-input"));
        input->setVariant(md::MdChipVariant::Input);
        input->setText(L("输入", "Input"));
        input->setIconName(QStringLiteral("person"));
        addRow({{QStringLiteral("a"), filter}, {QStringLiteral("b"), input}});
        context.detail(L("filter（可勾选） · input（可交互）",
                         "filter (checkable) · input (interactive)"));
        context.space(16.0);
    }

    // --- what this page is pinned to -----------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "以下每一条都由 TestMd3Chip 直接断言，改一处就会红。",
        "Every line below is asserted directly by TestMd3Chip; moving one turns it red."));
    context.detail(L("32dp 高、corner-small、18dp 图标、1dp flat 描边 · 头像 24dp "
                     "corner-full（仅 input）· ContentPadding 8 · 状态层 0.08/0.12/0.12/"
                     "0.16",
                     "32dp tall, corner-small, 18dp icons, a 1dp flat outline · the avatar "
                     "24dp corner-full (input only) · ContentPadding 8 · state layers "
                     "0.08/0.12/0.12/0.16"));
    context.detail(L("按下换色：**filter 未选中按下 on-secondary-container、选中按下 "
                     "on-surface-variant**；input 不换 · filter 选中悬停升 level 1（flat "
                     "唯一的阴影行）",
                     "the press swap: the **unselected filter press is "
                     "on-secondary-container, the selected press on-surface-variant**; "
                     "input does not swap · a selected filter rises to level 1 on hover "
                     "(flat's only shadow row)"));
    context.detail(L("排布：label 是加权的中间段——拉伸时它变宽，尾部图标钉在右内边距 · "
                     "filter/input 前导间距 4px · input 前导/尾部图标在 hover/focus/press/"
                     "drag 提到 primary",
                     "arrangement: the label is the weighted middle — a stretched chip "
                     "widens it and pins the trailing icon to the right padding · filter/"
                     "input tighten the leading gap to 4px · input's leading/trailing icons "
                     "lift to primary under hover/focus/press/drag"));
    context.detail(L("无颜色动画（SelectableChipColors 按状态解析）· Expressive 圆角形变"
                     "（medium/full/small + 空间弹簧）是 opt-in 重载，未移植 · elevated "
                     "阶梯 1/2/1/1/4/0",
                     "no colour animation (SelectableChipColors resolve by state) · the "
                     "Expressive corner morph (medium/full/small + the spatial spring) is "
                     "an opt-in overload, not ported · the elevated ladder 1/2/1/1/4/0"));
    context.detail(L("弃用/缺口行承载不读：ChipsTokens 的 Unselected/Selected/Pressed 三"
                     "形状（Expressive opt-in）· avatar 无颜色行（上游是照片槽）· "
                     "ChipArrangement 的 trailing 右对齐由本库布局自己读",
                     "deprecated/gap rows carried, not read: ChipsTokens' unselected/"
                     "selected/pressed trio of shapes (the Expressive opt-in) · the avatar "
                     "publishes no colour (a photo slot upstream) · ChipArrangement's "
                     "trailing right-alignment is read by this library's own layout"));
    context.space(8.0);
}

void ChipPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const ChipSlot &slot : m_slots) {
        const QSize hint = slot.widget->sizeHint();
        const QRect target(content.topLeft().toPoint() + slot.rect.topLeft().toPoint(), hint);
        if (slot.widget->geometry() != target) {
            slot.widget->setGeometry(target);
        }
        if (!slot.widget->isVisible()) {
            slot.widget->show();
        }
    }
}

void ChipPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void ChipPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void ChipPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void ChipPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createChipPage()
{
    return new ChipPage;
}

} // namespace gallery

#include "GalleryPages35.moc"
