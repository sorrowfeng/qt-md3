// Gallery page 23: Bottom sheets — the Containment family's closing pair,
// `md.comp.sheet.bottom.*`.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Family-specific notes:
// the sheet's anchors are parent-relative, so the static snapshots place
// standard sheets as fixed-height bands (state snapped via setState, no
// geometry management), and the live demo raises a real MdBottomSheetHost
// overlay across the whole page (scrim, bottom-anchored slide, Escape /
// scrim-click dismissal).

#include "GalleryPages.h"

#include "widgets/MdBottomSheet.h"
#include "widgets/MdBottomSheetHost.h"
#include "widgets/MdButton.h"

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
    md::MdBottomSheet *sheet = nullptr;
    QRectF rect;
};

/// One banked demo button plus its rect.
struct ButtonSlot
{
    md::MdButton *button = nullptr;
    QRectF rect;
};

} // namespace

class BottomSheetPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit BottomSheetPage(QWidget *parent = nullptr);
    ~BottomSheetPage() override;

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
    md::MdBottomSheet *sheet(const QString &id, md::MdSheetKind kind, qreal height,
                             const std::function<void(md::MdBottomSheet *)> &configure = {});
    md::MdButton *button(const QString &id, const QString &text,
                         const std::function<void(md::MdButton *)> &configure = {});

    QHash<QString, md::MdBottomSheet *> m_bank;
    QVector<SheetSlot> m_slots;
    QHash<QString, md::MdButton *> m_buttonBank;
    QVector<ButtonSlot> m_buttonSlots;
    md::MdBottomSheetHost *m_demoHost = nullptr;
};

BottomSheetPage::BottomSheetPage(QWidget *parent)
    : GalleryPage(parent)
{
}

BottomSheetPage::~BottomSheetPage() = default;

QString BottomSheetPage::title() const
{
    return L("Bottom sheet", "Bottom sheets");
}

QString BottomSheetPage::slug() const
{
    return QStringLiteral("bottom-sheets");
}

QString BottomSheetPage::subtitle() const
{
    return L(
        "md.comp.sheet.bottom：surface-container-low 容器静置 level1、顶部 corner-"
        "extra-large（28 px，底边齐平），拖拽把手 32×4 居中（上下触控内边距 22）"
        "在 on-surface-variant 上。锚点三态：Hidden 在父底、PartiallyExpanded 标准"
        "走 56 px peek / 模态走 min(父高, 内容高)/2、Expanded 全高可见。显示动画走 "
        "spatial-default 弹簧、隐藏走 fast-effects 弹簧；拖拽超过 56 px 阈值落位最近"
        "锚点。宿主负责 scrim（black 0.32）、Escape 与外点的 onDismissRequest 流。",
        "md.comp.sheet.bottom: a surface-container-low container at level1 with the "
        "extra-large TOP corner (28 px, the bottom edge flush) and a centred 32x4 drag "
        "handle (22 px touch padding) in on-surface-variant. Three anchors: Hidden at "
        "the parent's bottom, PartiallyExpanded at the 56 px peek (standard) or "
        "min(parent, sheet)/2 (modal), Expanded fully visible. Show runs the "
        "spatial-default spring, hide the fast-effects spring; a drag past the 56 px "
        "positional threshold settles to the nearest anchor. The host owns the scrim "
        "(black 0.32) and the Escape / scrim-click onDismissRequest flow.");
}

md::MdBottomSheet *BottomSheetPage::sheet(const QString &id, md::MdSheetKind kind, qreal height,
                                          const std::function<void(md::MdBottomSheet *)> &configure)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdBottomSheet(kind, this);
    created->resize(int(qMin<qreal>(md::MdBottomSheetTokens::kSheetMaxWidth, qreal(width()))),
                    int(height));
    created->setState(md::MdSheetState::PartiallyExpanded);
    if (configure) {
        configure(created);
    }
    m_bank.insert(id, created);
    return created;
}

md::MdButton *BottomSheetPage::button(const QString &id, const QString &text,
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

void BottomSheetPage::build(GalleryContext &context)
{
    m_slots.clear();
    m_buttonSlots.clear();
    const qreal contentWidth = context.width();
    const qreal sheetWidth = qMin<qreal>(320.0, contentWidth);

    // --- partial standard sheet ----------------------------------------------------
    context.section(L("标准底部面板（PartiallyExpanded）", "Standard sheet (PartiallyExpanded)"));
    context.paragraph(L(
        "常驻面板的收起形态：56 px peek 不是标准面板的可见高度——它是锚点，"
        "面板高度由内容决定。这里展示 160 px 内容高：拖拽把手 32×4 居中，"
        "顶部 28 px 圆角、底边与父容器齐平。",
        "The collapsed form of a persistent sheet: the 56 px peek is the anchor, not the "
        "visible height — a standard sheet's height is its content. This snapshot shows "
        "a 160 px content height: the 32x4 handle centred, the 28 px top radius, the "
        "bottom edge flush with the parent."));
    {
        md::MdBottomSheet *partial =
            sheet(QStringLiteral("partial"), md::MdSheetKind::Standard, 164.0);
        const QRectF band = context.band(164.0);
        m_slots.append(SheetSlot{partial, QRectF(band.topLeft(), QSizeF(sheetWidth, 164.0))});
    }

    // --- expanded standard sheet ---------------------------------------------------
    context.section(L("展开形态", "Expanded"));
    context.paragraph(L(
        "全高形态：内容完全可见，锚点落在父高减去内容高处。同一 surface、同一把手"
        "——区别只在锚点位置，这正是 Compose 把三种状态做成一个 Surface 的方式。",
        "The full-height form: the content fully visible, the anchor at the parent height "
        "minus the sheet height. Same surface, same handle — only the anchor moves, which "
        "is exactly how Compose models the three states on one Surface."));
    {
        md::MdBottomSheet *expanded =
            sheet(QStringLiteral("expanded"), md::MdSheetKind::Standard, 240.0,
                  [](md::MdBottomSheet *w) { w->setState(md::MdSheetState::Expanded); });
        const QRectF band = context.band(240.0);
        m_slots.append(SheetSlot{expanded, QRectF(band.topLeft(), QSizeF(sheetWidth, 240.0))});
    }

    // --- no-handle modal sheet -------------------------------------------------------
    context.section(L("无把手模态面板", "Modal sheet without handle"));
    context.paragraph(L(
        "把手是可选的（Compose 的 dragHandle 槽可传 null）：没有把手时内容区从"
        "容器顶部内边距直接开始，面板只随拖拽与编程调用移动。",
        "The handle is optional (Compose's dragHandle slot accepts null): without it the "
        "content area starts right at the container's top, and the sheet moves only by "
        "drag and programmatic calls."));
    {
        md::MdBottomSheet *bare = sheet(QStringLiteral("bare"), md::MdSheetKind::Modal, 164.0,
                                        [](md::MdBottomSheet *w) {
                                            w->setHandleVisible(false);
                                            w->setState(md::MdSheetState::Expanded);
                                        });
        const QRectF band = context.band(164.0);
        m_slots.append(SheetSlot{bare, QRectF(band.topLeft(), QSizeF(sheetWidth, 164.0))});
    }

    // --- live demo --------------------------------------------------------------------
    context.section(L("活动演示", "Live demo"));
    context.paragraph(L(
        "点击按钮弹出真正的模态底部面板宿主：scrim（black 0.32）随淡入变深、"
        "面板从底部滑上来停在半展开锚点；拖拽把手或面板本体在锚点间移动，"
        "拖到父底触发 onDismissRequest，Escape 先收起再关闭，点击 scrim 直接关闭。",
        "The button raises a real modal sheet host: the scrim (black 0.32) deepens as the "
        "sheet slides up to its partial anchor; dragging the handle or the surface moves "
        "between anchors, a drag past the parent's bottom runs onDismissRequest, Escape "
        "collapses first then closes, and a scrim click closes directly."));
    {
        md::MdButton *demo =
            button(QStringLiteral("demo"), L("显示底部面板", "Show bottom sheet"));
        if (!m_demoHost) {
            m_demoHost = new md::MdBottomSheetHost(this);
            m_demoHost->sheet()->resize(320, 260);
            connect(demo, &md::MdButton::clicked, m_demoHost, &md::MdBottomSheetHost::show);
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
        "src: material-web tokens/versions/latest/sass/_md-comp-sheet-bottom.scss "
        "(34.0.21) + Compose M3 BottomSheet.kt / BottomSheetScaffold.kt / "
        "ModalBottomSheet.kt / SheetDefaults.kt (no web component published)"));
    context.detail(QStringLiteral(
        "export      container surface-container-low level1 corner-extra-large-top · "
        "minimized corner-none (recorded) · drag handle 32x4 on-surface-variant "
        "(0.4 deprecated, not applied) · focus indicator secondary (recorded)"));
    context.detail(QStringLiteral(
        "compose     peek 56 · max width 640 · positional threshold 56 · velocity 125 "
        "(recorded) · handle padding 22 · scrim black 0.32 · show spatial-default / "
        "hide fast-effects springs"));
    context.space(8.0);
    context.detail(L("TestMd3BottomSheet 将 token 行表、覆盖解析、模态/标准锚点数学"
                     "（含确定性半高规则与锚点可用性）、布局几何、状态迁移、confirm "
                     "否决、把手切换、拖拽落位、静态摆放豁免、宿主几何/scrim/Escape/"
                     "外点与渲染烟测逐字段锁定。",
                     "TestMd3BottomSheet pins the token rows, the override parsing, the "
                     "modal/standard anchor math (the deterministic half rule and anchor "
                     "availability included), the layout geometry, the state transitions, "
                     "the confirm veto, the handle toggle, the drag settle, the static "
                     "placement exemption, the host geometry/scrim/Escape/scrim-click and "
                     "the render smoke field by field."));
}

void BottomSheetPage::placeChildren()
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

void BottomSheetPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void BottomSheetPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void BottomSheetPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void BottomSheetPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createBottomSheetPage()
{
    return new BottomSheetPage;
}

} // namespace gallery

#include "GalleryPages23.moc"
