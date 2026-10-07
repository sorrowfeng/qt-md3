// Gallery page 4: the Actions families.
//
// This is the first page whose subject is a *widget* rather than a token table,
// so it is also the first page with child widgets. Two rules shape it:
//
//   * The page never styles a button. It sets variant / size / shape / icons and
//     nothing else — no palette, no font, no stylesheet. If a button looks wrong
//     that is the library's fault and the page has to show it.
//   * build() records where each button belongs in content-local coordinates;
//     layoutChildren() maps those onto real geometry once the base class has
//     told us where the content rect is. Measurement and painting therefore run
//     the same placement code, exactly as the token pages do.
//
// The one thing a component page must not do is treat build() as a one-shot.
// The base class calls it from measure(), remeasure() and paintEvent() — that
// is how one code path serves both measurement and painting — so build() is
// called many times over a page's life, at changing widths. A page that
// allocated its widgets inside build() would therefore create a new generation
// of children on every layout pass, all parked at (0, 0) and all visible. The
// bank below is what keeps that from happening: widgets are created once, on
// first request, and every later build() call reuses them.

#include "GalleryPages.h"

#include "widgets/MdButton.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace gallery {

namespace {

/// One banked button plus the content-local rect the latest build() gave it.
struct ButtonSlot
{
    md::MdButton *button = nullptr;
    QRectF rect;
};

/// Gap between neighbouring buttons in a row.
constexpr qreal kRowGap = 16.0;
/// Gap between stacked rows inside one section.
constexpr qreal kStackGap = 12.0;

} // namespace

class ButtonPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit ButtonPage(QWidget *parent = nullptr);
    ~ButtonPage() override;

    QString title() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    /// Reserve a band for `buttons`, wrapping to a new band when the row would
    /// overflow the content width. Appends to m_slots.
    void layFlow(GalleryContext &context, const QVector<md::MdButton *> &buttons);

    /// Re-run build() against a null painter and move the buttons to the rects
    /// it produced. Safe outside a paint handler, which is the point: Qt paints
    /// children *after* the parent, so placing them from paintEvent would leave
    /// them one frame stale.
    void layoutChildren();
    /// Move the buttons to the rects from the most recent build().
    void placeChildren();

    /// Fetch the banked button called `id`, creating and parenting it on first
    /// use. Reusing the *same* widget across build() calls is what makes the
    /// measurement / painting parity claim true for a page with children.
    md::MdButton *button(const QString &id, const QString &text);

    /// Apply `configure` to the banked button exactly once, the first time the
    /// id is seen. Properties are not re-applied on later builds because they
    /// have not changed and re-setting them would emit spurious NOTIFYs.
    template <typename Configure>
    md::MdButton *button(const QString &id, const QString &text, Configure configure)
    {
        const bool created = !m_bank.contains(id);
        md::MdButton *widget = button(id, text);
        if (created) {
            configure(widget);
        }
        return widget;
    }

    QHash<QString, md::MdButton *> m_bank;
    QVector<ButtonSlot> m_slots;
};

ButtonPage::ButtonPage(QWidget *parent)
    : GalleryPage(parent)
{
}

ButtonPage::~ButtonPage() = default;

QString ButtonPage::title() const
{
    return QStringLiteral("Buttons");
}

QString ButtonPage::subtitle() const
{
    return QStringLiteral("Five colour styles, five Expressive sizes, two container shapes — "
                          "all live, so hover, click and Tab through them.");
}

md::MdButton *ButtonPage::button(const QString &id, const QString &text)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    // Parented to the page and shown once; Qt owns the deletion from here on.
    auto *created = new md::MdButton(text, this);
    created->show();
    m_bank.insert(id, created);
    return created;
}

void ButtonPage::layFlow(GalleryContext &context, const QVector<md::MdButton *> &buttons)
{
    const qreal available = context.width();

    QVector<md::MdButton *> row;
    qreal rowWidth = 0.0;
    qreal rowHeight = 0.0;

    // Reserve the band, then place. The band is consumed from the context
    // cursor so measurement and painting stay in step.
    const auto flush = [&] {
        if (row.isEmpty()) {
            return;
        }
        const QRectF band = context.band(rowHeight);
        qreal x = band.left();
        for (md::MdButton *button : row) {
            const QSize hint = button->sizeHint();
            m_slots.append(ButtonSlot{
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

    for (md::MdButton *button : buttons) {
        const QSize hint = button->sizeHint();
        const qreal needed = row.isEmpty() ? qreal(hint.width())
                                           : rowWidth + kRowGap + hint.width();
        // Wrap *before* placing, so a size that will not fit still gets its own
        // band rather than being clipped by the content gutter.
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

void ButtonPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- five colour styles ------------------------------------------------
    context.section(QStringLiteral("Five colour styles — md.comp.button.<style>"));
    context.paragraph(QStringLiteral(
        "Elevated, filled, tonal, outlined and text. The container-less two (outlined and "
        "text) paint no fill at all, which is a token fact rather than a transparency trick: "
        "their md.comp.button.<style>.container.color does not exist."));
    {
        const QVector<QPair<QString, md::ButtonVariant>> specs = {
            {QStringLiteral("Elevated"), md::ButtonVariant::Elevated},
            {QStringLiteral("Filled"), md::ButtonVariant::Filled},
            {QStringLiteral("Tonal"), md::ButtonVariant::Tonal},
            {QStringLiteral("Outlined"), md::ButtonVariant::Outlined},
            {QStringLiteral("Text"), md::ButtonVariant::Text},
        };
        QVector<md::MdButton *> row;
        for (const auto &spec : specs) {
            // An icon on every one, so the icon space and the icon-label gap
            // are exercised in the same band as the plain label case.
            row.append(button(QStringLiteral("style-") + md::buttonVariantName(spec.second),
                              spec.first,
                              [&spec](md::MdButton *widget) {
                                  widget->setVariant(spec.second);
                                  widget->setButtonSize(md::ButtonSize::Small);
                                  widget->setLeadingIcon(QStringLiteral("add"));
                              }));
        }
        layFlow(context, row);
    }

    // --- five sizes --------------------------------------------------------
    context.section(QStringLiteral("Five Expressive sizes"));
    context.paragraph(QStringLiteral(
        "md.comp.button.<size> sets the height, the icon size, the leading and trailing "
        "padding, the outline width and the type style. The container is exactly the token "
        "height; the widget around it is slightly taller because it reserves room for the "
        "outward focus indicator, which Qt would otherwise clip at the widget edge."));
    {
        const QVector<QPair<QString, md::ButtonSize>> specs = {
            {QStringLiteral("XSmall"), md::ButtonSize::XSmall},
            {QStringLiteral("Small"), md::ButtonSize::Small},
            {QStringLiteral("Medium"), md::ButtonSize::Medium},
            {QStringLiteral("Large"), md::ButtonSize::Large},
            {QStringLiteral("XLarge"), md::ButtonSize::XLarge},
        };
        QVector<md::MdButton *> row;
        for (const auto &spec : specs) {
            row.append(button(QStringLiteral("size-") + md::buttonSizeName(spec.second),
                              spec.first,
                              [&spec](md::MdButton *widget) {
                                  widget->setVariant(md::ButtonVariant::Filled);
                                  widget->setButtonSize(spec.second);
                              }));
        }
        layFlow(context, row);
    }

    // --- two shapes --------------------------------------------------------
    context.section(QStringLiteral("Two container shapes"));
    context.paragraph(QStringLiteral(
        "Container.shape.round is corner-full at every size; .square steps up with the size "
        "(12 / 12 / 16 / 28 / 28 px) and the pressed shape is one step below it. Press one to "
        "see the corners morph — it is a spring, not a fixed-duration easing curve."));
    {
        QVector<md::MdButton *> row;
        for (int i = 0; i < int(md::ButtonSize::Count); ++i) {
            const auto size = md::ButtonSize(i);
            row.append(button(QStringLiteral("shape-") + md::buttonSizeName(size),
                              md::buttonSizeName(size),
                              [size](md::MdButton *widget) {
                                  widget->setVariant(md::ButtonVariant::Tonal);
                                  widget->setButtonSize(size);
                                  widget->setButtonShape(md::ButtonShape::Square);
                              }));
        }
        layFlow(context, row);
    }

    // --- icons -------------------------------------------------------------
    context.section(QStringLiteral("Leading and trailing icons"));
    context.paragraph(QStringLiteral(
        "Leading and trailing are inline-start and inline-end, so they swap under the header's "
        "rtl toggle. The icon box is md.comp.button.<size>.icon.size and the gap to the label "
        "is icon-label-space, which only applies when there is a label to separate."));
    {
        // Shared configuration for the four icon permutations.
        const auto asSmallFilled = [](md::MdButton *widget) {
            widget->setVariant(md::ButtonVariant::Filled);
            widget->setButtonSize(md::ButtonSize::Small);
        };

        auto *plain = button(QStringLiteral("icon-plain"), QStringLiteral("Label"), asSmallFilled);

        auto *leading = button(QStringLiteral("icon-leading"), QStringLiteral("Send"),
                               [&asSmallFilled](md::MdButton *widget) {
                                   asSmallFilled(widget);
                                   widget->setLeadingIcon(QStringLiteral("send"));
                               });

        auto *trailing = button(QStringLiteral("icon-trailing"), QStringLiteral("Open"),
                                [&asSmallFilled](md::MdButton *widget) {
                                    asSmallFilled(widget);
                                    widget->setTrailingIcon(QStringLiteral("arrow_forward"));
                                });

        auto *both = button(QStringLiteral("icon-both"), QStringLiteral("Share"),
                            [&asSmallFilled](md::MdButton *widget) {
                                asSmallFilled(widget);
                                widget->setLeadingIcon(QStringLiteral("share"));
                                widget->setTrailingIcon(QStringLiteral("expand_more"));
                            });

        layFlow(context, {plain, leading, trailing, both});
    }

    // --- states ------------------------------------------------------------
    context.section(QStringLiteral("States"));
    context.paragraph(QStringLiteral(
        "Hover, focus and press are live on every button above and below — this page does not "
        "fake them, because the state layer and the ripple are the two mechanisms most likely "
        "to be wrong and they are worth poking at. Disabled and soft-disabled are shown here: "
        "a disabled button leaves the tab order, a soft-disabled one stays in it."));
    {
        QVector<md::MdButton *> row;

        for (int i = 0; i < int(md::ButtonVariant::Count); ++i) {
            const auto variant = md::ButtonVariant(i);
            row.append(button(QStringLiteral("off-") + md::buttonVariantName(variant),
                              md::buttonVariantName(variant) + QStringLiteral(" off"),
                              [variant](md::MdButton *widget) {
                                  widget->setVariant(variant);
                                  widget->setButtonSize(md::ButtonSize::Small);
                                  widget->setEnabled(false);
                              }));
        }
        layFlow(context, row);
    }
    {
        auto *softOutlined = button(QStringLiteral("soft-outlined"),
                                    QStringLiteral("Soft-disabled"),
                                    [](md::MdButton *widget) {
                                        widget->setVariant(md::ButtonVariant::Outlined);
                                        widget->setSoftDisabled(true);
                                    });

        auto *softFilled = button(QStringLiteral("soft-filled"), QStringLiteral("Tab to me"),
                                  [](md::MdButton *widget) {
                                      widget->setVariant(md::ButtonVariant::Filled);
                                      widget->setSoftDisabled(true);
                                  });

        auto *mnemonic = button(QStringLiteral("mnemonic"), QStringLiteral("&Save"),
                                [](md::MdButton *widget) {
                                    widget->setVariant(md::ButtonVariant::Filled);
                                });

        layFlow(context, {softOutlined, softFilled, mnemonic});
    }

    // Every banked button is referenced by some section above, so a rebuild
    // cannot strand a widget at its default geometry. Assert that rather than
    // assume it: a stranded child is invisible in the code and very visible on
    // screen.
    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts -------------------------------------------------------
    context.section(QStringLiteral("Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-button{,-<style>,-<size>}.scss"));
    context.detail(QStringLiteral(
        "size      xsmall 32 · small 40 · medium 56 · large 96 · xlarge 136  (container height)"));
    context.detail(QStringLiteral(
        "padding   12 · 16 · 24 · 48 · 64      icon  20 · 20 · 24 · 32 · 40"));
    context.detail(QStringLiteral(
        "outline   1 · 1 · 1 · 2 · 3 px        type  label-large · label-large · title-medium · "
        "headline-small · headline-large"));
    context.detail(QStringLiteral(
        "press     container.shape.round/.square -> pressed.container.shape, spring-fast-spatial"));
    context.detail(QStringLiteral(
        "focus     md.comp.button.focus.indicator: 3 px stroke, 2 px gap, secondary"));
    context.space(8.0);
    context.detail(QStringLiteral(
        "TestMd3Button pins all 5 x 5 x 2 combinations field by field against those files."));
}

void ButtonPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const ButtonSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        // Guarded because this also runs from paintEvent, where a redundant
        // setGeometry would still schedule an update and could settle into a
        // repaint loop.
        if (slot.button->geometry() != target) {
            slot.button->setGeometry(target);
        }
    }
}

void ButtonPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void ButtonPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void ButtonPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void ButtonPage::paintEvent(QPaintEvent *event)
{
    // The base paint runs build(), which refreshes m_slots for the width being
    // painted, and sets the content rect. The rects are the same ones
    // layoutChildren() already used, so this is a correctness backstop rather
    // than the primary placement.
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createButtonPage()
{
    return new ButtonPage;
}

} // namespace gallery

#include "GalleryPages4.moc"
