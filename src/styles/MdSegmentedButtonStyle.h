#ifndef MD_SEGMENTED_BUTTON_STYLE_H
#define MD_SEGMENTED_BUTTON_STYLE_H

#include "MdStyleBase.h"
#include "core/MdFocusRing.h"
#include "core/MdSegmentedButtonTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>
#include <QtCore/QStringList>

class QPainter;

namespace md {

class MdSegmentedButton;

/// Painter for MdSegmentedButton (Pattern A).
///
/// Geometry is the family's own trick: the segments overlap by exactly the
/// outline width, so each segment paints its *full* outline and the shared
/// edges stack into the 1 px divider. Position-dependent corner subsets come
/// from Compose `itemShape`: first segment rounds inline-start, last rounds
/// inline-end, middle segments are rectangles.
class QT_MD3_EXPORT MdSegmentedButtonStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdSegmentedButtonStyle(QObject *parent = nullptr);

    static MdSegmentedButtonStyle *shared();
    static bool isInstalled();

    /// The shared focus-indicator values (secondary, 3 px, offset 2).
    static MdFocusRingSpec focusRingSpec(const MdSegmentedButtonTokens &tokens);
    static qreal focusRingInset(const MdSegmentedButtonTokens &tokens);

    struct SegmentLayout
    {
        QRectF rect;
        /// TL / TR / BR / BL, position-dependent (pill end / rectangle).
        QList<qreal> radii;
        QRectF iconSlot;  ///< the reserved check/custom icon box
        QRectF label;
        bool hasLabel = false;
    };

    struct Layout
    {
        qreal inset = 0.0;
        /// Total content width: the segments' union, overlap-adjusted.
        qreal contentWidth = 0.0;
        QVector<SegmentLayout> segments;
    };

    static Layout layoutFor(const MdSegmentedButton &button,
                            const MdSegmentedButtonTokens &tokens);

    static void paintSegmentedButton(QPainter &painter,
                                     const MdSegmentedButton &button,
                                     const MdSegmentedButtonTokens &tokens,
                                     const Layout &layout);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_SEGMENTED_BUTTON_STYLE_H
