// Gallery page 6: Icon buttons — the third Actions family.
//
// Same rules as the other component pages: the page never styles anything,
// widgets are banked, and build() is re-entrant. The one family-specific note:
// an icon button is icon-only by token arithmetic (space + icon + space ==
// height on the default track), so every widget here shows exactly one glyph,
// and the names come from the classic SVG baseline the gallery ships.

#include "GalleryPages.h"

#include "widgets/MdIconButton.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace gallery {

namespace {

/// One banked button plus the content-local rect the latest build() gave it.
struct IconSlot
{
    md::MdIconButton *button = nullptr;
    QRectF rect;
};

/// Gap between neighbouring buttons in a row.
constexpr qreal kRowGap = 16.0;
/// Gap between stacked rows inside one section.
constexpr qreal kStackGap = 12.0;

} // namespace

class IconButtonPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit IconButtonPage(QWidget *parent = nullptr);
    ~IconButtonPage() override;

    QString title() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void layFlow(GalleryContext &context, const QVector<md::MdIconButton *> &buttons);
    void layoutChildren();
    void placeChildren();

    md::MdIconButton *button(const QString &id, const QString &iconName);
    template <typename Configure>
    md::MdIconButton *button(const QString &id, const QString &iconName, Configure configure)
    {
        const bool created = !m_bank.contains(id);
        md::MdIconButton *widget = button(id, iconName);
        if (created) {
            configure(widget);
        }
        return widget;
    }

    QHash<QString, md::MdIconButton *> m_bank;
    QVector<IconSlot> m_slots;
};

IconButtonPage::IconButtonPage(QWidget *parent)
    : GalleryPage(parent)
{
}

IconButtonPage::~IconButtonPage() = default;

QString IconButtonPage::title() const
{
    return QStringLiteral("Icon buttons");
}

QString IconButtonPage::subtitle() const
{
    return QStringLiteral("Four colour styles, five Expressive sizes, three padding tracks, and "
                          "the selected toggle form — every glyph is a live button.");
}

md::MdIconButton *IconButtonPage::button(const QString &id, const QString &iconName)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdIconButton(iconName, this);
    created->show();
    m_bank.insert(id, created);
    return created;
}

void IconButtonPage::layFlow(GalleryContext &context, const QVector<md::MdIconButton *> &buttons)
{
    const qreal available = context.width();

    QVector<md::MdIconButton *> row;
    qreal rowWidth = 0.0;
    qreal rowHeight = 0.0;

    const auto flush = [&] {
        if (row.isEmpty()) {
            return;
        }
        const QRectF band = context.band(rowHeight);
        qreal x = band.left();
        for (md::MdIconButton *button : row) {
            const QSize hint = button->sizeHint();
            m_slots.append(IconSlot{
                button,
                QRectF(x, band.top() + (rowHeight - hint.height()) / 2.0, hint.width(),
                       hint.height())});
            x += hint.width() + kRowGap;
        }
        context.space(kStackGap);
        row.clear();
        rowWidth = 0.0;
        rowHeight = 0.0;
    };

    for (md::MdIconButton *button : buttons) {
        const QSize hint = button->sizeHint();
        const qreal needed = row.isEmpty() ? qreal(hint.width())
                                           : rowWidth + kRowGap + hint.width();
        if (!row.isEmpty() && needed > available) {
            flush();
        }
        if (!row.isEmpty()) {
            rowWidth += kRowGap;
        }
        row.append(button);
        rowWidth += hint.width();
        rowHeight = qMax(rowHeight, qreal(hint.height()));
    }
    flush();
}

void IconButtonPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- four colour styles ---------------------------------------------------
    context.section(QStringLiteral("Four colour styles — md.comp.icon-button.<style>"));
    context.paragraph(QStringLiteral(
        "Standard, filled, tonal, outlined — there is no elevated icon button. Standard paints "
        "no container at all, which is why it looks like a bare glyph; the other three fill, and "
        "outlined strokes in outline-variant. Hover one to see the state layer."));
    {
        const QVector<QPair<QString, md::IconButtonVariant>> specs = {
            {QStringLiteral("Standard"), md::IconButtonVariant::Standard},
            {QStringLiteral("Filled"), md::IconButtonVariant::Filled},
            {QStringLiteral("Tonal"), md::IconButtonVariant::Tonal},
            {QStringLiteral("Outlined"), md::IconButtonVariant::Outlined},
        };
        QVector<md::MdIconButton *> row;
        for (const auto &spec : specs) {
            row.append(button(QStringLiteral("style-") + md::iconButtonVariantName(spec.second),
                              QStringLiteral("favorite"),
                              [&spec](md::MdIconButton *widget) {
                                  widget->setVariant(spec.second);
                                  widget->setButtonSize(md::ButtonSize::Small);
                              }));
        }
        layFlow(context, row);
    }

    // --- five sizes -----------------------------------------------------------
    context.section(QStringLiteral("Five Expressive sizes"));
    context.paragraph(QStringLiteral(
        "md.comp.icon-button.<size>: height 32 / 40 / 56 / 96 / 136, icon 20 / 24 / 24 / 32 / 40. "
        "On the default track the container is exactly square — leading space + icon + trailing "
        "space equals the height at every size, which the token table pins."));
    {
        QVector<md::MdIconButton *> row;
        for (int i = 0; i < int(md::ButtonSize::Count); ++i) {
            const auto size = md::ButtonSize(i);
            row.append(button(QStringLiteral("size-") + md::buttonSizeName(size),
                              QStringLiteral("star"),
                              [size](md::MdIconButton *widget) {
                                  widget->setVariant(md::IconButtonVariant::Filled);
                                  widget->setButtonSize(size);
                              }));
        }
        layFlow(context, row);
    }

    // --- two shapes -----------------------------------------------------------
    context.section(QStringLiteral("Two container shapes"));
    context.paragraph(QStringLiteral(
        "Round is corner-full at every size; square steps up (12 / 12 / 16 / 28 / 28 px). Press "
        "one and the corners morph to the pressed shape on the spring — and on a selected "
        "toggle the two knobs swap, which the next section shows."));
    {
        QVector<md::MdIconButton *> row;
        for (int i = 0; i < int(md::ButtonSize::Count); ++i) {
            const auto size = md::ButtonSize(i);
            row.append(button(QStringLiteral("shape-") + md::buttonSizeName(size),
                              QStringLiteral("settings"),
                              [size](md::MdIconButton *widget) {
                                  widget->setVariant(md::IconButtonVariant::Tonal);
                                  widget->setButtonSize(size);
                                  widget->setButtonShape(md::ButtonShape::Square);
                              }));
        }
        layFlow(context, row);
    }

    // --- toggle / selected ------------------------------------------------------
    context.section(QStringLiteral("The selected toggle form"));
    context.paragraph(QStringLiteral(
        "A selected icon button is a checkable one. The colours come from the selected / "
        "unselected token families — a filled toggle rests on a surface-container chip while "
        "unchecked and fills primary when checked — and the resting corners swap knobs: the "
        "selected round button has the square-ish corner. Click one to toggle it."));
    {
        auto *standardToggle = button(QStringLiteral("toggle-standard"),
                                      QStringLiteral("notifications"), [](md::MdIconButton *widget) {
                                          widget->setVariant(md::IconButtonVariant::Standard);
                                          widget->setToggleable(true);
                                      });
        auto *filledOff = button(QStringLiteral("toggle-filled-off"), QStringLiteral("star"),
                                 [](md::MdIconButton *widget) {
                                     widget->setVariant(md::IconButtonVariant::Filled);
                                     widget->setToggleable(true);
                                 });
        auto *filledOn = button(QStringLiteral("toggle-filled-on"), QStringLiteral("star"),
                                [](md::MdIconButton *widget) {
                                    widget->setVariant(md::IconButtonVariant::Filled);
                                    widget->setToggleable(true);
                                    widget->setSelected(true);
                                });
        auto *outlinedOn = button(QStringLiteral("toggle-outlined-on"), QStringLiteral("add"),
                                  [](md::MdIconButton *widget) {
                                      widget->setVariant(md::IconButtonVariant::Outlined);
                                      widget->setToggleable(true);
                                      widget->setSelected(true);
                                  });
        layFlow(context, {standardToggle, filledOff, filledOn, outlinedOn});
    }

    // --- space tracks ---------------------------------------------------------
    context.section(QStringLiteral("Three padding tracks"));
    context.paragraph(QStringLiteral(
        "The export publishes three leading/trailing space sets: default (the square track), "
        "narrow and wide. Same 40 px height, three widths — the tokens are a real axis, not a "
        "comment."));
    {
        QVector<md::MdIconButton *> row;
        const auto trackName = [](md::IconButtonSpaceTrack track) {
            switch (track) {
            case md::IconButtonSpaceTrack::Default: return QStringLiteral("default");
            case md::IconButtonSpaceTrack::Narrow: return QStringLiteral("narrow");
            case md::IconButtonSpaceTrack::Wide: return QStringLiteral("wide");
            case md::IconButtonSpaceTrack::Count: break;
            }
            return QStringLiteral("unknown");
        };
        for (int i = 0; i < int(md::IconButtonSpaceTrack::Count); ++i) {
            const auto track = md::IconButtonSpaceTrack(i);
            row.append(button(QStringLiteral("track-") + trackName(track),
                              QStringLiteral("search"),
                              [track](md::MdIconButton *widget) {
                                  widget->setVariant(md::IconButtonVariant::Outlined);
                                  widget->setSpaceTrack(track);
                              }));
        }
        layFlow(context, row);
    }

    // --- disabled ---------------------------------------------------------------
    context.section(QStringLiteral("Disabled"));
    context.paragraph(QStringLiteral(
        "on-surface at the published opacities — container 0.1, icon 0.38 — and the outlined "
        "style keeps its stroke. A disabled icon button leaves the tab order."));
    {
        QVector<md::MdIconButton *> row;
        for (int i = 0; i < int(md::IconButtonVariant::Count); ++i) {
            const auto variant = md::IconButtonVariant(i);
            row.append(button(QStringLiteral("off-") + md::iconButtonVariantName(variant),
                              QStringLiteral("delete"),
                              [variant](md::MdIconButton *widget) {
                                  widget->setVariant(variant);
                                  widget->setEnabled(false);
                              }));
        }
        layFlow(context, row);
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts ------------------------------------------------------------
    context.section(QStringLiteral("Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-icon-button{,-<style>,"
        "-<size>}.scss"));
    context.detail(QStringLiteral(
        "height    xsmall 32 · small 40 · medium 56 · large 96 · xlarge 136  icon  20 · 24 · "
        "24 · 32 · 40"));
    context.detail(QStringLiteral(
        "tracks    default 6/8/16/32/48 · narrow 4/4/12/16/32 · wide 10/14/24/48/72"));
    context.detail(QStringLiteral(
        "shapes    round corner-full · square 12/12/16/28/28 · pressed 8/8/12/16/16, "
        "spring-fast-spatial"));
    context.detail(QStringLiteral(
        "selected  the knobs swap: selected round is 12/12/16/28/28, selected square is full; "
        "outlined selected fills inverse-surface and drops the stroke"));
    context.detail(QStringLiteral(
        "disabled  container on-surface @ 0.1 · icon on-surface @ 0.38 · outlined keeps its "
        "stroke; selected-disabled carries its own 0.1 token"));
    context.space(8.0);
    context.detail(QStringLiteral(
        "TestMd3IconButton pins the size table and all three colour families field by field."));
}

void IconButtonPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const IconSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.button->geometry() != target) {
            slot.button->setGeometry(target);
        }
    }
}

void IconButtonPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void IconButtonPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void IconButtonPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void IconButtonPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createIconButtonPage()
{
    return new IconButtonPage;
}

} // namespace gallery

#include "GalleryPages6.moc"
