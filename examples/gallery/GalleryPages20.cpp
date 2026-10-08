// Gallery page 20: Tooltips — the Communication family's closing pair,
// `md.comp.plain-tooltip.*` + `md.comp.rich-tooltip.*`.
//
// One published set per family, the visuals only in the page banks (a tooltip
// never appears on its own — placement and the hover / focus / long-press
// triggers are MdTooltipHost's contract, demonstrated live at the bottom).

#include "GalleryPages.h"

#include "widgets/MdButton.h"
#include "widgets/MdTooltip.h"
#include "widgets/MdTooltipHost.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <functional>

namespace gallery {

namespace {

/// One banked tooltip plus the content-local rect the latest build() gave it.
struct TooltipSlot
{
    md::MdTooltip *tooltip = nullptr;
    QRectF rect;
};

struct HostSlot
{
    md::MdTooltipHost *host = nullptr;
    QRectF rect;
};


} // namespace

class TooltipPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit TooltipPage(QWidget *parent = nullptr);
    ~TooltipPage() override;

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

    md::MdTooltip *tooltip(const QString &id, const QString &text,
                           const std::function<void(md::MdTooltip *)> &configure = {});
    md::MdTooltipHost *host(const QString &id, QWidget *anchor);
    md::MdButton *demoButton(const QString &id, const QString &label);

    QHash<QString, md::MdTooltip *> m_bank;
    QVector<TooltipSlot> m_slots;
    QHash<QString, md::MdTooltipHost *> m_hostBank;
    QHash<QString, md::MdButton *> m_buttonBank;
    QVector<HostSlot> m_hostSlots;
};

TooltipPage::TooltipPage(QWidget *parent)
    : GalleryPage(parent)
{
}

TooltipPage::~TooltipPage() = default;

QString TooltipPage::title() const
{
    return L("Tooltip", "Tooltips");
}

QString TooltipPage::slug() const
{
    return QStringLiteral("tooltips");
}

QString TooltipPage::subtitle() const
{
    return L("md.comp.plain-tooltip 与 md.comp.rich-tooltip：plain 是 inverse-surface "
             "容器（extra-small 圆角、无阴影）上的 body-small inverse-on-surface 文本；"
             "rich 是 surface-container 容器（medium 圆角、level 2 阴影）上的 title-small "
             "subhead 与 body-medium 文本（on-surface-variant）加 label-large primary 的 "
             "action。material-web 只发布令牌导出，布局与行为移植自 Compose M3 的 "
             "Tooltip.kt。action 是唯一交互元素，导出为其发布 hover / focus / pressed "
             "状态行；caret 是 Expressive 的可选项。",
             "md.comp.plain-tooltip and md.comp.rich-tooltip: plain is a body-small "
             "inverse-on-surface text on an inverse-surface container (extra-small corner, no "
             "shadow); rich is a title-small subhead and body-medium text (on-surface-variant) "
             "with a label-large primary action on a surface-container container (medium "
             "corner, level 2 shadow). material-web ships the token export only — layout and "
             "behaviour port Compose M3's Tooltip.kt. The action is the only interactive "
             "element and the export publishes its hover / focus / pressed rows; the caret is "
             "the Expressive opt-in.");
}

md::MdTooltip *TooltipPage::tooltip(const QString &id, const QString &text,
                                    const std::function<void(md::MdTooltip *)> &configure)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTooltip(text, this);
    created->show();
    if (configure) {
        configure(created);
    }
    m_bank.insert(id, created);
    return created;
}

md::MdTooltipHost *TooltipPage::host(const QString &id, QWidget *anchor)
{
    const auto found = m_hostBank.constFind(id);
    if (found != m_hostBank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTooltipHost(this);
    created->setAnchorWidget(anchor);
    m_hostBank.insert(id, created);
    return created;
}

md::MdButton *TooltipPage::demoButton(const QString &id, const QString &label)
{
    const auto found = m_buttonBank.constFind(id);
    if (found != m_buttonBank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdButton(label, this);
    created->show();
    m_buttonBank.insert(id, created);
    return created;
}

void TooltipPage::build(GalleryContext &context)
{
    m_slots.clear();
    m_hostSlots.clear();
    const qreal contentWidth = context.width();

    // --- plain -----------------------------------------------------------------
    context.section(L("Plain tooltip", "Plain tooltip"));
    context.paragraph(L(
        "40 px 最小宽、24 px 最小高，内容边距 8/4，自然宽度钳制到 200 px；"
        "放不下时换行，容器随之长高。无阴影、无 action、无焦点环——"
        "它是一个纯提示面。",
        "A 40 px minimum width and 24 px minimum height with an 8/4 content padding; the "
        "natural width clamps at 200 px, wrapping grows the container. No shadow, no action, "
        "no focus ring — a pure hint surface."));
    {
        md::MdTooltip *plain = tooltip(QStringLiteral("plain"),
                                       L("已保存到草稿", "Saved as a draft"));
        const int width = int(qMin<qreal>(qreal(plain->sizeHint().width()), contentWidth));
        const int height = plain->heightForWidth(width);
        const QRectF band = context.band(qreal(height));
        m_slots.append(TooltipSlot{plain, QRectF(band.topLeft(), QSizeF(qreal(width), qreal(height)))});
    }
    {
        md::MdTooltip *wrapped = tooltip(
            QStringLiteral("plain-wrapped"),
            L("这条提示长到必须在 160 px 的容器里换成两行。",
              "This hint is long enough to wrap onto two lines in a 160 px container."));
        const int width = int(qMin<qreal>(160.0, contentWidth));
        const int height = wrapped->heightForWidth(width);
        const QRectF band = context.band(qreal(height));
        m_slots.append(
            TooltipSlot{wrapped, QRectF(band.topLeft(), QSizeF(qreal(width), qreal(height)))});
    }

    // --- rich --------------------------------------------------------------------
    context.section(L("Rich tooltip", "Rich tooltip"));
    context.paragraph(L(
        "subhead 首行基线在 28 px，正文基线在 subhead 盒下方 24 px、底部留 16 px，"
        "水平内边距 16；action 盒最小高 36、距底 8，命中区即按钮 chrome（标签居中"
        "其中）。只有正文没有 subhead 和 action 时退回 plain 的 4 px 垂直边距。",
        "The subhead's first baseline sits 28 px from the top, the text baseline 24 px below "
        "the subhead box with a 16 px bottom inset, and the horizontal padding is 16; the "
        "action box is 36 px minimum over an 8 px bottom inset, its hit region the button "
        "chrome with the label centred. A text-only rich tooltip falls back to the plain 4 px "
        "vertical padding."));
    {
        md::MdTooltip *rich = tooltip(QStringLiteral("rich"), L("正文说明文字。", "Supporting text."),
                                      [](md::MdTooltip *w) {
                                          w->setVariant(md::MdTooltipVariant::Rich);
                                          w->setTitle(L("这是什么", "What this does"));
                                          w->setActionLabel(L("了解更多", "Learn more"));
                                      });
        const int width = int(qMin<qreal>(280.0, contentWidth));
        const int height = rich->heightForWidth(width);
        const QRectF band = context.band(qreal(height));
        m_slots.append(
            TooltipSlot{rich, QRectF(band.topLeft(), QSizeF(qreal(width), qreal(height)))});
    }
    {
        md::MdTooltip *caret = tooltip(QStringLiteral("rich-caret"), L("正文说明文字。", "Supporting text."),
                                       [](md::MdTooltip *w) {
                                           w->setVariant(md::MdTooltipVariant::Rich);
                                           w->setTitle(L("这是什么", "What this does"));
                                           w->setActionLabel(L("了解更多", "Learn more"));
                                           w->setCaretSide(md::MdTooltipCaretSide::Bottom);
                                       });
        const int width = int(qMin<qreal>(280.0, contentWidth));
        const int height = caret->heightForWidth(width);
        const QRectF band = context.band(qreal(height));
        m_slots.append(
            TooltipSlot{caret, QRectF(band.topLeft(), QSizeF(qreal(width), qreal(height)))});
    }

    // --- live demo -----------------------------------------------------------------
    context.section(L("活动演示 —— 宿主触发", "Live demo — host triggers"));
    context.paragraph(L(
        "悬停立即显示（Compose 的 UserInput 优先级：永不自行消失，指针移出即隐藏）；"
        "键盘焦点触发 1500 ms 自动消失（BasicTooltipDefaults.TooltipDuration）；"
        "Escape 或程序 dismiss 随时收起。全局互斥保证同屏只有一个 tooltip。",
        "Hovering shows it immediately (Compose's UserInput priority: it never self-dismisses; "
        "the pointer leaving hides it); keyboard focus triggers the 1500 ms auto-hide "
        "(BasicTooltipDefaults.TooltipDuration); Escape or dismiss() hides it any time. The "
        "global mutator mutex keeps exactly one tooltip on screen."));
    {
        md::MdButton *anchorButton = demoButton(QStringLiteral("demo"),
                                                L("悬停在我上面", "Hover me"));
        md::MdTooltipHost *demoHost = host(QStringLiteral("demo"), anchorButton);
        demoHost->tooltip()->setText(L("这是宿主弹出的 tooltip。", "The host raised this tooltip."));
        const QRectF band = context.band(40.0);
        m_hostSlots.append(HostSlot{demoHost, QRectF(band.topLeft(), QSizeF(160.0, 40.0))});
    }

    // --- token facts -----------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-plain-tooltip.scss + "
        "_md-comp-rich-tooltip.scss (34.0.21) + Compose M3 Tooltip.kt / BasicTooltip.kt"));
    context.detail(QStringLiteral(
        "plain       container inverse-surface · corner-extra-small · supporting-text "
        "inverse-on-surface body-small"));
    context.detail(QStringLiteral(
        "rich        container surface-container · level 2 · shadow · corner-medium · "
        "subhead title-small + text body-medium on-surface-variant · action primary "
        "label-large"));
    context.detail(QStringLiteral(
        "state rows  rich action: hover 0.08 · focus 0.12 · pressed 0.12 (the ripple colour "
        "follows the pressed row)"));
    context.detail(QStringLiteral(
        "compose     min 40x24 · plain max 200 · rich max 320 · content 8/4 · rich "
        "horizontal 16 · subhead 28 · text 24/16 · action 36/8 · spacing 4 · caret 16x8"));
    context.detail(QStringLiteral(
        "host        hover show, no timeout · focus / programmatic show, 1500 ms auto-hide "
        "unless persistent · global mutex · fade+scale on effects-fast / spatial-fast "
        "springs"));
    context.space(8.0);
    context.detail(L("TestMd3Tooltip 将两族导出行、状态行表、覆盖解析、布局几何、定位"
                     "提供者、caret 几何、指针与键盘契约以及宿主触发/互斥/自动消失"
                     "规则逐字段锁定。",
                     "TestMd3Tooltip pins both exports, the state rows, the override parsing, "
                     "the layout geometry, the positioning providers, the caret geometry, the "
                     "pointer and keyboard contracts and the host trigger / mutex / auto-hide "
                     "rules field by field."));
}

void TooltipPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const TooltipSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.tooltip->geometry() != target) {
            slot.tooltip->setGeometry(target);
        }
    }
    for (const HostSlot &slot : m_hostSlots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.host->geometry() != target) {
            slot.host->setGeometry(target);
        }
    }
}

void TooltipPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void TooltipPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void TooltipPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void TooltipPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createTooltipPage()
{
    return new TooltipPage;
}

} // namespace gallery

#include "GalleryPages20.moc"
