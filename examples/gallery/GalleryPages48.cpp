// Gallery page 48: Stage 2 Qt extensions — MdScrollBar + MdSelect.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. These two are Appendix D
// P0 Qt extensions (MD3 has no official scrollbar or standalone select
// family); the page shows the derived overlay scrollbar and the exposed
// dropdown menu (text field + menu composition) as real live widgets.
//
// MD3 semantics these anchor to, from the Appendix D table:
//   * MdScrollBar — thumb on-surface-variant low alpha, hover / drag lift
//     (the 0.38 / 0.46 / 0.54 ladder). Overlay, no arrow buttons.
//   * MdSelect — exposed dropdown menu: an MdTextField anchor + an MdMenu
//     popup. Filled / outlined variants, leading icon, error, disabled.

#include "GalleryPages.h"

#include "widgets/MdSelect.h"
#include "widgets/MdScrollBar.h"
#include "widgets/MdTextField.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QScrollArea>

namespace gallery {

namespace {

struct Slot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class Stage2Page : public GalleryPage
{
    Q_OBJECT

public:
    explicit Stage2Page(QWidget *parent = nullptr);
    ~Stage2Page() override;

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

    md::MdScrollBar *scrollBar(const QString &id);
    md::MdSelect *select(const QString &id);

    QHash<QString, md::MdScrollBar *> m_bars;
    QHash<QString, md::MdSelect *> m_selects;
    QVector<Slot> m_slots;
};

Stage2Page::Stage2Page(QWidget *parent)
    : GalleryPage(parent)
{
}

Stage2Page::~Stage2Page() = default;

QString Stage2Page::title() const
{
    return L("Qt 扩展", "Qt extensions");
}

QString Stage2Page::slug() const
{
    return QStringLiteral("qt-extensions");
}

QString Stage2Page::subtitle() const
{
    return L(
        "阶段二 P0：MD3 没有滚动条与独立 Select 规范，本页展示从 "
        "token 推导的 overlay 滚动条（on-surface 低透明度 thumb，hover / drag "
        "提升 0.38 / 0.46 / 0.54）与 exposed dropdown menu（MdTextField + "
        "MdMenu 组合，filled / outlined、leading icon、error、disabled）。",
        "Stage 2 P0: MD3 has no scrollbar or standalone select spec. This page "
        "shows the token-derived overlay scrollbar (on-surface low-alpha thumb, "
        "hover / drag lift on the 0.38 / 0.46 / 0.54 ladder) and the exposed "
        "dropdown menu (MdTextField + MdMenu composition, filled / outlined, "
        "leading icon, error, disabled).");
}

md::MdScrollBar *Stage2Page::scrollBar(const QString &id)
{
    const auto found = m_bars.constFind(id);
    if (found != m_bars.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdScrollBar(Qt::Vertical, this);
    created->setRange(0, 100);
    created->setPageStep(25);
    m_bars.insert(id, created);
    return created;
}

md::MdSelect *Stage2Page::select(const QString &id)
{
    const auto found = m_selects.constFind(id);
    if (found != m_selects.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSelect(this);
    m_selects.insert(id, created);
    return created;
}

void Stage2Page::build(GalleryContext &context)
{
    m_slots.clear();

    // --- MdScrollBar ---------------------------------------------------------
    context.section(L("MdScrollBar — overlay 滚动条", "MdScrollBar — overlay scrollbar"));
    context.paragraph(L(
        "MD3 无滚动条规范。本控件按 Material overlay 约定推导：轨道静止时"
        "不可见，thumb 是 corner-full 药丸，on-surface @ 0.38 idle / 0.46 hover / "
        "0.54 drag / 0.12 disabled。可见宽度 8dp，命中区域 16dp，无箭头按钮。"
        "thickness 与 visibleMode（Auto / Always / Never）是 Q_PROPERTY。",
        "MD3 has no scrollbar spec. This control derives from the Material "
        "overlay convention: the track is invisible at rest, the thumb is a "
        "corner-full pill on on-surface at 0.38 idle / 0.46 hover / 0.54 drag / "
        "0.12 disabled. Visible width 8dp, hit area 16dp, no arrow buttons. "
        "thickness and visibleMode (Auto / Always / Never) are Q_PROPERTYs."));
    {
        // A short scrollable stack next to the bars so the thumbs have range.
        auto *demo = scrollBar(QStringLiteral("demo-v"));
        demo->setVisibleMode(md::MdScrollBarVisibility::Always);
        const QRectF band = context.band(180.0);
        m_slots.append({demo, QRectF(band.right() - 24.0, band.top(), 16.0, 180.0)});

        auto *thick = scrollBar(QStringLiteral("demo-thick"));
        thick->setThickness(16);
        thick->setVisibleMode(md::MdScrollBarVisibility::Always);
        m_slots.append({thick, QRectF(band.right() - 56.0, band.top(), 16.0, 180.0)});

        auto *hidden = scrollBar(QStringLiteral("demo-hidden"));
        hidden->setVisibleMode(md::MdScrollBarVisibility::Never);
        m_slots.append({hidden, QRectF(band.right() - 88.0, band.top(), 16.0, 180.0)});
    }
    context.detail(L(
        "左→右：Always（默认 thickness 8）、thickness 16、Never（无 chrome）。",
        "Left→right: Always (default thickness 8), thickness 16, Never (no chrome)."));

    context.space(16.0);

    // --- MdSelect -------------------------------------------------------------
    context.section(L("MdSelect — exposed dropdown menu", "MdSelect — exposed dropdown menu"));
    context.paragraph(L(
        "M3 的 exposed dropdown menu 是 Text field + Menu 组合：字段只读展示当前"
        "选项，trailing icon 是 arrow_drop_down，点击 / Enter / Space / Alt+Down "
        "打开 MdMenu 弹层。变体与 token 全部来自阶段一 text-field 与 menu 导出。",
        "M3's exposed dropdown menu is a Text field + Menu composition: the "
        "field is read-only and shows the current choice, the trailing icon is "
        "arrow_drop_down, and click / Enter / Space / Alt+Down open the MdMenu "
        "popup. Variants and tokens come entirely from the Stage 1 text-field "
        "and menu exports."));
    {
        auto *filled = select(QStringLiteral("sel-filled"));
        filled->setLabelText(L("口味", "Flavour"));
        filled->setSupportingText(L("必选", "Required"));
        filled->addItems({L("香草", "Vanilla"), L("巧克力", "Chocolate"),
                          L("草莓", "Strawberry"), L("抹茶", "Matcha")});
        const QRectF band = context.band(80.0);
        m_slots.append({filled, QRectF(band.left(), band.top(), qMin(280.0, band.width() - 16.0), 72.0)});

        auto *outlined = select(QStringLiteral("sel-outlined"));
        outlined->setVariant(md::MdTextFieldVariant::Outlined);
        outlined->setLabelText(L("尺寸", "Size"));
        outlined->setLeadingIconName(QStringLiteral("straighten"));
        outlined->addItems({L("小", "Small"), L("中", "Medium"), L("大", "Large")});
        const QRectF band2 = context.band(80.0);
        m_slots.append({outlined, QRectF(band2.left(), band2.top(), qMin(280.0, band2.width() - 16.0), 72.0)});

        auto *errored = select(QStringLiteral("sel-error"));
        errored->setLabelText(L("配送", "Delivery"));
        errored->setError(true);
        errored->setSupportingText(L("请选择配送方式", "Please pick a delivery method"));
        errored->addItems({L("标准", "Standard"), L("加急", "Express")});
        const QRectF band3 = context.band(80.0);
        m_slots.append({errored, QRectF(band3.left(), band3.top(), qMin(280.0, band3.width() - 16.0), 72.0)});

        auto *disabled = select(QStringLiteral("sel-disabled"));
        disabled->setLabelText(L("已售罄", "Sold out"));
        disabled->setEnabled(false);
        disabled->addItems({L("无货", "Out of stock")});
        const QRectF band4 = context.band(80.0);
        m_slots.append({disabled, QRectF(band4.left(), band4.top(), qMin(280.0, band4.width() - 16.0), 72.0)});
    }
    context.detail(L(
        "filled · outlined + leading icon · error · disabled",
        "filled · outlined + leading icon · error · disabled"));
}

void Stage2Page::layoutChildren()
{
    // Placeholder: real placement happens in placeChildren() from the
    // content-rect path so paint and layout share one source of truth.
}

void Stage2Page::placeChildren()
{
    for (const Slot &slot : m_slots) {
        if (slot.widget != nullptr) {
            slot.widget->setGeometry(slot.rect.toRect());
        }
    }
}

void Stage2Page::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    placeChildren();
}

void Stage2Page::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    placeChildren();
}

void Stage2Page::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createStage2Page()
{
    return new Stage2Page;
}

} // namespace gallery

#include "GalleryPages48.moc"
