// Gallery page 48: Stage 2 Qt extensions — MdScrollBar + MdSelect +
// MdMessageBox + MdScrollArea + MdTable + MdAvatar.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. These are Appendix D
// Qt extensions (MD3 has no official scrollbar, standalone select, message
// box, scroll area, data table or avatar family); the page shows the
// derived designs as real live widgets.
//
// MD3 semantics these anchor to, from the Appendix D table:
//   * MdScrollBar — thumb on-surface-variant low alpha, hover / drag lift
//     (the 0.38 / 0.46 / 0.54 ladder). Overlay, no arrow buttons.
//   * MdSelect — exposed dropdown menu: an MdTextField anchor + an MdMenu
//     popup. Filled / outlined variants, leading icon, error, disabled.
//   * MdMessageBox — Dialog tokens + Buttons + semantic colour (error uses
//     the error role).
//   * MdScrollArea — composes MdScrollBar over surface.
//   * MdTable — List row semantics + surface-container header.
//   * MdAvatar — shape Full + surface-variant + initials / icon.

#include "GalleryPages.h"

#include "widgets/MdAvatar.h"
#include "widgets/MdButton.h"
#include "widgets/MdMessageBox.h"
#include "widgets/MdScrollArea.h"
#include "widgets/MdScrollBar.h"
#include "widgets/MdSelect.h"
#include "widgets/MdTable.h"
#include "widgets/MdTextField.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QLabel>
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
    md::MdAvatar *avatar(const QString &id);
    md::MdMessageBox *messageBox(const QString &id);
    md::MdScrollArea *scrollArea(const QString &id);
    md::MdTable *table(const QString &id);

    QHash<QString, md::MdScrollBar *> m_bars;
    QHash<QString, md::MdSelect *> m_selects;
    QHash<QString, md::MdAvatar *> m_avatars;
    QHash<QString, md::MdMessageBox *> m_messageBoxes;
    QHash<QString, md::MdScrollArea *> m_scrollAreas;
    QHash<QString, md::MdTable *> m_tables;
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
        "阶段二 Qt 扩展：MD3 无官方规范的控件，全部用阶段一 token 重新设计。"
        "本页覆盖 overlay 滚动条、exposed dropdown、消息框（Dialog + 语义色）、"
        "滚动区域、数据表（List 行语义）与 Avatar（shape Full + "
        "surface-variant）。",
        "Stage 2 Qt extensions: controls MD3 does not specify, redesigned "
        "with Stage 1 tokens. This page covers the overlay scrollbar, the "
        "exposed dropdown, the message box (Dialog + semantic colour), the "
        "scroll area, the data table (List row semantics) and the avatar "
        "(shape Full + surface-variant).");
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

md::MdAvatar *Stage2Page::avatar(const QString &id)
{
    const auto found = m_avatars.constFind(id);
    if (found != m_avatars.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdAvatar(this);
    m_avatars.insert(id, created);
    return created;
}

md::MdMessageBox *Stage2Page::messageBox(const QString &id)
{
    const auto found = m_messageBoxes.constFind(id);
    if (found != m_messageBoxes.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdMessageBox(this);
    m_messageBoxes.insert(id, created);
    return created;
}

md::MdScrollArea *Stage2Page::scrollArea(const QString &id)
{
    const auto found = m_scrollAreas.constFind(id);
    if (found != m_scrollAreas.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdScrollArea(this);
    m_scrollAreas.insert(id, created);
    return created;
}

md::MdTable *Stage2Page::table(const QString &id)
{
    const auto found = m_tables.constFind(id);
    if (found != m_tables.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTable(this);
    m_tables.insert(id, created);
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

    context.space(16.0);

    // --- MdMessageBox ---------------------------------------------------------
    context.section(L("MdMessageBox — 对话框 + 语义色", "MdMessageBox — dialog + semantic colour"));
    context.paragraph(L(
        "QMessageBox 替代。容器锚定 md.comp.dialog.*（extra-large 圆角、"
        "surface-container-high、level 3）；动作按钮是真实 MdButton"
        "（肯定用 Filled，否定用 Text）；语义色只染图标：Information / "
        "Question 用 primary，Warning 用 tertiary，Error 用 error。",
        "QMessageBox replacement. The container anchors on md.comp.dialog.* "
        "(extra-large corner, surface-container-high, level 3); actions are "
        "real MdButton children (Filled for affirmative, Text for dismissive); "
        "the semantic colour only tints the icon: Information / Question use "
        "primary, Warning uses tertiary, Error uses error."));
    {
        auto *info = messageBox(QStringLiteral("mb-info"));
        info->setVariant(md::MdMessageBoxVariant::Information);
        info->setTitle(L("已保存", "Saved"));
        info->setText(L("你的更改已写入磁盘。", "Your changes were written to disk."));
        info->setStandardButtons(md::MdMessageBoxStandardButton::Ok);
        const QRectF band = context.band(200.0);
        m_slots.append({info, QRectF(band.left(), band.top(), qMin(360.0, band.width() - 16.0), 200.0)});

        auto *warn = messageBox(QStringLiteral("mb-warn"));
        warn->setVariant(md::MdMessageBoxVariant::Warning);
        warn->setTitle(L("电池电量低", "Battery low"));
        warn->setText(L("剩余 10%。", "10% remaining."));
        warn->setStandardButtons(md::MdMessageBoxStandardButton::Ok);
        const QRectF band2 = context.band(200.0);
        m_slots.append({warn, QRectF(band2.left(), band2.top(), qMin(360.0, band2.width() - 16.0), 200.0)});

        auto *err = messageBox(QStringLiteral("mb-error"));
        err->setVariant(md::MdMessageBoxVariant::Error);
        err->setTitle(L("无法删除", "Cannot delete"));
        err->setText(L("文件被其他程序占用。", "The file is in use by another program."));
        err->setStandardButtons(md::MdMessageBoxStandardButton::Close);
        const QRectF band3 = context.band(200.0);
        m_slots.append({err, QRectF(band3.left(), band3.top(), qMin(360.0, band3.width() - 16.0), 200.0)});

        auto *ask = messageBox(QStringLiteral("mb-ask"));
        ask->setVariant(md::MdMessageBoxVariant::Question);
        ask->setTitle(L("退出？", "Quit?"));
        ask->setText(L("未保存的更改将会丢失。", "Unsaved changes will be lost."));
        ask->setStandardButtons(md::MdMessageBoxStandardButton::Yes
                                | md::MdMessageBoxStandardButton::No);
        const QRectF band4 = context.band(200.0);
        m_slots.append({ask, QRectF(band4.left(), band4.top(), qMin(360.0, band4.width() - 16.0), 200.0)});
    }
    context.detail(L(
        "information · warning · error · question (Yes/No)",
        "information · warning · error · question (Yes/No)"));

    context.space(16.0);

    // --- MdScrollArea ----------------------------------------------------------
    context.section(L("MdScrollArea — 组合 MdScrollBar", "MdScrollArea — composes MdScrollBar"));
    context.paragraph(L(
        "QScrollArea 的 MD3 化：frame 关闭，viewport 铺 surface，滚动条是"
        " MdScrollBar（overlay thumb 0.38 / 0.46 / 0.54）。wheel / 键盘 / "
        "widgetResizable 沿用 QScrollArea 行为。",
        "QScrollArea in MD3 clothing: no frame, surface under the viewport, "
        "and MdScrollBar as the chrome (overlay thumb on the 0.38 / 0.46 / "
        "0.54 ladder). Wheel / keyboard / widgetResizable keep the QScrollArea "
        "behaviour."));
    {
        auto *area = scrollArea(QStringLiteral("sa-demo"));
        auto *content = new QLabel(
            L("内容比视口高，用滚轮或方向键滚动。\n\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20",
              "Content taller than the viewport — scroll with the wheel or "
              "arrow keys.\n\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20"));
        content->setWordWrap(true);
        area->setWidget(content);
        area->setWidgetResizable(true);
        const QRectF band = context.band(180.0);
        m_slots.append({area, QRectF(band.left(), band.top(), qMin(320.0, band.width() - 16.0), 180.0)});
    }
    context.detail(L(
        "wheel · 键盘 · widgetResizable · MdScrollBar overlay",
        "wheel · keyboard · widgetResizable · MdScrollBar overlay"));

    context.space(16.0);

    // --- MdTable ----------------------------------------------------------------
    context.section(L("MdTable — List 行语义 + surface 表头", "MdTable — List rows + surface header"));
    context.paragraph(L(
        "MD3 无数据表格规范。表头 surface-container + label-large；行高默认"
        "取 List one-line（56）；hover / selected 是 List 的 state layer 与 "
        "secondary-container；分割线 outline-variant；可选交替行用 "
        "surface-container-lowest / low。",
        "MD3 has no data-table spec. The header is surface-container + "
        "label-large; row height defaults to the list one-line (56); hover / "
        "selected are the list's state layer and secondary-container; "
        "dividers are outline-variant; optional alternating rows use "
        "surface-container-lowest / low."));
    {
        auto *grid = table(QStringLiteral("tbl-demo"));
        grid->setColumnCount(3);
        grid->setHorizontalHeaderLabels({L("姓名", "Name"), L("角色", "Role"), L("团队", "Team")});
        grid->setRowCount(4);
        grid->setCellText(0, 0, L("Ada", "Ada"));
        grid->setCellText(0, 1, L("工程师", "Engineer"));
        grid->setCellText(0, 2, L("核心", "Core"));
        grid->setCellText(1, 0, L("Grace", "Grace"));
        grid->setCellText(1, 1, L("负责人", "Lead"));
        grid->setCellText(1, 2, L("编译器", "Compilers"));
        grid->setCellText(2, 0, L("Alan", "Alan"));
        grid->setCellText(2, 1, L("研究", "Research"));
        grid->setCellText(2, 2, L("数学", "Math"));
        grid->setCellText(3, 0, L("Barbara", "Barbara"));
        grid->setCellText(3, 1, L("运维", "Ops"));
        grid->setCellText(3, 2, L("平台", "Platform"));
        grid->setAlternatingRowColors(true);
        grid->setCurrentRow(1);
        const QRectF band = context.band(240.0);
        m_slots.append({grid, QRectF(band.left(), band.top(), qMin(480.0, band.width() - 16.0), 240.0)});
    }
    context.detail(L(
        "header · hover · selected · alternating · dividers",
        "header · hover · selected · alternating · dividers"));

    context.space(16.0);

    // --- MdAvatar -----------------------------------------------------------------
    context.section(L("MdAvatar — shape Full + surface-variant", "MdAvatar — shape Full + surface-variant"));
    context.paragraph(L(
        "MD3 无独立 Avatar 规范（仅作 List / Chip 的 leading 元素）。独立化时"
        "用 shape Full + surface-variant 背景 + on-surface-variant 内容；内容"
        "为 initials 或 24px 图标。",
        "MD3 has no standalone Avatar family (only the list / chip leading "
        "element). Standalone it takes shape Full + surface-variant background "
        "+ on-surface-variant content; the content is initials or a 24 px "
        "icon."));
    {
        auto *initials = avatar(QStringLiteral("av-initials"));
        initials->setText(QStringLiteral("AL"));
        initials->setAvatarSize(48);
        const QRectF band = context.band(64.0);
        m_slots.append({initials, QRectF(band.left(), band.top(), 48, 48)});

        auto *name = avatar(QStringLiteral("av-name"));
        name->setText(md::MdAvatar::initialsFrom(QStringLiteral("Grace Hopper")));
        name->setAvatarSize(48);
        m_slots.append({name, QRectF(band.left() + 64.0, band.top(), 48, 48)});

        auto *icon = avatar(QStringLiteral("av-icon"));
        icon->setIconName(QStringLiteral("person"));
        icon->setAvatarSize(48);
        m_slots.append({icon, QRectF(band.left() + 128.0, band.top(), 48, 48)});

        auto *big = avatar(QStringLiteral("av-big"));
        big->setText(QStringLiteral("AB"));
        big->setAvatarSize(64);
        big->setCorner(md::ShapeCorner::Medium);
        m_slots.append({big, QRectF(band.left() + 192.0, band.top(), 64, 64)});
    }
    context.detail(L(
        "initials · derived initials · icon · medium corner",
        "initials · derived initials · icon · medium corner"));
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
