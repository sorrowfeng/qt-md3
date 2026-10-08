#ifndef MD_ICON_BUTTON_STYLE_H
#define MD_ICON_BUTTON_STYLE_H

#include "core/MdFocusRing.h"
#include "core/MdIconButtonTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtCore/QList>

class QPainter;

namespace md {

class MdIconButton;

/// Pattern A style for `MdIconButton`: painting and geometry for the
/// `md.comp.icon-button.*` family, registered in the paint hub so the first
/// `MdIconButton` construction installs it application-wide.
///
/// One lookup note: the paint hub resolves a widget's filter by walking up its
/// superclasses, and `MdIconButton` is a `QPushButton` — *not* an `MdButton` —
/// so this filter is found before any fallback and the common button's style
/// never paints an icon button.
class QT_MD3_EXPORT MdIconButtonStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdIconButtonStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern MdButtonStyle uses: the
    /// first call registers the paint filter, later calls are free.
    static MdIconButtonStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    /// The focus indicator spec the token set implies (thickness and offset
    /// from `md.comp.icon-button.focus.indicator.*`, the rest from the
    /// `md.comp.focus-ring` module defaults).
    static MdFocusRingSpec focusRingSpec(const MdIconButtonTokens &tokens);

    /// The margin the widget reserves around the container so the outward
    /// focus indicator is not clipped — the same 7.5 px derivation the common
    /// button makes, from the same indicator tokens.
    static qreal focusRingInset(const MdIconButtonTokens &tokens);

    /// The interaction state painting should use.
    static MdIconButtonState stateFor(const MdIconButton &button);

    struct Layout
    {
        /// The painted container, in widget coordinates. Square at the default
        /// space track; wider or narrower on the published narrow/wide tracks.
        QRectF container;
        /// Corner radii after the press morph, TL / TR / BR / BL.
        QList<qreal> radii;
        /// The icon box: the token icon size, centred in the container.
        QRectF icon;
        /// What sizeHint() reports: container plus the focus margin.
        QSizeF preferredSize;
    };

    static Layout layoutFor(const MdIconButton &button, const MdIconButtonTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite the same way the
    /// common button's are.
    static void paintIconButton(QPainter &painter, const MdIconButton &button,
                                const MdIconButtonTokens &tokens, const Layout &layout);
};

} // namespace md

#endif // MD_ICON_BUTTON_STYLE_H
