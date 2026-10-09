// Gallery page 34: Checkbox — `md.comp.checkbox.*` at export version 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Every box on this page is a
// live MdCheckBox — the check draw-in, the indeterminate morph, the press
// ripple and the focus ring are all the real component.
//
// Three things this page is explicit about because they are easy to misread:
//
//   * the **state-layer special cases** — an unchecked press ripples `primary`
//     (the colour the box is about to earn) while a checked press ripples
//     `on-surface`; the hover and focus layers are `on-surface` unchecked and
//     `primary` checked;
//   * **indeterminate is a selected state** with a gravitation of its own —
//     the dash is the check path gravitated onto the centre line, not a second
//     glyph;
//   * the **error variant** overrides the enabled rows only — a disabled error
//     box falls back to the base disabled rows.

#include "GalleryPages.h"

#include "widgets/MdCheckBox.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

namespace gallery {

namespace {

struct CheckboxSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class CheckboxPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit CheckboxPage(QWidget *parent = nullptr);
    ~CheckboxPage() override;

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

    md::MdCheckBox *box(const QString &id);

    QHash<QString, md::MdCheckBox *> m_boxes;
    QVector<CheckboxSlot> m_slots;
};

CheckboxPage::CheckboxPage(QWidget *parent)
    : GalleryPage(parent)
{
}

CheckboxPage::~CheckboxPage() = default;

QString CheckboxPage::title() const
{
    return L("复选框", "Checkbox");
}

QString CheckboxPage::slug() const
{
    return QStringLiteral("checkbox");
}

QString CheckboxPage::subtitle() const
{
    return L(
        "复选框：18dp 的方 2dp 圆角盒子居中在 48dp 的触控目标里，按下时背后是一层 40dp 的"
        "**圆形**状态层。未选中的盒子只画 2dp 的描边（on-surface-variant，交互时提到 "
        "on-surface），选中的盒子是**一整块 primary 填充**——描边与填充同色，Compose 的 "
        "drawBox 把这两条合并成一次填充。对号不是另一个图形：它就是一条按 Compose 比例 "
        "(0.25, 0.5) → (0.4, 0.65) → (0.75, 0.3) 画出的路径，沿长度用空间默认弹簧画入；"
        "indeterminate 的短横是**同一条路径**被引力拉到中线的结果。数值来自官方导出、"
        "行为来自 Compose 的 `Checkbox.kt`。",
        "Checkbox: an 18dp box with 2dp corners centred in a 48dp touch target, with a 40dp "
        "**circular** state layer behind it when pressed. An unchecked box paints only its "
        "2dp outline (on-surface-variant, lifting to on-surface under interaction); a checked "
        "box is **one primary fill** — the border and fill resolve to the same colour, and "
        "Compose's drawBox collapses the pair. The check is not a second glyph: it is one "
        "path through Compose's fractions (0.25, 0.5) → (0.4, 0.65) → (0.75, 0.3), drawn in "
        "along its length on the spatial default spring; the indeterminate dash is **the same "
        "path** gravitated onto the centre line. Numbers from the export, behaviour from "
        "Compose's `Checkbox.kt`.");
}

md::MdCheckBox *CheckboxPage::box(const QString &id)
{
    const auto found = m_boxes.constFind(id);
    if (found != m_boxes.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdCheckBox(this);
    created->hide();
    m_boxes.insert(id, created);
    return created;
}

void CheckboxPage::build(GalleryContext &context)
{
    m_slots.clear();

    // A row of checkboxes shares one band; each box is a 48 dp touch target.
    auto addRow = [&](const QVector<QPair<QString, md::MdCheckBox *>> &boxes) {
        if (boxes.isEmpty()) {
            return;
        }
        const qreal pitch = 64.0;
        const qreal bandHeight = boxes.first().second->sizeHint().height();
        const QRectF band = context.band(bandHeight);
        const qreal span = (boxes.size() - 1) * pitch + bandHeight;
        qreal x = band.left() + qMax<qreal>(0.0, (band.width() - span) / 2.0);
        for (const auto &entry : boxes) {
            m_slots.append(CheckboxSlot{entry.second,
                                        QRectF(x, band.top(), bandHeight, bandHeight)});
            x += pitch;
        }
    };

    // --- the three check states ----------------------------------------------
    context.section(L("三态：未选、选中、部分选中",
                      "Three states: unchecked, checked, indeterminate"));
    context.paragraph(L(
        "未选中的盒子只画描边；选中的盒子一整块 primary、对号 on-primary；部分选中的"
        "盒子填充与选中相同，对号被**引力**拉平成短横。三个都是活的控件：点击或按空格"
        "切换，对号沿长度画入，撤销时按 Compose 的 100ms snap 延迟保持原状再消失。"
        "tristate 的循环是 Off → On → Indeterminate → Off。",
        "An unchecked box paints only its outline; a checked box is one primary fill with an "
        "on-primary check; an indeterminate box fills the same but its check is gravitated "
        "flat into the dash. All three are live: click or press Space to cycle, the check "
        "draws in along its length, and an undo holds for Compose's 100ms snap delay before "
        "it vanishes. A tristate cycles Off → On → Indeterminate → Off."));
    {
        md::MdCheckBox *unchecked = box(QStringLiteral("plain-unchecked"));
        md::MdCheckBox *checked = box(QStringLiteral("plain-checked"));
        checked->setCheckState(Qt::Checked);
        md::MdCheckBox *indeterminate = box(QStringLiteral("plain-indeterminate"));
        indeterminate->setTristate(true);
        indeterminate->setCheckState(Qt::PartiallyChecked);
        addRow({{QStringLiteral("a"), unchecked},
                {QStringLiteral("b"), checked},
                {QStringLiteral("c"), indeterminate}});
        context.detail(L("未选 · 选中 · 部分选中（tristate）",
                         "unchecked · checked · indeterminate (tristate)"));
        context.space(16.0);
    }

    // --- disabled ------------------------------------------------------------
    context.section(L("禁用：整体 0.38，不做过渡",
                      "Disabled: the whole box at 0.38, with no transition"));
    context.paragraph(L(
        "禁用不是把内容涂灰：选中的盒子读 `selected.disabled.container.opacity` 的 0.38"
        "（on-surface），对号换 surface、全强度；未选中的盒子描边保持 2dp、on-surface、"
        "同样 0.38。进入禁用**直接落位**——Compose 的原话是 \"there should be no animations "
        "between enabled / disabled\"。",
        "Disabled is not a grey coat of paint: a checked box reads "
        "`selected.disabled.container.opacity` 0.38 (on-surface) with its check in surface at "
        "full strength; an unchecked box keeps its 2dp outline in on-surface at the same 0.38. "
        "Entering disabled **snaps** — Compose: \"there should be no animations between "
        "enabled / disabled\"."));
    {
        md::MdCheckBox *unchecked = box(QStringLiteral("disabled-unchecked"));
        unchecked->setEnabled(false);
        md::MdCheckBox *checked = box(QStringLiteral("disabled-checked"));
        checked->setCheckState(Qt::Checked);
        checked->setEnabled(false);
        md::MdCheckBox *indeterminate = box(QStringLiteral("disabled-indeterminate"));
        indeterminate->setCheckState(Qt::PartiallyChecked);
        indeterminate->setEnabled(false);
        addRow({{QStringLiteral("a"), unchecked},
                {QStringLiteral("b"), checked},
                {QStringLiteral("c"), indeterminate}});
        context.detail(L("未选 · 选中 · 部分选中（全部禁用）",
                         "unchecked · checked · indeterminate (all disabled)"));
        context.space(16.0);
    }

    // --- the error variant -----------------------------------------------------
    context.section(L("错误变体：表单校验色",
                      "The error variant: the form-validation colours"));
    context.paragraph(L(
        "error 不是另一套 token：选中的盒子容器 error、对号 on-error；未选中的描边 error。"
        "悬停/聚焦/按下的状态层也换 error。它只覆盖**启用态**的行——禁用的错误盒子落回"
        "基础 disabled 行。",
        "error is not a second token set: a checked box fills error with an on-error check; an "
        "unchecked one outlines error; the hover/focus/pressed state layers switch to error "
        "too. It overrides the **enabled** rows only — a disabled error box falls back to the "
        "base disabled rows."));
    {
        md::MdCheckBox *unchecked = box(QStringLiteral("error-unchecked"));
        unchecked->setError(true);
        md::MdCheckBox *checked = box(QStringLiteral("error-checked"));
        checked->setCheckState(Qt::Checked);
        checked->setError(true);
        addRow({{QStringLiteral("a"), unchecked}, {QStringLiteral("b"), checked}});
        context.detail(L("未选 · 选中（error）", "unchecked · checked (error)"));
        context.space(16.0);
    }

    // --- the live pair ------------------------------------------------------------
    context.section(L("活的两个：点它们",
                      "Two live ones: click them"));
    context.paragraph(L(
        "按下的涟漪颜色是特例所在：未选中按下时涟漪是 **primary**——盒子即将赢得的颜色；"
        "选中按下时是 **on-surface**。悬停与聚焦的平层跟随各自的表：未选中 on-surface、"
        "选中 primary、error 换 error。键盘聚焦（Tab）在盒子外圈画 secondary 的焦点环，"
        "外偏移 2dp、3dp 粗。",
        "The press ripple is where the special cases live: an unchecked press ripples "
        "**primary** — the colour the box is about to earn — and a checked press ripples "
        "**on-surface**. The hover and focus layers follow their own tables: on-surface "
        "unchecked, primary checked, error in the error variant. A keyboard focus (Tab) draws "
        "the secondary ring around the box, a 2dp gap and 3dp thick."));
    {
        md::MdCheckBox *plain = box(QStringLiteral("live-plain"));
        md::MdCheckBox *tristate = box(QStringLiteral("live-tristate"));
        tristate->setTristate(true);
        addRow({{QStringLiteral("a"), plain}, {QStringLiteral("b"), tristate}});
        context.detail(L("二态 · tristate（可交互）", "two-state · tristate (interactive)"));
        context.space(16.0);
    }

    // --- what this page is pinned to -----------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "以下每一条都由 TestMd3CheckBox 直接断言，改一处就会红。",
        "Every line below is asserted directly by TestMd3CheckBox; moving one turns it red."));
    context.detail(L("18dp 容器、2dp 圆角、18dp 图标、2dp 描边（CheckboxDefaults.StrokeWidth）"
                     "· 40dp 圆形状态层（corner-full）· 48dp 触控目标",
                     "18dp container, 2dp corners, 18dp icon, 2dp stroke "
                     "(CheckboxDefaults.StrokeWidth) · 40dp circular state layer (corner-full) "
                     "· 48dp touch target"));
    context.detail(L("状态层特例：**未选中按下 primary、选中按下 on-surface**（error 变体"
                     "整体换 error）；未选中悬停/聚焦层 on-surface，选中 primary",
                     "state-layer special cases: the **unselected press is primary, the "
                     "selected press on-surface** (the error variant switches all of them to "
                     "error); unchecked hover/focus layers on-surface, checked primary"));
    context.detail(L("对号路径：0.25/0.5 → 0.4/0.65 → 0.75/0.3（styling fix 的比例），沿长度"
                     "由空间默认弹簧画入；撤销走 `snap(delay = 100)`；Off → Indeterminate 的"
                     "引力直接 snap 到位",
                     "check path: 0.25/0.5 → 0.4/0.65 → 0.75/0.3 (the styling fix's "
                     "fractions), revealed along its length on the spatial default spring; an "
                     "undo runs `snap(delay = 100)`; Off → Indeterminate snaps the gravitation "
                     "into place"));
    context.detail(L("disabled：selected 容器 on-surface @0.38、对号 surface；unselected 描边 "
                     "2dp on-surface @0.38 · 焦点环 secondary、外偏移 2dp、3dp 粗",
                     "disabled: the selected container on-surface @0.38 with its check in "
                     "surface; the unselected outline 2dp on-surface @0.38 · the focus ring "
                     "secondary, a 2dp outer offset, 3dp thick"));
    context.detail(L("弃用行承载不读：unselected.*.icon.color、disabled.*.icon.*（渲染模型"
                     "改造前的残留）；error 的 outline-width 行标注冗余——数值与基础行相同；"
                     "Compose 未选中按下涟漪 Transparent 是上游疏漏，导出行 primary 赢",
                     "deprecated rows carried, not read: unselected.*.icon.color and "
                     "disabled.*.icon.* (pre-rework residue); the error outline-width rows are "
                     "marked redundant — the base rows carry the same numbers; Compose's "
                     "transparent unchecked ripple is an upstream oversight, the export's "
                     "primary row wins"));
    context.space(8.0);
}

void CheckboxPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const CheckboxSlot &slot : m_slots) {
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

void CheckboxPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void CheckboxPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void CheckboxPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void CheckboxPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createCheckboxPage()
{
    return new CheckboxPage;
}

} // namespace gallery

#include "GalleryPages34.moc"
