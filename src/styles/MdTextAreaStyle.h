#ifndef MD_TEXT_AREA_STYLE_H
#define MD_TEXT_AREA_STYLE_H

// MdTextAreaStyle — the paint of one multiline text field.
//
// The same four layers as `MdTextFieldStyle`, with the container wrapped
// around the whole multiline body and the active indicator under the last
// line:
//
//   1. the **container** — filled surface-container-highest with
//      corner-extra-small-top; outlined transparent with a 1 px outline.
//   2. the **active indicator** — the bottom band (1 px / 1 px / 2 px / error).
//   3. the **floating label** — body-large at rest, body-small floated.
//   4. the **supporting text** — body-small under the container.

#include "MdStyleBase.h"
#include "core/MdTextFieldTokens.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdTextArea;

/// Pattern A style for `MdTextArea`.
class QT_MD3_EXPORT MdTextAreaStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdTextAreaStyle(QObject *parent = nullptr);

    static MdTextAreaStyle *shared();
    static bool isInstalled();

    static void paintTextArea(QPainter &painter, const MdTextArea &area,
                              const MdTextFieldTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_TEXT_AREA_STYLE_H
