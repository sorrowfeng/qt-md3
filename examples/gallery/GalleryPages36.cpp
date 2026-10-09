// Gallery page 36: Radio button — `md.comp.radio-button.*` at export version
// 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Every circle on this page is
// a live MdRadioButton — the dot's fast-spatial scale, the colour fade, the
// press ripple and the focus ring are all the real component.
//
// Three things this page is explicit about because they are easy to misread:
//
//   * the **state-layer special cases** — an unselected press ripples
//     `primary` (the colour the button is about to earn) while a selected
//     press ripples `on-surface`; hover and focus read each side's own colour;
//   * **a checked radio cannot be unchecked** — not by clicking (Compose's
//     `selectable` never unchecks) and not even by a programmatic
//     `setChecked(false)`, which Qt ignores for an autoExclusive checked
//     button; the only deselection is another sibling's check;
//   * **one colour paints the stroke and the dot** — the icon table's row.

#include "GalleryPages.h"

#include "core/MdRadioButtonTokens.h"
#include "widgets/MdRadioButton.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QSizePolicy>

namespace gallery {

namespace {

struct RadioSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class RadioButtonPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit RadioButtonPage(QWidget *parent = nullptr);
    ~RadioButtonPage() override;

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

    md::MdRadioButton *radio(const QString &id);

    QHash<QString, md::MdRadioButton *> m_radios;
    QVector<RadioSlot> m_slots;
    /// One group per section: without these every radio on the page would
    /// share the page parent's autoExclusive group, and checking the disabled
    /// sample would silently uncheck the selected sample above it.
    QVector<QButtonGroup *> m_groups;
};

RadioButtonPage::RadioButtonPage(QWidget *parent)
    : GalleryPage(parent)
{
}

RadioButtonPage::~RadioButtonPage() = default;

QString RadioButtonPage::title() const
{
    return L("单选钮", "Radio button");
}

QString RadioButtonPage::slug() const
{
    return QStringLiteral("radio-button");
}

QString RadioButtonPage::subtitle() const
{
    return L(
        "单选钮：20dp 的 2dp 描边圆居中在 48dp 的触控目标里，按下时背后是一层 40dp 的"
        "**圆形**状态层。选中的点不是另一个图形：它是 12dp 直径（扣除半描边后 5dp 半径）"
        "的填充圆，沿 Compose 的**快速空间弹簧**从 0 缩放进来、取消时缩回去。描边与点"
        "**共用一个颜色**——图标表的那一行。导出只有 27 行：没有容器、没有 error 变体、"
        "没有 focus-indicator 行（环走系统值 + Compose 的 CircleShape）。按下涟漪的"
        "特例与复选框同形：**未选中按下 primary、选中按下 on-surface**。",
        "Radio button: a 20dp circle stroked 2dp, centred in a 48dp touch target, with a "
        "40dp **circular** state layer behind it when pressed. The selected dot is not a "
        "second glyph: it is a filled circle of 12dp diameter (5dp radius after the stroke "
        "inset) scaled in from 0 on Compose's **fast spatial** spring, and back out on "
        "deselect. Stroke and dot share **one colour** — the icon table's row. The export "
        "is 27 rows: no container, no error variant, no focus-indicator rows (the ring "
        "carries the system values + Compose's CircleShape). The press ripple's special "
        "cases match the checkbox's: the **unselected press is primary, the selected press "
        "on-surface**.");
}

md::MdRadioButton *RadioButtonPage::radio(const QString &id)
{
    const auto found = m_radios.constFind(id);
    if (found != m_radios.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdRadioButton(this);
    created->hide();
    m_radios.insert(id, created);
    return created;
}

void RadioButtonPage::build(GalleryContext &context)
{
    m_slots.clear();

    // One exclusive group per section row.
    auto addRow = [&](const QVector<QPair<QString, md::MdRadioButton *>> &radios) {
        if (radios.isEmpty()) {
            return;
        }
        auto *group = new QButtonGroup(this);
        group->setExclusive(true);
        for (const auto &entry : radios) {
            group->addButton(entry.second);
        }
        m_groups.append(group);

        const qreal pitch = 64.0;
        const qreal bandHeight = radios.first().second->sizeHint().height();
        const QRectF band = context.band(bandHeight);
        const qreal span = (radios.size() - 1) * pitch + bandHeight;
        qreal x = band.left() + qMax<qreal>(0.0, (band.width() - span) / 2.0);
        for (const auto &entry : radios) {
            m_slots.append(RadioSlot{entry.second,
                                     QRectF(x, band.top(), bandHeight, bandHeight)});
            x += pitch;
        }
    };

    // --- the two selection sides -----------------------------------------------
    context.section(L("两侧：未选与选中", "The two sides: unselected and selected"));
    context.paragraph(L(
        "未选中的圆只画 2dp 的描边（on-surface-variant，交互时提到 on-surface），中心"
        "透明；选中的圆描边 primary，点沿快速空间弹簧缩放进来。描边与点共用一个颜色——"
        "图标表的那一行。组互斥是 QRadioButton 的原生行为：点同组的另一颗，这颗的点就"
        "缩回去。",
        "An unselected circle paints only its 2dp stroke (on-surface-variant, lifting to "
        "on-surface under interaction) with a transparent centre; a selected one strokes "
        "primary and scales the dot in on the fast spatial spring. Stroke and dot share "
        "one colour — the icon table's row. Group exclusivity is QRadioButton's native: "
        "check a sibling and this dot scales back out."));
    {
        auto *plain = radio(QStringLiteral("plain-unchecked"));
        auto *checked = radio(QStringLiteral("plain-checked"));
        checked->setChecked(true);
        auto *grouped = radio(QStringLiteral("plain-sibling"));
        addRow({{QStringLiteral("a"), plain},
                {QStringLiteral("b"), checked},
                {QStringLiteral("c"), grouped}});
        context.detail(L("未选 · 选中 · 组员（标签是调用者的组件，本组件不绘制文字）",
                         "unselected · selected · a group member (the label is the "
                         "caller's widget; this component paints no text)"));
        context.space(16.0);
    }

    // --- disabled ---------------------------------------------------------------
    context.section(L("禁用：整体 0.38，直接落位",
                      "Disabled: the whole circle at 0.38, landing instantly"));
    context.paragraph(L(
        "禁用读 `disabled.*.icon.opacity` 的 0.38（on-surface），两侧同价——导出里"
        "selected 与 unselected 两行是完全相同的数值。进入禁用**直接落位**：点的缩放"
        "与色过渡的计时器都被掐停，Compose 的原话是 \"there should be no animations "
        "between enabled / disabled\"。",
        "Disabled reads `disabled.*.icon.opacity` 0.38 (on-surface), identical on both "
        "sides — the export's selected and unselected rows carry the same numbers. "
        "Entering disabled **snaps**: the dot's scale and the colour fade's timers are "
        "both cut — Compose: \"there should be no animations between enabled / "
        "disabled\"."));
    {
        auto *unchecked = radio(QStringLiteral("disabled-unchecked"));
        unchecked->setEnabled(false);
        auto *checked = radio(QStringLiteral("disabled-checked"));
        checked->setChecked(true);
        checked->setEnabled(false);
        addRow({{QStringLiteral("a"), unchecked}, {QStringLiteral("b"), checked}});
        context.detail(L("未选 · 选中（全部禁用）", "unselected · selected (all disabled)"));
        context.space(16.0);
    }

    // --- the live group ------------------------------------------------------------
    context.section(L("活的一组：点它们", "A live group: click them"));
    context.paragraph(L(
        "按下的涟漪颜色是特例所在：**未选中按下涟漪 primary**——即将赢得的颜色；选中"
        "按下是 **on-surface**。悬停与聚焦的平层读各自的表：未选中 on-surface、选中 "
        "primary。已选中的单选钮点它自己**不会取消**——Compose 的 selectable 从不"
        "反选，Qt 对 autoExclusive 的已选中钮连 setChecked(false) 都会忽略；唯一的"
        "取消路径是点同组的另一颗。键盘聚焦（Tab）画 secondary 的圆形焦点环，外偏移 "
        "2dp、3dp 粗。",
        "The press ripple is where the special cases live: an **unselected press ripples "
        "**primary** — the colour the button is about to earn — and a selected press "
        "**on-surface**. The hover and focus layers read their own tables: on-surface "
        "unselected, primary selected. A checked radio **cannot be unchecked** — not by "
        "clicking it (Compose's selectable never unchecks) and not even by a programmatic "
        "setChecked(false), which Qt ignores for an autoExclusive checked button; the "
        "only deselection is another sibling's check. A keyboard focus (Tab) draws the "
        "secondary circular ring, a 2dp gap and 3dp thick."));
    {
        auto *first = radio(QStringLiteral("live-first"));
        first->setText(L("选项一", "Option one"));
        auto *second = radio(QStringLiteral("live-second"));
        second->setText(L("选项二", "Option two"));
        auto *third = radio(QStringLiteral("live-third"));
        third->setText(L("选项三", "Option three"));
        addRow({{QStringLiteral("a"), first},
                {QStringLiteral("b"), second},
                {QStringLiteral("c"), third}});
        context.detail(L("同组的三颗（可交互）", "three siblings (interactive)"));
        context.space(16.0);
    }

    // --- what this page is pinned to -----------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "以下每一条都由 TestMd3RadioButton 直接断言，改一处就会红。",
        "Every line below is asserted directly by TestMd3RadioButton; moving one turns it "
        "red."));
    context.detail(L("20dp 图标、40dp 圆形状态层（corner-full）· 48dp 触控目标 · 描边 2dp、"
                     "点 12dp（画在 dotRadius − 半描边，即 5dp 半径）、padding 2dp——后"
                     "三者是 Compose 常量，导出只给 icon.size",
                     "20dp icon, 40dp circular state layer (corner-full) · 48dp touch "
                     "target · stroke 2dp, dot 12dp (drawn at dotRadius − half stroke, i.e. "
                     "a 5dp radius), padding 2dp — the latter three are Compose constants, "
                     "the export publishes only icon.size"));
    context.detail(L("状态层特例：**未选中按下 primary、选中按下 on-surface**；未选中悬停"
                     "/聚焦层 on-surface，选中 primary · 无 enabled/dragged/disabled 状态"
                     "层行",
                     "state-layer special cases: the **unselected press is primary, the "
                     "selected press on-surface**; unselected hover/focus layers "
                     "on-surface, selected primary · no enabled/dragged/disabled state-"
                     "layer rows"));
    context.detail(L("点的缩放走快速空间弹簧（进和出同一根，无 snap 延迟）；色过渡进 "
                     "DefaultEffects 出 FastEffects；禁用直接落位",
                     "the dot scales on the fast spatial spring (the same one in and out, "
                     "no snap delays); the colour fades in on DefaultEffects and out on "
                     "FastEffects; disabled snaps"));
    context.detail(L("导出无 error 变体、无 drag 行、无 focus-indicator 行——环走系统值"
                     "（外偏移 2dp、3dp 粗）+ Compose 的 CircleShape · 弃用行承载不读：无"
                     "（这个导出没有弃用行）",
                     "the export has no error variant, no drag rows, no focus-indicator "
                     "rows — the ring carries the system values (a 2dp gap, 3dp thick) + "
                     "Compose's CircleShape · deprecated rows carried, not read: none "
                     "(this export publishes no deprecated rows)"));
    context.detail(L("互斥语义：autoExclusive 的已选中钮**不可反选**——点击不取消"
                     "（Compose 的 selectable 同）、setChecked(false) 被 Qt 静默忽略；唯"
                     "一的取消路径是同组另一颗被选中",
                     "exclusivity semantics: an autoExclusive checked radio **cannot be "
                     "unchecked** — clicking does not (Compose's selectable either) and "
                     "setChecked(false) is silently ignored by Qt; the only deselection "
                     "path is a sibling's check"));
    context.space(8.0);
}

void RadioButtonPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const RadioSlot &slot : m_slots) {
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

void RadioButtonPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void RadioButtonPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void RadioButtonPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void RadioButtonPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createRadioButtonPage()
{
    return new RadioButtonPage;
}

} // namespace gallery

#include "GalleryPages36.moc"
