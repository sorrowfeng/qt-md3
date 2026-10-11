#ifndef MD_EXPRESSIVE_MENU_STYLE_H
#define MD_EXPRESSIVE_MENU_STYLE_H

// MdExpressiveMenuStyle — the paint of the Expressive menu surface and its
// items (`md.comp.menus.*`).
//
// The **surface** paints the variant's container colour with the level-2
// shadow, at the layout's corner: `container.shape` (corner-large) in the
// column, `horizontal.container.shape` (corner-full) in the bar.
//
// The **item** paints, in order:
//
//   1. the item container at its per-position shape (selected / first / last
//      → corner-medium, middle → corner-extra-small, pressed → corner-large
//      in the column; the `horizontal.menu-item.*.shape` rows in the bar);
//   2. the variant's state layer (hover / keyboard focus flat; the press is
//      the ripple's);
//   3. the content — the 20 px leading icon, the label (label-large), the
//      optional body-small supporting line and the trailing icon / trailing
//      supporting text, inset by the layout's item spaces;
//   4. the **inward** secondary focus ring, keyboard-only.

#include "MdStyleBase.h"
#include "core/MdExpressiveMenuTokens.h"
#include "core/MdFocusRing.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdExpressiveMenu;
class MdExpressiveMenuItem;

/// Pattern A styles for `MdExpressiveMenu` and `MdExpressiveMenuItem`.
class QT_MD3_EXPORT MdExpressiveMenuStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdExpressiveMenuStyle(QObject *parent = nullptr);

    static MdExpressiveMenuStyle *shared();
    static bool isInstalled();

    /// The focus-indicator spec: an *inward* ring (the export's
    /// `focus.indicator.outline.offset` is the system inner offset).
    static MdFocusRingSpec focusRingSpec(const MdExpressiveMenuTokens &tokens);

    /// The state an item is painted in. `Disabled` first, matching Compose's
    /// `!enabled -> ...` ordering.
    static MdNavigationItemState stateFor(const MdExpressiveMenuItem &item);

    static void paintMenuItem(QPainter &painter, const MdExpressiveMenuItem &item,
                              const MdExpressiveMenuTokens &tokens);
    static void paintMenu(QPainter &painter, MdExpressiveMenu &menu,
                          const MdExpressiveMenuTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_EXPRESSIVE_MENU_STYLE_H
