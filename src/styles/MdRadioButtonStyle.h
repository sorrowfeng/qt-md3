#ifndef MD_RADIO_BUTTON_STYLE_H
#define MD_RADIO_BUTTON_STYLE_H

// MdRadioButtonStyle — the paint of one radio button.
//
// Four layers, in paint order:
//
//   1. the **circular state layer** — a 40 px circle over the icon, for hover
//      and keyboard focus. The press is the ripple's ("按压不是平的状态层").
//   2. the **ripple** — Compose's unbounded circle of a *fixed* 20 px radius
//      (StateLayerSize / 2), clipped to the 40 px layer's circle. Its colour is
//      the pressed row's: `primary` unselected, `on-surface` selected.
//   3. the **icon** — the stroke circle (`iconSize / 2 - strokeWidth / 2`
//      radius, stroked 2 px) and the dot (`animatedDotDiameter / 2 -
//      strokeWidth / 2`, filled), both in one colour faded between the two
//      selection tables.
//   4. the **focus ring** — a circle (Compose's `focusRingShape = CircleShape`)
//      around the 20 px icon, outward, the system gap and thickness.

#include "MdStyleBase.h"
#include "core/MdFocusRing.h"
#include "core/MdRadioButtonTokens.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdRadioButton;

/// Pattern A style for `MdRadioButton`.
class QT_MD3_EXPORT MdRadioButtonStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdRadioButtonStyle(QObject *parent = nullptr);

    static MdRadioButtonStyle *shared();
    static bool isInstalled();

    /// The focus-indicator spec for the radio button: an *outward* ring shaped
    /// to the icon's circle.
    static MdFocusRingSpec focusRingSpec(const MdRadioButtonTokens &tokens);

    /// The state the button is painted in. `Disabled` first, matching Compose's
    /// `!enabled -> ...` ordering.
    static MdNavigationItemState stateFor(const MdRadioButton &button);

    static void paintRadioButton(QPainter &painter,
                                 const MdRadioButton &button,
                                 const MdRadioButtonTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_RADIO_BUTTON_STYLE_H
