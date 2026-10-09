#include "MdFloatingToolbarStyle.h"

#include "MdChildBox.h"
#include "core/MdElevation.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdFloatingToolbar.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSizeF>
#include <QtCore/QVector>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

/// The toolbar's own axis, so the horizontal and the vertical layouts share a
/// single body of arithmetic. Everything inside `layoutFor` is written in
/// `main` (the axis the toolbar grows along) and `cross`.
struct Axis
{
    bool horizontal = true;

    qreal mainOf(const QSizeF &size) const { return horizontal ? size.width() : size.height(); }
    qreal crossOf(const QSizeF &size) const { return horizontal ? size.height() : size.width(); }

    QRectF make(qreal mainPos, qreal mainExtent, qreal crossPos, qreal crossExtent) const
    {
        return horizontal ? QRectF(mainPos, crossPos, mainExtent, crossExtent)
                          : QRectF(crossPos, mainPos, crossExtent, mainExtent);
    }

    qreal mainStart(const QRectF &r) const { return horizontal ? r.left() : r.top(); }
    qreal mainEnd(const QRectF &r) const { return horizontal ? r.right() : r.bottom(); }
    qreal crossStart(const QRectF &r) const { return horizontal ? r.top() : r.left(); }
    qreal crossExtent(const QRectF &r) const { return horizontal ? r.height() : r.width(); }
};

/// Measure a slot's children once, keeping only the extent the layout advances
/// by — the *container* size, never the widget size (see `MdChildBox`).
qreal measureSlot(const QList<QWidget *> &children, QVector<QSizeF> *sizes, const Axis &axis)
{
    sizes->reserve(sizes->size() + children.size());
    qreal total = 0.0;
    for (QWidget *child : children) {
        const QSizeF size = MdChildBox::measure(child).containerSize();
        sizes->append(size);
        total += axis.mainOf(size);
    }
    return total;
}

} // namespace

MdFloatingToolbarStyle::MdFloatingToolbarStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdFloatingToolbarStyle *MdFloatingToolbarStyle::shared()
{
    static QMutex mutex;
    static MdFloatingToolbarStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdFloatingToolbarStyle;
        installPaintFilter<MdFloatingToolbar>(instance);
    }
    return instance;
}

bool MdFloatingToolbarStyle::isInstalled()
{
    return hasPaintFilter(&MdFloatingToolbar::staticMetaObject);
}

MdFloatingToolbarStyle::Layout MdFloatingToolbarStyle::layoutFor(
    const MdFloatingToolbar &toolbar, const MdFloatingToolbarTokens &tokens)
{
    Layout layout;

    const Axis axis{toolbar.orientation() != MdToolbarOrientation::Vertical};
    const qreal progress = qBound<qreal>(0.0, toolbar.expansionProgress(), 1.0);
    const QRectF outer(toolbar.rect());

    // --- the slots ----------------------------------------------------------
    //
    // Leading and trailing exist only while the toolbar is expanded, which is
    // Compose's `AnimatedVisibility(visible = expandedState)`. They are
    // *measured* either way, though: the pill's extent and the band it centres
    // in come from the toolbar's **maximum** intrinsic size, so a collapsed
    // toolbar keeps the room its slots had. Only the group that is actually
    // visible is centred — which is why the content re-centres when the extra
    // actions disappear.
    QVector<QSizeF> leadingSizes;
    QVector<QSizeF> contentSizes;
    QVector<QSizeF> trailingSizes;
    if (QWidget *leading = toolbar.leadingWidget()) {
        measureSlot({leading}, &leadingSizes, axis);
    }
    if (QWidget *trailing = toolbar.trailingWidget()) {
        measureSlot({trailing}, &trailingSizes, axis);
    }
    measureSlot(toolbar.contentWidgets(), &contentSizes, axis);

    const auto sumMain = [&](const QVector<QSizeF> &sizes) {
        qreal total = 0.0;
        for (const QSizeF &size : sizes) {
            total += axis.mainOf(size);
        }
        return total;
    };

    // The slots form one run — leading, then content, then trailing — and the
    // published `container.between-space` sits between each neighbouring pair.
    // Compose declares this row and never reads it (the arrangement belongs to
    // the caller's `Row` upstream); this widget owns the arrangement, so it
    // applies the row. See the header.
    const qreal gap = tokens.containerBetweenSpace;
    const auto runMain = [&](const QVector<QSizeF> &sizes) {
        return sumMain(sizes) + gap * qMax(0, int(sizes.size()) - 1);
    };

    const int fullCount = int(leadingSizes.size() + contentSizes.size() + trailingSizes.size());
    const bool slotsVisible = progress > 0.0;
    // The three groups are concatenated into one run, so the two gaps that sit
    // *between* them count exactly once and `runMain` is not summed here.
    const qreal fullGroupMain =
        sumMain(leadingSizes) + sumMain(contentSizes) + sumMain(trailingSizes)
        + gap * qMax(0, fullCount - 1);
    // Collapsed, the extra actions leave the run entirely, so the gaps that
    // separated them go with them and the content re-centres on its own.
    const qreal visibleGroupMain = slotsVisible ? fullGroupMain : runMain(contentSizes);

    // --- the pill and the action button's strip ----------------------------
    const qreal crossExtent = tokens.containerCrossExtent(toolbar.orientation());
    const QWidget *fab = toolbar.fab();
    const bool hasFab = fab != nullptr;

    // Compose reserves the strip at the *expanded* FAB size even while the FAB
    // is the larger, collapsed one — see the header.
    const qreal strip = hasFab ? tokens.fab.betweenSpace + tokens.fab.expandedSize : 0.0;
    const qreal outerCross = hasFab ? qMax(crossExtent, tokens.fab.collapsedSize) : crossExtent;

    const qreal paddingMainStart = tokens.containerLeadingSpace;
    const qreal paddingMainEnd = tokens.containerTrailingSpace;
    const qreal expandedPillMain =
        qMax<qreal>(fullGroupMain + paddingMainStart + paddingMainEnd, 0.0);

    // The pill's anchor. At `End` the FAB takes the outer's trailing edge and
    // the pill keeps its own trailing edge fixed, so collapsing sweeps the
    // pill's leading edge towards the FAB — Compose's `toolbarX =
    // maxToolbarWidth - toolbarWidth`.
    const bool fabAtEnd = toolbar.fabPosition() != MdToolbarFabPosition::Start;
    const qreal outerMainStart = axis.mainStart(outer);
    const qreal outerMainEnd = axis.mainEnd(outer);
    const qreal pillAnchorStart =
        (hasFab && !fabAtEnd) ? outerMainStart + strip : outerMainStart;
    const qreal pillAnchorEnd =
        (hasFab && fabAtEnd) ? outerMainEnd - strip : outerMainStart + expandedPillMain;

    const qreal pillCrossStart = axis.crossStart(outer) + (outerCross - crossExtent) / 2.0;

    // --- the painted container, at the current expansion -------------------
    //
    // `maxIntrinsicWidth * expandedProgress`, i.e. the pill is empty at
    // progress 0. The bands below are laid out against the *expanded* pill, so
    // the content does not reflow; this rectangle is what clips it.
    const qreal containerMain = expandedPillMain * progress;
    const qreal containerMainStart =
        fabAtEnd ? pillAnchorEnd - containerMain : pillAnchorStart;
    layout.container =
        axis.make(containerMainStart, containerMain, pillCrossStart, crossExtent);
    layout.radii = MdShape::resolvedRadii(tokens.containerShape, layout.container.size());

    // --- the slots' band, at full expansion --------------------------------
    const QRectF expandedPill = axis.make(pillAnchorStart, expandedPillMain, pillCrossStart, crossExtent);
    const qreal bandMainStart = axis.mainStart(expandedPill) + paddingMainStart;
    const qreal bandMainExtent = qMax<qreal>(
        expandedPillMain - paddingMainStart - paddingMainEnd, 0.0);
    const qreal bandCrossStart = axis.crossStart(expandedPill) + tokens.containerLeadingSpace;
    const qreal bandCrossExtent =
        qMax<qreal>(crossExtent - tokens.containerLeadingSpace - tokens.containerTrailingSpace, 0.0);
    layout.content = axis.make(bandMainStart, bandMainExtent, bandCrossStart, bandCrossExtent);

    // Compose's outer Row is `Arrangement.Center`: the visible slots are
    // centred as one group inside the band the *full* set defines. At the
    // natural size and full expansion the two coincide, so the offset is zero;
    // it does work the moment a slot disappears or a caller hands the widget a
    // wider rect.
    qreal cursor = bandMainStart + (bandMainExtent - visibleGroupMain) / 2.0;

    // `placedAny` inserts the between-space *before* every item but the first,
    // so the run carries `n - 1` gaps and none of them trails: a trailing gap
    // would push the centred run half a gap off centre.
    bool placedAny = false;
    const auto placeSlot = [&](const QVector<QSizeF> &sizes, QList<QRectF> *boxes) {
        boxes->reserve(sizes.size());
        for (const QSizeF &size : sizes) {
            if (placedAny) {
                cursor += gap;
            }
            const qreal cross =
                bandCrossStart + (bandCrossExtent - axis.crossOf(size)) / 2.0;
            boxes->append(axis.make(cursor, axis.mainOf(size), cross, axis.crossOf(size)));
            cursor += axis.mainOf(size);
            placedAny = true;
        }
    };
    // Only the visible slots advance the cursor: collapsed, the extra actions
    // are absent from the layout entirely, which is what lets the content
    // re-centre in the band.
    const QVector<QSizeF> none;
    placeSlot(slotsVisible ? leadingSizes : none, &layout.leadingChildBoxes);
    placeSlot(contentSizes, &layout.contentChildBoxes);
    placeSlot(slotsVisible ? trailingSizes : none, &layout.trailingChildBoxes);

    // --- the action button --------------------------------------------------
    if (hasFab) {
        const qreal fabSize = tokens.fab.sizeFor(progress);
        const qreal fabMainStart = fabAtEnd ? outerMainEnd - fabSize : outerMainStart;
        const qreal fabCrossStart = axis.crossStart(outer) + (outerCross - fabSize) / 2.0;
        layout.fabBox = axis.make(fabMainStart, fabSize, fabCrossStart, fabSize);
        layout.fabRadii =
            MdShape::resolvedRadii(tokens.fab.shapeFor(progress), layout.fabBox.size());
    }

    return layout;
}

void MdFloatingToolbarStyle::paintFloatingToolbar(QPainter &painter,
                                                  const MdFloatingToolbar &toolbar,
                                                  const MdFloatingToolbarTokens &tokens,
                                                  const Layout &layout)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdTheme &theme = MdTheme::instance();
    const MdToolbarScheme &scheme = tokens.schemeFor(toolbar.colorScheme());

    // A floating toolbar is the one toolbar that carries a shadow — it floats
    // over the content rather than sitting in a bar. The *level* comes from
    // the export (`container.elevation: level3`), which Compose never reads;
    // the *ramp* is Compose's, which lerps its own collapsed and expanded
    // constants by `expandedProgress` so a collapsed pill casts nothing. Both
    // divergences are recorded in docs/porting-todo.md.
    if (!layout.container.isEmpty()) {
        const qreal dp = MdElevation::shadowDp(tokens.containerElevation)
                         * qBound<qreal>(0.0, toolbar.expansionProgress(), 1.0);
        if (dp > 0.0 && !layout.radii.isEmpty()) {
            MdElevation::drawShadowDp(&painter, layout.container, layout.radii.first(), dp,
                                      theme.color(ColorRole::Shadow));
        }
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(theme.color(scheme.containerColor));
    painter.drawPath(MdShape::roundedRect(layout.container, layout.radii));

    painter.restore();
}

void MdFloatingToolbarStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *toolbar = qobject_cast<MdFloatingToolbar *>(widget);
    if (painter == nullptr || toolbar == nullptr) {
        return;
    }
    const MdFloatingToolbarTokens &tokens = toolbar->tokens();
    paintFloatingToolbar(*painter, *toolbar, tokens, layoutFor(*toolbar, tokens));
}

} // namespace md
