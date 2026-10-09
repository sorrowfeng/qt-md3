#ifndef MD_DIVIDER_STYLE_H
#define MD_DIVIDER_STYLE_H

#include "core/MdDividerTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>

class QPainter;

namespace md {

class MdDivider;

/// Pattern A style for `MdDivider`: painting and geometry for the
/// `md.comp.divider.*` family, registered in the paint hub so the first
/// `MdDivider` construction installs it application-wide.
///
/// Alongside the badge style, the quietest style in the library: the family
/// publishes no state rows at all, so there is no state layer, no ripple, no
/// focus indicator and no disabled form — one hairline rectangle, drawn
/// device-pixel aligned so the 1px token thickness stays crisp.
class QT_MD3_EXPORT MdDividerStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdDividerStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use:
    /// the first call registers the paint filter, later calls are free.
    static MdDividerStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    struct Layout
    {
        /// The painted hairline, in widget coordinates (before device-pixel
        /// snapping).
        QRectF line;
    };

    static Layout layoutFor(const MdDivider &divider, const MdDividerTokens &tokens);

    /// Snap a rect onto whole device pixels, outward. Shared with the test
    /// suite, which pins the crispness contract.
    static QRectF snappedToDevicePixels(const QRectF &rect, qreal devicePixelRatio);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite the same way the
    /// button families' are.
    static void paintDivider(QPainter &painter, const MdDivider &divider,
                             const MdDividerTokens &tokens, const Layout &layout);
};

} // namespace md

#endif // MD_DIVIDER_STYLE_H
