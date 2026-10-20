#include "MdSearchBarStyle.h"

#include "core/MdElevation.h"
#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdSearchBar.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

QColor resolveSlot(const MdNavigationColourSlot &slot)
{
    if (!slot.isPresent()) {
        return QColor();
    }
    QColor colour = MdTheme::instance().color(slot.role);
    if (!colour.isValid()) {
        return QColor();
    }
    colour.setAlphaF(qBound(0.0, colour.alphaF() * slot.opacity, 1.0));
    return colour;
}

/// The filled pill. The path covers the full container rect so the stadium's
/// corners are fully painted — a half-pixel inset fill left the rounded caps
/// open against the page. Any *stroke* uses `outlinePath` instead.
QPainterPath containerPath(const QRectF &rect, qreal radius)
{
    const qreal r = qMin(radius, qMin(rect.width(), rect.height()) / 2.0);
    return MdShape::roundedRect(rect, {r, r, r, r});
}

/// Half-pixel inset path for a 1 px stroke, so the outline is not clipped by
/// the widget edge.
QPainterPath outlinePath(const QRectF &rect, qreal radius)
{
    const QRectF inset = rect.adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal r = qMin(radius, qMin(inset.width(), inset.height()) / 2.0);
    return MdShape::roundedRect(inset, {r, r, r, r});
}

/// Stroke centre line sits `offset + activeWidth / 2 + width / 2` outside the
/// component, and the stroke reaches half of whatever width it currently has.
/// At the peak of the focus grow animation that width is `activeWidth`, so
/// this is the furthest the indicator ever reaches.
qreal ringInsetFor(const MdFocusRingSpec &spec)
{
    return spec.offset() + spec.activeWidth / 2.0 + spec.width / 2.0;
}

} // namespace

MdSearchBarStyle::MdSearchBarStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdSearchBarStyle *MdSearchBarStyle::shared()
{
    static QMutex mutex;
    static MdSearchBarStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdSearchBarStyle;
        installPaintFilter<MdSearchBar>(instance);
    }
    return instance;
}

bool MdSearchBarStyle::isInstalled()
{
    return hasPaintFilter(&MdSearchBar::staticMetaObject);
}

MdFocusRingSpec MdSearchBarStyle::focusRingSpec(const MdSearchTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.outwardOffset = tokens.focusIndicatorOuterOffset;
    spec.inward = false;
    // The search token set publishes `thickness` and `outer-offset` only.
    // `active-width`, `duration` and the easing are md.comp.focus-ring's, and
    // the module's defaults already carry them.
    return spec;
}

qreal MdSearchBarStyle::focusRingInset(const MdSearchTokens &tokens)
{
    return ringInsetFor(focusRingSpec(tokens));
}

void MdSearchBarStyle::paintSearchBar(QPainter &painter, const MdSearchBar &bar,
                                      const MdSearchTokens &tokens)
{
    const MdSearchState state = bar.isEffectivelyDisabled()
        ? MdSearchState::Disabled
        : (bar.isHovered() ? MdSearchState::Hovered : MdSearchState::Enabled);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1 — the container: a full pill (corner-full) via MdShape so the
    //     stadium is complete — fill the full rect so the rounded caps are
    //     painted, then one crisp outline stroke on the half-pixel inset path
    //     (a rectangular QFrame border used to cut the corners and leave two
    //     horizontal lines).
    {
        const QRectF container = bar.containerRect();
        const qreal radius = tokens.containerRadius;
        if (bar.surface() == MdSearchSurface::Bar) {
            // The elevation halo must stay inside the stadium. The bar's
            // container *is* the widget rect, so any ring that grows past the
            // pill is clipped by the QPainter into a hard edge — two dark
            // horizontal lines across the top and bottom that cut through the
            // rounded caps. Clip to the pill; the fill below covers it, which
            // is also the AGENTS rule (elevation is tonal surface, not an
            // outer drop shadow). Also take the real level dp: passing
            // int(ElevationLevel) used the enum ordinal, not elevationDp().
            painter.save();
            painter.setClipPath(containerPath(container, radius));
            MdElevation::drawShadowDp(&painter, container, radius,
                                      MdElevation::shadowDp(tokens.containerElevation),
                                      MdTheme::instance().color(ColorRole::Shadow));
            painter.restore();
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(MdTheme::instance().color(tokens.containerColor));
        painter.drawPath(containerPath(container, radius));
        const QColor outline = MdTheme::instance().color(ColorRole::OutlineVariant);
        if (outline.isValid() && outline.alpha() > 0) {
            QPen pen(outline);
            pen.setWidthF(1.0);
            pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(outlinePath(container, radius));
        }
    }

    // 2 — the state layer.
    {
        const QColor overlay = resolveSlot(tokens.stateLayer[int(state)]);
        if (overlay.isValid() && overlay.alpha() > 0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawPath(containerPath(bar.containerRect(), tokens.containerRadius));
        }
    }

    // 3 — the leading icon (the search glyph).
    {
        const QRectF icon = bar.leadingIconRect();
        const QColor colour = MdTheme::instance().color(tokens.leadingIconColor);
        painter.setPen(Qt::NoPen);
        painter.setBrush(colour);
        // A circle + handle stands in for the search glyph's geometry when
        // the icon font is absent; with MdIcon available the glyph paints.
        const qreal r = icon.width() * 0.32;
        painter.drawEllipse(icon.center() + QPointF(-icon.width() * 0.06, -icon.height() * 0.06),
                            r, r);
        painter.setPen(QPen(colour, 2.0));
        painter.drawLine(icon.center() + QPointF(r * 0.75, r * 0.75),
                         icon.bottomRight() + QPointF(-2.0, -2.0));
    }

    // 4 — the focus indicator (the system focus-indicator rows). The ring
    //     paints outward from the container; the widget carries a transparent
    //     margin (see focusRingInset) so Qt never clips the stroke.
    if (bar.hasFocus()) {
        const qreal radius = tokens.containerRadius;
        MdFocusRing::paint(&painter, bar.containerRect(),
                           {radius, radius, radius, radius},
                           MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens));
    }

    painter.restore();
}

void MdSearchBarStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    MdSearchBar *bar = qobject_cast<MdSearchBar *>(widget);
    if (!bar || !painter) {
        return;
    }
    paintSearchBar(*painter, *bar, bar->searchTokens());

    // The paint filter replaces QLineEdit::paintEvent, so the input line has
    // to be drawn here or the typed text never appears.
    const QString contents = bar->text().isEmpty() ? bar->placeholderText() : bar->text();
    if (contents.isEmpty()) {
        return;
    }
    painter->save();
    painter->setFont(bar->font());
    const bool showingPlaceholder = bar->text().isEmpty();
    QColor colour = MdTheme::instance().color(
        showingPlaceholder ? bar->searchTokens().supportingTextColor
                           : bar->searchTokens().inputTextColor);
    if (bar->isEffectivelyDisabled()) {
        colour.setAlphaF(colour.alphaF() * 0.38);
    }
    painter->setPen(colour);
    const QRectF container = bar->containerRect();
    painter->drawText(container.adjusted(int(bar->searchTokens().leadingSpace
                                                   + bar->searchTokens().iconSize + 8.0),
                                              0, -int(bar->searchTokens().trailingSpace), 0),
                      int(Qt::AlignVCenter | MdStyleBase::leadingAlignment(bar)), contents);
    painter->restore();
}

} // namespace md
