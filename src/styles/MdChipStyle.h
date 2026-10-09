#ifndef MD_CHIP_STYLE_H
#define MD_CHIP_STYLE_H

// MdChipStyle — the paint of one chip, for all four families and both kinds.
//
// Five layers, in paint order:
//
//   1. the **container** — the flat chip is transparent with a 1 px outline
//      (gone entirely when a filter/input chip is selected); the elevated one
//      fills `surface-container-low` (a selected filter chip:
//      `secondary-container`) under a level-1 shadow that rises to level 2 on
//      hover and level 4 while dragged. Elevations change instantly, as they
//      do upstream.
//   2. the **state layer** — clipped to the chip's corner-small shape; the
//      pressed row carries the families' special cases (a filter chip's two
//      sides swap the pressed colour).
//   3. the **ripple** — bounded to the chip, coloured with the pressed row.
//   4. the **content** — the avatar circle (corner-full, 24 px) or the 18 px
//      leading icon, the weighted `label-large` label, the trailing icon; the
//      disabled content fades at the content's 0.38, the disabled elevated
//      container and flat outline at the container's 0.12.
//   5. the **focus ring** — outward around the chip, secondary, the system
//      outer offset and thickness.

#include "MdStyleBase.h"
#include "core/MdChipTokens.h"
#include "core/MdFocusRing.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdChip;

/// Pattern A style for `MdChip`.
class QT_MD3_EXPORT MdChipStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdChipStyle(QObject *parent = nullptr);

    static MdChipStyle *shared();
    static bool isInstalled();

    /// The focus-indicator spec: an *outward* ring around the chip.
    static MdFocusRingSpec focusRingSpec(const MdChipVariantTokens &tokens);

    /// The state the chip is painted in. `Disabled` wins, then the programmatic
    /// drag, then the interaction states.
    static MdChipInteraction stateFor(const MdChip &chip);

    /// The chip's corner radius from the tokens.
    static qreal cornerRadius(const MdChipVariantTokens &tokens);

    static void paintChip(QPainter &painter, const MdChip &chip, const MdChipVariantTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_CHIP_STYLE_H
