#ifndef MD_FAB_STYLE_H
#define MD_FAB_STYLE_H

#include "core/MdFabTokens.h"
#include "core/MdFocusRing.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;

namespace md {

class MdFab;

/// Pattern A style for `MdFab`: painting and geometry for the
/// `md.comp.fab.*` family, registered in the paint hub so the first `MdFab`
/// construction installs it application-wide.
///
/// One lookup note: the paint hub resolves a widget's filter by walking up its
/// superclasses, and `MdFab` is a `QPushButton` — *not* an `MdButton` — so
/// this filter is found before any fallback and the common button's style
/// never paints a FAB.
class QT_MD3_EXPORT MdFabStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdFabStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use:
    /// the first call registers the paint filter, later calls are free.
    static MdFabStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    /// The focus indicator spec the token set implies (thickness and offset
    /// from `md-sys-state-focus-indicator`, the rest from the
    /// `md.comp.focus-ring` module defaults).
    static MdFocusRingSpec focusRingSpec(const MdFabTokens &tokens);

    /// The margin the widget reserves around the container so the outward
    /// focus indicator is not clipped — the same derivation the button
    /// families make, from the same indicator tokens.
    static qreal focusRingInset(const MdFabTokens &tokens);

    /// The interaction state painting should use.
    static MdFabState stateFor(const MdFab &fab);

    struct Layout
    {
        /// The painted container, in widget coordinates.
        QRectF container;
        /// Corner radii, TL / TR / BR / BL. Static — this family has no press
        /// shape morph.
        QList<qreal> radii;
        /// The icon box: the token icon size, centred in the container.
        QRectF icon;
        /// What sizeHint() reports: container plus the focus margin.
        QSizeF preferredSize;
    };

    static Layout layoutFor(const MdFab &fab, const MdFabTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite the same way the
    /// button families' are.
    static void paintFab(QPainter &painter, const MdFab &fab, const MdFabTokens &tokens,
                         const Layout &layout);
};

} // namespace md

#endif // MD_FAB_STYLE_H
