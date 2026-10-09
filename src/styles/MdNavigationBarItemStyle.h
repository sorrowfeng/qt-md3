#ifndef MD_NAVIGATION_BAR_ITEM_STYLE_H
#define MD_NAVIGATION_BAR_ITEM_STYLE_H

// MdNavigationBarItemStyle — the paint of one navigation destination.
//
// The whole component is: a pill, an icon, and a label that sits under the icon
// or inside the pill. What makes it worth its own style rather than a few lines
// in the bar's is **where the interaction feedback goes**:
//
// Compose selects the entire item but attaches the indication to the
// *indicator*, re-mapping pointer coordinates with a `MappedInteractionSource`
// so that a press in the far corner of a wide item still ripples from the
// pill's centre. Qt has no such re-mapping, but it does have clip paths, and
// the two produce the same picture: the ripple is drawn in the pill's local
// coordinates against a clip path that is the pill's own outline, so nothing
// escapes it. The press position itself is translated in
// `MdNavigationBarItem::mousePressEvent`.
//
// The focus indicator is the second thing this file decides. The export puts
// the ring **inside** the component — `focus-indicator-outline-offset` is
// `md-sys.state.focus-indicator.inner-offset` — and on a navigation item the
// component the ring belongs to is the *pill*, not the item, because Compose
// hands the ring's shape to the indicator's own `ripple(focusRingShape = ...)`.
// So the ring is drawn at the pill's bounds with `MdFocusRingSpec::inward` set
// and an inward offset of 0, and the item reserves no room outside its rect.
// That is why `MdNavigationBarItem::sizeHint()` is its own size and not
// `container + 2 * margin` the way the button families are.

#include "MdStyleBase.h"
#include "core/MdFocusRing.h"
#include "core/MdNavigationBarTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>

class QPainter;
class QWidget;

namespace md {

class MdNavigationBarItem;

/// Pattern A style for `MdNavigationBarItem`.
class QT_MD3_EXPORT MdNavigationBarItemStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdNavigationBarItemStyle(QObject *parent = nullptr);

    static MdNavigationBarItemStyle *shared();
    static bool isInstalled();

    /// The focus-indicator spec for this family: an *inward* ring at the pill's
    /// bounds. `MdFocusRing`'s own defaults are the outward variant the buttons
    /// use, so this is the field that differs.
    static MdFocusRingSpec focusRingSpec(const MdNavigationBarVariantTokens &tokens);

    /// The state the item is painted in. `Disabled` first, matching Compose's
    /// `!enabled -> ... -> selected` ordering.
    static MdNavigationItemState stateFor(const MdNavigationBarItem &item);

    static void paintNavigationBarItem(QPainter &painter, const MdNavigationBarItem &item,
                                       const MdNavigationBarVariantTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_NAVIGATION_BAR_ITEM_STYLE_H
