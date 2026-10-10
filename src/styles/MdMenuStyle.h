#ifndef MD_MENU_STYLE_H
#define MD_MENU_STYLE_H

// MdMenuStyle — the paint of the menu surface and its items.
//
// The **surface** paints the `surface-container` corner-extra-small rounded
// rect with its level-2 shadow and, while opening or closing, wraps the whole
// content (itself and its children, through `render`) in Compose's
// scale+alpha pair around the anchor corner.
//
// The **item** paints, in order:
//
//   1. the selected container (`secondary-container`; the unselected item is
//      transparent over the surface);
//   2. the `on-surface` state layer (hover/focus flat; the press is the
//      ripple's, coloured with the pressed row);
//   3. the content — the 24 px leading icon, the label-large text, the 24 px
//      trailing icon (a submenu's cascading indicator rides this slot);
//   4. the **inward** secondary focus ring (the export's focus-indicator
//      rows point at the system inner offset).

#include "MdStyleBase.h"
#include "core/MdFocusRing.h"
#include "core/MdMenuTokens.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdMenu;
class MdMenuItem;

/// Pattern A styles for `MdMenu` and `MdMenuItem`.
class QT_MD3_EXPORT MdMenuStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdMenuStyle(QObject *parent = nullptr);

    static MdMenuStyle *shared();
    static bool isInstalled();

    /// The focus-indicator spec for a menu item: an *inward* ring.
    static MdFocusRingSpec focusRingSpec(const MdMenuTokens &tokens);

    /// The state an item is painted in. `Disabled` first, matching Compose's
    /// `!enabled -> ...` ordering.
    static MdNavigationItemState stateFor(const MdMenuItem &item);

    static void paintMenuItem(QPainter &painter, const MdMenuItem &item,
                              const MdMenuTokens &tokens);
    static void paintMenu(QPainter &painter, MdMenu &menu, const MdMenuTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_MENU_STYLE_H
