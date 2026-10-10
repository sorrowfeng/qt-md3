#ifndef MD_TEXT_FIELD_STYLE_H
#define MD_TEXT_FIELD_STYLE_H

// MdTextFieldStyle — the paint of one text field.
//
// Layers, in paint order:
//
//   1. the **container** — filled: surface-container-highest with
//      corner-extra-small-top; outlined: transparent with a 1 px outline and
//      corner-extra-small.
//   2. the **active indicator** — the bottom band (1 px resting / 1 px hover /
//      2 px focus / error), on-surface-variant → on-surface → primary → error.
//   3. the **floating label** — body-large at rest in the input line,
//      body-small floated on the container's top edge; on-surface-variant,
//      primary under focus, error when errored.
//   4. the **supporting text** — body-small under the container,
//      on-surface-variant (error when errored).

#include "MdStyleBase.h"
#include "core/MdFocusRing.h"
#include "core/MdTextFieldTokens.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdTextField;

/// Pattern A style for `MdTextField`.
class QT_MD3_EXPORT MdTextFieldStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdTextFieldStyle(QObject *parent = nullptr);

    static MdTextFieldStyle *shared();
    static bool isInstalled();

    static void paintTextField(QPainter &painter, const MdTextField &field,
                               const MdTextFieldTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_TEXT_FIELD_STYLE_H
