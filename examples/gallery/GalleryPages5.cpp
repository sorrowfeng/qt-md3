// Gallery page 5: Button groups — the second Actions family.
//
// The same two rules as the button page apply, plus one that is specific to
// this family: the group itself paints *nothing*. The spec calls it "an
// invisible container" with no colour attributes, so a page can only show what
// the group does to its items — spacing, corner shapes, and press growth. A
// reader who sees a coloured outline around the group is seeing a bug, and the
// page has no way to paint one even by accident, because there is no paint
// filter for this component at all.

#include "GalleryPages.h"

#include "widgets/MdButton.h"
#include "widgets/MdButtonGroup.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace gallery {

namespace {

/// One banked group plus the content-local rect the latest build() gave it.
struct GroupSlot
{
    md::MdButtonGroup *group = nullptr;
    QRectF rect;
};

/// Gap between neighbouring groups in a row.
constexpr qreal kRowGap = 24.0;
/// Gap between stacked rows inside one section.
constexpr qreal kStackGap = 12.0;

} // namespace

class ButtonGroupPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit ButtonGroupPage(QWidget *parent = nullptr);
    ~ButtonGroupPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    /// Reserve a band for `groups`, wrapping to a new band when the row would
    /// overflow the content width. Appends to m_slots.
    void layFlow(GalleryContext &context, const QVector<md::MdButtonGroup *> &groups);

    /// Re-run build() against a null painter and move the groups to the rects
    /// it produced. Safe outside a paint handler, which is the point: Qt paints
    /// children *after* the parent, so placing them from paintEvent would leave
    /// them one frame stale.
    void layoutChildren();
    /// Move the groups to the rects from the most recent build().
    void placeChildren();

    /// Fetch the banked group called `id`, creating and parenting it on first
    /// use. The children of the group are likewise created once — `configure`
    /// is the only place that touches them, and it runs exactly once per id.
    template <typename Configure>
    md::MdButtonGroup *group(const QString &id, Configure configure)
    {
        const auto found = m_bank.constFind(id);
        if (found != m_bank.constEnd()) {
            return found.value();
        }
        // Parented to the page and shown once; Qt owns the deletion from here on.
        auto *created = new md::MdButtonGroup(this);
        created->show();
        configure(created);
        m_bank.insert(id, created);
        return created;
    }

    /// The four labels the demo groups share, so a reader comparing sections
    /// compares one thing at a time.
    static void addTextItems(md::MdButtonGroup *group);
    /// Icon-only items — the case the spec reserves for the toggle style.
    static void addIconItems(md::MdButtonGroup *group);

    QHash<QString, md::MdButtonGroup *> m_bank;
    QVector<GroupSlot> m_slots;
};

ButtonGroupPage::ButtonGroupPage(QWidget *parent)
    : GalleryPage(parent)
{
}

ButtonGroupPage::~ButtonGroupPage() = default;

QString ButtonGroupPage::title() const
{
    return L("按钮组", "Button groups");
}

QString ButtonGroupPage::slug() const
{
    return QStringLiteral("Button groups");
}

QString ButtonGroupPage::subtitle() const
{
    return L("两种变体、五种 Expressive 尺寸、四种选择模式——按钮组本身不可见；你看到的"
             "一切都属于它的条目。",
             "Two variants, five Expressive sizes, four selection modes — "
             "the group is invisible; everything you see belongs to its items.");
}

void ButtonGroupPage::addTextItems(md::MdButtonGroup *group)
{
    group->addItem(L("日", "Day"));
    group->addItem(L("周", "Week"));
    group->addItem(L("月", "Month"));
}

void ButtonGroupPage::addIconItems(md::MdButtonGroup *group)
{
    // A view switcher — the canonical button group use case. The names are
    // picked from the classic SVG baseline the gallery ships, because the
    // Material Symbols font is a system dependency here and the page has to
    // look right without it.
    group->addItem(QString(), QStringLiteral("list"), L("列表视图", "List view"));
    group->addItem(QString(), QStringLiteral("dashboard"), L("网格视图", "Grid view"));
    group->addItem(QString(), QStringLiteral("apps"), L("全部应用", "All apps"));
}

void ButtonGroupPage::layFlow(GalleryContext &context, const QVector<md::MdButtonGroup *> &groups)
{
    const qreal available = context.width();

    QVector<md::MdButtonGroup *> row;
    qreal rowWidth = 0.0;
    qreal rowHeight = 0.0;

    const auto flush = [&] {
        if (row.isEmpty()) {
            return;
        }
        const QRectF band = context.band(rowHeight);
        qreal x = band.left();
        for (md::MdButtonGroup *group : row) {
            const QSize hint = group->sizeHint();
            m_slots.append(GroupSlot{
                group,
                QRectF(x, band.top() + (rowHeight - hint.height()) / 2.0, hint.width(),
                       hint.height())});
            x += hint.width() + kRowGap;
        }
        context.space(kStackGap);
        row.clear();
        rowWidth = 0.0;
        rowHeight = 0.0;
    };

    for (md::MdButtonGroup *group : groups) {
        const QSize hint = group->sizeHint();
        const qreal needed = row.isEmpty() ? qreal(hint.width())
                                           : rowWidth + kRowGap + hint.width();
        if (!row.isEmpty() && needed > available) {
            flush();
        }
        if (!row.isEmpty()) {
            rowWidth += kRowGap;
        }
        row.append(group);
        rowWidth += hint.width();
        rowHeight = qMax(rowHeight, qreal(hint.height()));
    }
    flush();
}

void ButtonGroupPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- two variants --------------------------------------------------------
    context.section(L("两种变体 —— 标准与连接", "Two variants — standard and connected"));
    context.paragraph(L(
        "标准变体是一个弹簧：按下某个条目会让其宽度增长 15%，并把相邻条目推开。连接"
        "变体什么都不移动——按压只改变被按条目自身的圆角，整行在每个尺寸下都保持 "
        "2 px 间距。按钮组本身没有任何颜色；这些条目是坐落在色调选中态上的填充按钮。",
        "Standard is a spring: pressing an item grows its width by 15 % and pushes its "
        "neighbours aside. Connected moves nothing — pressing only changes the pressed "
        "item's own corners, and the whole row keeps a 2 px gap at every size. The group "
        "itself has no colours; these items are filled buttons on a tonal selected state."));
    {
        auto *standard = group(QStringLiteral("variant-standard"), [](md::MdButtonGroup *g) {
            addTextItems(g);
        });
        auto *connected = group(QStringLiteral("variant-connected"), [](md::MdButtonGroup *g) {
            g->setVariant(md::ButtonGroupVariant::Connected);
            addTextItems(g);
        });
        layFlow(context, {standard, connected});
    }

    // --- five sizes ----------------------------------------------------------
    context.section(L("五种 Expressive 尺寸", "Five Expressive sizes"));
    context.paragraph(L(
        "md.comp.button-group.<size>.container.height —— 32 / 40 / 56 / 96 / 136。标准"
        "变体的 between-space 为 18 / 12 / 8 / 8 / 8；连接变体在每个尺寸下都是 2。特小"
        "与小的连接组保持 48 px 的条目最小边长，这正是它们的条目看起来比标签更宽的原因。",
        "md.comp.button-group.<size>.container.height — 32 / 40 / 56 / 96 / 136. Standard "
        "between-space is 18 / 12 / 8 / 8 / 8; connected is 2 at every size. Extra small and "
        "small connected groups keep a 48 px minimum item extent, which is why their items "
        "look wider than their labels."));
    {
        QVector<md::MdButtonGroup *> row;
        for (int i = 0; i < int(md::ButtonSize::Count); ++i) {
            const auto size = md::ButtonSize(i);
            row.append(group(QStringLiteral("size-") + md::buttonSizeName(size),
                             [size](md::MdButtonGroup *g) {
                                 g->setGroupSize(size);
                                 addIconItems(g);
                             }));
        }
        layFlow(context, row);
    }
    {
        QVector<md::MdButtonGroup *> row;
        for (int i = 0; i < int(md::ButtonSize::Count); ++i) {
            const auto size = md::ButtonSize(i);
            row.append(group(QStringLiteral("size-connected-") + md::buttonSizeName(size),
                             [size](md::MdButtonGroup *g) {
                                 g->setVariant(md::ButtonGroupVariant::Connected);
                                 g->setGroupSize(size);
                                 addIconItems(g);
                             }));
        }
        layFlow(context, row);
    }

    // --- two shapes ----------------------------------------------------------
    context.section(L("两种容器形状", "Two container shapes"));
    context.paragraph(L(
        "形状旋钮属于连接形式：内部圆角为方形，外侧圆角随尺寸递增（small 及以上为 "
        "8 / 8 / 16 / 20 px）。在标准组中，形状决定选中的开关形变成什么——在下方选中"
        "一个看它变圆，再按住它看圆角移动。",
        "The shape knob belongs to the connected form, where inner corners are square and "
        "the outer corners step with size (8 / 8 / 16 / 20 px for small and up). In a "
        "standard group the shape instead decides what a selected toggle morphs into — "
        "select one below and watch it go round, then press it and watch the corners move."));
    {
        auto *round = group(QStringLiteral("shape-round"), [](md::MdButtonGroup *g) {
            g->setVariant(md::ButtonGroupVariant::Connected);
            g->setGroupShape(md::ButtonShape::Round);
            g->setSelectionMode(md::ButtonGroupSelection::Single);
            g->setCurrentIndex(1);
            addTextItems(g);
        });
        auto *square = group(QStringLiteral("shape-square"), [](md::MdButtonGroup *g) {
            g->setVariant(md::ButtonGroupVariant::Connected);
            g->setGroupShape(md::ButtonShape::Square);
            g->setSelectionMode(md::ButtonGroupSelection::Single);
            g->setCurrentIndex(1);
            addTextItems(g);
        });
        auto *standardToggle = group(QStringLiteral("shape-standard"), [](md::MdButtonGroup *g) {
            g->setSelectionMode(md::ButtonGroupSelection::Multiple);
            addTextItems(g);
            g->itemAt(1)->setCheckable(true);
            g->setSelected(1, true);
        });
        layFlow(context, {round, square, standardToggle});
    }

    // --- selection modes -----------------------------------------------------
    context.section(L("四种选择模式", "Four selection modes"));
    context.paragraph(L(
        "None 是工具栏——点击只上报，不保留选中，方向键也不改写任何东西。Single 是"
        "单选组：选中跟随焦点，方向键循环。Multiple 是相互独立的开关；再点一次已选中的"
        "即取消。Required 不允许为空——点击已选中的条目没有效果。用方向键走一遍，看看"
        "焦点与选中如何表现。",
        "None is a tool bar — clicking reports, nothing stays selected, and the arrow keys "
        "do not rewrite anything. Single is a radio group: selection follows focus and the "
        "arrow keys wrap. Multiple is independent toggles; clicking the selected one clears "
        "it. Required forbids empty — clicking the selected item is a no-op. Walk one with "
        "the arrow keys to see focus and selection behave."));
    {
        auto *none = group(QStringLiteral("mode-none"), [](md::MdButtonGroup *g) {
            g->setSelectionMode(md::ButtonGroupSelection::None);
            addTextItems(g);
        });
        auto *single = group(QStringLiteral("mode-single"), [](md::MdButtonGroup *g) {
            g->setSelectionMode(md::ButtonGroupSelection::Single);
            g->setCurrentIndex(1);
            addTextItems(g);
        });
        layFlow(context, {none, single});
    }
    {
        auto *multi = group(QStringLiteral("mode-multi"), [](md::MdButtonGroup *g) {
            g->setSelectionMode(md::ButtonGroupSelection::Multiple);
            addTextItems(g);
            g->itemAt(0)->setCheckable(true);
            g->itemAt(2)->setCheckable(true);
            g->setSelected(0, true);
            g->setSelected(2, true);
        });
        auto *required = group(QStringLiteral("mode-required"), [](md::MdButtonGroup *g) {
            g->setSelectionMode(md::ButtonGroupSelection::Required);
            g->setCurrentIndex(2);
            addTextItems(g);
        });
        layFlow(context, {multi, required});
    }

    // --- vertical -------------------------------------------------------------
    context.section(L("垂直方向", "Vertical orientation"));
    context.paragraph(L(
        "同一组令牌，转了个方向：between-space 沿列向下排列，横轴仍是令牌容器高度，"
        "连接组的内部圆角变成上下两条边。",
        "The same tokens, rotated: between-space runs down the column, the cross axis is "
        "still the token container height, and in a connected group the inner corners are "
        "the top and bottom edges."));
    {
        auto *verticalStandard = group(QStringLiteral("vertical-standard"),
                                       [](md::MdButtonGroup *g) {
                                           g->setOrientation(
                                               md::ButtonGroupOrientation::Vertical);
                                           addTextItems(g);
                                       });
        auto *verticalConnected = group(QStringLiteral("vertical-connected"),
                                        [](md::MdButtonGroup *g) {
                                            g->setOrientation(
                                                md::ButtonGroupOrientation::Vertical);
                                            g->setVariant(md::ButtonGroupVariant::Connected);
                                            addTextItems(g);
                                        });
        layFlow(context, {verticalStandard, verticalConnected});
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts ----------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-button-group-{standard,"
        "connected}-<size>.scss"));
    context.detail(QStringLiteral(
        "height    xsmall 32 · small 40 · medium 56 · large 96 · xlarge 136"));
    context.detail(QStringLiteral(
        "space     standard 18 · 12 · 8 · 8 · 8   connected 2 at every size"));
    context.detail(QStringLiteral(
        "press     standard.pressed.item.width.multiplier 15 %, spring-fast-spatial "
        "(stiffness 1400, damping 0.9)"));
    context.detail(QStringLiteral(
        "connected inner corners  small / extra-small / small / large / large-increased, "
        "outer corners full"));
    context.detail(QStringLiteral(
        "selected  connected.selected.inner-corner.corner-size is the literal 50 % of the "
        "cross extent"));
    context.space(8.0);
    context.detail(L("上游差异，如实记录而不抹平：导出文件称 connected.xsmall.inner-corner "
                     "为 corner-small（8 px），而规格页面写的是 4 px——以导出为准。",
                     "Upstream gap, recorded not smoothed: the export says "
                     "connected.xsmall.inner-corner is corner-small (8 px) but the spec page "
                     "says 4 px — the export wins."));
    context.detail(L("TestMd3ButtonGroup 将令牌表与选择模型逐字段锁定。",
                     "TestMd3ButtonGroup pins the token table and the selection model field "
                     "by field."));
}

void ButtonGroupPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const GroupSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.group->geometry() != target) {
            slot.group->setGeometry(target);
        }
    }
}

void ButtonGroupPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void ButtonGroupPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void ButtonGroupPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void ButtonGroupPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createButtonGroupPage()
{
    return new ButtonGroupPage;
}

} // namespace gallery

#include "GalleryPages5.moc"
