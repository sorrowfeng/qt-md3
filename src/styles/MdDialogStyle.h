#ifndef MD_DIALOG_STYLE_H
#define MD_DIALOG_STYLE_H

#include "core/MdDialogTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>
#include <QtCore/QString>

class QPainter;

namespace md {

class MdDialog;

/// Pattern A style for `MdDialog`: painting and geometry for the
/// `md.comp.dialog.*` family, registered in the paint hub so the first
/// `MdDialog` construction installs it application-wide.
///
/// A dialog is a non-interactive surface — the container publishes no state
/// rows; the only interaction the export describes lives on the action
/// buttons, which are real `MdButton` children (Text variant — the colours
/// the export publishes for the action row are exactly the text button's
/// own: label-large in primary at the standard state opacities). The style
/// therefore paints the container, the headline and the supporting text, and
/// publishes the geometry the widget positions its icon and button children
/// into.
class QT_MD3_EXPORT MdDialogStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdDialogStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdDialogStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    /// The content a layout is computed for — the value types of the dialog's
    /// slots, so the pure layout functions never need a widget.
    struct ContentSpec
    {
        /// True once an icon widget has been handed to `setIconWidget`.
        bool hasIcon = false;
        QString title;
        QString text;
        /// The action buttons' widths in dialog order: confirm first, then
        /// dismiss. Zero to two entries; empty = no action row.
        QList<qreal> actionWidths;
    };

    struct Layout
    {
        /// The painted container: the widget rect minus the shadow margin.
        QRectF container;
        /// Corner radii, TL / TR / BR / BL.
        QList<qreal> radii;
        /// The icon slot — centred, `with-icon.icon.size` square. Invalid
        /// when the content has no icon.
        QRectF iconRect;
        /// The headline. Invalid when the title is empty.
        QRectF titleRect;
        /// The supporting text, wrapped to the content width. Invalid when
        /// the text is empty.
        QRectF textRect;
        /// The action buttons, one or two end-aligned rows (a row wraps when
        /// the buttons plus the 8 px spacing overflow the content width).
        /// Empty when there are no actions.
        QList<QList<QRectF>> actionRows;
    };

    /// The layout for a dialog `width` (the widget width, shadow margins
    /// included). Pure: everything derives from the spec, the fonts and the
    /// tokens, so the tests pin geometry without constructing widgets.
    static Layout layoutFor(qreal width, const ContentSpec &content,
                            const MdDialogTokens &tokens);

    /// The widget height for a given width — the container height plus the
    /// shadow margins.
    static qreal heightForWidth(qreal width, const ContentSpec &content,
                                const MdDialogTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite the same way the
    /// other families' are. `opacity` carries the host's enter/exit fade (a
    /// child widget has no window opacity — the snackbar idiom).
    static void paintDialog(QPainter &painter, const MdDialog &dialog,
                            const MdDialogTokens &tokens, const Layout &layout,
                            qreal opacity);
};

} // namespace md

#endif // MD_DIALOG_STYLE_H
