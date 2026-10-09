#ifndef MD_NAVIGATION_DRAWER_ITEM_STYLE_H
#define MD_NAVIGATION_DRAWER_ITEM_STYLE_H

// MdNavigationDrawerItemStyle — the paint of `MdNavigationDrawerItem`.
//
// The bar item's paint without the animation: the pill is the item's own
// rect, it does not grow, and the label is left-aligned inside the
// `weight(1f)` box instead of fading in under the icon. The state
// precedence, the ripple-as-pressed-layer rule and the inward focus ring are
// the family's shared rules — see MdNavigationBarItemStyle for the full
// story.

#include "MdStyleBase.h"
#include "core/MdFocusRing.h"
#include "core/MdNavigationDrawerTokens.h"
#include "core/MdNavigationBarTokens.h"
#include "core/QtMd3Export.h"

class QPainter;

namespace md {

class MdNavigationDrawerItem;

/// Pattern A style for `MdNavigationDrawerItem`.
class QT_MD3_EXPORT MdNavigationDrawerItemStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdNavigationDrawerItemStyle(QObject *parent = nullptr);

    static MdNavigationDrawerItemStyle *shared();
    static bool isInstalled();

    /// The ring draws **inside** the pill on the export's `inner-offset` —
    /// this family publishes its own `focus-indicator-*` rows.
    static MdFocusRingSpec focusRingSpec(const MdNavigationDrawerItemTokens &tokens);

    /// Disabled wins over pressed, pressed over focus, focus over hover — the
    /// shared precedence.
    static MdNavigationItemState stateFor(const MdNavigationDrawerItem &item);

    static void paintNavigationDrawerItem(QPainter &painter, const MdNavigationDrawerItem &item,
                                          const MdNavigationDrawerItemTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_NAVIGATION_DRAWER_ITEM_STYLE_H
