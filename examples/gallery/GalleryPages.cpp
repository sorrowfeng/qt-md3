// Gallery pages, part 1: overview, colour, type, surface.

#include "GalleryPages.h"

#include "core/MdColorMath.h"
#include "core/MdCore.h"
#include "core/MdFont.h"

#include "I18n.h"

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
    return L("概览", "Overview");
}

QString OverviewPage::slug() const
{
    return QStringLiteral("Overview");
}

QString OverviewPage::subtitle() const
{
    return L("qt-md3 是什么、已完成哪些内容，以及本次构建的运行环境。",
             "What qt-md3 is, what is finished, and what this build is running on.");
}

void OverviewPage::build(GalleryContext &context)
{
    QPainter *painter = context.painter();

    context.section(L("这是什么", "What this is"));
    context.paragraph(L(
        "qt-md3 是 Material Design 3 的 Qt Widgets 移植，完全用 QPainter 绘制。"
        "库中没有任何 QSS：每个颜色、圆角、字号、时长和阴影都来自设计令牌，每个令牌值都"
        "转录自其权威来源，而非凭记忆手打。material-web 未实现的 M3 Expressive 行为则"
        "取自 Jetpack Compose Material3。",
        "qt-md3 is a Qt Widgets port of Material Design 3, drawn entirely with QPainter. "
        "There is no QSS anywhere: every colour, corner radius, type size, duration and "
        "shadow comes from a design token, and every token value is transcribed from its "
        "authoritative source rather than typed from memory. M3 Expressive behaviour that "
        "material-web does not implement is taken from Jetpack Compose Material3."));

    context.section(L("本次构建", "This build"));
    const QString qtVersion = QString::fromUtf8(qVersion());
    context.detail(QStringLiteral("qt-md3            %1")
                       .arg(QString::fromUtf8(md::libraryVersion())));
    context.detail(QStringLiteral("Qt                %1").arg(qtVersion));
    context.detail(L("内核              %1 %2", "kernel            %1 %2")
                       .arg(QSysInfo::kernelType(), QSysInfo::kernelVersion()));
    context.detail(L("处理器              %1", "cpu                %1")
                       .arg(QSysInfo::currentCpuArchitecture()));
    context.detail(L("构建时间           %1", "build             %1")
                       .arg(QStringLiteral(__DATE__ " " __TIME__)));
    context.space(8.0);

    context.section(L("图标后端", "Icon back ends"));
    context.paragraph(L(
        "MD3 图标来自 Material Symbols，一款四轴可变字体。完整的 4299 项码点表随库一起"
        "发布；字体文件本身是可选的，缺失时图标系统会回退到内置的经典 Material Icons "
        "SVG 基线，而不是什么都不画。",
        "MD3 icons are Material Symbols, a four-axis variable font. The complete 4299-entry "
        "codepoint table ships with the library; the font file itself is optional, and when "
        "it is missing the icon system falls back to the bundled classic Material Icons "
        "SVG baseline instead of drawing nothing."));
    const md::MdIconSet resolved = md::MdIcon::resolveSet(md::MdIconSet::Auto);
    context.detail(L("解析的后端             %1", "resolved back end      %1")
                       .arg(md::iconSetName(resolved)));
    context.detail(L("Symbols 字体存在        %1", "symbols font present   %1")
                       .arg(md::MdIcon::isFontAvailable(md::MdIconFamily::Outlined)
                                ? QStringLiteral("yes")
                                : QStringLiteral("no")));
    context.detail(L("可变轴                 %1", "variable axes          %1")
                       .arg(md::MdIcon::axesSupported()
                                ? L("支持（Qt 6.7+）", "supported (Qt 6.7+)")
                                : L("当前 Qt 不支持", "not supported on this Qt")));
    context.detail(L("经典 SVG 基线          %1 个", "classic svg baseline   %1 icons")
                       .arg(md::MdIcon::classicNames().size()));
    context.detail(L("Symbols 码点            %1 个名称", "symbols codepoints     %1 names")
                       .arg(md::MdIcon::count()));
    context.space(8.0);

    context.section(L("阶段一 · 批次 0 —— 基础模块", "Stage 1 · Batch 0 — foundation modules"));
    context.paragraph(L(
        "批次 0 刻意不产出任何组件：它是日后所有组件都要对照度量的那一层。以下模块均已"
        "就位，并已纳入令牌审计的覆盖范围。",
        "Batch 0 produces no components on purpose: it is the layer every component will be "
        "measured against. All of the following are in place and covered by the token audit."));
    struct Entry
    {
        const char *name;
        const char *zh;
        const char *role;
    };
    static const Entry kEntries[] = {
        {"MdTokens", "md.ref / md.sys / md.comp 三层令牌，支持全局与逐实例覆盖",
         "md.ref / md.sys / md.comp, three layers, global + per-instance override"},
        {"MdColorScheme", "49 个色彩角色，浅色与深色，含固定色族",
         "49 colour roles, light and dark, including the fixed families"},
        {"MdColorMath", "CAM16 / HCT、TonalPalette、CorePalette —— 移植自 material-color-utilities",
         "CAM16 / HCT, TonalPalette, CorePalette — ported from material-color-utilities"},
        {"MdDynamicColor", "9 种方案变体、对比度级别、harmonize 与 HCT 混色",
         "9 scheme variants, contrast levels, harmonize and HCT blending"},
        {"MdTheme", "带 about-to-change / changed / mode-changed 生命周期的单例",
         "singleton with the about-to-change / changed / mode-changed lifecycle"},
        {"MdTypeScale", "15 个基准 + 15 个强调样式，行高随文种自适应",
         "15 baseline + 15 emphasized styles, script-aware line height"},
        {"MdShape", "完整圆角刻度、半径插值与路径形变",
         "the full corner scale, radius interpolation and path morphing"},
        {"MdMotion", "缓动与时长令牌，外加六个 Expressive 弹簧",
         "easing and duration tokens plus the six Expressive springs"},
        {"MdStateLayer", "悬停 / 聚焦 / 按压 / 拖动的透明度",
         "hover / focus / pressed / dragged opacities"},
        {"MdRipple", "按压涟漪的几何与时序，按当前形状裁剪",
         "press ripple geometry and timing, clipped to the current shape"},
        {"MdFocusRing", "3dp 指示环，带 2dp 间距与生长-回落动画",
         "the 3dp indicator with its 2dp gap and grow-and-settle animation"},
        {"MdElevation", "0..5 级色调表面，阴影仅按需启用",
         "tonal surfaces for levels 0..5, with shadows opt-in only"},
        {"MdIcon", "Material Symbols 四轴外加经典 SVG 基线",
         "Material Symbols axes plus the classic SVG baseline"},
        {"MdStyleBase", "所有 Md*Style 所派生的 QProxyStyle 基类",
         "the QProxyStyle base every Md*Style derives from"},
        {"MdFont", "内置字体的注册与全局应用",
         "bundled font registration and global application"},
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
                         L(entry.zh, entry.role));
            drawTextLine(*painter,
                         QRectF(row.right() - 80.0, row.top(), 80.0, row.height()),
                         fontFor(md::TypeStyle::LabelMedium),
                         role(md::ColorRole::Primary), QStringLiteral("done"),
                         Qt::AlignRight | Qt::AlignVCenter);
        }
    }
    context.space(12.0);

    context.section(L("门禁测试", "Gate tests"));
    context.paragraph(L(
        "正确性建立在每次构建都会运行的两道自动门禁之上。令牌审计将每个对外发布的数值与"
        "其来源逐一比对，包括 material-color-utilities 的 CAM16 参考值；策略门禁拒绝 "
        "QSS，也拒绝在阶段一完成之前出现阶段二的头文件。",
        "Correctness rests on two automated gates that run on every build. The token audit "
        "compares every published number against its source, including the CAM16 reference "
        "values from material-color-utilities; the policy gate refuses QSS and refuses to let "
        "a Stage 2 header appear before Stage 1 is complete."));
    context.chip(L("TestMd3Tokens · 22 项断言", "TestMd3Tokens · 22 assertions"),
                 role(md::ColorRole::SecondaryContainer),
                 role(md::ColorRole::OnSecondaryContainer));
    context.chip(L("TestMd3Primitives · 涟漪、焦点环、图标",
                   "TestMd3Primitives · ripple, focus ring, icons"),
                 role(md::ColorRole::TertiaryContainer),
                 role(md::ColorRole::OnTertiaryContainer));
    context.chip(QStringLiteral("TestMd3NoQss"), role(md::ColorRole::SurfaceContainerHighest),
                 role(md::ColorRole::OnSurfaceVariant));
    context.chip(QStringLiteral("TestMd3CoveragePolicy"), role(md::ColorRole::SurfaceContainerHighest),
                 role(md::ColorRole::OnSurfaceVariant));
    context.space(12.0);

    context.section(L("关于本二进制", "About this binary"));
    context.paragraph(L(
        "本画廊构建在项目的 build/ 目录中，供本地查看。构建产物不会提交——仓库只保存"
        "源码、资源与文档。",
        "This gallery is built into the project's build/ directory for local inspection. "
        "Build output is not committed — only sources, resources and docs are."));
}

// ---------------------------------------------------------------------------
// ColourPage
// ---------------------------------------------------------------------------

QString ColourPage::title() const
{
    return L("颜色", "Colour");
}

QString ColourPage::slug() const
{
    return QStringLiteral("Colour");
}

QString ColourPage::subtitle() const
{
    return L("浅色与深色两种模式下的全部 md.sys.color 角色，从当前主题实时解析。",
             "Every md.sys.color role in both modes, resolved live from the current theme.");
}

void ColourPage::build(GalleryContext &context)
{
    QPainter *painter = context.painter();
    const md::MdTheme &theme = md::MdTheme::instance();
    const md::MdColorScheme light = schemeForMode(md::ThemeMode::Light);
    const md::MdColorScheme dark = schemeForMode(md::ThemeMode::Dark);

    context.section(L("方案", "Scheme"));
    context.paragraph(L(
        "浅色与深色不是两套调色板。它们是同一组生成的色调调色板在不同色调值上的读取，"
        "因此更改种子色会同时重新生成两种模式下的每一个角色。下方两列并排展示每个角色"
        "在浅色与深色下的取值，无论当前处于哪种模式。",
        "Light and dark are not two palettes. They are the same generated tonal palettes read "
        "at different tones, so changing the seed colour regenerates every role in both modes "
        "at once. The columns below show the light and dark value of every role side by side, "
        "whichever mode is currently active."));
    context.detail(L("来源              %1", "source            %1")
                       .arg(theme.isDynamicColor() ? L("种子动态生成", "dynamic from seed")
                                                   : L("静态 MD3 基线", "static MD3 baseline")));
    context.detail(L("种子              %1", "seed              %1")
                       .arg(theme.seedColor().name(QColor::HexRgb)));
    context.detail(L("变体              %1", "variant           %1")
                       .arg(md::schemeVariantName(theme.schemeVariant())));
    context.detail(L("对比度            %1", "contrast          %1")
                       .arg(md::contrastLevelName(theme.contrastLevel())));
    context.detail(L("角色数            %1", "roles             %1").arg(int(md::ColorRole::Count)));
    context.space(14.0);

    context.section(L("色彩角色", "Roles"));

    // Column header.
    const QRectF header = context.band(26.0);
    const qreal swatchWidth = qMax<qreal>((header.width() - kNameColumn) / 2.0 - 8.0, 120.0);
    if (painter != nullptr) {
        drawTextLine(*painter, header, fontFor(md::TypeStyle::LabelMedium),
                     role(md::ColorRole::OnSurfaceVariant), L("角色", "role"));
        drawTextLine(*painter,
                     QRectF(header.left() + kNameColumn, header.top(), swatchWidth, header.height()),
                     fontFor(md::TypeStyle::LabelMedium), role(md::ColorRole::OnSurfaceVariant),
                     L("浅色", "light"));
        drawTextLine(*painter,
                     QRectF(header.left() + kNameColumn + swatchWidth + 16.0, header.top(),
                            swatchWidth, header.height()),
                     fontFor(md::TypeStyle::LabelMedium), role(md::ColorRole::OnSurfaceVariant),
                     L("深色", "dark"));
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

    context.section(L("说明", "Notes"));
    context.paragraph(L(
        "固定角色族（primary-fixed、secondary-fixed、tertiary-fixed 及其各自的 on- 与 "
        "dim- 伙伴）在浅色与深色下刻意取值相同。它们服务于必须在模式切换后保持颜色身份"
        "的内容。",
        "The fixed role families (primary-fixed, secondary-fixed, tertiary-fixed and their "
        "on- and dim- companions) deliberately have the same value in light and dark. They "
        "exist for content that must keep its colour identity across a mode switch."));
}

// ---------------------------------------------------------------------------
// TypePage
// ---------------------------------------------------------------------------

QString TypePage::title() const
{
    return L("字体", "Type");
}

QString TypePage::slug() const
{
    return QStringLiteral("Type");
}

QString TypePage::subtitle() const
{
    return L("15 个基准与 15 个强调样式，行高随文种自适应。",
             "15 baseline and 15 emphasized styles, with script-aware line height.");
}

void TypePage::build(GalleryContext &context)
{
    QPainter *painter = context.painter();
    const md::MdTheme &theme = md::MdTheme::instance();

    context.section(L("文种类别", "Script category"));
    context.paragraph(L(
        "行高并不是一个常量。拉丁文种属于 small 类别；中日韩文种属于 medium 类别，由于"
        "其字形填满 em 框，大约需要多 7% 的行距。qt-md3 读取主题的语言标签并据此调整每个"
        "样式的行高，因此切换语言会让文本正确重排，而不是被裁切。",
        "Line height is not a constant. Latin script sits in the small category; Chinese, "
        "Japanese and Korean sit in medium and need roughly 7% more leading, because their "
        "glyphs fill the em box. qt-md3 reads the theme's language tag and adjusts every "
        "style's line height accordingly, so switching language reflows text correctly "
        "instead of clipping it."));
    context.detail(L("语言标签          %1", "language tag      %1").arg(theme.languageTag()));
    context.detail(L("文种类别          %1", "script category   %1")
                       .arg(md::scriptCategoryName(theme.scriptCategory())));
    for (int i = 0; i < int(md::ScriptCategory::Count); ++i) {
        const auto category = md::ScriptCategory(i);
        context.detail(L("  %1 乘数  x%2", "  %1 multiplier  x%2")
                           .arg(md::scriptCategoryName(category), 0)
                           .arg(md::MdTypeScale::lineHeightMultiplier(category), 0, 'f', 3));
    }
    context.space(14.0);

    const struct
    {
        md::TypeEmphasis emphasis;
        const char *zh;
        const char *heading;
    } groups[] = {
        {md::TypeEmphasis::Baseline, "基准样式（md.sys.typescale，material-web v0.192）",
         "Baseline styles (md.sys.typescale, material-web v0.192)"},
        {md::TypeEmphasis::Emphasized,
         "强调样式（M3 Expressive；Compose Material3 的 TypeScaleTokens.kt）",
         "Emphasized styles (M3 Expressive; Compose Material3 TypeScaleTokens.kt)"},
    };

    for (const auto &group : groups) {
        context.section(L(group.zh, group.heading));
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
                                              ? L("强调", "emphasized")
                                              : L("基准", "baseline"));
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
                              L("春眠不觉晓，处处闻啼鸟",
                                "The quick brown fox jumps over the lazy dog"));
            Q_UNUSED(metrics2);
            painter->restore();
        }
        context.space(18.0);
    }

    context.section(L("数值出处", "Where the numbers come from"));
    context.paragraph(L(
        "基准字号、字重、字距与行高转录自 material-web v0_192 的 _md-sys-typescale.scss。"
        "material-web 尚无 Expressive 支持，因此强调样式取自 Compose Material3 的 "
        "TypeScaleTokens.kt——M3 Expressive 实际正是在那里定义的。",
        "Baseline sizes, weights, tracking and line heights are transcribed from material-web "
        "v0_192's _md-sys-typescale.scss. Material-web has no Expressive support, so the "
        "emphasized set comes from Compose Material3's TypeScaleTokens.kt, which is where "
        "M3 Expressive is actually specified."));
}

} // namespace gallery
