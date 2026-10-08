#ifndef MD_EXTENDED_FAB_STYLE_H
#define MD_EXTENDED_FAB_STYLE_H

#include "core/MdExtendedFabTokens.h"
#include "core/MdFocusRing.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtGui/QFont>
#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;

namespace md {

class MdExtendedFab;

/// Pattern A style for `MdExtendedFab`: painting and geometry for the
/// `md.comp.extended-fab.*` family, registered in the paint hub so the first
/// `MdExtendedFab` construction installs it application-wide.
///
/// One lookup note: the paint hub resolves a widget's filter by walking up
/// its superclasses, and `MdExtendedFab` is a `QPushButton` — *not* an
/// `MdButton` — so this filter is found before any fallback and the common
/// button's style never paints an extended FAB.
class QT_MD3_EXPORT MdExtendedFabStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdExtendedFabStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use:
    /// the first call registers the paint filter, later calls are free.
    static MdExtendedFabStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    /// The focus indicator spec the token set implies (thickness and offset
    /// from `md-sys-state-focus-indicator`, the rest from the
    /// `md.comp.focus-ring` module defaults).
    static MdFocusRingSpec focusRingSpec(const MdExtendedFabTokens &tokens);

    /// The margin the widget reserves around the container so the outward
    /// focus indicator is not clipped — the same derivation the button
    /// families make, from the same indicator tokens.
    static qreal focusRingInset(const MdExtendedFabTokens &tokens);

    /// The interaction state painting should use.
    static MdFabState stateFor(const MdExtendedFab &fab);

    struct Layout
    {
        /// The painted container, in widget coordinates.
        QRectF container;
        /// Corner radii, TL / TR / BR / BL. Static — this family has no press
        /// shape morph.
        QList<qreal> radii;
        /// The icon box, at the container's vertical centre.
        QRectF icon;
        /// The label box, after `icon-label-space`.
        QRectF label;
        /// The type-scale font the size set publishes for the label.
        QFont labelFont;
        /// What sizeHint() reports: container plus the focus margin.
        QSizeF preferredSize;
    };

    static Layout layoutFor(const MdExtendedFab &fab, const MdExtendedFabTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite the same way the
    /// button families' are.
    static void paintExtendedFab(QPainter &painter, const MdExtendedFab &fab,
                                 const MdExtendedFabTokens &tokens, const Layout &layout);
};

} // namespace md

#endif // MD_EXTENDED_FAB_STYLE_H
