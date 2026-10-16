// Gallery page 50: Stage 2 Qt extensions — MdMenuBar + MdStatusBar +
// MdSplitter + MdPagination + MdEmptyState.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. These are Appendix D
// Qt extensions; the page shows the derived designs as real live widgets.
//
// MD3 semantics these anchor to, from the Appendix D table:
//   * MdMenuBar    — Menu semantics horizontalised (surface + label-large +
//                    state layers; popups are real MdMenu).
//   * MdStatusBar  — surface-container + label typography.
//   * MdSplitter   — outline-variant 1px divider; hover shows the handle.
//   * MdPagination — Text button + Icon button composition.
//   * MdEmptyState — Display typography + shape + Text button.

#include "GalleryPages.h"

#include "widgets/MdEmptyState.h"
#include "widgets/MdMenu.h"
#include "widgets/MdMenuBar.h"
#include "widgets/MdPagination.h"
#include "widgets/MdSplitter.h"
#include "widgets/MdStatusBar.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QLabel>

namespace gallery {

namespace {

struct Slot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class Stage2ChromePage : public GalleryPage
{
    Q_OBJECT

public:
    explicit Stage2ChromePage(QWidget *parent = nullptr);
    ~Stage2ChromePage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void placeChildren();

    md::MdMenuBar *menuBar(const QString &id);
    md::MdStatusBar *statusBar(const QString &id);
    md::MdSplitter *splitter(const QString &id);
    md::MdPagination *pagination(const QString &id);
    md::MdEmptyState *emptyState(const QString &id);

    QHash<QString, md::MdMenuBar *> m_menuBars;
    QHash<QString, md::MdStatusBar *> m_statusBars;
    QHash<QString, md::MdSplitter *> m_splitters;
    QHash<QString, md::MdPagination *> m_paginations;
    QHash<QString, md::MdEmptyState *> m_emptyStates;
    QVector<Slot> m_slots;
};

Stage2ChromePage::Stage2ChromePage(QWidget *parent)
    : GalleryPage(parent)
{
}

Stage2ChromePage::~Stage2ChromePage() = default;

QString Stage2ChromePage::title() const
{
    return L("Qt 扩展 · 界面外壳", "Qt extensions · app chrome");
}

QString Stage2ChromePage::slug() const
{
    return QStringLiteral("qt-extensions-chrome");
}

QString Stage2ChromePage::subtitle() const
{
    return L(
        "阶段二 Qt 扩展：菜单栏（Menu 横向化）、状态栏（surface-container "
        "+ label）、分割器（outline-variant + hover handle）、分页（Text "
        "+ Icon button）与空状态（Display + shape + Text button）。",
        "Stage 2 Qt extensions: the menu bar (Menu semantics horizontalised), "
        "the status bar (surface-container + label typography), the splitter "
        "(outline-variant + hover handle), the pager (Text + Icon button "
        "composition) and the empty state (Display + shape + Text button).");
}

md::MdMenuBar *Stage2ChromePage::menuBar(const QString &id)
{
    const auto found = m_menuBars.constFind(id);
    if (found != m_menuBars.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdMenuBar(this);
    m_menuBars.insert(id, created);
    return created;
}

md::MdStatusBar *Stage2ChromePage::statusBar(const QString &id)
{
    const auto found = m_statusBars.constFind(id);
    if (found != m_statusBars.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdStatusBar(this);
    m_statusBars.insert(id, created);
    return created;
}

md::MdSplitter *Stage2ChromePage::splitter(const QString &id)
{
    const auto found = m_splitters.constFind(id);
    if (found != m_splitters.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSplitter(this);
    m_splitters.insert(id, created);
    return created;
}

md::MdPagination *Stage2ChromePage::pagination(const QString &id)
{
    const auto found = m_paginations.constFind(id);
    if (found != m_paginations.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdPagination(this);
    m_paginations.insert(id, created);
    return created;
}

md::MdEmptyState *Stage2ChromePage::emptyState(const QString &id)
{
    const auto found = m_emptyStates.constFind(id);
    if (found != m_emptyStates.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdEmptyState(this);
    m_emptyStates.insert(id, created);
    return created;
}

void Stage2ChromePage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- MdMenuBar ----------------------------------------------------------
    context.section(L("MdMenuBar — Menu 语义横向化",
                      "MdMenuBar — Menu semantics horizontalised"));
    context.paragraph(L(
        "水平排列的顶层条目，每个条目打开真正的 MdMenu 弹出层。surface "
        "背景 + label-large 文案；hover 状态层 0.08，打开时 0.12。",
        "A horizontal strip of top-level items, each opening a real MdMenu "
        "popup. Surface background + label-large labels; hover state layer "
        "0.08, the open item takes the 0.12 focus layer."));
    {
        auto *bar = menuBar(QStringLiteral("mb-demo"));
        bar->clearItems();
        const int fileIndex = bar->addItem(L("文件", "File"));
        bar->addItem(L("编辑", "Edit"));
        bar->addItem(L("视图", "View"));
        bar->addItem(L("帮助", "Help"));
        auto *fileMenu = new md::MdMenu(bar);
        fileMenu->addItem(L("新建", "New"));
        fileMenu->addItem(L("打开…", "Open…"));
        fileMenu->addDivider();
        fileMenu->addItem(L("退出", "Quit"));
        bar->setMenu(fileIndex, fileMenu);
        const QRectF band = context.band(56.0);
        m_slots.append({bar, QRectF(band.left(), band.top(),
                                    qMin(520.0, band.width() - 16.0), 48.0)});
    }
    context.detail(L("surface + label-large + state layers + MdMenu",
                     "surface + label-large + state layers + MdMenu"));

    context.space(16.0);

    // --- MdStatusBar --------------------------------------------------------
    context.section(L("MdStatusBar — Surface container + label typography",
                      "MdStatusBar — Surface container + label typography"));
    context.paragraph(L(
        "窗口底部消息条：surface-container 背景，左侧 label-large "
        "主消息，右侧 body-medium 辅助消息，可选右下角尺寸拖拽。",
        "The window's bottom message strip: surface-container background, "
        "a label-large leading message, a body-medium trailing message and "
        "an optional trailing size grip."));
    {
        auto *bar = statusBar(QStringLiteral("sb-demo"));
        bar->setLeftMessage(L("就绪", "Ready"));
        bar->setRightMessage(L("42 项", "42 items"));
        bar->setSizeGripVisible(true);
        const QRectF band = context.band(40.0);
        m_slots.append({bar, QRectF(band.left(), band.top(),
                                    qMin(520.0, band.width() - 16.0), 36.0)});
    }
    context.detail(L("surface-container + on-surface / on-surface-variant",
                     "surface-container + on-surface / on-surface-variant"));

    context.space(16.0);

    // --- MdSplitter ---------------------------------------------------------
    context.section(L("MdSplitter — outline-variant 分割线 + hover handle",
                      "MdSplitter — outline-variant divider + hover handle"));
    context.paragraph(L(
        "静止时是 Divider 的 outline-variant 1px 发丝线；悬停/拖拽时"
        "在中线上浮出 32×3 的 outline 圆角把手。",
        "At rest it is the Divider's outline-variant 1 px hairline; on hover "
        "or drag a 32×3 outline pill handle fades in on the line."));
    {
        auto *split = splitter(QStringLiteral("sp-demo"));
        if (split->count() == 0) {
            auto *left = new QLabel(L("左侧面板", "Left pane"), split);
            left->setAlignment(Qt::AlignCenter);
            auto *right = new QLabel(L("右侧面板", "Right pane"), split);
            right->setAlignment(Qt::AlignCenter);
            split->addWidget(left);
            split->addWidget(right);
        }
        const QRectF band = context.band(160.0);
        m_slots.append({split, QRectF(band.left(), band.top(),
                                      qMin(520.0, band.width() - 16.0), 150.0)});
    }
    context.detail(L("outline-variant 1px + outline hover bar",
                     "outline-variant 1px + outline hover bar"));

    context.space(16.0);

    // --- MdPagination -------------------------------------------------------
    context.section(L("MdPagination — Text button + Icon button 组合",
                      "MdPagination — Text button + Icon button composition"));
    context.paragraph(L(
        "页码是 Text/Filled 按钮（当前页 Filled），前后翻页是 "
        "standard/xsmall IconButton。省略号是不可聚焦的标签。",
        "Page numbers are Text/Filled buttons (the current page is Filled); "
        "prev/next are standard/xsmall icon buttons. Ellipsis hops are "
        "non-interactive labels."));
    {
        auto *pager = pagination(QStringLiteral("pg-demo"));
        pager->setPageCount(12);
        pager->setCurrentPage(5);
        pager->setVisiblePages(5);
        pager->setShowFirstLast(true);
        const QRectF band = context.band(56.0);
        m_slots.append({pager, QRectF(band.left(), band.top(),
                                      qMin(520.0, band.width() - 16.0), 48.0)});
    }
    context.detail(L("current = Filled primary · prev/next = IconButton",
                     "current = Filled primary · prev/next = IconButton"));

    context.space(16.0);

    // --- MdEmptyState -------------------------------------------------------
    context.section(L("MdEmptyState — Display typography + shape + Text button",
                      "MdEmptyState — Display typography + shape + Text button"));
    context.paragraph(L(
        "surface-variant 艺术圆盘（ShapeCorner::ExtraLarge）+ "
        "Headline 标题 + Body 说明 + Text button 动作。",
        "A surface-variant art disc (ShapeCorner::ExtraLarge) + a Headline "
        "title + a Body description + a Text button action."));
    {
        auto *empty = emptyState(QStringLiteral("es-demo"));
        empty->setTitle(L("暂无内容", "Nothing here yet"));
        empty->setDescription(L("创建第一条记录后，它会出现在这里。",
                                "Once you create the first record it will show up here."));
        empty->setIconName(QStringLiteral("inbox"));
        empty->setActionText(L("创建", "Create"));
        const QRectF band = context.band(300.0);
        m_slots.append({empty, QRectF(band.left(), band.top(),
                                      qMin(520.0, band.width() - 16.0), 300.0)});
    }
    context.detail(L("surface-variant art + headline + text button",
                     "surface-variant art + headline + text button"));
}

void Stage2ChromePage::placeChildren()
{
    for (const Slot &slot : m_slots) {
        if (slot.widget != nullptr) {
            slot.widget->setGeometry(slot.rect.toRect());
        }
    }
}

void Stage2ChromePage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    placeChildren();
}

void Stage2ChromePage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    placeChildren();
}

void Stage2ChromePage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createStage2ChromePage()
{
    return new Stage2ChromePage;
}

} // namespace gallery

#include "GalleryPages50.moc"
