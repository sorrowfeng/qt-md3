#include "MdScrollBarStyle.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdScrollBar.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionSlider>

#include <algorithm>
#include <cmath>

namespace md {

namespace {

/// Visible overlay handle width. Compose Desktop's `ScrollbarDefaults`
/// thickness is 8.dp; Material overlay bars stay thin rather than eating a
/// full 16 dp groove.
constexpr int kOverlayWidth = 8;
/// Pointer hit area along the cross axis. Qt's stock extent is comfortable
/// for grabbing; only the *painted* pill shrinks to the overlay width.
constexpr int kHitExtent = 16;
/// Minimum handle length along the track so a long document still has
/// something to grab. Compose uses 16.dp for the same purpose; 24 keeps a
/// clear pill shape at corner-full.
constexpr int kMinHandleLength = 24;

/// Resting handle alpha. 0.38 is the M3 medium-emphasis / secondary-content
/// opacity (`md.sys.state` content at "disabled" strength is the same value),
/// and it is also the alpha Compose Desktop paints an unhovered scrollbar
/// thumb with — so the idle bar matches the rest of the token set instead of
/// inventing a one-off grey.
constexpr qreal kIdleAlpha = 0.38;
/// Hover raises the handle by the published hover state-layer opacity (0.08)
/// on top of the resting emphasis: 0.38 + 0.08.
constexpr qreal kHoverAlpha = 0.46;
/// Pressed / dragged raises it by the dragged state-layer opacity (0.16):
/// 0.38 + 0.16.
constexpr qreal kPressedAlpha = 0.54;
/// Disabled handle: on-surface at the disabled-content emphasis. Lower than
/// idle so a scrollbar in a disabled scroll area does not look grabbable.
constexpr qreal kDisabledAlpha = 0.12;
/// Subtle track while hovered / dragged — the hover state-layer opacity.
constexpr qreal kTrackAlpha = 0.08;

QColor onSurfaceAt(qreal alpha)
{
    QColor colour = MdTheme::instance().color(ColorRole::OnSurface);
    const qreal base = colour.alphaF() > 0.0 ? colour.alphaF() : 1.0;
    colour.setAlphaF(std::clamp(base * alpha, 0.0, 1.0));
    return colour;
}

/// Per-widget overlay width: `MdScrollBar::thickness` when the widget is one,
/// otherwise the Compose 8.dp default.
int overlayWidthFor(const QWidget *widget)
{
    if (const auto *bar = qobject_cast<const MdScrollBar *>(widget)) {
        return bar->thickness();
    }
    return kOverlayWidth;
}

} // namespace

MdScrollBarStyle::MdScrollBarStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdScrollBarStyle *MdScrollBarStyle::shared()
{
    static QMutex mutex;
    static MdScrollBarStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a QProxyStyle set on a widget must
        // outlive that widget.
        instance = new MdScrollBarStyle;
    }
    return instance;
}

int MdScrollBarStyle::overlayWidth()
{
    return kOverlayWidth;
}

int MdScrollBarStyle::hitExtent()
{
    return kHitExtent;
}

int MdScrollBarStyle::minimumHandleLength()
{
    return kMinHandleLength;
}

qreal MdScrollBarStyle::idleAlpha()
{
    return kIdleAlpha;
}

qreal MdScrollBarStyle::hoverAlpha()
{
    return kHoverAlpha;
}

qreal MdScrollBarStyle::pressedAlpha()
{
    return kPressedAlpha;
}

qreal MdScrollBarStyle::disabledAlpha()
{
    return kDisabledAlpha;
}

QRect MdScrollBarStyle::handleRectFor(const QRect &rect,
                                      Qt::Orientation orientation,
                                      int minimum,
                                      int maximum,
                                      int pageStep,
                                      int sliderPosition,
                                      bool upsideDown)
{
    return handleRectFor(rect, orientation, minimum, maximum, pageStep, sliderPosition, upsideDown,
                         kOverlayWidth);
}

QRect MdScrollBarStyle::handleRectFor(const QRect &rect,
                                      Qt::Orientation orientation,
                                      int minimum,
                                      int maximum,
                                      int pageStep,
                                      int sliderPosition,
                                      bool upsideDown,
                                      int overlayWidth)
{
    const bool vertical = orientation == Qt::Vertical;
    const int trackPos = vertical ? rect.top() : rect.left();
    const int trackLen = vertical ? rect.height() : rect.width();
    const int crossPos = vertical ? rect.left() : rect.top();
    const int crossLen = vertical ? rect.width() : rect.height();

    const int extent = qMin(kHitExtent, crossLen);
    const int visible = qBound(1, qMin(overlayWidth, extent), extent);
    const int crossInset = qMax(0, (extent - visible) / 2);
    const int handleCrossPos = crossPos + qMin(crossInset, qMax(0, (crossLen - visible) / 2));

    const int range = maximum - minimum;
    int handleLen = trackLen;
    if (range > 0) {
        const int span = range + qMax(pageStep, 1);
        handleLen = qBound(kMinHandleLength, int(std::lround(qreal(trackLen) * pageStep / qreal(span))),
                           trackLen);
    }

    int handlePos = trackPos;
    if (range > 0) {
        const qreal t = qBound(0.0, qreal(sliderPosition - minimum) / qreal(range), 1.0);
        const qreal along = upsideDown ? (1.0 - t) : t;
        handlePos = trackPos + int(std::lround(qreal(trackLen - handleLen) * along));
    }

    if (vertical) {
        return QRect(handleCrossPos, handlePos, visible, handleLen);
    }
    return QRect(handlePos, handleCrossPos, handleLen, visible);
}

bool MdScrollBarStyle::chromeSuppressed(const QWidget *widget)
{
    if (const auto *bar = qobject_cast<const MdScrollBar *>(widget)) {
        return bar->visibleMode() == MdScrollBarVisibility::Never;
    }
    return false;
}

void MdScrollBarStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                          QPainter *painter, const QWidget *widget) const
{
    if (control != CC_ScrollBar || !option || !painter) {
        MdStyleBase::drawComplexControl(control, option, painter, widget);
        return;
    }
    const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option);
    if (!slider) {
        MdStyleBase::drawComplexControl(control, option, painter, widget);
        return;
    }

    if (chromeSuppressed(widget)) {
        return;
    }

    const QRect bar = slider->rect;
    const bool vertical = slider->orientation == Qt::Vertical;

    // The handle rect already carries the overlay width; the track uses
    // the same cross-axis so the rail and the pill share a centre line.
    const QRect handle = subControlRect(control, option, SC_ScrollBarSlider, widget);
    QRect track = bar;
    if (vertical) {
        track.setLeft(handle.left());
        track.setWidth(handle.width());
    } else {
        track.setTop(handle.top());
        track.setHeight(handle.height());
    }

    const bool enabled = (slider->state & State_Enabled) != 0;
    const bool hovered = (slider->state & State_MouseOver) != 0;
    const bool pressed = (slider->state & State_Sunken) != 0
                         && (slider->activeSubControls & SC_ScrollBarSlider) != 0;

    const bool forceChrome = widget
                             && qobject_cast<const MdScrollBar *>(widget)
                             && qobject_cast<const MdScrollBar *>(widget)->visibleMode()
                                    == MdScrollBarVisibility::Always;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);

    // Track: invisible at rest, a very subtle on-surface rail while the bar
    // is being pointed at or dragged (M3 overlay style — no permanent groove).
    if (enabled && (hovered || pressed || forceChrome)) {
        painter->setBrush(onSurfaceAt(kTrackAlpha));
        painter->drawPath(MdShape::roundedRect(QRectF(track), ShapeCorner::Full));
    }

    qreal alpha = kIdleAlpha;
    if (!enabled) {
        alpha = kDisabledAlpha;
    } else if (pressed) {
        alpha = kPressedAlpha;
    } else if (hovered) {
        alpha = kHoverAlpha;
    }
    painter->setBrush(onSurfaceAt(alpha));
    painter->drawPath(MdShape::roundedRect(QRectF(handle), ShapeCorner::Full));
    painter->restore();
}

int MdScrollBarStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
                                  const QWidget *widget) const
{
    switch (metric) {
    case PM_ScrollBarExtent:
        return kHitExtent;
    case PM_ScrollBarSliderMin:
        return kMinHandleLength;
    default:
        break;
    }
    return MdStyleBase::pixelMetric(metric, option, widget);
}

QRect MdScrollBarStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                       SubControl subControl, const QWidget *widget) const
{
    if (control != CC_ScrollBar) {
        return MdStyleBase::subControlRect(control, option, subControl, widget);
    }
    const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option);
    if (!slider) {
        return MdStyleBase::subControlRect(control, option, subControl, widget);
    }

    // Overlay bars have no arrow buttons; returning an empty rect keeps the
    // whole extent available for the track.
    if (subControl == SC_ScrollBarAddLine || subControl == SC_ScrollBarSubLine) {
        return QRect();
    }

    const QRect bar = slider->rect;
    const bool vertical = slider->orientation == Qt::Vertical;

    const QRect handle = handleRectFor(bar, slider->orientation, slider->minimum, slider->maximum,
                                       slider->pageStep, slider->sliderPosition,
                                       slider->upsideDown, overlayWidthFor(widget));

    switch (subControl) {
    case SC_ScrollBarSlider:
        return handle;
    case SC_ScrollBarGroove:
        return bar;
    case SC_ScrollBarSubPage:
        // Between the leading end and the handle.
        if (vertical) {
            return QRect(bar.left(), bar.top(), bar.width(), qMax(0, handle.top() - bar.top()));
        }
        return QRect(bar.left(), bar.top(), qMax(0, handle.left() - bar.left()), bar.height());
    case SC_ScrollBarAddPage:
        // Between the handle and the trailing end.
        if (vertical) {
            return QRect(bar.left(), handle.bottom() + 1, bar.width(),
                         qMax(0, bar.bottom() - handle.bottom()));
        }
        return QRect(handle.right() + 1, bar.top(), qMax(0, bar.right() - handle.right()),
                     bar.height());
    default:
        break;
    }
    return MdStyleBase::subControlRect(control, option, subControl, widget);
}

void MdScrollBarStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                     QPainter *painter, const QWidget *widget) const
{
    // Arrow buttons and the classic groove fill are not part of the overlay
    // look; painting nothing here keeps the fallback from drawing native
    // chrome underneath drawComplexControl.
    switch (element) {
    case PE_IndicatorButtonDropDown:
    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown:
    case PE_IndicatorSpinPlus:
    case PE_IndicatorSpinMinus:
        return;
    default:
        break;
    }
    MdStyleBase::drawPrimitive(element, option, painter, widget);
}

} // namespace md
