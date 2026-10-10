#include "MdSearchBarStyle.h"

#include "core/MdElevation.h"
#include "core/MdIcon.h"
#include "core/MdTheme.h"
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

void MdSearchBarStyle::paintSearchBar(QPainter &painter, const MdSearchBar &bar,
                                      const MdSearchTokens &tokens)
{
    const MdSearchState state = bar.isEffectivelyDisabled()
        ? MdSearchState::Disabled
        : (bar.isHovered() ? MdSearchState::Hovered : MdSearchState::Enabled);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1 — the container.
    {
        const QRectF container = bar.containerRect();
        if (bar.surface() == MdSearchSurface::Bar) {
            MdElevation::drawShadowDp(&painter, container, tokens.containerRadius,
                                      int(tokens.containerElevation),
                                      MdTheme::instance().color(ColorRole::Shadow));
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(MdTheme::instance().color(tokens.containerColor));
        painter.drawRoundedRect(container, tokens.containerRadius, tokens.containerRadius);
    }

    // 2 — the state layer.
    {
        const QColor overlay = resolveSlot(tokens.stateLayer[int(state)]);
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawRoundedRect(bar.containerRect(), tokens.containerRadius,
                                    tokens.containerRadius);
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

    painter.restore();
}

void MdSearchBarStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    MdSearchBar *bar = qobject_cast<MdSearchBar *>(widget);
    if (!bar || !painter) {
        return;
    }
    paintSearchBar(*painter, *bar, bar->searchTokens());
}

} // namespace md
