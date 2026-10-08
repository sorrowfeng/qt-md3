#ifndef MD_FAB_MENU_STYLE_H
#define MD_FAB_MENU_STYLE_H

#include "core/MdFabMenuTokens.h"
#include "core/MdFabTokens.h"
#include "core/MdFocusRing.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtGui/QFont>
#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;

namespace md {

class MdFabMenuItem;

/// Pattern A style for `MdFabMenuItem`: painting and geometry for the
/// `md.comp.fab-menu.*` elements — the close button and the list items,
/// which share one paint path with different token rows.
class QT_MD3_EXPORT MdFabMenuStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdFabMenuStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdFabMenuStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    /// The focus indicator spec (the shared md-sys-state-focus-indicator
    /// fallback — this family publishes no indicator rows of its own).
    static MdFocusRingSpec focusRingSpec(const MdFabMenuTokens &tokens);

    /// The margin the widget reserves around the container so the outward
    /// focus indicator is not clipped.
    static qreal focusRingInset(const MdFabMenuTokens &tokens);

    /// The interaction state painting should use.
    static MdFabState stateFor(const MdFabMenuItem &item);

    struct Layout
    {
        QRectF container;
        QList<qreal> radii;
        QRectF icon;
        QRectF label;
        QFont labelFont;
        QSizeF preferredSize;
    };

    static Layout layoutFor(const MdFabMenuItem &item, const MdFabMenuElementTokens &element);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite. `reveal` drives
    /// the staggered open animation (opacity + downward offset); 1.0 paints
    /// the resting element.
    static void paintItem(QPainter &painter, const MdFabMenuItem &item,
                          const MdFabMenuElementTokens &element, const Layout &layout,
                          qreal reveal, qreal revealOffsetY);
};

} // namespace md

#endif // MD_FAB_MENU_STYLE_H
