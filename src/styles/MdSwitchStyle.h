#ifndef MD_SWITCH_STYLE_H
#define MD_SWITCH_STYLE_H

// MdSwitchStyle — the paint of one switch.
//
// Five layers, in paint order:
//
//   1. the **track** — the 52×32 corner-full fill with its 2 px outline (the
//      outline is absent when selected — Compose's default checked border is
//      transparent).
//   2. the **state layer riding the thumb** — the hover/focus flat circle at
//      the thumb's centre (the press is the ripple's).
//   3. the **ripple** — the pressed row's colour inside the 40 px circle at
//      the thumb's centre (the bounded stand-in for Compose's fixed-radius
//      unbounded circle on the thumb).
//   4. the **handle** — the animated-diameter circle with its level-1 shadow
//      (level 0 disabled), then the 16 px thumb icon in the icon row's colour.
//   5. the **focus ring** — outward around the *track*, secondary, the
//      system gap and thickness.

#include "MdStyleBase.h"
#include "core/MdFocusRing.h"
#include "core/MdSwitchTokens.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdSwitch;

/// Pattern A style for `MdSwitch`.
class QT_MD3_EXPORT MdSwitchStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdSwitchStyle(QObject *parent = nullptr);

    static MdSwitchStyle *shared();
    static bool isInstalled();

    /// The focus-indicator spec for the switch: an *outward* ring around the
    /// track (corner-full).
    static MdFocusRingSpec focusRingSpec(const MdSwitchTokens &tokens);

    /// The state the switch is painted in. `Disabled` first, matching Compose's
    /// `!enabled -> ...` ordering.
    static MdNavigationItemState stateFor(const MdSwitch &sw);

    static void paintSwitch(QPainter &painter, const MdSwitch &sw, const MdSwitchTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_SWITCH_STYLE_H
