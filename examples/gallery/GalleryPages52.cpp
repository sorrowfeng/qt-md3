// Gallery page 52: Stage 2 Qt extensions — MdGrid + MdSpace + MdCollapse +
// MdNotification + MdTree / MdTreeSelect.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. These are Appendix D
// Qt extensions; the page shows the derived designs as real live widgets.
//
// MD3 semantics these anchor to, from the Appendix D table:
//   * MdGrid         — MD3 responsive grid + Window size classes.
//   * MdSpace        — MD3 spacing scale.
//   * MdCollapse     — List expand / collapse semantics.
//   * MdNotification — Snackbar persistent variant.
//   * MdTree / MdTreeSelect — List tree rows + Menu select popup.

#include "GalleryPages.h"

#include "widgets/MdCollapse.h"
#include "widgets/MdGrid.h"
#include "widgets/MdNotification.h"
#include "widgets/MdSpace.h"
#include "widgets/MdTree.h"
#include "widgets/MdTreeSelect.h"

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

class Stage2LayoutPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit Stage2LayoutPage(QWidget *parent = nullptr);
    ~Stage2LayoutPage() override;

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

    md::MdGrid *grid(const QString &id);
    md::MdSpace *space(const QString &id);
    md::MdCollapse *collapse(const QString &id);
    md::MdNotification *notification(const QString &id);
    md::MdTree *tree(const QString &id);
    md::MdTreeSelect *treeSelect(const QString &id);

    QHash<QString, md::MdGrid *> m_grids;
    QHash<QString, md::MdSpace *> m_spaces;
    QHash<QString, md::MdCollapse *> m_collapses;
    QHash<QString, md::MdNotification *> m_notifications;
    QHash<QString, md::MdTree *> m_trees;
    QHash<QString, md::MdTreeSelect *> m_treeSelects;
    QVector<Slot> m_slots;
};

Stage2LayoutPage::Stage2LayoutPage(QWidget *parent)
    : GalleryPage(parent)
{
}

Stage2LayoutPage::~Stage2LayoutPage() = default;

QString Stage2LayoutPage::title() const
{
    return L("Qt 扩展 · 布局与结构", "Qt extensions · layout & structure");
}

QString Stage2LayoutPage::slug() const
{
    return QStringLiteral("qt-extensions-layout");
}

QString Stage2LayoutPage::subtitle() const
{
    return L(
        "阶段二 Qt 扩展：响应式栅格（window size classes）、间距标尺、"
        "列表折叠（spring 高度）、持久通知（Snackbar 变体）与树形列表"
        "（List 展开 + Menu 弹层）。",
        "Stage 2 Qt extensions: the responsive grid (window size classes), "
        "the spacing scale, list collapse (spring height), the persistent "
        "notification (Snackbar variant) and the tree (List expand + Menu "
        "popup).");
}

md::MdGrid *Stage2LayoutPage::grid(const QString &id)
{
    const auto found = m_grids.constFind(id);
    if (found != m_grids.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdGrid(this);
    m_grids.insert(id, created);
    return created;
}

md::MdSpace *Stage2LayoutPage::space(const QString &id)
{
    const auto found = m_spaces.constFind(id);
    if (found != m_spaces.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSpace(this);
    m_spaces.insert(id, created);
    return created;
}

md::MdCollapse *Stage2LayoutPage::collapse(const QString &id)
{
    const auto found = m_collapses.constFind(id);
    if (found != m_collapses.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdCollapse(this);
    m_collapses.insert(id, created);
    return created;
}

md::MdNotification *Stage2LayoutPage::notification(const QString &id)
{
    const auto found = m_notifications.constFind(id);
    if (found != m_notifications.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdNotification(this);
    m_notifications.insert(id, created);
    return created;
}

md::MdTree *Stage2LayoutPage::tree(const QString &id)
{
    const auto found = m_trees.constFind(id);
    if (found != m_trees.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTree(this);
    m_trees.insert(id, created);
    return created;
}

md::MdTreeSelect *Stage2LayoutPage::treeSelect(const QString &id)
{
    const auto found = m_treeSelects.constFind(id);
    if (found != m_treeSelects.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTreeSelect(this);
    m_treeSelects.insert(id, created);
    return created;
}

void Stage2LayoutPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- MdGrid ------------------------------------------------------------
    context.section(L("MdGrid — 响应式栅格 + Window size classes",
                      "MdGrid — responsive grid + window size classes"));
    context.paragraph(L(
        "自适应布局栅格：Compact (<600) 4 列 / Medium (600–840) 8 列 / "
        "Expanded (840+) 12 列。gutter 与 margin 跟随窗口尺寸类。",
        "Adaptive layout grid: 4 columns under 600 dp, 8 at 600–840, "
        "12 from 840 up. Gutter and margin follow the window size class."));
    {
        auto *g = grid(QStringLiteral("gr-demo"));
        g->setSizeClass(md::MdWindowSizeClass::Compact);
        g->setColumns(4);
        g->setMargin(8.0);
        g->setGutter(8.0);
        if (g->findChildren<QWidget *>().isEmpty()) {
            for (int i = 0; i < 4; ++i) {
                auto *cell = new QLabel(QString::number(i + 1), g);
                cell->setAlignment(Qt::AlignCenter);
                cell->setMinimumHeight(40);
                cell->setFont(GalleryPage::fontFor(md::TypeStyle::LabelLarge));
            }
            // Stretch one cell across two tracks to show spans.
            const QList<QWidget *> cells = g->findChildren<QWidget *>();
            if (cells.size() >= 4) {
                g->setItemSpan(cells.at(0), 2);
            }
        }
        const QRectF band = context.band(96.0);
        m_slots.append({g, QRectF(band.left(), band.top(),
                                  qMin(520.0, band.width() - 16.0), 88.0)});
    }
    context.detail(L("compact 4 / medium 8 / expanded 12",
                     "compact 4 / medium 8 / expanded 12"));

    context.space(16.0);

    // --- MdSpace -----------------------------------------------------------
    context.section(L("MdSpace — MD3 spacing scale",
                      "MdSpace — MD3 spacing scale"));
    context.paragraph(L(
        "间距标尺：0 / 4 / 8 / 12 / 16 / 24 / 32（4dp 基准）。水平或垂直占位。",
        "Spacing scale: 0 / 4 / 8 / 12 / 16 / 24 / 32 on the 4 dp base "
        "unit. Horizontal or vertical placeholder."));
    {
        auto *row = space(QStringLiteral("sp-row"));
        row->setDirection(md::MdSpaceDirection::Horizontal);
        row->setSize(md::MdFlexSpacing::ExtraLarge);
        const QRectF band = context.band(8.0);
        m_slots.append({row, QRectF(band.left(), band.top(), 24.0, 8.0)});

        auto *column = space(QStringLiteral("sp-col"));
        column->setDirection(md::MdSpaceDirection::Vertical);
        column->setSize(md::MdFlexSpacing::Huge);
        const QRectF band2 = context.band(32.0);
        m_slots.append({column, QRectF(band2.left(), band2.top(), 8.0, 32.0)});
    }
    context.detail(L("0 / 4 / 8 / 12 / 16 / 24 / 32",
                     "0 / 4 / 8 / 12 / 16 / 24 / 32"));

    context.space(16.0);

    // --- MdCollapse --------------------------------------------------------
    context.section(L("MdCollapse — List 展开折叠",
                      "MdCollapse — List expand / collapse"));
    context.paragraph(L(
        "列表分区：表头行（56px List 一行）+ FastSpatial spring 高度动画的"
        "折叠体。Space/Enter 切换。",
        "List section: a header row (56 px List one-line) plus a body whose "
        "height animates on the FastSpatial spring. Space/Enter toggles."));
    {
        auto *panel = collapse(QStringLiteral("cl-demo"));
        panel->setTitle(L("通知设置", "Notification settings"));
        panel->setSupportingText(L("3 项", "3 items"));
        if (panel->bodyWidget() == nullptr) {
            auto *body = new QLabel(
                L("推送、邮件与桌面通知的详细开关。",
                  "Detailed toggles for push, email and desktop alerts."),
                panel);
            body->setWordWrap(true);
            body->setFont(GalleryPage::fontFor(md::TypeStyle::BodyMedium));
            panel->setBodyWidget(body);
        }
        panel->setExpanded(true);
        const QRectF band = context.band(128.0);
        m_slots.append({panel, QRectF(band.left(), band.top(),
                                      qMin(420.0, band.width() - 16.0), 120.0)});

        auto *closed = collapse(QStringLiteral("cl-closed"));
        closed->setTitle(L("高级选项", "Advanced options"));
        if (closed->bodyWidget() == nullptr) {
            auto *body = new QLabel(L("已折叠。", "Collapsed."), closed);
            body->setFont(GalleryPage::fontFor(md::TypeStyle::BodyMedium));
            closed->setBodyWidget(body);
        }
        closed->setExpanded(false);
        const QRectF band2 = context.band(64.0);
        m_slots.append({closed, QRectF(band2.left(), band2.top(),
                                       qMin(420.0, band2.width() - 16.0), 56.0)});
    }
    context.detail(L("56px List 表头 + FastSpatial spring",
                     "56 px List header + FastSpatial spring"));

    context.space(16.0);

    // --- MdNotification ----------------------------------------------------
    context.section(L("MdNotification — Snackbar 持久化变体",
                      "MdNotification — persistent Snackbar variant"));
    context.paragraph(L(
        "桌面通知：Snackbar 令牌集，不自动消失，带关闭动作与可选操作按钮。",
        "Desktop notification: the Snackbar token set, never self-dismissing, "
        "with a dismiss action and an optional action button."));
    {
        auto *note = notification(QStringLiteral("nt-demo"));
        note->setMessage(L("设计稿已同步到云端。", "Design file synced to the cloud."));
        note->setActionLabel(L("撤销", "Undo"));
        const QRectF band = context.band(80.0);
        m_slots.append({note, QRectF(band.left(), band.top(),
                                     qMin(480.0, band.width() - 16.0), 72.0)});
    }
    context.detail(L("inverse-surface + dismiss (persistent)",
                     "inverse-surface + dismiss (persistent)"));

    context.space(16.0);

    // --- MdTree / MdTreeSelect ---------------------------------------------
    context.section(L("MdTree / MdTreeSelect — List 树 + Menu 弹层",
                      "MdTree / MdTreeSelect — List tree + Menu popup"));
    context.paragraph(L(
        "树形行：缩进 + 折叠箭头，List 一行语义。选择器是文本框 + 树弹层。",
        "Tree rows: indent plus a disclosure chevron on List one-line "
        "semantics. The select is a text field over a tree popup."));
    {
        auto *t = tree(QStringLiteral("tr-demo"));
        if (t->topLevelItemCount() == 0) {
            auto *documents = new md::MdTreeItem(L("文档", "Documents"));
            documents->appendChild(new md::MdTreeItem(L("设计稿", "Designs")));
            documents->appendChild(new md::MdTreeItem(L("规格", "Specs")));
            documents->setExpanded(true);
            t->addTopLevelItem(documents);

            auto *settings = new md::MdTreeItem(L("设置", "Settings"));
            settings->appendChild(new md::MdTreeItem(L("主题", "Theme")));
            t->addTopLevelItem(settings);
        }
        const QRectF band = context.band(176.0);
        m_slots.append({t, QRectF(band.left(), band.top(),
                                  qMin(360.0, band.width() - 16.0), 168.0)});

        auto *select = treeSelect(QStringLiteral("ts-demo"));
        select->setLabelText(L("选择分类", "Category"));
        if (select->tree()->topLevelItemCount() == 0) {
            auto *root = new md::MdTreeItem(L("全部", "All"));
            root->appendChild(new md::MdTreeItem(L("文章", "Articles")));
            root->appendChild(new md::MdTreeItem(L("视频", "Videos")));
            root->setExpanded(true);
            select->addTopLevelItem(root);
        }
        const QRectF band2 = context.band(72.0);
        m_slots.append({select, QRectF(band2.left(), band2.top(),
                                       qMin(320.0, band2.width() - 16.0), 56.0)});
    }
    context.detail(L("16px indent + 56px List rows + popup",
                     "16 px indent + 56 px List rows + popup"));
}

void Stage2LayoutPage::placeChildren()
{
    for (const Slot &slot : m_slots) {
        if (slot.widget != nullptr) {
            slot.widget->setGeometry(slot.rect.toRect());
        }
    }
}

void Stage2LayoutPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    placeChildren();
}

void Stage2LayoutPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    placeChildren();
}

void Stage2LayoutPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createStage2LayoutPage()
{
    return new Stage2LayoutPage;
}

} // namespace gallery

#include "GalleryPages52.moc"
