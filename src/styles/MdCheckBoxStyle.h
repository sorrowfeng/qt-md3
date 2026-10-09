#ifndef MD_CHECKBOX_STYLE_H
#define MD_CHECKBOX_STYLE_H

// MdCheckBoxStyle — the paint of one checkbox.
//
// Four layers, in paint order:
//
//   1. the **circular state layer** — a 40 px circle over the box, for hover
//      and keyboard focus. The press is the ripple's, as in the button
//      families ("按压不是平的状态层").
//   2. the **ripple** — Compose's unbounded circle of the state layer's
//      diameter, clipped to that circle. Its colour is the pressed row's:
//      `primary` unchecked, `on-surface` checked, `error` in the error variant
//      (the export's rows; Compose's default colours ripple transparent when
//      unchecked, which the porting-todo records).
//   3. the **box** — Compose's `drawBox`: when the fill and the border resolve
//      to the same colour (a checked box) it is one filled round rect;
//      otherwise the fill insets by the stroke width and the border strokes
//      around it. The check (or indeterminate dash) is the same path revealed
//      along its length and gravitated towards the centre line.
//   4. the **focus ring** — outward around the 18 px box, secondary, the gap
//      and thickness the export shares with the system focus indicator.

#include "MdStyleBase.h"
#include "core/MdCheckBoxTokens.h"
#include "core/MdFocusRing.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdCheckBox;

/// Pattern A style for `MdCheckBox`.
class QT_MD3_EXPORT MdCheckBoxStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdCheckBoxStyle(QObject *parent = nullptr);

    static MdCheckBoxStyle *shared();
    static bool isInstalled();

    /// The focus-indicator spec for the checkbox: an *outward* ring around the
    /// box, following the box's corner radii.
    static MdFocusRingSpec focusRingSpec(const MdCheckBoxTokens &tokens);

    /// The state the box is painted in. `Disabled` first, matching Compose's
    /// `!enabled -> ...` ordering.
    static MdNavigationItemState stateFor(const MdCheckBox &box);

    static void paintCheckBox(QPainter &painter,
                              const MdCheckBox &box,
                              const MdCheckBoxTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_CHECKBOX_STYLE_H
