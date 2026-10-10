#ifndef MD_SLIDER_STYLE_H
#define MD_SLIDER_STYLE_H

// MdSliderStyle — the paint of one slider.
//
// Layers, in paint order:
//
//   1. the **inactive track** — the full-width pill in `secondary-container`
//      (on-surface @ 0.12 disabled).
//   2. the **active track** — the pill from the track's start to the handle's
//      centre in `primary` (on-surface @ 0.38 disabled). In range form it
//      spans the two handles.
//   3. the **tick marks** (optional) — the deprecated with-tick-marks family's
//      2 px dots along the track at every step, active/inactive colours at
//      their 0.38 opacities; the **stop indicators** at the track's ends.
//   4. the **handle** — the vertical pill (per-state width, 44/68/108 px tall)
//      with its level-1 shadow (level 0 disabled).
//   5. the **value indicator** (optional) — the inverse-surface label scaling
//      in above the handle while hovered / focused / pressed.
//   6. the **focus ring** — outward around the handle's 40 px state-layer
//      circle, secondary, the system gap and thickness, keyboard focus only.

#include "MdStyleBase.h"
#include "core/MdFocusRing.h"
#include "core/MdSliderTokens.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdSlider;

/// Pattern A style for `MdSlider`.
class QT_MD3_EXPORT MdSliderStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdSliderStyle(QObject *parent = nullptr);

    static MdSliderStyle *shared();
    static bool isInstalled();

    /// The focus-indicator spec: an outward ring around the handle's state
    /// layer circle.
    static MdFocusRingSpec focusRingSpec(const MdSliderTokens &tokens);

    static void paintSlider(QPainter &painter, const MdSlider &slider,
                            const MdSliderTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_SLIDER_STYLE_H
