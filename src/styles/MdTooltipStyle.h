#ifndef MD_TOOLTIP_STYLE_H
#define MD_TOOLTIP_STYLE_H

#include "MdStyleBase.h"
#include "core/MdRipple.h"
#include "core/MdTooltipTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtCore/QString>

class QFont;
class QPainter;
class QPen;

namespace md {

class MdColorScheme;

/// Pattern A style for `MdTooltip`: the layout math, painting and popup
/// positioning of the `md.comp.plain-tooltip.*` / `md.comp.rich-tooltip.*`
/// families, registered in the paint hub so the first construction installs
/// it application-wide.
///
/// material-web ships no tooltip component (token export only), so the
/// layout is a faithful port of Compose M3's Tooltip.kt measure logic —
/// exposed as pure functions so the test suite pins every rect without a
/// widget.
class QT_MD3_EXPORT MdTooltipStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdTooltipStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdTooltipStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    // --- layout -------------------------------------------------------------
    /// The plain layout: the body-small text inside the 8/4 content padding,
    /// pre-wrapped to the container width. `bounds` is the container rect.
    struct PlainLayout
    {
        /// The text block, already positioned; `height` spans wrapped lines.
        QRectF textRect;
    };

    static PlainLayout plainLayout(const QRectF &bounds,
                                   const QString &text,
                                   const QFont &font,
                                   const MdTooltipTokens &tokens);

    /// The rich layout: subhead (paddingFromBaseline 28), supporting text
    /// (baseline 24 below the subhead box, 16 above the container bottom) and
    /// the action label centred in its hit region (min height 36, bottom 8).
    /// With neither subhead nor action the text keeps the plain 4 px vertical
    /// padding instead (Compose textVerticalPadding).
    struct RichLayout
    {
        QRectF subheadRect;   ///< The title-small subhead, invalid without one.
        QRectF textRect;      ///< The body-medium supporting text.
        QRectF actionRect;    ///< The label's text rect, invalid without an action.
        QRectF actionHitRect; ///< The label grown by the 12 px button chrome.
        qreal containerHeight = 0.0;
    };

    static RichLayout richLayout(const QRectF &bounds,
                                 const QString &title,
                                 const QString &text,
                                 const QString &actionLabel,
                                 const QFont &subheadFont,
                                 const QFont &textFont,
                                 const QFont &actionFont,
                                 const MdTooltipTokens &tokens);

    /// The plain container height for a container `width`: the text wrapped
    /// at `width - 16`, at least the 24 px minimum.
    static qreal plainHeightForWidth(qreal width, const QString &text, const MdTooltipTokens &tokens);

    /// The rich container height for a container `width`.
    static qreal richHeightForWidth(qreal width,
                                    const QString &title,
                                    const QString &text,
                                    const QString &actionLabel,
                                    const MdTooltipTokens &tokens);

    /// The natural container width before clamping: the widest content plus
    /// the horizontal paddings, then clamped into [40, maxWidth].
    static qreal plainWidthFor(const QString &text, const MdTooltipTokens &tokens);
    static qreal richWidthFor(const QString &title,
                              const QString &text,
                              const QString &actionLabel,
                              const MdTooltipTokens &tokens);

    // --- popup positioning (Compose TooltipPositionProviderImpl) ------------
    /// The `TooltipAnchorPosition.Above` provider: horizontally centred with
    /// start / end fallbacks, above the anchor or below it when the top would
    /// clip, always coerced into the window. `anchorRect` and the returned
    /// point share the window coordinate space; `size` is the tooltip
    /// SURFACE (container) size.
    static QPoint abovePopupPosition(const QRect &anchorRect,
                                     const QSize &size,
                                     const QSize &windowSize,
                                     qreal spacing);

    /// The rich provider: start-aligned to the anchor, shifted left when it
    /// would clip the window's right edge, centred when both edges clip.
    /// Compose keeps shipping this for RichTooltip even though the generic
    /// provider supersedes it, so the behaviour difference is preserved.
    static QPoint richPopupPosition(const QRect &anchorRect,
                                    const QSize &size,
                                    const QSize &windowSize,
                                    qreal spacing);

    // --- painting -----------------------------------------------------------
    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// Paint one tooltip. `containerRect` is the surface inside the widget's
    /// shadow / caret margins; `caretRect` is the protruding triangle (invalid
    /// with MdTooltipCaretSide::None). `opacity` carries the host's enter/exit
    /// transition (a top-level popup could use window opacity, but a child
    /// widget painting into the gallery cannot).
    void paintTooltip(QPainter *painter,
                      const QRectF &containerRect,
                      MdTooltipVariant variant,
                      const QString &title,
                      const QString &text,
                      const QString &actionLabel,
                      MdTooltipActionState actionState,
                      const MdRippleFrame &actionRipple,
                      bool actionHasKeyboardFocus,
                      MdTooltipCaretSide caretSide,
                      const QRectF &caretRect,
                      qreal opacity) const;

    void onThemeUpdate() override;
};

} // namespace md

#endif // MD_TOOLTIP_STYLE_H
