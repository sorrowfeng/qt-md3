// Gallery page 24: Side sheets — the Containment family's edge-docked pair,
// `md.comp.sheet.side.*`.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Family-specific notes:
// the sheet's anchors are parent-relative, so the static snapshots place
// sheets as fixed-width fixed-height bands (state snapped via setState, no
// geometry management), and the live demo raises a real MdSideSheetHost
// overlay across the whole page (scrim, edge-docked slide, Escape /
// scrim-click dismissal).

#include "GalleryPages.h"

#include "widgets/MdButton.h"
#include "widgets/MdSideSheet.h"
#include "widgets/MdSideSheetHost.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <functional>

namespace gallery {

namespace {

/// One banked snapshot sheet plus the content-local rect the latest build()
/// gave it.
struct SheetSlot
{
    md::MdSideSheet *sheet = nullptr;
    QRectF rect;
};

/// One banked demo button plus its rect.
struct ButtonSlot
{
    md::MdButton *button = nullptr;
    QRectF rect;
};

} // namespace

class SideSheetPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit SideSheetPage(QWidget *parent = nullptr);
    ~SideSheetPage() override;

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
    md::MdSideSheet *sheet(const QString &id, md::MdSideSheetKind kind, md::MdSideSheetEdge edge,
                           qreal height,
                           const std::function<void(md::MdSideSheet *)> &configure = {});
    md::MdButton *button(const QString &id, const QString &text,
                         const std::function<void(md::MdButton *)> &configure = {});

    QHash<QString, md::MdSideSheet *> m_bank;
    QVector<SheetSlot> m_slots;
    QHash<QString, md::MdButton *> m_buttonBank;
    QVector<ButtonSlot> m_buttonSlots;
    md::MdSideSheetHost *m_demoHost = nullptr;
};

SideSheetPage::SideSheetPage(QWidget *parent)
    : GalleryPage(parent)
{
}

SideSheetPage::~SideSheetPage() = default;

QString SideSheetPage::title() const
{
    return L("Side sheet", "Side sheets");
}

QString SideSheetPage::slug() const
{
    return QStringLiteral("side-sheets");
}

QString SideSheetPage::subtitle() const
{
    return L(
        "md.comp.sheet.side：256 px 宽停靠在屏幕边缘、高度撑满父容器。标准形态是 "
        "surface 静置 level0、四角直角（corner-none），可选 outline 分隔线与内容区分开；"
        "模态形态是 surface-container-low 静置 level1、朝向内容的一对 start 角 "
        "corner-large-start 圆角（右缘停靠圆 TL/BL，左缘停靠圆 TR/BR）。双稳态 "
        "Hidden/Expanded，拖拽落位走 midpoint 规则。行为源 MDC-Android sidesheet"
        "（Compose 无 side sheet 实现），宿主负责 scrim（black 0.32）与 Escape/外点 "
        "cancel 流。",
        "md.comp.sheet.side: 256 px wide, docked against the screen's edge, full parent "
        "height. The standard form is a surface at level0, square corners (corner-none), "
        "with an optional outline divider facing the content; the modal form is "
        "surface-container-low at level1 with the corner-large-start radius on the "
        "content-facing pair (TL/BL on a right-docked sheet, TR/BR on a left-docked one). "
        "Two stable states, Hidden and Expanded, and the drag settles by the midpoint "
        "rule. The behaviour source is MDC-Android's sidesheet package (no Compose side "
        "sheet exists); the host owns the scrim (black 0.32) and the Escape / scrim-click "
        "cancel flow.");
}

md::MdSideSheet *SideSheetPage::sheet(const QString &id, md::MdSideSheetKind kind,
                                      md::MdSideSheetEdge edge, qreal height,
                                      const std::function<void(md::MdSideSheet *)> &configure)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSideSheet(kind, edge, this);
    created->resize(260, int(height));
    created->setState(md::MdSideSheetState::Expanded);
    if (configure) {
        configure(created);
    }
    m_bank.insert(id, created);
    return created;
}

md::MdButton *SideSheetPage::button(const QString &id, const QString &text,
                                    const std::function<void(md::MdButton *)> &configure)
{
    const auto found = m_buttonBank.constFind(id);
    if (found != m_buttonBank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdButton(text, this);
    if (configure) {
        configure(created);
    }
    m_buttonBank.insert(id, created);
    return created;
}

void SideSheetPage::build(GalleryContext &context)
{
    m_slots.clear();
    m_buttonSlots.clear();
    const qreal contentWidth = context.width();
    const qreal sheetWidth = qMin<qreal>(260.0, contentWidth);

    // --- modal right-docked sheet ---------------------------------------------------
    context.section(L("模态侧边面板（右缘停靠）", "Modal sheet (right-docked)"));
    context.paragraph(L(
        "模态形态：surface-container-low 静置 level1，朝向内容的一对 start 角 "
        "24 px 圆角（右缘停靠即 TL/BL），贴边的一对保持直角。内容区内边距 "
        "24 px 来自规范测量表。",
        "The modal form: surface-container-low at level1, the 24 px radius on the "
        "content-facing START pair (TL/BL when docked right), the edge pair square. The "
        "24 px content padding comes from the spec's measurement table."));
    {
        md::MdSideSheet *modal = sheet(QStringLiteral("modal-right"), md::MdSideSheetKind::Modal,
                                       md::MdSideSheetEdge::Right, 300.0);
        const QRectF band = context.band(300.0);
        m_slots.append(SheetSlot{modal, QRectF(band.topLeft(), QSizeF(sheetWidth, 300.0))});
    }

    // --- standard sheet with divider -------------------------------------------------
    context.section(L("标准常驻面板（含分隔线）", "Standard sheet with divider"));
    context.paragraph(L(
        "常驻形态：surface 静置 level0、四角直角（corner-none）——它不挡在内容前面，"
        "而是与屏幕共存，一条 1 px outline 分隔线把它与内容区分开。这是桌面与平板"
        "尺寸下常见的音频播放器形态。",
        "The persistent form: surface at level0, square corners (corner-none) — it does not "
        "block the content but co-exists with it, separated by a 1 px outline divider. This "
        "is the audio-player form common at desktop and tablet sizes."));
    {
        md::MdSideSheet *standard =
            sheet(QStringLiteral("standard-right"), md::MdSideSheetKind::Standard,
                  md::MdSideSheetEdge::Right, 220.0,
                  [](md::MdSideSheet *w) { w->setDividerVisible(true); });
        const QRectF band = context.band(220.0);
        m_slots.append(SheetSlot{standard, QRectF(band.topLeft(), QSizeF(sheetWidth, 220.0))});
    }

    // --- left-docked modal sheet ------------------------------------------------------
    context.section(L("左缘停靠（RTL 镜像）", "Left-docked (the RTL mirror)"));
    context.paragraph(L(
        "左缘停靠是 RightSheetDelegate 的镜像（MDC 的 LeftSheetDelegate）：圆角对换成 "
        "TR/BR，分隔线在右缘，锚点 Hidden 在负宽度处——Hidden 的滑出方向随边翻转。",
        "Docking left mirrors the RightSheetDelegate (MDC's LeftSheetDelegate): the radius "
        "pair moves to TR/BR, the divider to the right edge, and the Hidden anchor sits at "
        "negative width — the slide-out direction flips with the edge."));
    {
        md::MdSideSheet *left = sheet(QStringLiteral("modal-left"), md::MdSideSheetKind::Modal,
                                      md::MdSideSheetEdge::Left, 300.0);
        const QRectF band = context.band(300.0);
        m_slots.append(SheetSlot{left, QRectF(band.topLeft(), QSizeF(sheetWidth, 300.0))});
    }

    // --- live demo --------------------------------------------------------------------
    context.section(L("活动演示", "Live demo"));
    context.paragraph(L(
        "点击按钮弹出真正的模态侧边面板宿主：scrim（black 0.32）随淡入变深、"
        "面板从右缘滑入停满高；拖拽面板在两锚点间移动（midpoint 落位），"
        "拖回边缘触发 cancel 流，Escape 与点击 scrim 同样关闭。",
        "The button raises a real modal side sheet host: the scrim (black 0.32) deepens as "
        "the sheet slides in full-height from the right edge; dragging the surface moves "
        "between the two anchors (midpoint settle), a drag back to the edge runs the cancel "
        "flow, and Escape or a scrim click closes it."));
    {
        md::MdButton *demo =
            button(QStringLiteral("demo"), L("显示侧边面板", "Show side sheet"));
        if (!m_demoHost) {
            m_demoHost = new md::MdSideSheetHost(this);
            connect(demo, &md::MdButton::clicked, m_demoHost, &md::MdSideSheetHost::show);
        }
        const qreal buttonWidth = qreal(demo->sizeHint().width());
        const qreal buttonHeight = qreal(demo->sizeHint().height());
        const QRectF band = context.band(buttonHeight);
        m_buttonSlots.append(
            ButtonSlot{demo, QRectF(band.topLeft(), QSizeF(buttonWidth, buttonHeight))});
    }

    // --- token facts -------------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-sheet-side.scss "
        "(34.0.21) + MDC-Android com.google.android.material.sidesheet "
        "(SideSheetBehavior / Left+RightSheetDelegate / SideSheetDialog) + "
        "m3.material.io/components/side-sheets/specs (no web component, no Compose)"));
    context.detail(QStringLiteral(
        "export      container 256x100% · standard surface level0 corner-none · modal "
        "surface-container-low level1 corner-large-start · detached corner-large 16px "
        "margin (recorded) · headline title-large on-surface-variant · divider outline · "
        "action primary · focus indicator secondary (recorded)"));
    context.detail(QStringLiteral(
        "mdc         significant velocity 500 px/s (recorded) · hide threshold 0.5 · "
        "hide friction 0.1 (recorded) · scrim black 0.32 · content padding 24 · "
        "dialog starts Expanded"));
    context.space(8.0);
    context.detail(L("TestMd3SideSheet 将 token 行表、覆盖解析（含坏值忽略）、左右 delegate "
                     "锚点数学（含宽面板钳制）、三种布局几何（边缘边距/圆角对/分隔线/"
                     "内容内边距）、状态默认值、弹簧迁移、dismissed 时机、confirm 否决、"
                     "拖拽 midpoint 落位、托管几何随父、宿主 scrim/Escape/外点与渲染烟测"
                     "逐字段锁定。",
                     "TestMd3SideSheet pins the token rows, the override parsing (bad values "
                     "ignored), the left/right delegate anchor math (the wide-sheet clamp "
                     "included), the three layout geometries (edge margins, radius pairs, "
                     "divider, content padding), the state defaults, the spring "
                     "transitions, the dismissed timing, the confirm veto, the drag "
                     "midpoint settle, the managed geometry following the parent, the host "
                     "scrim/Escape/scrim-click and the render smoke field by field."));
}

void SideSheetPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const SheetSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.sheet->geometry() != target) {
            slot.sheet->setGeometry(target);
        }
    }
    for (const ButtonSlot &slot : m_buttonSlots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.button->geometry() != target) {
            slot.button->setGeometry(target);
        }
    }
}

void SideSheetPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void SideSheetPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void SideSheetPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void SideSheetPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createSideSheetPage()
{
    return new SideSheetPage;
}

} // namespace gallery

#include "GalleryPages24.moc"
