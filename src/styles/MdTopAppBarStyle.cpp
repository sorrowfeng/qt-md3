#include "MdTopAppBarStyle.h"

#include "MdChildBox.h"
#include "core/MdColorMath.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdTopAppBar.h"

#include <QtCore/QEasingCurve>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

/// `TopTitleAlphaEasing` — Compose publishes it in code and nowhere else:
/// "An easing function used to compute the alpha value that is applied to the
/// top title part of a Medium or Large app bar."
constexpr qreal kTopTitleEasingX1 = 0.8;
constexpr qreal kTopTitleEasingY1 = 0.0;
constexpr qreal kTopTitleEasingX2 = 0.8;
constexpr qreal kTopTitleEasingY2 = 0.15;

/// `FastOutLinearInEasing`, which is what `TopAppBarColors.containerColor`
/// runs its transition fraction through before interpolating.
constexpr qreal kFastOutLinearInX1 = 0.4;
constexpr qreal kFastOutLinearInY1 = 0.0;
constexpr qreal kFastOutLinearInX2 = 1.0;
constexpr qreal kFastOutLinearInY2 = 1.0;

/// Builds the `QEasingCurve` for a cubic-bezier's two control points. Called
/// exactly once per named curve (see the two `const QEasingCurve &` helpers
/// below) rather than per sample: the evaluation runs inside `paintEvent`, so
/// allocating here per call would put a heap allocation on every frame of a
/// scrolling bar.
QEasingCurve makeBezier(qreal x1, qreal y1, qreal x2, qreal y2)
{
    QEasingCurve curve(QEasingCurve::BezierSpline);
    // A cubic-bezier has two control points and an implicit endpoint at (1, 1).
    curve.addCubicBezierSegment(QPointF(x1, y1), QPointF(x2, y2), QPointF(1.0, 1.0));
    return curve;
}

const QEasingCurve &topTitleCurve()
{
    static const QEasingCurve curve =
        makeBezier(kTopTitleEasingX1, kTopTitleEasingY1, kTopTitleEasingX2, kTopTitleEasingY2);
    return curve;
}

const QEasingCurve &fastOutLinearInCurve()
{
    static const QEasingCurve curve = makeBezier(kFastOutLinearInX1, kFastOutLinearInY1,
                                                 kFastOutLinearInX2, kFastOutLinearInY2);
    return curve;
}

qreal bezier(const QEasingCurve &curve, qreal t)
{
    const qreal progress = qBound(0.0, t, 1.0);
    // A cubic-bezier's endpoints are (0, 0) and (1, 1) by construction, so
    // short-circuit them: it keeps the identity exact at both ends (rather
    // than within a solver's tolerance) and skips the solve entirely for the
    // two fractions a settled bar spends all its time at.
    if (progress <= 0.0) {
        return 0.0;
    }
    if (progress >= 1.0) {
        return 1.0;
    }
    return curve.valueForProgress(progress);
}

qreal lineHeightFor(TypeStyle style)
{
    return MdTypeScale::lineHeight(style, TypeEmphasis::Baseline,
                                   MdTheme::instance().scriptCategory());
}

QFont fontFor(TypeStyle style)
{
    return MdTypeScale::font(style, TypeEmphasis::Baseline, MdTheme::instance().scriptCategory());
}

/// The wrapped height of `text` within `width`.
qreal wrappedHeight(const QString &text, const QFont &font, qreal width)
{
    if (text.isEmpty()) {
        return 0.0;
    }
    const QFontMetrics metrics(font);
    const QRect bounds = metrics.boundingRect(QRect(0, 0, int(qMax<qreal>(width, 1.0)), 10000),
                                              int(Qt::AlignLeft | Qt::TextWordWrap), text);
    return qMax<qreal>(bounds.height(), qreal(metrics.height()));
}

/// The *container* a slot widget paints — what Compose measures as the slot's
/// placeable, and what decides where the title has to start.
///
/// Deliberately not `sizeHint()`: every child that carries a focus ring is
/// `container + 15` wide (`MdChildBox` has the arithmetic), so laying the title
/// out against those 15 px of invisible margin would push it 15 px off the
/// position the spec gives it. The widget's own margin is allowed to overhang
/// into the title's box — it is transparent, and Compose's indicator would draw
/// over the title there anyway.
QSizeF containerSize(QWidget *widget)
{
    if (widget == nullptr) {
        return QSizeF();
    }
    return MdChildBox::measure(widget).containerSize();
}

qreal containerWidth(QWidget *widget)
{
    return qMax<qreal>(containerSize(widget).width(), 0.0);
}

/// A stack of one or two text blocks, measured against a width limit.
struct TextColumn
{
    qreal titleHeight = 0.0;
    qreal subtitleHeight = 0.0;
    qreal titleLineHeight = 0.0;
    qreal titleAscent = 0.0;
    qreal subtitleLineHeight = 0.0;
    qreal subtitleAscent = 0.0;
    qreal width = 0.0;

    qreal height() const { return titleHeight + subtitleHeight; }

    /// Distance from the column's bottom edge up to its *last* baseline.
    /// Compose reads `titlePlaceable[LastBaseline]`; on a column that is the
    /// last child's baseline, so a subtitle (when present) owns it.
    qreal lastBaselineFromBottom() const
    {
        const qreal lineH = subtitleHeight > 0.0 ? subtitleLineHeight : titleLineHeight;
        const qreal ascent = subtitleHeight > 0.0 ? subtitleAscent : titleAscent;
        return lineH - ascent;
    }
};

TextColumn measureColumn(const QFont &titleFont, const QFont &subtitleFont, qreal titleLineHeight,
                         qreal subtitleLineHeight, const QString &title, const QString &subtitle,
                         qreal widthLimit)
{
    const QFontMetrics titleMetrics(titleFont);

    TextColumn column;
    column.titleLineHeight = titleLineHeight;
    column.titleAscent = titleMetrics.ascent();
    column.titleHeight = wrappedHeight(title, titleFont, widthLimit);

    if (!subtitle.isEmpty()) {
        const QFontMetrics subtitleMetrics(subtitleFont);
        column.subtitleLineHeight = subtitleLineHeight;
        column.subtitleAscent = subtitleMetrics.ascent();
        column.subtitleHeight = wrappedHeight(subtitle, subtitleFont, widthLimit);
    }

    column.width = 0.0;
    const QFontMetrics wideTitle(titleFont);
    column.width = qMax(column.width, qreal(wideTitle.boundingRect(title).width()));
    if (!subtitle.isEmpty()) {
        const QFontMetrics wideSubtitle(subtitleFont);
        column.width = qMax(column.width, qreal(wideSubtitle.boundingRect(subtitle).width()));
    }
    return column;
}

/// One text line (or wrapped block), clipped to `clip` and faded by `alpha`.
///
/// The clip is the point: Compose clips a two-row bar's two `TopAppBarLayout`s
/// to their own rows, which is what keeps the expanded title from spilling over
/// the icon row as the bar collapses.
void drawClippedText(QPainter &painter, const QRectF &clip, const QRectF &rect, const QFont &font,
                     const QColor &colour, qreal alpha, const QString &text)
{
    if (rect.isEmpty() || text.isEmpty() || alpha <= 0.0) {
        return;
    }
    painter.save();
    painter.setClipRect(clip);
    painter.setOpacity(qBound(0.0, alpha, 1.0));
    painter.setPen(colour);
    painter.setFont(font);
    painter.drawText(rect, int(Qt::AlignLeft | Qt::AlignTop), text);
    painter.restore();
}

} // namespace

MdTopAppBarStyle::MdTopAppBarStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdTopAppBarStyle *MdTopAppBarStyle::shared()
{
    static QMutex mutex;
    static MdTopAppBarStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdTopAppBarStyle;
        installPaintFilter<MdTopAppBar>(instance);
    }
    return instance;
}

bool MdTopAppBarStyle::isInstalled()
{
    return hasPaintFilter(&MdTopAppBar::staticMetaObject);
}

qreal MdTopAppBarStyle::fastOutLinearIn(qreal t)
{
    return bezier(fastOutLinearInCurve(), t);
}

qreal MdTopAppBarStyle::colorTransitionFraction(const MdAppBarTokens &tokens,
                                                qreal collapsedFraction, qreal overlappedFraction)
{
    if (!tokens.isTwoRows()) {
        // `SingleRowTopAppBar` asks for the scrolled container colour as soon
        // as `overlappedFraction > 0.01f`; the ramp is the *animated* one, and
        // it lives in the widget, not here.
        return overlappedFraction > MdAppBarTokens::scrolledColourThreshold ? 1.0 : 0.0;
    }
    // A two-row bar's colour "changes at the same rate the app bar expands or
    // collapses", so it reads the collapsed fraction directly.
    return qBound(0.0, collapsedFraction, 1.0);
}

QColor MdTopAppBarStyle::containerColorFor(const MdAppBarTokens &tokens, qreal fraction)
{
    const QColor base = MdTheme::instance().color(tokens.containerColor);
    const QColor scrolled = MdTheme::instance().color(tokens.onScrollContainerColor);
    if (fraction <= 0.0) {
        return base;
    }
    if (fraction >= 1.0) {
        return scrolled;
    }
    // `TopAppBarColors.containerColor` lerps through `FastOutLinearInEasing`,
    // and `lerp(Color, Color, Float)` interpolates in Oklab.
    const qreal eased = fastOutLinearIn(fraction);
    return QColor::fromRgba(MdColorMath::lerpOklab(base.rgba(), scrolled.rgba(), eased));
}

MdTopAppBarStyle::Layout MdTopAppBarStyle::layoutFor(const MdTopAppBar &bar,
                                                     const MdAppBarTokens &tokens,
                                                     qreal collapsedFraction)
{
    Layout layout;
    const QRectF rect = QRectF(bar.rect());
    layout.container = rect;
    layout.radii = MdShape::resolvedRadii(tokens.containerShape, rect.size());

    const qreal pad = tokens.leadingSpace;
    const qreal width = rect.width();
    const bool hasSubtitle = !bar.subtitle().isEmpty();

    // Every slot in Compose's `TopAppBarLayout` is wrapped in a Box whose box
    // carries the 4 px on its own edge, and those Boxes exist even when their
    // content is empty. Reproducing that is what makes the second row's title
    // start at `max(12, 4)` rather than at 0.
    const qreal navigationBox = pad + containerWidth(bar.navigationWidget());

    qreal actionsContent = 0.0;
    const QList<QWidget *> actions = bar.actionWidgets();
    for (int i = 0; i < actions.size(); ++i) {
        if (i > 0) {
            actionsContent += tokens.iconButtonSpace;
        }
        actionsContent += containerWidth(actions.at(i));
    }
    const qreal actionsBox = actionsContent + pad;

    const qreal start = qMax(tokens.titleInset(), navigationBox);
    const qreal end = actionsBox;
    const qreal maxTitleWidth = qMax<qreal>(width - start - end, 0.0);
    // The title Box carries its own 4 px on both sides.
    const qreal titleTextWidth = qMax<qreal>(maxTitleWidth - 2.0 * pad, 0.0);

    layout.twoRows = tokens.isTwoRows();
    if (layout.twoRows) {
        const qreal leadingHeight = qMin(tokens.collapsedRowHeight, rect.height());
        layout.leadingRow = QRectF(rect.left(), rect.top(), width, leadingHeight);
        layout.textRow = QRectF(rect.left(), rect.top() + leadingHeight, width,
                                qMax<qreal>(rect.height() - leadingHeight, 0.0));
    } else {
        layout.leadingRow = rect;
        layout.textRow = QRectF();
    }

    // --- the leading and trailing slots ------------------------------------
    // Both report the *container* box, vertically centred, so a caller can drop
    // a widget onto it with `MdChildBox::geometryOn` and get the 4 px the token
    // publishes instead of 4 plus a focus margin that is never drawn. Compose
    // gets the same result from `Box(fillMaxHeight)` around a centred icon
    // button on the leading side and `Row(CenterVertically)` on the trailing
    // one.
    const auto centredInRow = [&](const QRectF &slot, const QSizeF &size) {
        return QRectF(slot.left(), slot.top() + (slot.height() - size.height()) / 2.0, size.width(),
                      size.height());
    };

    const QSizeF navigation = containerSize(bar.navigationWidget());
    if (!navigation.isEmpty()) {
        layout.navigation =
            centredInRow(QRectF(layout.leadingRow.left() + pad, layout.leadingRow.top(),
                                navigation.width(), layout.leadingRow.height()),
                         navigation);
    }

    layout.actions = QRectF(layout.leadingRow.right() - pad - actionsContent, layout.leadingRow.top(),
                            actionsContent, layout.leadingRow.height());
    layout.actionBoxes.reserve(actions.size());
    qreal cursor = layout.actions.left();
    for (int i = 0; i < actions.size(); ++i) {
        if (i > 0) {
            cursor += tokens.iconButtonSpace;
        }
        const QSizeF size = containerSize(actions.at(i));
        layout.actionBoxes.append(
            centredInRow(QRectF(cursor, layout.leadingRow.top(), size.width(),
                                layout.leadingRow.height()),
                         size));
        cursor += size.width();
    }

    if (QWidget *center = bar.centerWidget()) {
        const qreal centerLeft = start + pad;
        const qreal centerWidth = qMax<qreal>(width - end - pad - centerLeft, 0.0);
        // The centre slot is the one that is *not* a container box: Compose
        // gives the search field `fillMaxWidth()` between the two slots, so the
        // widget takes the whole gap and its own layout centres whatever it
        // paints inside that. Its height is the widget's, not the container's —
        // handing a padded widget a container-sized box would clip it.
        const qreal centerHeight =
            qMin(layout.leadingRow.height(),
                 qMax<qreal>(qreal(MdChildBox::measure(center).widgetSize().height()), 0.0));
        layout.center = QRectF(centerLeft,
                               layout.leadingRow.top() + (layout.leadingRow.height() - centerHeight) / 2.0,
                               centerWidth, centerHeight);
    }

    // --- the alphas ---------------------------------------------------------
    // The two titles cross-fade: the expanded one on `1 - collapsedFraction`,
    // the small one on `TopTitleAlphaEasing`.
    layout.titleAlpha = layout.twoRows ? 1.0 - qBound(0.0, collapsedFraction, 1.0) : 1.0;
    layout.leadingTitleAlpha =
        layout.twoRows ? bezier(topTitleCurve(), collapsedFraction) : 0.0;

    // A centre widget takes the title's place, which is what makes a search
    // app bar a *configuration* of the bar rather than a sixth layout.
    if (bar.centerWidget() != nullptr) {
        layout.hasTitle = false;
        return layout;
    }

    const QFont expandedTitleFont = fontFor(tokens.titleTypeStyle);
    const QFont expandedSubtitleFont = fontFor(tokens.subtitleTypeStyle);

    /// Compose aligns the title within the *whole* bar first and only then
    /// pushes it past its neighbours — so a centred title stays centred on the
    /// bar, not in the gap between the leading button and the actions.
    const auto alignTitle = [&](qreal boxWidth) {
        qreal x = 0.0;
        if (bar.alignment() == MdAppBarAlignment::Center) {
            x = (width - boxWidth) / 2.0;
        }
        if (x < start) {
            x = start;
        } else if (x + boxWidth > width - end) {
            x = (width - end) - boxWidth;
        }
        return x;
    };

    if (layout.twoRows) {
        // Row 1 carries the *small* title, centred in the collapsed row; row 2
        // carries the expanded title and subtitle, bottom-aligned from their
        // last baseline.
        const TextColumn leading =
            measureColumn(expandedTitleFont, expandedSubtitleFont,
                          lineHeightFor(tokens.collapsedTitleTypeStyle),
                          lineHeightFor(tokens.collapsedSubtitleTypeStyle), bar.title(),
                          bar.subtitle(), titleTextWidth);

        if (leading.width > 0.0) {
            const qreal boxWidth = leading.width + 2.0 * pad;
            const qreal x = alignTitle(boxWidth);
            const qreal top =
                layout.leadingRow.top() + (layout.leadingRow.height() - leading.height()) / 2.0;
            layout.leadingTitle = QRectF(x + pad, top, leading.width, leading.titleHeight);
            if (leading.subtitleHeight > 0.0) {
                layout.leadingSubtitle = QRectF(x + pad, top + leading.titleHeight, leading.width,
                                                leading.subtitleHeight);
            }
        }

        const TextColumn expanded =
            measureColumn(expandedTitleFont, expandedSubtitleFont,
                          lineHeightFor(tokens.titleTypeStyle),
                          lineHeightFor(tokens.subtitleTypeStyle), bar.title(), bar.subtitle(),
                          titleTextWidth);

        if (expanded.width > 0.0) {
            const qreal boxWidth = expanded.width + 2.0 * pad;
            const qreal x = alignTitle(boxWidth);

            // `Arrangement.Bottom` with a baseline padding: Compose measures
            // the padding from the column's *last* baseline, and shrinks it if
            // the text would not otherwise fit in the row.
            const qreal rowHeight = layout.textRow.height();
            const qreal columnHeight = expanded.height();
            const qreal maxLayoutHeight = qMax(rowHeight, columnHeight);
            qreal paddingFromBottom = tokens.titleBottomPadding() - expanded.lastBaselineFromBottom();
            const qreal heightWithPadding = paddingFromBottom + columnHeight;
            if (heightWithPadding > maxLayoutHeight) {
                paddingFromBottom -= (heightWithPadding - maxLayoutHeight);
            }
            const qreal top = layout.textRow.bottom() - columnHeight
                - qMax<qreal>(0.0, paddingFromBottom);

            layout.title = QRectF(x + pad, top, expanded.width, expanded.titleHeight);
            if (expanded.subtitleHeight > 0.0) {
                layout.subtitle = QRectF(x + pad, top + expanded.titleHeight, expanded.width,
                                         expanded.subtitleHeight);
            }
        }
        return layout;
    }

    // --- a single-row bar centres one column -------------------------------
    const TextColumn column =
        measureColumn(expandedTitleFont, expandedSubtitleFont, lineHeightFor(tokens.titleTypeStyle),
                      lineHeightFor(tokens.subtitleTypeStyle), bar.title(), bar.subtitle(),
                      titleTextWidth);

    if (column.width <= 0.0) {
        layout.hasTitle = false;
        return layout;
    }

    const qreal boxWidth = column.width + 2.0 * pad;
    const qreal x = alignTitle(boxWidth);
    const qreal top = rect.top() + (rect.height() - column.height()) / 2.0;
    layout.title = QRectF(x + pad, top, column.width, column.titleHeight);
    if (column.subtitleHeight > 0.0) {
        layout.subtitle = QRectF(x + pad, top + column.titleHeight, column.width,
                                 column.subtitleHeight);
    }
    return layout;
}

void MdTopAppBarStyle::paintTopAppBar(QPainter &painter, const MdTopAppBar &bar,
                                      const MdAppBarTokens &tokens, const Layout &layout,
                                      const QColor &containerColor)
{
    Q_UNUSED(bar);
    const MdTheme &theme = MdTheme::instance();

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. The container. `container.shape` is corner-none, so this is a plain
    //    rect in practice — but it is the token that says so.
    painter.setPen(Qt::NoPen);
    painter.setBrush(containerColor);
    painter.drawPath(MdShape::roundedRect(layout.container, layout.radii));

    // 2. The titles. A two-row bar draws the small title inside its collapsed
    //    row and the expanded one inside the text row, each clipped to that
    //    row; a single-row bar draws one column centred in the container.
    const QColor titleColour = theme.color(tokens.titleColor);
    const QColor subtitleColour = theme.color(tokens.subtitleColor);

    if (layout.twoRows) {
        // Row 1 — the small title, fading *in* as the bar collapses.
        drawClippedText(painter, layout.leadingRow, layout.leadingTitle,
                        fontFor(tokens.collapsedTitleTypeStyle), titleColour,
                        layout.leadingTitleAlpha, bar.title());
        drawClippedText(painter, layout.leadingRow, layout.leadingSubtitle,
                        fontFor(tokens.collapsedSubtitleTypeStyle), subtitleColour,
                        layout.leadingTitleAlpha, bar.subtitle());

        // Row 2 — the expanded title, fading out on `1 - collapsedFraction`.
        drawClippedText(painter, layout.textRow, layout.title, fontFor(tokens.titleTypeStyle),
                        titleColour, layout.titleAlpha, bar.title());
        drawClippedText(painter, layout.textRow, layout.subtitle,
                        fontFor(tokens.subtitleTypeStyle), subtitleColour, layout.titleAlpha,
                        bar.subtitle());
    } else {
        drawClippedText(painter, layout.container, layout.title, fontFor(tokens.titleTypeStyle),
                        titleColour, 1.0, bar.title());
        drawClippedText(painter, layout.container, layout.subtitle,
                        fontFor(tokens.subtitleTypeStyle), subtitleColour, 1.0, bar.subtitle());
    }

    painter.restore();
}

void MdTopAppBarStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *bar = qobject_cast<MdTopAppBar *>(widget);
    if (painter == nullptr || bar == nullptr) {
        return;
    }
    const MdAppBarTokens &tokens = bar->tokens();
    const Layout layout = layoutFor(*bar, tokens, bar->collapsedFraction());
    paintTopAppBar(*painter, *bar, tokens, layout, bar->containerColor());
}

} // namespace md
