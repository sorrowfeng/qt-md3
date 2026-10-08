#ifndef MD_BADGE_STYLE_H
#define MD_BADGE_STYLE_H

#include "core/MdBadgeTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtCore/QSizeF>

class QPainter;

namespace md {

class MdBadge;

/// Pattern A style for `MdBadge`: painting and geometry for the
/// `md.comp.badge.*` family, registered in the paint hub so the first
/// `MdBadge` construction installs it application-wide.
///
/// The quietest style in the library, on purpose: the badge family publishes
/// no state rows at all, so there is no state layer, no ripple, no focus
/// indicator and no disabled form here — a container and, in the content
/// form, one label-small text. Non-interactivity is the contract (see
/// MdBadge's header comment).
class QT_MD3_EXPORT MdBadgeStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdBadgeStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use:
    /// the first call registers the paint filter, later calls are free.
    static MdBadgeStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    struct Layout
    {
        /// The painted container (circle for the dot, pill otherwise), in
        /// widget coordinates.
        QRectF container;
        /// The corner-full radius — half the shorter container side.
        qreal radius = 0.0;
        /// The label box; invalid for the dot form.
        QRectF label;
        /// What sizeHint() reports.
        QSizeF preferredSize;
    };

    static Layout layoutFor(const MdBadge &badge, const MdBadgeTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite the same way the
    /// button families' are.
    static void paintBadge(QPainter &painter, const MdBadge &badge, const MdBadgeTokens &tokens,
                           const Layout &layout);
};

} // namespace md

#endif // MD_BADGE_STYLE_H
