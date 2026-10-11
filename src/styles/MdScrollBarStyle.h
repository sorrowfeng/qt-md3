#ifndef MD_SCROLL_BAR_STYLE_H
#define MD_SCROLL_BAR_STYLE_H

// MdScrollBarStyle — Pattern B paint for QScrollBar / QScrollArea chrome.
//
// Material Design 3 does not publish a standalone Scrollbar family (nothing
// under m3.material.io/components), so this style follows the Material
// overlay-scrollbar convention that Compose Desktop and AndroidX foundation
// use on top of the same token set:
//
//   * overlay, not a permanent groove — the track stays invisible at rest
//     and only shows a very subtle rail while hovered or dragged;
//   * the handle is a corner-full pill in `on-surface` at the M3
//     medium-emphasis content opacity (0.38 at rest — the same opacity
//     md.sys gives secondary / disabled content), raised by the published
//     state-layer opacities while interacting (hover 0.08, dragged 0.16);
//   * the visible handle is 8 dp wide (the overlay width), while the hit
//     area stays at 16 dp so a pointer can still grab it comfortably;
//   * there are no add/sub arrow buttons — the whole extent is the track.
//
// Every colour is read from MdTheme at paint time, so light / dark, seed
// changes and contrast levels all follow the usual theme lifecycle.

#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRect>

class QPainter;
class QWidget;

namespace md {

class QT_MD3_EXPORT MdScrollBarStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdScrollBarStyle(QObject *parent = nullptr);

    /// Create-or-return the singleton. Unlike the Pattern A styles there is
    /// no paint filter to install: consumers call `scrollBar->setStyle(this)`
    /// (or set it on the owning QScrollArea) and QProxyStyle handles the rest.
    static MdScrollBarStyle *shared();

    /// Visible overlay handle width in logical px (Compose Desktop's default
    /// scrollbar thickness is 8.dp; Material overlay bars stay thin).
    static int overlayWidth();
    /// Pointer hit area along the cross axis (Qt's default extent, kept so
    /// the thinner overlay handle remains grabbable).
    static int hitExtent();
    /// Minimum handle length along the track.
    static int minimumHandleLength();

    /// Track / handle alphas on top of `on-surface`.
    static qreal idleAlpha();
    static qreal hoverAlpha();
    static qreal pressedAlpha();
    static qreal disabledAlpha();

    /// The painted handle rect for a scrollbar option (also the test hook).
    /// `rect` is the scrollbar's full rect; the handle is the 8 dp overlay
    /// pill inset into it and positioned from the slider value.
    static QRect handleRectFor(const QRect &rect,
                               Qt::Orientation orientation,
                               int minimum,
                               int maximum,
                               int pageStep,
                               int sliderPosition,
                               bool upsideDown);

    /// Pattern B: draws CC_ScrollBar. Everything else falls through to the
    /// base (the application style).
    void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                            QPainter *painter, const QWidget *widget) const override;

    /// Overlay metrics: 16 dp hit extent, no arrow buttons, inset handle.
    int pixelMetric(PixelMetric metric, const QStyleOption *option = nullptr,
                    const QWidget *widget = nullptr) const override;

    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                         SubControl subControl, const QWidget *widget) const override;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override;
};

} // namespace md

#endif // MD_SCROLL_BAR_STYLE_H
