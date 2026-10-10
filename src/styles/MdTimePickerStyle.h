#ifndef MD_TIME_PICKER_STYLE_H
#define MD_TIME_PICKER_STYLE_H

// MdTimePickerStyle — the paint of one time picker.
//
// Layers, in paint order:
//
//   1. the **container** — surface-container-high at level 3 with
//      corner-extra-large.
//   2. the **headline** — "Select time" in label-medium, on-surface-variant.
//   3. the **time selectors** — two 96×80 corner-small boxes around the `:`
//      separator (display-large); the selected one is primary-container /
//      on-primary-container, the other surface-container-highest /
//      on-surface. In 24h mode a single 114 px selector.
//   4. the **period selector** — the 216×38 outlined corner-small split into
//      AM / PM halves; selected tertiary-container / on-tertiary-container.
//   5. the **clock dial** — the 256 px surface-container-highest circle with
//      the 2 px primary track from centre to the 48 px handle, the 8 px centre
//      dot and the hour / minute numbers around the rim.

#include "MdStyleBase.h"
#include "core/MdFocusRing.h"
#include "core/MdTimePickerTokens.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdTimePicker;

/// Pattern A style for `MdTimePicker`.
class QT_MD3_EXPORT MdTimePickerStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdTimePickerStyle(QObject *parent = nullptr);

    static MdTimePickerStyle *shared();
    static bool isInstalled();

    static void paintTimePicker(QPainter &painter, const MdTimePicker &picker,
                                const MdTimePickerTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_TIME_PICKER_STYLE_H
