#ifndef MD_CARD_STYLE_H
#define MD_CARD_STYLE_H

#include "core/MdCardTokens.h"
#include "core/MdFocusRing.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;

namespace md {

class MdCard;

/// Pattern A style for `MdCard`: painting and geometry for the
/// `md.comp.<variant>-card.*` family, registered in the paint hub so the
/// first `MdCard` construction installs it application-wide.
///
/// Cards are surfaces, not controls: the visual centre of gravity is the
/// elevation ladder (filled level0 → hover level1 → dragged level3; elevated
/// one step up; outlined flat with a stroke) plus the state layers the
/// export publishes for hover / focus / dragged. Press never paints a flat
/// layer — it is the ripple, exactly as the button families treat it.
class QT_MD3_EXPORT MdCardStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdCardStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use:
    /// the first call registers the paint filter, later calls are free.
    static MdCardStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    /// The focus indicator spec the token set implies (thickness and offset
    /// from `md-sys-state-focus-indicator`).
    static MdFocusRingSpec focusRingSpec(const MdCardTokens &tokens);

    /// The margin a *clickable* card reserves around the container so the
    /// outward focus indicator is not clipped. Non-clickable cards publish
    /// no ring, so they reserve nothing.
    static qreal focusRingInset(const MdCardTokens &tokens);

    /// The interaction state painting should use. Disabled requires the
    /// card to be clickable (Compose only applies the disabled colours to
    /// the clickable overload); a non-clickable card always paints the
    /// enabled row.
    static MdCardState stateFor(const MdCard &card);

    struct Layout
    {
        /// The painted container, in widget coordinates: the widget rect
        /// minus the focus margin (zero for a non-clickable card).
        QRectF container;
        /// Corner radii, TL / TR / BR / BL — static, this family has no
        /// press shape morph.
        QList<qreal> radii;
    };

    static Layout layoutFor(const MdCard &card, const MdCardTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite the same way the
    /// button families' are.
    static void paintCard(QPainter &painter, const MdCard &card, const MdCardTokens &tokens,
                          const Layout &layout);
};

} // namespace md

#endif // MD_CARD_STYLE_H
