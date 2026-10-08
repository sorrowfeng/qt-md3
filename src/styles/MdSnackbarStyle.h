#ifndef MD_SNACKBAR_STYLE_H
#define MD_SNACKBAR_STYLE_H

#include "MdStyleBase.h"
#include "core/MdRipple.h"
#include "core/MdSnackbarTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtCore/QString>

class QFontMetricsF;
class QPainter;

namespace md {

class MdColorScheme;

/// Pattern A style for `MdSnackbar`: the layout math and painting of the
/// `md.comp.snackbar.*` family, registered in the paint hub so the first
/// construction installs it application-wide.
///
/// The layout is a faithful port of Compose's OneRowSnackbar and
/// NewLineButtonSnackbar measure policies (the styling-fix versions — the
/// legacy ones are dead code behind a disabled flag). It is exposed as pure
/// functions so the test suite pins every rect without a widget.
class QT_MD3_EXPORT MdSnackbarStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdSnackbarStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdSnackbarStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    // --- layout -------------------------------------------------------------
    /// The one-row layout: the message left, the action and dismiss icon
    /// right-aligned, everything vertically centred. `bounds` is the widget
    /// rect; the container fills it.
    struct OneRowLayout
    {
        /// The message text box, already positioned. `height` spans every
        /// wrapped line.
        QRectF textRect;
        /// The action label's text rect, or invalid without an action.
        QRectF actionRect;
        /// The action's interactive region: the label grown by the text
        /// button chrome the export does not publish (12 px each side).
        QRectF actionHitRect;
        /// The 24 px dismiss icon, or invalid without one.
        QRectF dismissRect;
        /// The dismiss interactive region: the 40 px icon-button chrome
        /// Compose places the icon in.
        QRectF dismissHitRect;
        /// max(48, content) for one line; first line at 30 px and
        /// max(68, …) when the message wraps.
        qreal containerHeight = 48.0;
        bool oneLine = true;
    };

    static OneRowLayout oneRowLayout(const QRectF &bounds,
                                     const QString &message,
                                     const QString &actionLabel,
                                     bool withDismissAction,
                                     const QFont &supportingFont,
                                     const QFont &actionFont,
                                     qreal iconSize,
                                     const MdSnackbarTokens &tokens);

    /// The new-line layout: the message on top, the action row bottom-right.
    struct NewLineLayout
    {
        QRectF textRect;
        QRectF actionRect;
        QRectF actionHitRect;
        QRectF dismissRect;
        QRectF dismissHitRect;
        /// Natural height: text (14 px padding) + action row + 4 px bottom.
        qreal containerHeight = 0.0;
    };

    static NewLineLayout newLineLayout(const QRectF &bounds,
                                       const QString &message,
                                       const QString &actionLabel,
                                       bool withDismissAction,
                                       const QFont &supportingFont,
                                       const QFont &actionFont,
                                       qreal iconSize,
                                       const MdSnackbarTokens &tokens);

    /// The height the widget's sizeHint needs: single line 48, wrapped at the
    /// given width per the one-row rules (68 minimum), or the new-line
    /// natural height.
    static qreal heightForWidth(qreal width,
                                const QString &message,
                                bool actionOnNewLine,
                                const QString &actionLabel,
                                bool withDismissAction,
                                const MdSnackbarTokens &tokens);

    // --- painting -----------------------------------------------------------
    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// Paint one snackbar into `rect` — shared by the widget and the host's
    /// fade/scale animation (which paints through a transform).
    void paintSnackbar(QPainter *painter,
                       const QRectF &rect,
                       const QString &message,
                       const QString &actionLabel,
                       bool withDismissAction,
                       bool actionOnNewLine,
                       MdSnackbarState actionState,
                       MdSnackbarState iconState,
                       const MdRippleFrame &actionRipple,
                       const MdRippleFrame &iconRipple,
                       bool actionHasKeyboardFocus,
                       bool iconHasKeyboardFocus) const;

    void onThemeUpdate() override;
};

} // namespace md

#endif // MD_SNACKBAR_STYLE_H
