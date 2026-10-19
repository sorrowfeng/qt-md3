// Gallery page 47: Dynamic colour — the live wallpaper-seed demo.
//
// Same rules as the other pages: widgets are banked, build() is re-entrant,
// and the page never styles anything. The family-specific note: every seed,
// variant, mode and contrast control goes through md::MdTheme, so the whole
// gallery (and the live board on this page) repaints through the normal
// theme-changed path. Seed swatches themselves paint their literal seed —
// that is the data being chosen, not a theme role.

#include "GalleryPages.h"

#include "core/MdColorMath.h"
#include "core/MdIcon.h"

#include "widgets/MdButton.h"
#include "widgets/MdCard.h"
#include "widgets/MdChip.h"
#include "widgets/MdFab.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSwitch.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <functional>

namespace gallery {

namespace {

/// Wallpaper-style seeds, the set official MD3 demos reach for. The first
/// entry is the baseline purple everyone starts from.
struct SeedSpec
{
    const char *hex;
    const char *zh;
    const char *en;
};

const SeedSpec kSeeds[] = {
    {"#6750A4", "紫", "Purple"},
    {"#00A3A3", "青", "Teal"},
    {"#E8710A", "橙", "Orange"},
    {"#2E7D32", "绿", "Green"},
    {"#D81B60", "粉", "Pink"},
    {"#1B6EF3", "蓝", "Blue"},
    {"#F9A825", "金", "Gold"},
};

constexpr int kSeedCount = int(sizeof(kSeeds) / sizeof(kSeeds[0]));

void drawTextLine(QPainter &painter, const QRectF &rect, const QFont &font, const QColor &color,
                  const QString &text,
                  int flags = Qt::AlignLeft | Qt::AlignVCenter)
{
    painter.save();
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(rect, flags, text);
    painter.restore();
}

} // namespace

/// Colour-block probe the GalleryClick suite grabs before and after a seed
/// click. Paints primary / secondary / tertiary so a seed change is visible
/// even when the primary hue alone is close.
class ColourProbe : public QWidget
{
public:
    explicit ColourProbe(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("dynamic-colour-probe"));
        setMinimumSize(120, 36);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF strip = QRectF(rect()).adjusted(2.0, 2.0, -2.0, -2.0);
        const md::MdTheme &theme = md::MdTheme::instance();
        const md::ColorRole roles[3] = {md::ColorRole::Primary, md::ColorRole::Secondary,
                                        md::ColorRole::Tertiary};
        const qreal cell = strip.width() / 3.0;
        for (int i = 0; i < 3; ++i) {
            painter.fillRect(QRectF(strip.left() + i * cell, strip.top(), cell, strip.height()),
                             theme.color(roles[i]));
        }
    }
};

/// One wallpaper-seed swatch. Clicking it installs the seed on the theme.
class SeedSwatch : public QWidget
{
    Q_OBJECT

public:
    SeedSwatch(const QColor &seed, const QString &label, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_seed(seed)
        , m_label(label)
    {
        setFixedSize(88, 72);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
    }

    QColor seed() const { return m_seed; }
    void setSelected(bool selected)
    {
        if (m_selected == selected) {
            return;
        }
        m_selected = selected;
        update();
    }

signals:
    void picked(const QColor &seed);

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF body = QRectF(rect()).adjusted(8.0, 4.0, -8.0, -20.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_seed);
        painter.drawPath(md::MdShape::roundedRect(
            body, md::MdShape::resolvedRadii(md::ShapeCorner::Medium, body.size())));

        const md::MdTheme &theme = md::MdTheme::instance();
        if (m_selected) {
            QPen ring(theme.color(md::ColorRole::Primary));
            ring.setWidthF(3.0);
            ring.setCosmetic(true);
            painter.setPen(ring);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(md::MdShape::roundedRect(
                body.adjusted(-3.0, -3.0, 3.0, 3.0),
                md::MdShape::resolvedRadii(md::ShapeCorner::Medium,
                                           body.adjusted(-3, -3, 3, 3).size())));
        }

        painter.setFont(GalleryPage::fontFor(md::TypeStyle::LabelSmall));
        painter.setPen(theme.color(md::ColorRole::OnSurfaceVariant));
        painter.drawText(QRectF(0.0, rect().height() - 18.0, rect().width(), 16.0),
                         Qt::AlignHCenter | Qt::AlignVCenter, m_label);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit picked(m_seed);
        }
        QWidget::mousePressEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return
            || event->key() == Qt::Key_Enter) {
            emit picked(m_seed);
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
    }

private:
    QColor m_seed;
    QString m_label;
    bool m_selected = false;
};

/// Scheme-variant chip. Clicking it installs the variant on the theme.
class VariantChip : public QWidget
{
    Q_OBJECT

public:
    explicit VariantChip(md::SchemeVariant variant, const QString &label,
                         QWidget *parent = nullptr)
        : QWidget(parent)
        , m_variant(variant)
        , m_label(label)
    {
        setFixedSize(int(label.size() * 7.0 + 28.0), 32);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
    }

    md::SchemeVariant variant() const { return m_variant; }
    void setSelected(bool selected)
    {
        if (m_selected == selected) {
            return;
        }
        m_selected = selected;
        update();
    }

signals:
    void picked(md::SchemeVariant variant);

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const md::MdTheme &theme = md::MdTheme::instance();
        const QRectF body = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
        const QColor fill = m_selected ? theme.color(md::ColorRole::SecondaryContainer)
                                       : theme.color(md::ColorRole::SurfaceContainer);
        const QColor ink = m_selected ? theme.color(md::ColorRole::OnSecondaryContainer)
                                      : theme.color(md::ColorRole::OnSurfaceVariant);
        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawPath(md::MdShape::roundedRect(
            body, md::MdShape::resolvedRadii(md::ShapeCorner::Full, body.size())));
        painter.setFont(GalleryPage::fontFor(md::TypeStyle::LabelMedium));
        painter.setPen(ink);
        painter.drawText(body, Qt::AlignCenter, m_label);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit picked(m_variant);
        }
        QWidget::mousePressEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return
            || event->key() == Qt::Key_Enter) {
            emit picked(m_variant);
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
    }

private:
    md::SchemeVariant m_variant;
    QString m_label;
    bool m_selected = false;
};

/// Small painted toggle chip used for mode / contrast / dynamic-colour.
class ActionChip : public QWidget
{
    Q_OBJECT

public:
    explicit ActionChip(const QString &label, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_label(label)
    {
        setFixedHeight(32);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
    }

    void setLabel(const QString &label)
    {
        m_label = label;
        updateGeometry();
        update();
    }

    QSize sizeHint() const override
    {
        const QFontMetricsF metrics(GalleryPage::fontFor(md::TypeStyle::LabelMedium));
        return QSize(int(metrics.horizontalAdvance(m_label) + 28.0), 32);
    }

signals:
    void picked();

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const md::MdTheme &theme = md::MdTheme::instance();
        const QRectF body = QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme.color(md::ColorRole::PrimaryContainer));
        painter.drawPath(md::MdShape::roundedRect(
            body, md::MdShape::resolvedRadii(md::ShapeCorner::Full, body.size())));
        painter.setFont(GalleryPage::fontFor(md::TypeStyle::LabelMedium));
        painter.setPen(theme.color(md::ColorRole::OnPrimaryContainer));
        painter.drawText(body, Qt::AlignCenter, m_label);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            emit picked();
        }
        QWidget::mousePressEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return
            || event->key() == Qt::Key_Enter) {
            emit picked();
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
    }

private:
    QString m_label;
};

class DynamicColourPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit DynamicColourPage(QWidget *parent = nullptr);
    ~DynamicColourPage() override;

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
    void syncSelection();
    void applySeed(const QColor &seed);
    void applyVariant(md::SchemeVariant variant);

    template <typename T>
    T *bank(const QString &id, const std::function<T *()> &factory)
    {
        const auto found = m_bank.constFind(id);
        if (found != m_bank.constEnd()) {
            return static_cast<T *>(found.value());
        }
        T *created = factory();
        created->show();
        m_bank.insert(id, created);
        return created;
    }

    struct Slot
    {
        QWidget *widget = nullptr;
        QRectF rect;
    };

    QHash<QString, QWidget *> m_bank;
    QVector<Slot> m_slots;
    QVector<SeedSwatch *> m_seeds;
    QVector<VariantChip *> m_variants;
    ActionChip *m_modeChip = nullptr;
    ActionChip *m_contrastChip = nullptr;
    ActionChip *m_dynamicChip = nullptr;
    ColourProbe *m_probe = nullptr;
};

DynamicColourPage::DynamicColourPage(QWidget *parent)
    : GalleryPage(parent)
{
}

DynamicColourPage::~DynamicColourPage() = default;

QString DynamicColourPage::title() const
{
    return L("动态色彩", "Dynamic colour");
}

QString DynamicColourPage::slug() const
{
    return QStringLiteral("dynamic-colour");
}

QString DynamicColourPage::subtitle() const
{
    return L("点选壁纸种子色、方案变体、对比度与深浅模式，整页控件与色板实时重绘。",
             "Pick a wallpaper seed, scheme variant, contrast and light/dark — every control "
             "and swatch on this page repaints live.");
}

void DynamicColourPage::applySeed(const QColor &seed)
{
    md::MdTheme::instance().setDynamicColor(true);
    md::MdTheme::instance().setSeedColor(seed);
    syncSelection();
    update();
}

void DynamicColourPage::applyVariant(md::SchemeVariant variant)
{
    md::MdTheme::instance().setDynamicColor(true);
    md::MdTheme::instance().setSchemeVariant(variant);
    syncSelection();
    update();
}

void DynamicColourPage::syncSelection()
{
    const md::MdTheme &theme = md::MdTheme::instance();
    for (SeedSwatch *swatch : m_seeds) {
        if (swatch != nullptr) {
            swatch->setSelected(swatch->seed() == theme.seedColor());
        }
    }
    for (VariantChip *chip : m_variants) {
        if (chip != nullptr) {
            chip->setSelected(chip->variant() == theme.schemeVariant());
        }
    }
    if (m_modeChip != nullptr) {
        m_modeChip->setLabel(theme.themeMode() == md::ThemeMode::Dark ? L("深色", "Dark")
                                                                     : L("浅色", "Light"));
    }
    if (m_contrastChip != nullptr) {
        m_contrastChip->setLabel(md::contrastLevelName(theme.contrastLevel()));
    }
    if (m_dynamicChip != nullptr) {
        m_dynamicChip->setLabel(theme.isDynamicColor() ? L("动态 开", "Dynamic on")
                                                       : L("动态 关", "Dynamic off"));
    }
}

void DynamicColourPage::build(GalleryContext &context)
{
    m_slots.clear();
    const md::MdTheme &theme = md::MdTheme::instance();

    context.section(L("说明", "About"));
    context.paragraph(L(
        "官方 MD3 演示之所以五彩缤纷，是因为它们从壁纸取色，并选用 Vibrant / Expressive "
        "等方案变体，让 tertiary 与 primary 拉开色相；再把 secondary / tertiary 用在"
        "组件上。本页把这套开关做成可点的：选种子、换变体、切对比度与深浅，观察下方"
        "组件板与角色色条如何跟着变。",
        "Official MD3 demos look colourful because they seed from the wallpaper, pick "
        "Vibrant / Expressive scheme variants so tertiary pulls away from primary in hue, "
        "and spend secondary / tertiary on the components. This page makes those switches "
        "clickable: pick a seed, change variant, cycle contrast and light/dark, and watch "
        "the board and role strip below follow."));
    context.space(8.0);

    // --- seed picker -------------------------------------------------------
    context.section(L("种子色", "Seed"));
    context.paragraph(L(
        "点击色块即把该色设为 MdTheme::setSeedColor 的种子，并打开动态取色。"
        "全部角色由同一 DynamicScheme 重新生成——没有硬编码的调色板。",
        "Click a swatch to install it via MdTheme::setSeedColor and turn dynamic colour "
        "on. Every role is regenerated from the same DynamicScheme — there is no "
        "hard-coded palette."));
    {
        const qreal pitch = 96.0;
        const qreal rowWidth = kSeedCount * pitch;
        const QRectF band = context.band(80.0);
        qreal x = band.left() + qMax<qreal>(0.0, (band.width() - rowWidth) / 2.0);
        for (int i = 0; i < kSeedCount; ++i) {
            SeedSwatch *swatch = bank<SeedSwatch>(
                QStringLiteral("seed-%1").arg(i), [this, i] {
                    auto *created = new SeedSwatch(QColor(QString::fromUtf8(kSeeds[i].hex)),
                                                   L(kSeeds[i].zh, kSeeds[i].en), this);
                    created->setObjectName(QStringLiteral("seed-swatch-%1").arg(i));
                    connect(created, &SeedSwatch::picked, this, &DynamicColourPage::applySeed);
                    m_seeds.append(created);
                    return created;
                });
            m_slots.append(Slot{swatch, QRectF(x + (pitch - 88.0) / 2.0, band.top(), 88.0, 72.0)});
            x += pitch;
        }
    }
    context.space(6.0);

    // --- variant + mode / contrast ----------------------------------------
    context.section(L("方案变体与对比度", "Variant, mode & contrast"));
    context.paragraph(L(
        "TonalSpot 是默认；Vibrant / Expressive 会把 tertiary 推到对比色相，Rainbow 与 "
        "FruitSalad 更是直接铺开色环。对比度档位改变的是色调值，不是另一套调色板。",
        "TonalSpot is the default; Vibrant / Expressive push tertiary into a contrasting "
        "hue, and Rainbow / FruitSalad spread a full hue ring. Contrast levels move the "
        "tones — they are not a second palette."));
    {
        struct VariantSpec
        {
            md::SchemeVariant variant;
            const char *label;
        };
        const VariantSpec variants[] = {
            {md::SchemeVariant::TonalSpot, "TonalSpot"},
            {md::SchemeVariant::Vibrant, "Vibrant"},
            {md::SchemeVariant::Expressive, "Expressive"},
            {md::SchemeVariant::Content, "Content"},
            {md::SchemeVariant::Fidelity, "Fidelity"},
            {md::SchemeVariant::Monochrome, "Monochrome"},
            {md::SchemeVariant::Neutral, "Neutral"},
            {md::SchemeVariant::Rainbow, "Rainbow"},
            {md::SchemeVariant::FruitSalad, "FruitSalad"},
        };
        constexpr int kVariantCount = int(sizeof(variants) / sizeof(variants[0]));
        const qreal bandHeight = 40.0;
        const QRectF band = context.band(bandHeight);
        qreal x = band.left();
        for (int i = 0; i < kVariantCount; ++i) {
            VariantChip *chip = bank<VariantChip>(
                QStringLiteral("var-%1").arg(i), [this, &variants, i] {
                    auto *created =
                        new VariantChip(variants[i].variant, QString::fromUtf8(variants[i].label),
                                        this);
                    created->setObjectName(QStringLiteral("variant-chip-%1").arg(i));
                    connect(created, &VariantChip::picked, this, &DynamicColourPage::applyVariant);
                    m_variants.append(created);
                    return created;
                });
            const qreal w = chip->sizeHint().width();
            m_slots.append(Slot{chip, QRectF(x, band.top() + 4.0, w, 32.0)});
            x += w + 6.0;
        }
    }
    {
        const QRectF band = context.band(40.0);
        m_modeChip = bank<ActionChip>(QStringLiteral("mode"), [this] {
            auto *created = new ActionChip(QStringLiteral("mode"), this);
            created->setObjectName(QStringLiteral("action-mode"));
            connect(created, &ActionChip::picked, this, [] {
                md::MdTheme::instance().toggleThemeMode();
            });
            return created;
        });
        m_contrastChip = bank<ActionChip>(QStringLiteral("contrast"), [this] {
            auto *created = new ActionChip(QStringLiteral("contrast"), this);
            created->setObjectName(QStringLiteral("action-contrast"));
            connect(created, &ActionChip::picked, this, [] {
                md::MdTheme &theme = md::MdTheme::instance();
                const int next = (int(theme.contrastLevel()) + 1) % int(md::ContrastLevel::Count);
                theme.setContrastLevel(md::ContrastLevel(next));
            });
            return created;
        });
        m_dynamicChip = bank<ActionChip>(QStringLiteral("dynamic"), [this] {
            auto *created = new ActionChip(QStringLiteral("dynamic"), this);
            created->setObjectName(QStringLiteral("action-dynamic"));
            connect(created, &ActionChip::picked, this, [] {
                md::MdTheme &theme = md::MdTheme::instance();
                theme.setDynamicColor(!theme.isDynamicColor());
            });
            return created;
        });
        m_modeChip->setMinimumWidth(96);
        m_contrastChip->setMinimumWidth(110);
        m_dynamicChip->setMinimumWidth(120);
        m_slots.append(Slot{m_modeChip, QRectF(band.left(), band.top() + 4.0, 96.0, 32.0)});
        m_slots.append(
            Slot{m_contrastChip, QRectF(band.left() + 108.0, band.top() + 4.0, 110.0, 32.0)});
        m_slots.append(
            Slot{m_dynamicChip, QRectF(band.left() + 230.0, band.top() + 4.0, 120.0, 32.0)});
    }
    context.space(6.0);

    // --- role strip --------------------------------------------------------
    context.section(L("角色色条", "Role strip"));
    context.paragraph(L(
        "primary / secondary / tertiary 与它们的 container 是三个色相家族；surfaceContainer "
        "阶梯是同色相的不同色调；outline 把描边从文字里拆出来。变体一改，三族的色相差"
        "立刻看得出来。",
        "Primary / secondary / tertiary and their containers are three hue families; the "
        "surfaceContainer ladder is one hue at different tones; outline splits stroke from "
        "ink. Change the variant and the hue gap between the three families is immediate."));
    {
        const md::ColorRole roles[] = {
            md::ColorRole::Primary,           md::ColorRole::OnPrimary,
            md::ColorRole::PrimaryContainer,  md::ColorRole::Secondary,
            md::ColorRole::SecondaryContainer, md::ColorRole::Tertiary,
            md::ColorRole::TertiaryContainer, md::ColorRole::Error,
            md::ColorRole::ErrorContainer,    md::ColorRole::SurfaceContainerLowest,
            md::ColorRole::SurfaceContainerLow, md::ColorRole::SurfaceContainer,
            md::ColorRole::SurfaceContainerHigh, md::ColorRole::SurfaceContainerHighest,
            md::ColorRole::Outline,           md::ColorRole::OutlineVariant,
        };
        constexpr int kRoleCount = int(sizeof(roles) / sizeof(roles[0]));
        const qreal cell = 56.0;
        const qreal pitch = 64.0;
        const int perRow = qMax(1, int(context.width() / pitch));
        int col = 0;
        QRectF band;
        for (int i = 0; i < kRoleCount; ++i) {
            if (col == 0) {
                band = context.band(cell + 22.0);
            }
            const QRectF rect(band.left() + col * pitch, band.top() + 2.0, cell, cell);
            m_slots.append(Slot{nullptr, rect});
            if (col + 1 >= perRow) {
                col = 0;
            } else {
                ++col;
            }
        }
    }

    // --- live component board ---------------------------------------------
    context.section(L("组件板", "Component board"));
    context.paragraph(L(
        "真实 Md* 控件，全部读主题：按钮五种风格、筛选芯片、卡片、滑条、开关、"
        "tertiary FAB 与图标按钮。换种子时它们一起变色，而不是只有色板好看。",
        "Real Md* widgets, all theme-driven: the five button styles, filter chips, a "
        "card, slider, switch, a tertiary FAB and icon buttons. Change the seed and "
        "they recolour together — the palette is not just a pretty strip."));
    {
        const struct
        {
            const char *id;
            md::ButtonVariant variant;
            const char *zh;
            const char *en;
        } buttons[] = {
            {"btn-elevated", md::ButtonVariant::Elevated, "抬升", "Elevated"},
            {"btn-filled", md::ButtonVariant::Filled, "填充", "Filled"},
            {"btn-tonal", md::ButtonVariant::Tonal, "调性", "Tonal"},
            {"btn-outlined", md::ButtonVariant::Outlined, "描边", "Outlined"},
            {"btn-text", md::ButtonVariant::Text, "文字", "Text"},
        };
        const QRectF band = context.band(48.0);
        qreal x = band.left();
        for (const auto &spec : buttons) {
            md::MdButton *button = bank<md::MdButton>(QString::fromUtf8(spec.id), [&spec, this] {
                auto *created = new md::MdButton(
                    L(spec.zh, spec.en), this);
                created->setVariant(spec.variant);
                created->setButtonSize(md::ButtonSize::Small);
                return created;
            });
            const qreal w = qMax<qreal>(button->sizeHint().width(), 88.0);
            m_slots.append(Slot{button, QRectF(x, band.top() + 2.0, w, 40.0)});
            x += w + 10.0;
        }
    }
    {
        const QRectF band = context.band(48.0);
        md::MdChip *filter = bank<md::MdChip>(QStringLiteral("chip-filter"), [this] {
            auto *created = new md::MdChip(L("筛选", "Filter"), this);
            created->setVariant(md::MdChipVariant::Filter);
            created->setChecked(true);
            return created;
        });
        md::MdChip *assist = bank<md::MdChip>(QStringLiteral("chip-assist"), [this] {
            auto *created = new md::MdChip(L("辅助", "Assist"), this);
            created->setVariant(md::MdChipVariant::Assist);
            return created;
        });
        md::MdChip *suggestion = bank<md::MdChip>(QStringLiteral("chip-suggestion"), [this] {
            auto *created = new md::MdChip(L("建议", "Suggestion"), this);
            created->setVariant(md::MdChipVariant::Suggestion);
            return created;
        });
        md::MdSwitch *toggle = bank<md::MdSwitch>(QStringLiteral("switch"), [this] {
            auto *created = new md::MdSwitch(L("开关", "Switch"), this);
            created->setChecked(true);
            return created;
        });
        md::MdSlider *slider = bank<md::MdSlider>(QStringLiteral("slider"), [this] {
            auto *created = new md::MdSlider(this);
            created->setValue(0.55);
            return created;
        });
        qreal x = band.left();
        m_slots.append(Slot{filter, QRectF(x, band.top() + 8.0, qMax<qreal>(filter->sizeHint().width(), 72.0), 32.0)});
        x += qMax<qreal>(filter->sizeHint().width(), 72.0) + 8.0;
        m_slots.append(Slot{assist, QRectF(x, band.top() + 8.0, qMax<qreal>(assist->sizeHint().width(), 72.0), 32.0)});
        x += qMax<qreal>(assist->sizeHint().width(), 72.0) + 8.0;
        m_slots.append(Slot{suggestion, QRectF(x, band.top() + 8.0, qMax<qreal>(suggestion->sizeHint().width(), 88.0), 32.0)});
        x += qMax<qreal>(suggestion->sizeHint().width(), 88.0) + 16.0;
        m_slots.append(Slot{toggle, QRectF(x, band.top() + 4.0, qMax<qreal>(toggle->sizeHint().width(), 100.0), 40.0)});
        x += qMax<qreal>(toggle->sizeHint().width(), 100.0) + 16.0;
        m_slots.append(Slot{slider, QRectF(x, band.top() + 10.0, qMin<qreal>(220.0, band.right() - x), 28.0)});
    }
    {
        const QRectF band = context.band(100.0);
        md::MdCard *card = bank<md::MdCard>(QStringLiteral("card"), [this] {
            return new md::MdCard(md::MdCardVariant::Filled, this);
        });
        md::MdFab *fab = bank<md::MdFab>(QStringLiteral("fab"), [this] {
            auto *created = new md::MdFab(QStringLiteral("add"), this);
            created->setVariant(md::FabVariant::Tertiary);
            return created;
        });
        md::MdIconButton *filled = bank<md::MdIconButton>(QStringLiteral("ib-filled"), [this] {
            auto *created = new md::MdIconButton(QStringLiteral("favorite"), this);
            created->setVariant(md::IconButtonVariant::Filled);
            return created;
        });
        md::MdIconButton *tonal = bank<md::MdIconButton>(QStringLiteral("ib-tonal"), [this] {
            auto *created = new md::MdIconButton(QStringLiteral("star"), this);
            created->setVariant(md::IconButtonVariant::Tonal);
            return created;
        });
        md::MdIconButton *outlined = bank<md::MdIconButton>(QStringLiteral("ib-outlined"), [this] {
            auto *created = new md::MdIconButton(QStringLiteral("share"), this);
            created->setVariant(md::IconButtonVariant::Outlined);
            return created;
        });
        m_probe = bank<ColourProbe>(QStringLiteral("probe"), [this] {
            return new ColourProbe(this);
        });

        const qreal cardW = qMin<qreal>(280.0, context.width() * 0.35);
        m_slots.append(Slot{card, QRectF(band.left(), band.top(), cardW, 88.0)});
        const qreal x0 = band.left() + cardW + 20.0;
        m_slots.append(Slot{fab, QRectF(x0, band.top() + 16.0, 56.0, 56.0)});
        m_slots.append(Slot{filled, QRectF(x0 + 72.0, band.top() + 20.0, 48.0, 48.0)});
        m_slots.append(Slot{tonal, QRectF(x0 + 132.0, band.top() + 20.0, 48.0, 48.0)});
        m_slots.append(Slot{outlined, QRectF(x0 + 192.0, band.top() + 20.0, 48.0, 48.0)});
        m_slots.append(
            Slot{m_probe, QRectF(x0 + 256.0, band.top() + 26.0, 120.0, 36.0)});
    }

    context.space(10.0);
    context.detail(L("种子  %1 · 变体  %2 · 对比  %3 · 模式  %4",
                     "seed  %1 · variant  %2 · contrast  %3 · mode  %4")
                       .arg(theme.seedColor().name(QColor::HexRgb),
                            md::schemeVariantName(theme.schemeVariant()),
                            md::contrastLevelName(theme.contrastLevel()),
                            theme.themeMode() == md::ThemeMode::Dark ? QStringLiteral("dark")
                                                                    : QStringLiteral("light")));
    context.detail(L("探针控件 objectName = dynamic-colour-probe，画 primary / secondary / "
                     "tertiary 三段，GalleryClick 用它验证换种子。",
                     "The probe control (objectName = dynamic-colour-probe) paints primary / "
                     "secondary / tertiary strips; GalleryClick uses it to prove a seed click "
                     "recolours live widgets."));

    syncSelection();
}

void DynamicColourPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const Slot &slot : m_slots) {
        if (slot.widget == nullptr) {
            continue;
        }
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.widget->geometry() != target) {
            slot.widget->setGeometry(target);
        }
    }
}

void DynamicColourPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void DynamicColourPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void DynamicColourPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
    syncSelection();
}

void DynamicColourPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
    syncSelection();

    // Role strip cells are painted in content coordinates (no banked widget).
    QPainter painter(this);
    beginOverlay(painter);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    const md::MdTheme &theme = md::MdTheme::instance();

    // Re-walk the same bands as build() so the painted cells line up with the
    // reserved slots that carry a null widget.
    const md::ColorRole roles[] = {
        md::ColorRole::Primary,      md::ColorRole::OnPrimary,
        md::ColorRole::PrimaryContainer, md::ColorRole::Secondary,
        md::ColorRole::SecondaryContainer, md::ColorRole::Tertiary,
        md::ColorRole::TertiaryContainer, md::ColorRole::Error,
        md::ColorRole::ErrorContainer,   md::ColorRole::SurfaceContainerLowest,
        md::ColorRole::SurfaceContainerLow, md::ColorRole::SurfaceContainer,
        md::ColorRole::SurfaceContainerHigh, md::ColorRole::SurfaceContainerHighest,
        md::ColorRole::Outline,     md::ColorRole::OutlineVariant,
    };
    constexpr int kRoleCount = int(sizeof(roles) / sizeof(roles[0]));

    // Find the role-strip slots (null widget) by walking m_slots and pairing
    // them with roles in order.
    int roleIndex = 0;
    for (const Slot &slot : m_slots) {
        if (slot.widget != nullptr || roleIndex >= kRoleCount) {
            continue;
        }
        const md::ColorRole roleValue = roles[roleIndex++];
        const QColor colour = theme.color(roleValue);
        const QColor foreground = QColor::fromRgba(QRgb(md::MdColorMath::readableForegroundFor(
            md::MdColorMath::argbFromHex(colour.name(QColor::HexRgb)))));
        GalleryPage::fillRounded(painter, slot.rect, colour, md::ShapeCorner::Small);
        drawTextLine(painter, slot.rect, GalleryPage::fontFor(md::TypeStyle::LabelSmall),
                     foreground, md::colorRoleName(roleValue), Qt::AlignCenter);
    }
    painter.restore();
    // Pair with beginOverlay()'s save(); QPainter::end() warns if a stack
    // frame is left on the painter.
    painter.restore();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createDynamicColourPage()
{
    return new DynamicColourPage;
}

} // namespace gallery

#include "GalleryPages47.moc"
