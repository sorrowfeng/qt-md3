// Gallery page 37: Switch — `md.comp.switch.*` at export version 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Every track on this page is
// a live MdSwitch — the thumb's travel+resize spring, the pressed snap, the
// thumb-riding ripple and the focus ring are all the real component.
//
// Three things this page is explicit about because they are easy to misread:
//
//   * **the press snaps, the release springs** — while pressed the thumb's
//     size and offset land instantly (Compose's `SnapSpec`); releasing hands
//     them back to the fast spatial spring;
//   * **the colours do not animate** — the track/handle/icon resolve by state
//     and land this frame;
//   * **the disabled selected handle keeps full strength** — only the track
//     (0.12), the icon (0.38) and the unselected handle (0.38) fade.

#include "GalleryPages.h"

#include "core/MdSwitchTokens.h"
#include "widgets/MdSwitch.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

namespace gallery {

namespace {

struct SwitchSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class SwitchPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit SwitchPage(QWidget *parent = nullptr);
    ~SwitchPage() override;

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

    md::MdSwitch *sw(const QString &id);

    QHash<QString, md::MdSwitch *> m_switches;
    QVector<SwitchSlot> m_slots;
};

SwitchPage::SwitchPage(QWidget *parent)
    : GalleryPage(parent)
{
}

SwitchPage::~SwitchPage() = default;

QString SwitchPage::title() const
{
    return L("开关", "Switch");
}

QString SwitchPage::slug() const
{
    return QStringLiteral("switch");
}

QString SwitchPage::subtitle() const
{
    return L(
        "开关：52×32dp 的 corner-full 轨道带 2dp 描边，拇指在里面**旅行并变大**——"
        "未选中 16dp 停在轨道左内侧（内缩 8dp），选中 24dp 走到远端（内缩 4dp），"
        "按下 28dp **瞬间**落位并向内再让 2dp（Compose 的 SnapSpec），松手交给快速"
        "空间弹簧弹回。颜色**不做动画**：轨道/拇指/图标的颜色按状态解析、这一帧就落位。"
        "涟漪骑在拇指上——40dp 的圆跟着拇指走。禁用只有轨道（0.12）、图标与未选中拇指"
        "（0.38）淡出——**选中的拇指保持全强度**（disabled.selected.handle.opacity 是 1）。",
        "Switch: a 52×32dp corner-full track with a 2dp outline, and a thumb that "
        "**travels and grows** inside it — 16dp resting at the track's left inset (8dp "
        "in), 24dp at the far bound when checked (4dp in), and **28dp landing "
        "instantly** when pressed, 2dp further inward (Compose's SnapSpec); releasing "
        "hands it back to the fast spatial spring. The colours **do not animate**: "
        "track/handle/icon resolve by state and land this frame. The ripple rides the "
        "thumb — a 40dp circle that follows it. Disabled fades only the track (0.12), "
        "the icon and the unselected handle (0.38) — **the selected handle keeps full "
        "strength** (disabled.selected.handle.opacity is 1).");
}

md::MdSwitch *SwitchPage::sw(const QString &id)
{
    const auto found = m_switches.constFind(id);
    if (found != m_switches.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSwitch(this);
    created->hide();
    m_switches.insert(id, created);
    return created;
}

void SwitchPage::build(GalleryContext &context)
{
    m_slots.clear();

    // Switches are fixed content — place by sizeHint (52×48).
    auto addRow = [&](const QVector<QPair<QString, md::MdSwitch *>> &switches) {
        if (switches.isEmpty()) {
            return;
        }
        const qreal pitch = 72.0;
        const qreal bandHeight = switches.first().second->sizeHint().height();
        const QRectF band = context.band(bandHeight);
        const qreal span = (switches.size() - 1) * pitch + bandHeight;
        qreal x = band.left() + qMax<qreal>(0.0, (band.width() - span) / 2.0);
        for (const auto &entry : switches) {
            m_slots.append(SwitchSlot{entry.second,
                                      QRectF(x, band.top(), bandHeight, bandHeight)});
            x += pitch;
        }
    };

    // --- the two sides ----------------------------------------------------------
    context.section(L("两侧与图标拇指", "The two sides, and the icon thumb"));
    context.paragraph(L(
        "未选中的轨道 surface-container-highest、2dp outline 描边、16dp 的 outline 色"
        "拇指；选中的轨道 primary、拇指 on-primary。带图标时拇指是 24dp（Compose 的 "
        "hasContent 分支——与选中同径）、描边不消失，图标 16dp 读 icon 行：未选中与轨道"
        "同色（surface-container-highest）、选中 primary。",
        "The unselected track is surface-container-highest with a 2dp outline and a 16dp "
        "outline-coloured thumb; the selected track is primary with an on-primary thumb. "
        "With an icon the thumb is 24dp even unchecked (Compose's hasContent branch — "
        "the same diameter as checked), the outline stays, and the 16dp icon reads the "
        "icon row: the track's own colour unchecked, primary selected."));
    {
        auto *plain = sw(QStringLiteral("plain-unchecked"));
        auto *checked = sw(QStringLiteral("plain-checked"));
        checked->setChecked(true);
        auto *withIcon = sw(QStringLiteral("plain-icon"));
        withIcon->setIconName(QStringLiteral("check"));
        auto *iconChecked = sw(QStringLiteral("plain-icon-checked"));
        iconChecked->setIconName(QStringLiteral("check"));
        iconChecked->setChecked(true);
        addRow({{QStringLiteral("a"), plain},
                {QStringLiteral("b"), checked},
                {QStringLiteral("c"), withIcon},
                {QStringLiteral("d"), iconChecked}});
        context.detail(L("未选 · 选中 · 带图标未选 · 带图标选中",
                         "unselected · selected · with-icon unchecked · with-icon checked"));
        context.space(16.0);
    }

    // --- disabled -----------------------------------------------------------------
    context.section(L("禁用：按行淡出，选中拇指不淡",
                      "Disabled: fading by row, the selected handle does not"));
    context.paragraph(L(
        "禁用不是一刀切的 0.38：轨道与描边走自己的 0.12（on-surface），未选中的拇指 "
        "on-surface @0.38、图标 @0.38——而**选中的拇指是 surface @1**，全强度。导出的 "
        "disabled.selected.handle.opacity 行就是这么写的；selected 与 unselected 的"
        "淡出行各自独立。",
        "Disabled is not one blanket 0.38: the track and its outline fade at their own "
        "0.12 (on-surface), the unselected handle at on-surface @0.38 and the icon at "
        "0.38 — while **the selected handle is surface @1**, full strength. That is what "
        "the export's disabled.selected.handle.opacity row publishes; the selected and "
        "unselected fade rows are independent."));
    {
        auto *unchecked = sw(QStringLiteral("disabled-unchecked"));
        unchecked->setEnabled(false);
        auto *checked = sw(QStringLiteral("disabled-checked"));
        checked->setChecked(true);
        checked->setEnabled(false);
        addRow({{QStringLiteral("a"), unchecked}, {QStringLiteral("b"), checked}});
        context.detail(L("未选 · 选中（全部禁用）", "unselected · selected (all disabled)"));
        context.space(16.0);
    }

    // --- the live pair ---------------------------------------------------------------
    context.section(L("活的两个：按住它们", "Two live ones: press and hold"));
    context.paragraph(L(
        "按下是这套组件最特别的时刻：拇指**瞬间**跳到按压目标（28dp、向内再让 2dp——"
        "选中的从远端 24dp 处收到 22dp，未选中的从 8dp 处收到 2dp），涟漪同时在拇指上"
        "漾开（未选中 on-surface、选中 primary）；松手的一刻尺寸与位置交还给快速空间"
        "弹簧弹回。颜色全程无动画。键盘聚焦（Tab）在**轨道**外圈画 secondary 的焦点环。",
        "The press is this component's special moment: the thumb **snaps** to its pressed "
        "target (28dp, 2dp further inward — the checked one pulls from the 24dp far bound "
        "to 22dp, the unchecked from 8dp to 2dp) while the ripple blooms at the thumb "
        "(on-surface unselected, primary selected); releasing hands size and offset back "
        "to the fast spatial spring. No colour animates at any point. A keyboard focus "
        "(Tab) draws the secondary ring around the **track**."));
    {
        auto *plain = sw(QStringLiteral("live-plain"));
        auto *checked = sw(QStringLiteral("live-checked"));
        checked->setChecked(true);
        addRow({{QStringLiteral("a"), plain}, {QStringLiteral("b"), checked}});
        context.detail(L("未选 · 选中（可交互）", "unselected · selected (interactive)"));
        context.space(16.0);
    }

    // --- what this page is pinned to -----------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "以下每一条都由 TestMd3Switch 直接断言，改一处就会红。",
        "Every line below is asserted directly by TestMd3Switch; moving one turns it "
        "red."));
    context.detail(L("轨道 52×32dp、corner-full、2dp 描边 · 触控目标 52×48 · 拇指 16/24/"
                     "28/24dp（未选/选中/按压/带图标）· 图标 16dp · 状态层 40dp",
                     "a 52×32dp corner-full track with a 2dp outline · the 52×48 touch "
                     "target · handle 16/24/28/24dp (unselected/selected/pressed/with-"
                     "icon) · a 16dp icon · the 40dp state layer"));
    context.detail(L("拇指旅行：未选中内缩 8dp、选中 24dp（maxBound = 52 − 24 − 4）、按压 "
                     "SnapSpec 内缩再让 2dp · 尺寸与位置同一根快速空间弹簧",
                     "the thumb's travel: 8dp in unchecked, 24dp checked (maxBound = 52 − "
                     "24 − 4), the pressed SnapSpec pulling 2dp further in · size and "
                     "offset share one fast spatial spring"));
    context.detail(L("颜色无动画（按状态直解）· 特例行：未选中按下涟漪 on-surface、选中 "
                     "primary；选中的描边行缺席（Compose 的 checkedBorderColor 默认透明）",
                     "no colour animation (resolved by state) · the special rows: the "
                     "unselected press ripples on-surface, the selected primary; the "
                     "selected outline rows are absent (Compose's default checked border "
                     "is transparent)"));
    context.detail(L("禁用按行淡出：轨道/描边 0.12 · 图标与未选中拇指 0.38 · **选中拇指 "
                     "surface @1**（全强度）· 拇指阴影 level1 → disabled level0",
                     "disabled fades by row: the track/outline at 0.12 · the icon and the "
                     "unselected handle at 0.38 · **the selected handle surface @1** (full "
                     "strength) · the handle's shadow level1 → level0 disabled"));
    context.detail(L("弃用行承载不读：handle.height/width（20dp，尺寸改造前的残留）· "
                     "inset-ring 焦点变体是上游 opt-in，未移植 · 导出的 focus-indicator 行"
                     "指向轨道（secondary、系统外偏移/粗细）",
                     "deprecated rows carried, not read: handle.height/width (20dp, "
                     "pre-rework residue) · the inset-ring focus variant is an upstream "
                     "opt-in, not ported · the export's focus-indicator rows point at the "
                     "track (secondary, the system offset/thickness)"));
    context.space(8.0);
}

void SwitchPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const SwitchSlot &slot : m_slots) {
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

void SwitchPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void SwitchPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void SwitchPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void SwitchPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createSwitchPage()
{
    return new SwitchPage;
}

} // namespace gallery

#include "GalleryPages37.moc"
