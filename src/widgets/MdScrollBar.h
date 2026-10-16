#ifndef MD_SCROLL_BAR_H
#define MD_SCROLL_BAR_H

// MdScrollBar — Stage 2 Qt extension (Appendix D, P0).
//
// MD3 has no standalone scrollbar family (nothing under
// m3.material.io/components). This control derives from the Material
// overlay-scrollbar convention that Compose Desktop and AndroidX foundation
// use on top of the same token set, already painted by `MdScrollBarStyle`:
//
//   * the track is invisible at rest and only shows a very subtle rail while
//     hovered or dragged;
//   * the handle is a corner-full pill in `on-surface` at the medium-emphasis
//     content opacity (0.38 idle / 0.46 hover / 0.54 dragged / 0.12
//     disabled) — the same state-layer opacity ladder the rest of the library
//     uses, not a Qt groove-and-handle pair;
//   * the visible handle is 8 dp wide (the overlay width) while the hit area
//     stays at 16 dp;
//   * there are no add/sub arrow buttons.
//
// This class promotes that style into a first-class control with
// `Q_PROPERTY`s for thickness and visibility mode so callers no longer have
// to install the proxy style by hand. Theme changes re-resolve through
// `MdStyleBase::connectThemeUpdate` like every other component.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QScrollBar>

namespace md {

class MdScrollBarStyle;

/// When the scrollbar chrome is drawn.
enum class MdScrollBarVisibility {
    /// Shown while the content overflows and the pointer is over the
    /// scroll area (Compose Desktop's default overlay behaviour).
    Auto,
    /// Always drawn, even when the content fits.
    Always,
    /// Never drawn; the bar still scrolls programmatically / by wheel.
    Never,
    Count
};

class QT_MD3_EXPORT MdScrollBar : public QScrollBar
{
    Q_OBJECT

    /// Visible overlay handle width in logical pixels. Clamped to
    /// [4, 24]; the pointer hit area stays at `MdScrollBarStyle::hitExtent()`.
    Q_PROPERTY(int thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
    /// When the overlay chrome is painted (`MdScrollBarVisibility`).
    Q_PROPERTY(md::MdScrollBarVisibility visibleMode READ visibleMode WRITE setVisibleMode NOTIFY
                   visibleModeChanged)
    /// True while the pointer is over the bar or the handle is dragged —
    /// the hover/drag lift the derived spec calls for.
    Q_PROPERTY(bool lifted READ isLifted NOTIFY liftedChanged)

public:
    explicit MdScrollBar(QWidget *parent = nullptr);
    explicit MdScrollBar(Qt::Orientation orientation, QWidget *parent = nullptr);
    ~MdScrollBar() override;

    int thickness() const { return m_thickness; }
    void setThickness(int thickness);

    MdScrollBarVisibility visibleMode() const { return m_visibleMode; }
    void setVisibleMode(MdScrollBarVisibility mode);

    /// Hover or drag: the handle paints at the lifted alpha.
    bool isLifted() const { return m_hovered || m_dragging; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void thicknessChanged(int thickness);
    void visibleModeChanged(md::MdScrollBarVisibility mode);
    void liftedChanged(bool lifted);

protected:
    void enterEvent(MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void init();
    void pushMetricsToStyle();
    void setLifted(bool lifted);

    int m_thickness = 8;
    MdScrollBarVisibility m_visibleMode = MdScrollBarVisibility::Auto;
    bool m_hovered = false;
    bool m_dragging = false;
    bool m_lifted = false;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdScrollBarVisibility)

#endif // MD_SCROLL_BAR_H
