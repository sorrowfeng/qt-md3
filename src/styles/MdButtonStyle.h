#ifndef MD_BUTTON_STYLE_H
#define MD_BUTTON_STYLE_H

// MdButtonStyle — everything about how an MdButton is painted and measured.
//
// Rendering pattern: the brief assigns Buttons to Pattern B (a QPushButton
// subclass), and MdButton is one. The *painting* still goes through Pattern A's
// paint hub rather than drawControl(), because a button needs things
// `QStyle::drawControl` has no vocabulary for:
//
//   * a container whose corner radii are morphed by a spring on press
//   * a state layer that is a single composited overlay, not a Qt state
//   * a ripple clipped to the *current* (morphing) corner path
//   * an outward focus indicator that overflows the widget rect
//
// So one MdButtonStyle instance is registered for the MdButton meta-object and
// every button's paint event lands in drawWidget(). Subclasses inherit it for
// free, which is what the button family needs — MdSplitButton and
// MdSegmentedButton both extend MdButton.
//
// The layout code and the paint code are the same code path: layoutFor()
// produces the rectangles, paintButton() consumes them, and sizeHint() asks
// layoutFor() too, so a button can never report a size it does not draw.

#include "core/MdButtonTokens.h"
#include "core/MdFocusRing.h"
#include "core/MdTypes.h"
#include "styles/MdStyleBase.h"

#include <QtCore/QRectF>
#include <QtCore/QList>
#include <QtCore/QSizeF>

class QPainter;
class QWidget;

namespace md {

class MdButton;

class QT_MD3_EXPORT MdButtonStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdButtonStyle(QObject *parent = nullptr);

    /// The single style instance that paints every MdButton. Creates and
    /// registers it on first use; deliberately never destroyed, because it must
    /// outlive every widget (the same rule MdPaintFilterHub follows).
    static MdButtonStyle *shared();

    /// True once shared() has registered the paint filter.
    static bool isInstalled();

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// Resolved rectangles for one button. All coordinates are widget-local.
    struct Layout
    {
        /// The painted container. Deliberately *not* the whole widget rect for
        /// the filled variants' purposes: see focusRingInset().
        QRectF container;
        QRectF leadingIcon;
        QRectF trailingIcon;
        /// The rect the label is centred in. Empty when there is no label.
        QRectF label;
        /// The size the token set asks for — what `md.comp.button.<size>`'s
        /// height and leading/trailing/icon-label spaces add up to. sizeHint()
        /// adds the focus margin to this. The painted container may be wider
        /// when a layout stretches the widget.
        QSizeF preferredContainerSize;
        /// Corner radii for the container, already morphed for the press.
        QList<qreal> radii;
        /// Icon names resolved to inline-start / inline-end, so the RTL swap is
        /// decided once here rather than again in the painter.
        QString startIconName;
        QString endIconName;
        bool hasLeadingIcon = false;
        bool hasTrailingIcon = false;
    };

    /// Measure and place a button. Uses the same token resolution and the same
    /// font metrics that painting uses.
    static Layout layoutFor(const MdButton &button, const MdButtonTokens &tokens);

    /// The outward focus indicator for a token set.
    ///
    /// `color` is left invalid on purpose: the caller resolves it from the
    /// theme at paint time so a seed change is picked up without rebuilding.
    static MdFocusRingSpec focusRingSpec(const MdButtonTokens &tokens);

    /// Margin the widget has to reserve around the container.
    ///
    /// The M3 focus indicator is specified to sit *outside* the component — a
    /// 2 px gap plus a 3 px stroke, growing to 8 px in the first 150 ms of the
    /// focus animation. Qt clips a child widget to its own rectangle, so that
    /// outline is simply cut off unless the widget makes room for it. The value
    /// is therefore the indicator's peak extent, derived from the spec rather
    /// than hard-coded: `offset + activeWidth / 2 + width / 2`.
    static qreal focusRingInset(const MdFocusRingSpec &spec);

    /// Paint an already-measured button.
    static void paintButton(QPainter &painter,
                            const MdButton &button,
                            const MdButtonTokens &tokens,
                            const Layout &layout);

    /// The state whose token row should be used for `button` right now.
    static MdButtonState stateFor(const MdButton &button);

    /// Token-drawn colours for one state, with the disabled opacities folded in.
    /// Label and icon are separate calls because the token export publishes a
    /// separate opacity for each, even though the five variants currently
    /// happen to agree on 0.38.
    static QColor containerColor(const MdButtonTokens &tokens, MdButtonState state);
    static QColor labelColor(const MdButtonTokens &tokens, MdButtonState state);
    static QColor iconColor(const MdButtonTokens &tokens, MdButtonState state);
    static QColor outlineColor(const MdButtonTokens &tokens, MdButtonState state);
};

} // namespace md

#endif // MD_BUTTON_STYLE_H
