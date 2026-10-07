// Gallery pages, part 1: overview, colour, type, surface.

#include "GalleryPages.h"

#include "core/MdColorMath.h"
#include "core/MdCore.h"
#include "core/MdFont.h"

#include <QtCore/QLibraryInfo>
#include <QtCore/QSysInfo>
#include <QtGui/QFontMetricsF>
#include <QtGui/QMouseEvent>

namespace gallery {

namespace {

/// Height of one row in the colour grid.
constexpr qreal kSwatchRow = 30.0;
/// Width reserved for a role name / token string in the grids.
constexpr qreal kNameColumn = 230.0;

/// The theme's settings applied to the *other* mode, so both columns of the
/// colour page reflect the user's current seed, variant and contrast choice.
md::MdColorScheme schemeForMode(md::ThemeMode mode)
{
    const md::MdTheme &theme = md::MdTheme::instance();
    if (!theme.isDynamicColor()) {
        return md::MdColorScheme::baseline(mode);
    }
    return md::MdColorScheme::dynamic(theme.seedColor(), mode, theme.schemeVariant(),
                                      theme.contrastLevel());
}

void drawTextLine(QPainter &painter, const QRectF &rect, const QFont &font,
                  const QColor &color, const QString &text, int flags = Qt::AlignLeft | Qt::AlignVCenter)
{
    painter.save();
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(rect, flags, text);
    painter.restore();
}

} // namespace

// ---------------------------------------------------------------------------
// OverviewPage
// ---------------------------------------------------------------------------

QString OverviewPage::title() const
{
    return QStringLiteral("Overview");
}

QString OverviewPage::subtitle() const
{
    return QStringLiteral("What qt-md3 is, what is finished, and what this build is running on.");
}

void OverviewPage::build(GalleryContext &context)
{
    QPainter *painter = context.painter();

    context.section(QStringLiteral("What this is"));
    context.paragraph(QStringLiteral(
        "qt-md3 is a Qt Widgets port of Material Design 3, drawn entirely with QPainter. "
        "There is no QSS anywhere: every colour, corner radius, type size, duration and "
        "shadow comes from a design token, and every token value is transcribed from its "
        "authoritative source rather than typed from memory. M3 Expressive behaviour that "
        "material-web does not implement is taken from Jetpack Compose Material3."));

    context.section(QStringLiteral("This build"));
    const QString qtVersion = QString::fromUtf8(qVersion());
    context.detail(QStringLiteral("qt-md3            %1")
                       .arg(QString::fromUtf8(md::libraryVersion())));
    context.detail(QStringLiteral("Qt                %1").arg(qtVersion));
    context.detail(QStringLiteral("kernel            %1 %2")
                       .arg(QSysInfo::kernelType(), QSysInfo::kernelVersion()));
    context.detail(QStringLiteral("cpu                %1").arg(QSysInfo::currentCpuArchitecture()));
    context.detail(QStringLiteral("build             %1")
                       .arg(QStringLiteral(__DATE__ " " __TIME__)));
    context.space(8.0);

    context.section(QStringLiteral("Icon back ends"));
    context.paragraph(QStringLiteral(
        "MD3 icons are Material Symbols, a four-axis variable font. The complete 4299-entry "
        "codepoint table ships with the library; the font file itself is optional, and when "
        "it is missing the icon system falls back to the bundled classic Material Icons "
        "SVG baseline instead of drawing nothing."));
    const md::MdIconSet resolved = md::MdIcon::resolveSet(md::MdIconSet::Auto);
    context.detail(QStringLiteral("resolved back end      %1").arg(md::iconSetName(resolved)));
    context.detail(QStringLiteral("symbols font present   %1")
                       .arg(md::MdIcon::isFontAvailable(md::MdIconFamily::Outlined)
                                ? QStringLiteral("yes")
                                : QStringLiteral("no")));
    context.detail(QStringLiteral("variable axes          %1")
                       .arg(md::MdIcon::axesSupported()
                                ? QStringLiteral("supported (Qt 6.7+)")
                                : QStringLiteral("not supported on this Qt")));
    context.detail(QStringLiteral("classic svg baseline   %1 icons")
                       .arg(md::MdIcon::classicNames().size()));
    context.detail(QStringLiteral("symbols codepoints     %1 names").arg(md::MdIcon::count()));
    context.space(8.0);

    context.section(QStringLiteral("Stage 1 · Batch 0 — foundation modules"));
    context.paragraph(QStringLiteral(
        "Batch 0 produces no components on purpose: it is the layer every component will be "
        "measured against. All of the following are in place and covered by the token audit."));
    struct Entry
    {
        const char *name;
        const char *role;
    };
    static const Entry kEntries[] = {
        {"MdTokens", "md.ref / md.sys / md.comp, three layers, global + per-instance override"},
        {"MdColorScheme", "49 colour roles, light and dark, including the fixed families"},
        {"MdColorMath", "CAM16 / HCT, TonalPalette, CorePalette — ported from material-color-utilities"},
        {"MdDynamicColor", "9 scheme variants, contrast levels, harmonize and HCT blending"},
        {"MdTheme", "singleton with the about-to-change / changed / mode-changed lifecycle"},
        {"MdTypeScale", "15 baseline + 15 emphasized styles, script-aware line height"},
        {"MdShape", "the full corner scale, radius interpolation and path morphing"},
        {"MdMotion", "easing and duration tokens plus the six Expressive springs"},
        {"MdStateLayer", "hover / focus / pressed / dragged opacities"},
        {"MdRipple", "press ripple geometry and timing, clipped to the current shape"},
        {"MdFocusRing", "the 3dp indicator with its 2dp gap and grow-and-settle animation"},
        {"MdElevation", "tonal surfaces for levels 0..5, with shadows opt-in only"},
        {"MdIcon", "Material Symbols axes plus the classic SVG baseline"},
        {"MdStyleBase", "the QProxyStyle base every Md*Style derives from"},
        {"MdFont", "bundled font registration and global application"},
    };
    for (const Entry &entry : kEntries) {
        const QRectF row = context.band(24.0);
        if (painter != nullptr) {
            const QFont nameFont = fontFor(md::TypeStyle::LabelLarge);
            drawTextLine(*painter, QRectF(row.left(), row.top(), kNameColumn, row.height()),
                         nameFont, role(md::ColorRole::OnSurface),
                         QString::fromUtf8(entry.name));
            const QFont detailFont = fontFor(md::TypeStyle::BodySmall);
            drawTextLine(*painter,
                         QRectF(row.left() + kNameColumn, row.top(),
                                row.width() - kNameColumn - 90.0, row.height()),
                         detailFont, role(md::ColorRole::OnSurfaceVariant),
                         QString::fromUtf8(entry.role));
            drawTextLine(*painter,
                         QRectF(row.right() - 80.0, row.top(), 80.0, row.height()),
                         fontFor(md::TypeStyle::LabelMedium),
                         role(md::ColorRole::Primary), QStringLiteral("done"),
                         Qt::AlignRight | Qt::AlignVCenter);
        }
    }
    context.space(12.0);

    context.section(QStringLiteral("Gate tests"));
    context.paragraph(QStringLiteral(
        "Correctness rests on two automated gates that run on every build. The token audit "
        "compares every published number against its source, including the CAM16 reference "
        "values from material-color-utilities; the policy gate refuses QSS and refuses to let "
        "a Stage 2 header appear before Stage 1 is complete."));
    context.chip(QStringLiteral("TestMd3Tokens · 22 assertions"),
                 role(md::ColorRole::SecondaryContainer),
                 role(md::ColorRole::OnSecondaryContainer));
    context.chip(QStringLiteral("TestMd3Primitives · ripple, focus ring, icons"),
                 role(md::ColorRole::TertiaryContainer),
                 role(md::ColorRole::OnTertiaryContainer));
    context.chip(QStringLiteral("TestMd3NoQss"), role(md::ColorRole::SurfaceContainerHighest),
                 role(md::ColorRole::OnSurfaceVariant));
    context.chip(QStringLiteral("TestMd3CoveragePolicy"), role(md::ColorRole::SurfaceContainerHighest),
                 role(md::ColorRole::OnSurfaceVariant));
    context.space(12.0);

    context.section(QStringLiteral("About this binary"));
    context.paragraph(QStringLiteral(
        "This gallery is built into the project's build/ directory for local inspection. "
        "Build output is not committed — only sources, resources and docs are."));
}

// ---------------------------------------------------------------------------
// ColourPage
// ---------------------------------------------------------------------------

QString ColourPage::title() const
{
    return QStringLiteral("Colour");
}

QString ColourPage::subtitle() const
{
    return QStringLiteral("Every md.sys.color role in both modes, resolved live from the current theme.");
}

void ColourPage::build(GalleryContext &context)
{
    QPainter *painter = context.painter();
    const md::MdTheme &theme = md::MdTheme::instance();
    const md::MdColorScheme light = schemeForMode(md::ThemeMode::Light);
    const md::MdColorScheme dark = schemeForMode(md::ThemeMode::Dark);

    context.section(QStringLiteral("Scheme"));
    context.paragraph(QStringLiteral(
        "Light and dark are not two palettes. They are the same generated tonal palettes read "
        "at different tones, so changing the seed colour regenerates every role in both modes "
        "at once. The columns below show the light and dark value of every role side by side, "
        "whichever mode is currently active."));
    context.detail(QStringLiteral("source            %1")
                       .arg(theme.isDynamicColor() ? QStringLiteral("dynamic from seed")
                                                   : QStringLiteral("static MD3 baseline")));
    context.detail(QStringLiteral("seed              %1")
                       .arg(theme.seedColor().name(QColor::HexRgb)));
    context.detail(QStringLiteral("variant           %1")
                       .arg(md::schemeVariantName(theme.schemeVariant())));
    context.detail(QStringLiteral("contrast          %1")
                       .arg(md::contrastLevelName(theme.contrastLevel())));
    context.detail(QStringLiteral("roles             %1").arg(int(md::ColorRole::Count)));
    context.space(14.0);

    context.section(QStringLiteral("Roles"));

    // Column header.
    const QRectF header = context.band(26.0);
    const qreal swatchWidth = qMax<qreal>((header.width() - kNameColumn) / 2.0 - 8.0, 120.0);
    if (painter != nullptr) {
        drawTextLine(*painter, header, fontFor(md::TypeStyle::LabelMedium),
                     role(md::ColorRole::OnSurfaceVariant), QStringLiteral("role"));
        drawTextLine(*painter,
                     QRectF(header.left() + kNameColumn, header.top(), swatchWidth, header.height()),
                     fontFor(md::TypeStyle::LabelMedium), role(md::ColorRole::OnSurfaceVariant),
                     QStringLiteral("light"));
        drawTextLine(*painter,
                     QRectF(header.left() + kNameColumn + swatchWidth + 16.0, header.top(),
                            swatchWidth, header.height()),
                     fontFor(md::TypeStyle::LabelMedium), role(md::ColorRole::OnSurfaceVariant),
                     QStringLiteral("dark"));
    }

    for (int i = 0; i < int(md::ColorRole::Count); ++i) {
        const auto roleValue = md::ColorRole(i);
        const QRectF row = context.band(kSwatchRow);
        if (painter == nullptr) {
            continue;
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        drawTextLine(*painter, QRectF(row.left(), row.top(), kNameColumn, row.height()),
                     fontFor(md::TypeStyle::LabelMedium), role(md::ColorRole::OnSurface),
                     md::colorRoleName(roleValue));

        struct Cell
        {
            QRectF rect;
            const md::MdColorScheme *scheme;
        };
        const Cell cells[2] = {
            {QRectF(row.left() + kNameColumn, row.top() + 3.0, swatchWidth, row.height() - 6.0),
             &light},
            {QRectF(row.left() + kNameColumn + swatchWidth + 16.0, row.top() + 3.0, swatchWidth,
                    row.height() - 6.0),
             &dark},
        };
        for (const Cell &cell : cells) {
            const QColor colour = cell.scheme->color(roleValue);
            const QColor foreground = QColor::fromRgba(QRgb(md::MdColorMath::readableForegroundFor(
                md::MdColorMath::argbFromHex(colour.name(QColor::HexRgb)))));
            fillRounded(*painter, cell.rect, colour, md::ShapeCorner::ExtraSmall);
            QPen border(foreground);
            border.setCosmetic(true);
            painter->setPen(border);
            painter->setBrush(Qt::NoBrush);
            painter->drawPath(
                md::MdShape::roundedRect(cell.rect, md::MdShape::resolvedRadii(
                                                        md::ShapeCorner::ExtraSmall,
                                                        cell.rect.size())));
            drawTextLine(*painter, cell.rect, fontFor(md::TypeStyle::LabelSmall),
                         foreground, cell.scheme->hex(roleValue), Qt::AlignCenter);
        }
        painter->restore();
    }
    context.space(16.0);

    context.section(QStringLiteral("Notes"));
    context.paragraph(QStringLiteral(
        "The fixed role families (primary-fixed, secondary-fixed, tertiary-fixed and their "
        "on- and dim- companions) deliberately have the same value in light and dark. They "
        "exist for content that must keep its colour identity across a mode switch."));
}

// ---------------------------------------------------------------------------
// TypePage
// ---------------------------------------------------------------------------

QString TypePage::title() const
{
    return QStringLiteral("Type");
}

QString TypePage::subtitle() const
{
    return QStringLiteral("15 baseline and 15 emphasized styles, with script-aware line height.");
}

void TypePage::build(GalleryContext &context)
{
    QPainter *painter = context.painter();
    const md::MdTheme &theme = md::MdTheme::instance();

    context.section(QStringLiteral("Script category"));
    context.paragraph(QStringLiteral(
        "Line height is not a constant. Latin script sits in the small category; Chinese, "
        "Japanese and Korean sit in medium and need roughly 7% more leading, because their "
        "glyphs fill the em box. qt-md3 reads the theme's language tag and adjusts every "
        "style's line height accordingly, so switching language reflows text correctly "
        "instead of clipping it."));
    context.detail(QStringLiteral("language tag      %1").arg(theme.languageTag()));
    context.detail(QStringLiteral("script category   %1")
                       .arg(md::scriptCategoryName(theme.scriptCategory())));
    for (int i = 0; i < int(md::ScriptCategory::Count); ++i) {
        const auto category = md::ScriptCategory(i);
        context.detail(QStringLiteral("  %1 multiplier  x%2")
                           .arg(md::scriptCategoryName(category), 0)
                           .arg(md::MdTypeScale::lineHeightMultiplier(category), 0, 'f', 3));
    }
    context.space(14.0);

    const struct
    {
        md::TypeEmphasis emphasis;
        const char *heading;
    } groups[] = {
        {md::TypeEmphasis::Baseline, "Baseline styles (md.sys.typescale, material-web v0.192)"},
        {md::TypeEmphasis::Emphasized,
         "Emphasized styles (M3 Expressive; Compose Material3 TypeScaleTokens.kt)"},
    };

    for (const auto &group : groups) {
        context.section(QString::fromUtf8(group.heading));
        for (int i = 0; i < int(md::TypeStyle::Count); ++i) {
            const auto style = md::TypeStyle(i);
            const md::MdTypeStyleSpec spec = md::MdTypeScale::spec(style, group.emphasis);
            const qreal lineHeight =
                md::MdTypeScale::lineHeight(style, group.emphasis, theme.scriptCategory());
            const QRectF row = context.band(52.0);
            if (painter == nullptr) {
                continue;
            }

            const QString label = QStringLiteral("%1 · %2").arg(
                md::typeStyleName(style), group.emphasis == md::TypeEmphasis::Emphasized
                                              ? QStringLiteral("emphasized")
                                              : QStringLiteral("baseline"));
            drawTextLine(*painter, QRectF(row.left(), row.top(), 300.0, 18.0),
                         fontFor(md::TypeStyle::LabelSmall), role(md::ColorRole::OnSurfaceVariant),
                         label);
            const QString metrics = QStringLiteral("%1px  lh %2  ls %3  w%4  %5")
                                        .arg(spec.size, 0, 'f', 1)
                                        .arg(lineHeight, 0, 'f', 1)
                                        .arg(spec.tracking, 0, 'f', 2)
                                        .arg(spec.weight)
                                        .arg(spec.familyToken);
            drawTextLine(*painter, QRectF(row.left() + 300.0, row.top(), row.width() - 300.0, 18.0),
                         fontFor(md::TypeStyle::LabelSmall),
                         role(md::ColorRole::OnSurfaceVariant), metrics);

            // Sample line, rendered in the very style being described.
            QFont sample = fontFor(style, group.emphasis);
            painter->save();
            painter->setFont(sample);
            painter->setPen(role(md::ColorRole::OnSurface));
            const QFontMetricsF metrics2(sample);
            painter->drawText(QPointF(row.left(), row.bottom() - 10.0),
                              QStringLiteral("The quick brown fox jumps over the lazy dog"));
            Q_UNUSED(metrics2);
            painter->restore();
        }
        context.space(18.0);
    }

    context.section(QStringLiteral("Where the numbers come from"));
    context.paragraph(QStringLiteral(
        "Baseline sizes, weights, tracking and line heights are transcribed from material-web "
        "v0_192's _md-sys-typescale.scss. Material-web has no Expressive support, so the "
        "emphasized set comes from Compose Material3's TypeScaleTokens.kt, which is where "
        "M3 Expressive is actually specified."));
}

} // namespace gallery
