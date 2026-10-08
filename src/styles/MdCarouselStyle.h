#ifndef MD_CAROUSEL_STYLE_H
#define MD_CAROUSEL_STYLE_H

#include "MdStyleBase.h"
#include "core/MdCarouselTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;

namespace md
{

class MdCarousel;

/// Pattern A style for `MdCarousel`: painting, item geometry and the keyline
/// math for the `md.comp.carousel-item.*` family, registered in the paint hub
/// so the first `MdCarousel` construction installs it application-wide.
///
/// The keyline math is the port of the Compose M3 carousel package —
/// `Arrangement.kt`, `Keylines.kt`, `KeylineList.kt` and `Strategy.kt`
/// condensed into pure functions. The model, verbatim from the sources:
///
///   * An *arrangement* picks how many large / medium / small items fill the
///     container (the multi-browse strategy's `findLowestCostArrangement`):
///     the large size is solved exactly so the counts fit the available
///     space, the medium is `(large + small) / 2`, and the medium's ±10%
///     flex absorbs what adjusting the small items could not.
///   * A *keyline* is a slot: `size` is the item width when its centre sits
///     at `offset` (container coordinates), and `unadjustedOffset` is where
///     that centre lives in the end-to-end scrolling model (all items laid
///     out at the large size). An item scrolled to `u` between two keylines
///     lerps its size and position from them — that lerp IS the carousel's
///     signature resize-as-it-scrolls behaviour.
///   * *Steps* generate the keyline lists for the scroll extremes: each step
///     moves one end keyline across the focal range so every item passes
///     through the large slot as it scrolls. A scroll offset inside the
///     default range uses the default keylines; outside it interpolates
///     between adjacent steps.
class QT_MD3_EXPORT MdCarouselStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdCarouselStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdCarouselStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    // --- keyline model (pure) -------------------------------------------------

    /// One slot of the carousel layout.
    struct Keyline
    {
        qreal size = 0.0;             // the item width when centred at `offset`
        qreal offset = 0.0;           // the centre's container position
        qreal unadjustedOffset = 0.0; // the centre in the end-to-end scroll model
        qreal cutoff = 0.0;           // the bleed beyond the container bounds
        bool isFocal = false;
        bool isAnchor = false;
        bool isPivot = false;

        bool operator==(const Keyline &other) const
        {
            return size == other.size && offset == other.offset
                   && unadjustedOffset == other.unadjustedOffset && cutoff == other.cutoff
                   && isFocal == other.isFocal && isAnchor == other.isAnchor
                   && isPivot == other.isPivot;
        }
    };

    /// The counts and sizes one arrangement of items resolves to.
    struct Arrangement
    {
        int priority = 0;
        qreal smallSize = 0.0;
        int smallCount = 0;
        qreal mediumSize = 0.0;
        int mediumCount = 0;
        qreal largeSize = 0.0;
        int largeCount = 0;

        int itemCount() const { return smallCount + mediumCount + largeCount; }
        bool isValid() const;
        qreal cost(qreal targetLargeSize) const;
    };

    /// `Arrangement.kt`'s `findLowestCostArrangement`: walks the count
    /// permutations in priority order and returns the cheapest fit.
    static bool findLowestCostArrangement(qreal availableSpace, qreal itemSpacing,
                                          qreal targetSmallSize, qreal minSmallSize,
                                          qreal maxSmallSize, const QList<int> &smallCounts,
                                          qreal targetMediumSize, const QList<int> &mediumCounts,
                                          qreal targetLargeSize, const QList<int> &largeCounts,
                                          Arrangement *out);

    /// `Arrangement.kt`'s `fit`: adjusts the small items inside their min-max
    /// range, solves the large size exactly, and flexes the medium items.
    static Arrangement fit(int priority, qreal availableSpace, qreal itemSpacing, int smallCount,
                           qreal smallSize, qreal minSmallSize, qreal maxSmallSize,
                           int mediumCount, qreal mediumSize, int largeCount, qreal largeSize);

    /// `Arrangement.kt`'s `calculateLargeSize`: solves
    /// `available = large*l + ((large+small)/2)*m + small*s` for `large`.
    static qreal calculateLargeSize(qreal availableSpace, int smallCount, qreal smallSize,
                                    int mediumCount, int largeCount);

    // --- keyline list construction (pure) --------------------------------------

    /// The default keyline list for the multi-browse configuration —
    /// `Keylines.kt`'s `multiBrowseKeylineList`, including the small-count
    /// relaxation for tiny containers and the surplus trimming against the
    /// real item count.
    static QList<Keyline> multiBrowseKeylineList(qreal carouselMainAxisSize,
                                                 qreal preferredItemSize, qreal itemSpacing,
                                                 int itemCount, qreal minSmallItemSize,
                                                 qreal maxSmallItemSize, qreal anchorSize);

    /// Builds a keyline list from sizes via the pivot rule —
    /// `KeylineList.kt`'s `createKeylinesWithPivot` for a Start alignment:
    /// offsets accumulate by each slot's own size, unadjusted offsets by the
    /// focal (large) size.
    static QList<Keyline> keylineListFromSizes(const QList<QPair<qreal, bool>> &sizesAnchors,
                                               int pivotIndex, qreal pivotOffset,
                                               qreal carouselMainAxisSize, qreal itemSpacing);

    // --- scroll interpolation (pure) ---------------------------------------------

    /// The keyline lists used while scrolling: the default, then one step per
    /// non-anchor slot, each moving that slot across the focal range —
    /// `Strategy.kt`'s `getStartKeylineSteps`.
    static QList<QList<Keyline>> startKeylineSteps(const QList<Keyline> &defaultKeylines,
                                                   qreal carouselMainAxisSize, qreal itemSpacing);

    /// The mirror of `startKeylineSteps` for the scroll end — moves the last
    /// non-anchor slots back before the focal range.
    static QList<QList<Keyline>> endKeylineSteps(const QList<Keyline> &defaultKeylines,
                                                 qreal carouselMainAxisSize, qreal itemSpacing);

    /// `KeylineList.kt`'s `getKeylineBefore` / `getKeylineAfter`.
    static Keyline keylineBefore(const QList<Keyline> &keylines, qreal unadjustedOffset);
    static Keyline keylineAfter(const QList<Keyline> &keylines, qreal unadjustedOffset);

    /// `Carousel.kt`'s per-item interpolation: the item's size and container
    /// offset when its end-to-end centre is at `unadjustedCenter`.
    struct ItemMetrics
    {
        qreal size = 0.0;        // the masked (visible) item width
        qreal translation = 0.0; // the offset from the item's unclamped position
    };

    static ItemMetrics itemMetrics(const QList<Keyline> &keylines, qreal unadjustedCenter);

    /// `Carousel.kt`'s `calculateMaxScrollOffset`: the end-to-end extent the
    /// container cannot show at once.
    static qreal maxScrollOffset(int itemCount, qreal largeSize, qreal itemSpacing,
                                 qreal availableSpace);

    // --- geometry + painting ------------------------------------------------------

    /// The visible keyline list for a scroll offset — the σ piecewise
    /// interpolation of `Strategy.kt`'s `getKeylineListForScrollOffset`.
    static QList<Keyline> keylineListForScrollOffset(const QList<Keyline> &defaultKeylines,
                                                     const QList<QList<Keyline>> &startSteps,
                                                     const QList<QList<Keyline>> &endSteps,
                                                     qreal carouselMainAxisSize, qreal itemSpacing,
                                                     qreal scrollOffset, qreal maxScrollOffset);

    /// An item wrapper's paint: the surface container clipped to the
    /// extra-large corners, the state layer, the optional outline and the
    /// elevation shadow. `maskedWidth` is the item's visible width.
    static void paintItem(QPainter &painter, const QRectF &rect, qreal maskedWidth, bool hovered,
                          bool focused, bool pressed, bool enabled,
                          const MdCarouselTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_CAROUSEL_STYLE_H
