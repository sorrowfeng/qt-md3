#ifndef MD_SPLIT_BUTTON_STYLE_H
#define MD_SPLIT_BUTTON_STYLE_H

#include "MdStyleBase.h"
#include "core/MdButtonTokens.h"
#include "core/MdFocusRing.h"
#include "core/MdSplitButtonTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>
#include <QtCore/QString>

class QPainter;

#include "widgets/MdSplitButton.h"

namespace md {

class MdSplitButton;

/// Painter for MdSplitButton (Pattern A). The split button borrows the button
/// family's colour rows, so the state-layer / ripple / focus rules are the
/// same ones MdButtonStyle implements — including the three
/// interaction-fidelity rules: `:focus-visible`-only focus ring, ripple
/// coloured by the pressed row's state-layer colour, and no flat pressed
/// layer (the ripple *is* the press response).
class QT_MD3_EXPORT MdSplitButtonStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdSplitButtonStyle(QObject *parent = nullptr);

    /// Install-on-first-use accessor, same contract as the other families.
    static MdSplitButtonStyle *shared();
    static bool isInstalled();

    /// The button family's focus-indicator spec (the split-button export
    /// publishes none).
    static MdFocusRingSpec focusRingSpec(const MdButtonTokens &tokens);
    /// Focus margin on one side: `offset + activeWidth/2 + width/2`.
    static qreal focusRingInset(const MdButtonTokens &tokens);

    /// The button-state row painting uses for one half.
    static MdButtonState stateFor(const MdSplitButton &button, MdSplitButton::Zone zone);

    struct Layout
    {
        /// Focus margin; the container sits this far inside the widget.
        qreal inset = 0.0;
        /// The whole split: both halves plus the between-space.
        QRectF container;
        /// The halves, leading = inline-start (swaps under RTL).
        QRectF leading;
        QRectF trailing;
        /// Per-corner radii in TL / TR / BR / BL order, outer corners at the
        /// container height's half, facing corners at the animated inner
        /// radius. Already RTL-adjusted.
        QList<qreal> leadingRadii;
        QList<qreal> trailingRadii;

        bool hasLabel = false;
        bool hasLeadingIcon = false;
        QRectF label;
        QRectF leadingIcon;   ///< the inline-start icon box
        QRectF trailingIcon;  ///< the dropdown icon box (trailing half)
        QString startIconName;
        QString endIconName;
    };

    static Layout layoutFor(const MdSplitButton &button, const MdSplitButtonTokens &tokens);

    static void paintSplitButton(QPainter &painter,
                                 const MdSplitButton &button,
                                 const MdSplitButtonTokens &tokens,
                                 const Layout &layout);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_SPLIT_BUTTON_STYLE_H
