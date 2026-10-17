// Gallery page 55: Stage 2 Qt extensions — the appendix D remainder.
//
// MdInputDialog / MdStackedWidget / MdResult / MdBreadcrumb / MdLog /
// MdPlainTextEdit / MdDrawer / MdNav / MdQRCode / MdMasonry / MdAnchor /
// MdTour / MdRibbon / MdCascader.
//
// Same rules as the other component pages: widgets are banked, build() is
// content-local rects only, placeChildren() runs from resize/show/paint.
// Qt::Popup surfaces (MdCascader) are opened from trigger buttons, never
// setVisible(true) from the bank.

#include "GalleryPage.h"
#include "I18n.h"

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdBreadcrumb.h"
#include "widgets/MdButton.h"
#include "widgets/MdDrawer.h"
#include "widgets/MdLog.h"
#include "widgets/MdNav.h"
#include "widgets/MdQRCode.h"
#include "widgets/MdResult.h"
#include "widgets/MdStackedWidget.h"
#include "widgets/MdPlainTextEdit.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <QtWidgets/QLabel>

namespace gallery {

class Stage2RemainderPage : public GalleryPage
{
public:
    explicit Stage2RemainderPage(QWidget *parent = nullptr);
    ~Stage2RemainderPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;

    void build(GalleryContext &context) override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    struct Slot
    {
        QWidget *widget = nullptr;
        QRectF rect;
    };

    template <typename T>
    T *banked(const QString &id, QHash<QString, T *> &map)
    {
        const auto found = map.constFind(id);
        if (found != map.constEnd()) {
            return found.value();
        }
        auto *created = new T(this);
        created->setVisible(false);
        map.insert(id, created);
        return created;
    }

    void placeChildren();

    QHash<QString, md::MdResult *> m_results;
    QHash<QString, md::MdBreadcrumb *> m_crumbs;
    QHash<QString, md::MdLog *> m_logs;
    QHash<QString, md::MdQRCode *> m_codes;
    QHash<QString, md::MdNav *> m_navs;
    QHash<QString, md::MdStackedWidget *> m_stacks;
    QHash<QString, md::MdDrawer *> m_drawers;
    QHash<QString, md::MdPlainTextEdit *> m_edits;
    QList<Slot> m_slots;
};

Stage2RemainderPage::Stage2RemainderPage(QWidget *parent)
    : GalleryPage(parent)
{
}

Stage2RemainderPage::~Stage2RemainderPage() = default;

QString Stage2RemainderPage::title() const
{
    return QStringLiteral("Stage 2 · Remainder");
}

QString Stage2RemainderPage::slug() const
{
    return QStringLiteral("qt-extensions-remainder");
}

QString Stage2RemainderPage::subtitle() const
{
    return L("MdResult / MdBreadcrumb / MdLog / MdQRCode / MdNav / "
             "MdStackedWidget / MdDrawer / MdPlainTextEdit",
             "MdResult / MdBreadcrumb / MdLog / MdQRCode / MdNav / "
             "MdStackedWidget / MdDrawer / MdPlainTextEdit");
}

void Stage2RemainderPage::build(GalleryContext &context)
{
    m_slots.clear();

    context.section(L("MdResult — 状态结果页", "MdResult — status result"));
    context.paragraph(L(
        "Display 标题 + shape 艺术盘 + Text button，状态色走 primary / "
        "tertiary / error。",
        "Display title + shape art disc + Text button; status tints use "
        "primary / tertiary / error."));
    {
        const QRectF band = context.band(220.0);
        auto *r = banked(QStringLiteral("result-demo"), m_results);
        r->setStatus(md::MdResultStatus::Success);
        r->setTitle(L("导出完成", "Export complete"));
        r->setSubtitle(L("文件已保存到下载目录。",
                         "The file was saved to your downloads folder."));
        r->setActionText(L("打开文件夹", "Open folder"));
        r->setVisible(true);
        m_slots.append({r, QRectF(band.left(), band.top(),
                                  qMin(360.0, band.width() - 16.0), 212.0)});
    }
    context.detail(L("primary-container / on-primary-container",
                     "primary-container / on-primary-container"));
    context.space(16.0);

    context.section(L("MdBreadcrumb — 面包屑", "MdBreadcrumb — breadcrumb"));
    context.paragraph(L(
        "label-large 链路 + chevron_right 分隔符；末项 on-surface 为当前。",
        "label-large trail with chevron_right separators; the last crumb is "
        "on-surface."));
    {
        const QRectF band = context.band(48.0);
        auto *b = banked(QStringLiteral("crumb-demo"), m_crumbs);
        b->setItems(QStringList() << L("首页", "Home") << L("文档", "Docs")
                                  << L("组件", "Components"));
        b->setVisible(true);
        m_slots.append({b, QRectF(band.left(), band.top(),
                                  qMin(420.0, band.width() - 16.0), 40.0)});
    }
    context.space(16.0);

    context.section(L("MdLog — 日志视图", "MdLog — log view"));
    context.paragraph(L(
        "surface-container 等宽行视图；Info / Success / Warning / Error 映射 "
        "on-surface / primary / tertiary / error。",
        "surface-container monospace line view; Info / Success / Warning / "
        "Error map to on-surface / primary / tertiary / error."));
    {
        const QRectF band = context.band(140.0);
        auto *log = banked(QStringLiteral("log-demo"), m_logs);
        log->clearLines();
        log->appendLine(QStringLiteral("[09:00] boot"), md::MdLogLevel::Info);
        log->appendLine(QStringLiteral("[09:01] ready"), md::MdLogLevel::Success);
        log->appendLine(QStringLiteral("[09:02] retry"), md::MdLogLevel::Warning);
        log->appendLine(QStringLiteral("[09:03] fail"), md::MdLogLevel::Error);
        log->setVisible(true);
        m_slots.append({log, QRectF(band.left(), band.top(),
                                    qMin(420.0, band.width() - 16.0), 132.0)});
    }
    context.space(16.0);

    context.section(L("MdQRCode — 二维码面", "MdQRCode — code surface"));
    context.paragraph(L(
        "surface-container + 模块矩阵 + 4 模块静区。",
        "surface-container plus a module matrix with a 4-module quiet zone."));
    {
        const QRectF band = context.band(150.0);
        auto *qr = banked(QStringLiteral("qr-demo"), m_codes);
        qr->setText(QStringLiteral("https://m3.material.io"));
        qr->setVisible(true);
        const QSize hint = qr->sizeHint();
        m_slots.append({qr, QRectF(band.left(), band.top(),
                                   hint.width() + 8.0, hint.height() + 8.0)});
    }
    context.space(16.0);

    context.section(L("MdNav — 目的地列表", "MdNav — destination list"));
    context.paragraph(L(
        "复用 navigation-drawer item 语义：56 px 行、secondary-container 选中。",
        "Reuses the navigation-drawer item language: 56 px rows, "
        "secondary-container selection."));
    {
        const QRectF band = context.band(128.0);
        auto *nav = banked(QStringLiteral("nav-demo"), m_navs);
        if (nav->count() == 0) {
            nav->addItem(new md::MdNavItem(L("首页", "Home"), QStringLiteral("home")));
            nav->addItem(new md::MdNavItem(L("搜索", "Search"), QStringLiteral("search")));
        }
        nav->setVisible(true);
        m_slots.append({nav, QRectF(band.left(), band.top(),
                                    qMin(220.0, band.width() - 16.0), 120.0)});
    }
    context.space(16.0);

    context.section(L("MdStackedWidget — 层叠页",
                      "MdStackedWidget — stacked pages"));
    context.paragraph(L(
        "surface 容器，页切换走 effects-fast 弹簧淡入。",
        "surface container; page changes fade on the effects-fast spring."));
    {
        const QRectF band = context.band(120.0);
        auto *stack = banked(QStringLiteral("stack-demo"), m_stacks);
        if (stack->count() == 0) {
            auto *one = new QLabel(L("第一页", "Page one"), stack);
            auto *two = new QLabel(L("第二页", "Page two"), stack);
            stack->addWidget(one);
            stack->addWidget(two);
        }
        stack->setCurrentIndex(1);
        stack->setVisible(true);
        m_slots.append({stack, QRectF(band.left(), band.top(),
                                      qMin(280.0, band.width() - 16.0), 112.0)});
    }
    context.space(16.0);

    context.section(L("MdDrawer — 抽屉", "MdDrawer — drawer"));
    context.paragraph(L(
        "复用 Navigation drawer / Side sheet：modal scrim + spatial-fast 滑入。",
        "Reuses the navigation drawer / side sheet: modal scrim plus a "
        "spatial-fast slide-in."));
    {
        const QRectF band = context.band(120.0);
        auto *drawer = banked(QStringLiteral("drawer-demo"), m_drawers);
        drawer->setHeadline(L("导航", "Navigation"));
        drawer->setPanelSize(160);
        drawer->setOpen(true);
        drawer->setVisible(true);
        m_slots.append({drawer, QRectF(band.left(), band.top(),
                                       qMin(280.0, band.width() - 16.0), 112.0)});
    }
    context.space(16.0);

    context.section(L("MdPlainTextEdit — 多行编辑",
                      "MdPlainTextEdit — multiline edit"));
    context.paragraph(L(
        "Text field 多行外观 + 行号 / 当前行高亮 / 查找替换 / Ctrl+Wheel 缩放。",
        "Multiline text-field look plus line numbers, current-line highlight, "
        "find/replace, and Ctrl+Wheel zoom."));
    {
        const QRectF band = context.band(140.0);
        auto *edit = banked(QStringLiteral("edit-demo"), m_edits);
        edit->setPlainText(QStringLiteral("void paint() {\n  // …\n}"));
        edit->setVisible(true);
        m_slots.append({edit, QRectF(band.left(), band.top(),
                                     qMin(420.0, band.width() - 16.0), 132.0)});
    }
}

void Stage2RemainderPage::placeChildren()
{
    const QRectF origin = contentRectForCurrentSize();
    const qreal baseY = buildOriginY();
    for (const Slot &slot : m_slots) {
        if (!slot.widget) {
            continue;
        }
        slot.widget->setGeometry(qRound(origin.left() + slot.rect.left()),
                                 qRound(baseY + slot.rect.top()),
                                 qRound(slot.rect.width()), qRound(slot.rect.height()));
    }
}

void Stage2RemainderPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    remeasure();
    placeChildren();
}

void Stage2RemainderPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    remeasure();
    placeChildren();
}

void Stage2RemainderPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

GalleryPage *createStage2RemainderPage()
{
    return new Stage2RemainderPage;
}

} // namespace gallery
