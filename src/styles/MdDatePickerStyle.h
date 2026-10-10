#ifndef MD_DATE_PICKER_STYLE_H
#define MD_DATE_PICKER_STYLE_H

// MdDatePickerStyle — the paint of one date picker.
//
// Layers, in paint order:
//
//   1. the **container** — surface-container-high at level 3 with
//      corner-extra-large.
//   2. the **header** — the selected date's headline (headline-large) plus
//      the supporting text (label-large) that switches to the year face.
//   3. the **weekday row** — Mon..Sun in body-large, on-surface.
//   4. the **month subhead** — title-small, on-surface-variant.
//   5. the **calendar grid** — 40 px corner-full date cells: the selected
//      date fills primary / on-primary, today carries the 1 px primary
//      outline with a primary label, the rest are on-surface. In range form
//      the in-range days fill secondary-container with an
//      on-secondary-container label and the endpoints carry the
//      active-indicator pill.
//   6. the **year list** (the Years face) — 72×36 corner-full year cells;
//      selected primary / on-primary, unselected on-surface-variant.

#include "MdStyleBase.h"
#include "core/MdDatePickerTokens.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdDatePicker;

/// Pattern A style for `MdDatePicker`.
class QT_MD3_EXPORT MdDatePickerStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdDatePickerStyle(QObject *parent = nullptr);

    static MdDatePickerStyle *shared();
    static bool isInstalled();

    static void paintDatePicker(QPainter &painter, const MdDatePicker &picker,
                                const MdDatePickerTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_DATE_PICKER_STYLE_H
