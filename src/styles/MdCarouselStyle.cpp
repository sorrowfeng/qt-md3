#include "MdCarouselStyle.h"

#include "core/MdElevation.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdCarousel.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace md {

namespace {

using Keyline = MdCarouselStyle::Keyline;
using Arrangement = MdCarouselStyle::Arrangement;

qreal lerp(qreal a, qreal b, qreal t)
{
    return a + (b - a) * t;
}

Keyline lerpKeyline(const Keyline &from, const Keyline &to, qreal t)
{
    Keyline out;
    out.size = lerp(from.size, to.size, t);
    out.offset = lerp(from.offset, to.offset, t);
    out.unadjustedOffset = lerp(from.unadjustedOffset, to.unadjustedOffset, t);
    out.cutoff = lerp(from.cutoff, to.cutoff, t);
    out.isFocal = from.isFocal;
    out.isAnchor = from.isAnchor;
    out.isPivot = from.isPivot;
    return out;
}

// --- KeylineList.kt's index helpers -----------------------------------------

int firstNonAnchorIndex(const QList<Keyline> &keylines)
{
    for (int i = 0; i < keylines.size(); ++i) {
        if (!keylines[i].isAnchor) {
            return i;
        }
    }
    return -1;
}

int lastNonAnchorIndex(const QList<Keyline> &keylines)
{
    for (int i = keylines.size() - 1; i >= 0; --i) {
        if (!keylines[i].isAnchor) {
            return i;
        }
    }
    return -1;
}

int firstFocalIndex(const QList<Keyline> &keylines)
{
    for (int i = 0; i < keylines.size(); ++i) {
        if (keylines[i].isFocal) {
            return i;
        }
    }
    return -1;
}

int lastFocalIndex(const QList<Keyline> &keylines)
{
    for (int i = keylines.size() - 1; i >= 0; --i) {
        if (keylines[i].isFocal) {
            return i;
        }
    }
    return -1;
}

/// `firstIndexAfterFocalRangeWithSize`: the first index after the focal
/// range whose size matches, or `lastIndex` when none does.
int firstIndexAfterFocalRangeWithSize(const QList<Keyline> &keylines, qreal size)
{
    const int lastIndex = keylines.size() - 1;
    for (int i = lastFocalIndex(keylines) + 1; i < keylines.size(); ++i) {
        if (keylines[i].size == size) {
            return i;
        }
    }
    return lastIndex;
}

/// `lastIndexBeforeFocalRangeWithSize`: the last index before the focal
/// range whose size matches, or 0 when none does.
int lastIndexBeforeFocalRangeWithSize(const QList<Keyline> &keylines, qreal size)
{
    for (int i = firstFocalIndex(keylines) - 1; i >= 0; --i) {
        if (keylines[i].size == size) {
            return i;
        }
    }
    return 0;
}

bool isFirstFocalItemAtStartOfContainer(const QList<Keyline> &keylines)
{
    const int index = firstFocalIndex(keylines);
    if (index < 0) {
        return false;
    }
    const Keyline &k = keylines[index];
    return k.offset - k.size / 2.0 <= 0.0;
}

bool isLastFocalItemAtEndOfContainer(const QList<Keyline> &keylines, qreal mainAxisSize)
{
    const int index = lastFocalIndex(keylines);
    if (index < 0) {
        return false;
    }
    const Keyline &k = keylines[index];
    return k.offset + k.size / 2.0 >= mainAxisSize;
}

/// `Strategy.kt`'s `moveKeylineAndCreateShiftedKeylineList`: moves the
/// keyline at `srcIndex` to `dstIndex` and rebuilds the offsets around the
/// shifted pivot.
QList<Keyline> moveKeylineAndShift(const QList<Keyline> &from, int srcIndex, int dstIndex,
                                   qreal carouselMainAxisSize, qreal itemSpacing)
{
    const qreal pivotDir = srcIndex > dstIndex ? 1.0 : -1.0;
    int pivotIndex = -1;
    for (int i = 0; i < from.size(); ++i) {
        if (from[i].isPivot) {
            pivotIndex = i;
            break;
        }
    }
    const qreal pivotDelta = (from[srcIndex].size - from[srcIndex].cutoff + itemSpacing) * pivotDir;
    const int newPivotIndex = pivotIndex + int(pivotDir);
    const qreal newPivotOffset = from[pivotIndex].offset + pivotDelta;

    QList<QPair<qreal, bool>> sizesAnchors;
    sizesAnchors.reserve(from.size());
    for (const Keyline &k : from) {
        sizesAnchors.append({k.size, k.isAnchor});
    }
    QList<QPair<qreal, bool>> moved = sizesAnchors;
    const QPair<qreal, bool> item = moved.takeAt(srcIndex);
    moved.insert(dstIndex, item);

    return MdCarouselStyle::keylineListFromSizes(moved, newPivotIndex, newPivotOffset,
                                                 carouselMainAxisSize, itemSpacing);
}

/// `Strategy.kt`'s `getStepInterpolationPoints` for the start direction.
QList<qreal> startShiftPoints(qreal totalShiftDistance, const QList<QList<Keyline>> &steps)
{
    QList<qreal> points = {0.0};
    if (totalShiftDistance == 0.0 || steps.isEmpty()) {
        return points;
    }
    for (int i = 1; i < steps.size(); ++i) {
        const qreal distanceShifted = steps[i].first().unadjustedOffset
            - steps[i - 1].first().unadjustedOffset;
        const qreal stepPercentage = distanceShifted / totalShiftDistance;
        const qreal point = i == steps.size() - 1 ? 1.0 : points[i - 1] + stepPercentage;
        points.append(point);
    }
    return points;
}

/// `Strategy.kt`'s `getStepInterpolationPoints` for the end direction.
QList<qreal> endShiftPoints(qreal totalShiftDistance, const QList<QList<Keyline>> &steps)
{
    QList<qreal> points = {0.0};
    if (totalShiftDistance == 0.0 || steps.isEmpty()) {
        return points;
    }
    for (int i = 1; i < steps.size(); ++i) {
        const qreal distanceShifted = steps[i - 1].last().unadjustedOffset
            - steps[i].last().unadjustedOffset;
        const qreal stepPercentage = distanceShifted / totalShiftDistance;
        const qreal point = i == steps.size() - 1 ? 1.0 : points[i - 1] + stepPercentage;
        points.append(point);
    }
    return points;
}

QList<Keyline> lerpKeylineLists(const QList<Keyline> &from, const QList<Keyline> &to, qreal t)
{
    QList<Keyline> out;
    out.reserve(from.size());
    for (int i = 0; i < from.size(); ++i) {
        out.append(lerpKeyline(from[i], to[i], t));
    }
    return out;
}

} // namespace

// --- Arrangement ------------------------------------------------------------

MdCarouselStyle *MdCarouselStyle::shared()
{
    static QMutex mutex;
    static MdCarouselStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdCarouselStyle;
        installPaintFilter<MdCarousel>(instance);
    }
    return instance;
}

bool MdCarouselStyle::isInstalled()
{
    return hasPaintFilter(&MdCarousel::staticMetaObject);
}

MdCarouselStyle::MdCarouselStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

bool Arrangement::isValid() const
{
    if (largeCount > 0 && smallCount > 0 && mediumCount > 0) {
        return largeSize > mediumSize && mediumSize > smallSize;
    }
    if (largeCount > 0 && smallCount > 0) {
        return largeSize > smallSize;
    }
    return true;
}

qreal Arrangement::cost(qreal targetLargeSize) const
{
    if (!isValid()) {
        return std::numeric_limits<qreal>::max();
    }
    return std::abs(targetLargeSize - largeSize) * priority;
}

qreal MdCarouselStyle::calculateLargeSize(qreal availableSpace, int smallCount, qreal smallSize,
                                          int mediumCount, int largeCount)
{
    return (availableSpace - (qreal(smallCount) + qreal(mediumCount) / 2.0) * smallSize)
        / (qreal(largeCount) + qreal(mediumCount) / 2.0);
}

MdCarouselStyle::Arrangement MdCarouselStyle::fit(int priority, qreal availableSpace,
                                                  qreal itemSpacing, int smallCount,
                                                  qreal smallSize, qreal minSmallSize,
                                                  qreal maxSmallSize, int mediumCount,
                                                  qreal mediumSize, int largeCount,
                                                  qreal largeSize)
{
    const int totalItemCount = largeCount + mediumCount + smallCount;
    const qreal availableSpaceWithoutSpacing =
        availableSpace - qreal(totalItemCount - 1) * itemSpacing;
    qreal arrangedSmallSize = std::clamp(smallSize, minSmallSize, maxSmallSize);
    qreal arrangedMediumSize = mediumSize;
    qreal arrangedLargeSize = largeSize;

    const qreal totalSpaceTakenByArrangement = arrangedLargeSize * largeCount
        + arrangedMediumSize * mediumCount + arrangedSmallSize * smallCount;
    const qreal delta = availableSpaceWithoutSpacing - totalSpaceTakenByArrangement;

    // First, resize the small items within their min-max range to fit.
    if (smallCount > 0 && delta > 0.0) {
        arrangedSmallSize += std::min(delta / smallCount, maxSmallSize - arrangedSmallSize);
    } else if (smallCount > 0 && delta < 0.0) {
        arrangedSmallSize += std::max(delta / smallCount, minSmallSize - arrangedSmallSize);
    }
    if (smallCount == 0) {
        arrangedSmallSize = 0.0;
    }

    // Then solve the large size exactly.
    arrangedLargeSize = calculateLargeSize(availableSpaceWithoutSpacing, smallCount,
                                           arrangedSmallSize, mediumCount, largeCount);
    arrangedMediumSize = (arrangedLargeSize + arrangedSmallSize) / 2.0;

    // The medium flex absorbs what solving the large size could not.
    if (mediumCount > 0 && arrangedLargeSize != largeSize) {
        const qreal targetAdjustment = (largeSize - arrangedLargeSize) * largeCount;
        const qreal availableMediumFlex =
            arrangedMediumSize * 0.1 * mediumCount; // MediumItemFlexPercentage
        const qreal distribute = std::min(std::abs(targetAdjustment), availableMediumFlex);
        if (targetAdjustment > 0.0) {
            arrangedMediumSize -= distribute / mediumCount;
            arrangedLargeSize += distribute / largeCount;
        } else {
            arrangedMediumSize += distribute / mediumCount;
            arrangedLargeSize -= distribute / largeCount;
        }
    }

    Arrangement out;
    out.priority = priority;
    out.smallSize = arrangedSmallSize;
    out.smallCount = smallCount;
    out.mediumSize = arrangedMediumSize;
    out.mediumCount = mediumCount;
    out.largeSize = arrangedLargeSize;
    out.largeCount = largeCount;
    return out;
}

bool MdCarouselStyle::findLowestCostArrangement(qreal availableSpace, qreal itemSpacing,
                                                qreal targetSmallSize, qreal minSmallSize,
                                                qreal maxSmallSize, const QList<int> &smallCounts,
                                                qreal targetMediumSize,
                                                const QList<int> &mediumCounts,
                                                qreal targetLargeSize,
                                                const QList<int> &largeCounts, Arrangement *out)
{
    bool found = false;
    int priority = 1;
    std::optional<Arrangement> lowest;
    for (const int largeCount : largeCounts) {
        for (const int mediumCount : mediumCounts) {
            for (const int smallCount : smallCounts) {
                const Arrangement arrangement =
                    fit(priority, availableSpace, itemSpacing, smallCount, targetSmallSize,
                        minSmallSize, maxSmallSize, mediumCount, targetMediumSize, largeCount,
                        targetLargeSize);
                if (!lowest.has_value()
                    || arrangement.cost(targetLargeSize) < lowest->cost(targetLargeSize)) {
                    lowest = arrangement;
                    if (lowest->cost(targetLargeSize) == 0.0) {
                        // Cost 0: no size was altered and the permutations are
                        // generated in priority order, so nothing can beat this.
                        *out = *lowest;
                        return true;
                    }
                }
                ++priority;
            }
        }
    }
    if (lowest.has_value()) {
        *out = *lowest;
        found = true;
    }
    return found;
}

// --- keyline construction ----------------------------------------------------

QList<MdCarouselStyle::Keyline> MdCarouselStyle::keylineListFromSizes(
    const QList<QPair<qreal, bool>> &sizesAnchors, int pivotIndex, qreal pivotOffset,
    qreal carouselMainAxisSize, qreal itemSpacing)
{
    QList<Keyline> keylines;
    if (sizesAnchors.isEmpty() || pivotIndex < 0 || pivotIndex >= sizesAnchors.size()) {
        return keylines;
    }

    // The focal range is the run of the largest non-anchor sizes.
    qreal focalSize = 0.0;
    int firstFocal = -1;
    for (int i = 0; i < sizesAnchors.size(); ++i) {
        const qreal size = sizesAnchors[i].first;
        const bool isAnchor = sizesAnchors[i].second;
        if (!isAnchor && size > focalSize) {
            firstFocal = i;
            focalSize = size;
        }
    }
    int lastFocal = firstFocal;
    while (lastFocal + 1 < sizesAnchors.size() && !sizesAnchors[lastFocal + 1].second
           && sizesAnchors[lastFocal + 1].first == focalSize) {
        ++lastFocal;
    }

    const auto isCutoffLeft = [&](qreal size, qreal offset) {
        return offset - size / 2.0 < 0.0;
    };
    const auto isCutoffRight = [&](qreal size, qreal offset) {
        return offset + size / 2.0 > carouselMainAxisSize;
    };

    // The pivot keyline first.
    {
        const qreal size = sizesAnchors[pivotIndex].first;
        const bool isAnchor = sizesAnchors[pivotIndex].second;
        qreal cutoff = 0.0;
        if (isCutoffLeft(size, pivotOffset)) {
            cutoff = pivotOffset - size / 2.0;
        } else if (isCutoffRight(size, pivotOffset)) {
            cutoff = (pivotOffset + size / 2.0) - carouselMainAxisSize;
        }
        Keyline k;
        k.size = size;
        k.offset = pivotOffset;
        k.unadjustedOffset = pivotOffset;
        k.cutoff = cutoff;
        k.isFocal = pivotIndex >= firstFocal && pivotIndex <= lastFocal;
        k.isAnchor = isAnchor;
        k.isPivot = true;
        keylines.append(k);
    }

    // Slots before the pivot: offsets accumulate by the slot's own size,
    // unadjusted offsets by the focal (large) size.
    qreal offset = pivotOffset - focalSize / 2.0 - itemSpacing;
    qreal unadjustedOffset = pivotOffset - focalSize / 2.0 - itemSpacing;
    for (int i = pivotIndex - 1; i >= 0; --i) {
        const qreal size = sizesAnchors[i].first;
        const bool isAnchor = sizesAnchors[i].second;
        const qreal slotOffset = offset - size / 2.0;
        const qreal slotUnadjusted = unadjustedOffset - focalSize / 2.0;
        Keyline k;
        k.size = size;
        k.offset = slotOffset;
        k.unadjustedOffset = slotUnadjusted;
        k.cutoff = isCutoffLeft(size, slotOffset) ? std::abs(slotOffset - size / 2.0) : 0.0;
        k.isFocal = i >= firstFocal && i <= lastFocal;
        k.isAnchor = isAnchor;
        k.isPivot = false;
        keylines.prepend(k);

        offset -= size + itemSpacing;
        unadjustedOffset -= focalSize + itemSpacing;
    }

    // Slots after the pivot, mirrored.
    offset = pivotOffset + focalSize / 2.0 + itemSpacing;
    unadjustedOffset = pivotOffset + focalSize / 2.0 + itemSpacing;
    for (int i = pivotIndex + 1; i < sizesAnchors.size(); ++i) {
        const qreal size = sizesAnchors[i].first;
        const bool isAnchor = sizesAnchors[i].second;
        const qreal slotOffset = offset + size / 2.0;
        const qreal slotUnadjusted = unadjustedOffset + focalSize / 2.0;
        Keyline k;
        k.size = size;
        k.offset = slotOffset;
        k.unadjustedOffset = slotUnadjusted;
        k.cutoff = isCutoffRight(size, slotOffset) ? (slotOffset + size / 2.0)
                                                        - carouselMainAxisSize
                                                   : 0.0;
        k.isFocal = i >= firstFocal && i <= lastFocal;
        k.isAnchor = isAnchor;
        k.isPivot = false;
        keylines.append(k);

        offset += size + itemSpacing;
        unadjustedOffset += focalSize + itemSpacing;
    }

    return keylines;
}

QList<MdCarouselStyle::Keyline> MdCarouselStyle::multiBrowseKeylineList(
    qreal carouselMainAxisSize, qreal preferredItemSize, qreal itemSpacing, int itemCount,
    qreal minSmallItemSize, qreal maxSmallItemSize, qreal anchorSize)
{
    if (carouselMainAxisSize == 0.0 || preferredItemSize == 0.0) {
        return {};
    }

    QList<int> smallCounts = {1};
    const QList<int> mediumCounts = {1, 0};

    const qreal targetLargeSize = std::min(preferredItemSize, carouselMainAxisSize);
    // Ideally a small item is 1/3 the large size; clamp into the min-max
    // range as close to that third as possible.
    const qreal targetSmallSize =
        std::clamp(targetLargeSize / 3.0, minSmallItemSize, maxSmallItemSize);
    const qreal targetMediumSize = (targetLargeSize + targetSmallSize) / 2.0;

    if (carouselMainAxisSize < minSmallItemSize * 2.0) {
        // Too small to fit a large item and a small item: allow arrangements
        // with no small items.
        smallCounts = {0};
    }

    // Reserve the minimum space for the large items after filling the
    // carousel with the most permissible medium and small items.
    int maxMediumCount = 0;
    for (const int count : mediumCounts) {
        maxMediumCount = std::max(maxMediumCount, count);
    }
    int maxSmallCount = 0;
    for (const int count : smallCounts) {
        maxSmallCount = std::max(maxSmallCount, count);
    }
    const qreal minAvailableLargeSpace = carouselMainAxisSize
        - targetMediumSize * maxMediumCount - maxSmallItemSize * maxSmallCount;
    const int minLargeCount =
        std::max(1, int(std::floor(minAvailableLargeSpace / targetLargeSize)));
    const int maxLargeCount = int(std::ceil(carouselMainAxisSize / targetLargeSize));

    QList<int> largeCounts;
    for (int count = maxLargeCount; count >= minLargeCount; --count) {
        largeCounts.append(count);
    }

    Arrangement arrangement;
    if (!findLowestCostArrangement(carouselMainAxisSize, itemSpacing, targetSmallSize,
                                   minSmallItemSize, maxSmallItemSize, smallCounts,
                                   targetMediumSize, mediumCounts, targetLargeSize, largeCounts,
                                   &arrangement)) {
        return {};
    }

    if (arrangement.itemCount() > itemCount) {
        int keylineSurplus = arrangement.itemCount() - itemCount;
        int smallCount = arrangement.smallCount;
        int mediumCount = arrangement.mediumCount;
        while (keylineSurplus > 0) {
            if (smallCount > 0) {
                --smallCount;
            } else if (mediumCount > 1) {
                // Keep at least 1 medium so the large items don't fill the
                // entire carousel.
                --mediumCount;
            }
            --keylineSurplus;
        }
        if (!findLowestCostArrangement(carouselMainAxisSize, itemSpacing, targetSmallSize,
                                       minSmallItemSize, maxSmallItemSize, {smallCount},
                                       targetMediumSize, {mediumCount}, targetLargeSize,
                                       largeCounts, &arrangement)) {
            return {};
        }
    }

    // `createLeftAlignedKeylineList`: the anchor, the arrangement's slots,
    // the anchor. The pivot is the first focal item at the Start alignment.
    QList<QPair<qreal, bool>> sizesAnchors;
    sizesAnchors.append({anchorSize, true});
    for (int i = 0; i < arrangement.largeCount; ++i) {
        sizesAnchors.append({arrangement.largeSize, false});
    }
    for (int i = 0; i < arrangement.mediumCount; ++i) {
        sizesAnchors.append({arrangement.mediumSize, false});
    }
    for (int i = 0; i < arrangement.smallCount; ++i) {
        sizesAnchors.append({arrangement.smallSize, false});
    }
    sizesAnchors.append({anchorSize, true});

    // The pivot offset for the Start alignment: the focal item's centre.
    qreal focalSize = 0.0;
    int pivotIndex = -1;
    for (int i = 0; i < sizesAnchors.size(); ++i) {
        if (!sizesAnchors[i].second && sizesAnchors[i].first > focalSize) {
            focalSize = sizesAnchors[i].first;
            pivotIndex = i;
        }
    }
    return keylineListFromSizes(sizesAnchors, pivotIndex, focalSize / 2.0, carouselMainAxisSize,
                                itemSpacing);
}

// --- scroll steps --------------------------------------------------------------

QList<QList<MdCarouselStyle::Keyline>> MdCarouselStyle::startKeylineSteps(
    const QList<Keyline> &defaultKeylines, qreal carouselMainAxisSize, qreal itemSpacing)
{
    QList<QList<Keyline>> steps;
    if (defaultKeylines.isEmpty()) {
        return steps;
    }
    steps.append(defaultKeylines);

    if (isFirstFocalItemAtStartOfContainer(defaultKeylines)) {
        return steps;
    }

    const int startIndex = firstNonAnchorIndex(defaultKeylines);
    const int endIndex = firstFocalIndex(defaultKeylines);
    const int numberOfSteps = endIndex - startIndex;

    if (numberOfSteps <= 0) {
        return steps;
    }

    int i = 0;
    while (i < numberOfSteps) {
        const QList<Keyline> &prevStep = steps.last();
        const int originalItemIndex = startIndex + i;
        int dstIndex = defaultKeylines.size() - 1;
        if (originalItemIndex > 0) {
            const qreal neighborBeforeSize = defaultKeylines[originalItemIndex - 1].size;
            dstIndex = firstIndexAfterFocalRangeWithSize(prevStep, neighborBeforeSize) - 1;
        }
        steps.append(moveKeylineAndShift(prevStep, firstNonAnchorIndex(defaultKeylines),
                                         dstIndex, carouselMainAxisSize, itemSpacing));
        ++i;
    }
    return steps;
}

QList<QList<MdCarouselStyle::Keyline>> MdCarouselStyle::endKeylineSteps(
    const QList<Keyline> &defaultKeylines, qreal carouselMainAxisSize, qreal itemSpacing)
{
    QList<QList<Keyline>> steps;
    if (defaultKeylines.isEmpty()) {
        return steps;
    }
    steps.append(defaultKeylines);

    if (isLastFocalItemAtEndOfContainer(defaultKeylines, carouselMainAxisSize)) {
        return steps;
    }

    const int startIndex = lastFocalIndex(defaultKeylines);
    const int endIndex = lastNonAnchorIndex(defaultKeylines);
    const int numberOfSteps = endIndex - startIndex;

    if (numberOfSteps <= 0) {
        return steps;
    }

    int i = 0;
    while (i < numberOfSteps) {
        const QList<Keyline> &prevStep = steps.last();
        const int originalItemIndex = endIndex - i;
        int dstIndex = 0;
        if (originalItemIndex < defaultKeylines.size() - 1) {
            const qreal neighborAfterSize = defaultKeylines[originalItemIndex + 1].size;
            dstIndex = lastIndexBeforeFocalRangeWithSize(prevStep, neighborAfterSize) + 1;
        }
        steps.append(moveKeylineAndShift(prevStep, lastNonAnchorIndex(defaultKeylines), dstIndex,
                                         carouselMainAxisSize, itemSpacing));
        ++i;
    }
    return steps;
}

MdCarouselStyle::Keyline MdCarouselStyle::keylineBefore(const QList<Keyline> &keylines,
                                                        qreal unadjustedOffset)
{
    for (int i = keylines.size() - 1; i >= 0; --i) {
        if (keylines[i].unadjustedOffset < unadjustedOffset) {
            return keylines[i];
        }
    }
    return keylines.first();
}

MdCarouselStyle::Keyline MdCarouselStyle::keylineAfter(const QList<Keyline> &keylines,
                                                       qreal unadjustedOffset)
{
    for (const Keyline &k : keylines) {
        if (k.unadjustedOffset >= unadjustedOffset) {
            return k;
        }
    }
    return keylines.last();
}

MdCarouselStyle::ItemMetrics MdCarouselStyle::itemMetrics(const QList<Keyline> &keylines,
                                                          qreal unadjustedCenter)
{
    const Keyline before = keylineBefore(keylines, unadjustedCenter);
    const Keyline after = keylineAfter(keylines, unadjustedCenter);

    const qreal delta = after.unadjustedOffset - before.unadjustedOffset;
    const qreal progress = delta == 0.0 ? 0.0
        : std::clamp((unadjustedCenter - before.unadjustedOffset) / delta, 0.0, 1.0);
    const Keyline interpolated = lerpKeyline(before, after, progress);
    const bool isOutOfKeylineBounds = before == after;

    ItemMetrics out;
    out.size = interpolated.size;
    out.translation = interpolated.offset - unadjustedCenter;
    if (isOutOfKeylineBounds) {
        // Beyond the first or last keyline: keep offsetting the item by
        // cutting its unadjusted offset according to its masked size.
        out.translation += (unadjustedCenter - interpolated.unadjustedOffset)
            / interpolated.size;
    }
    return out;
}

qreal MdCarouselStyle::maxScrollOffset(int itemCount, qreal largeSize, qreal itemSpacing,
                                       qreal availableSpace)
{
    const qreal maxScrollPossible =
        largeSize * qreal(itemCount) + itemSpacing * qreal(itemCount - 1);
    return std::max(0.0, maxScrollPossible - availableSpace);
}

QList<MdCarouselStyle::Keyline> MdCarouselStyle::keylineListForScrollOffset(
    const QList<Keyline> &defaultKeylines, const QList<QList<Keyline>> &startSteps,
    const QList<QList<Keyline>> &endSteps, qreal carouselMainAxisSize, qreal itemSpacing,
    qreal scrollOffset, qreal maxScrollOffset)
{
    Q_UNUSED(carouselMainAxisSize);
    Q_UNUSED(itemSpacing);
    if (defaultKeylines.isEmpty()) {
        return defaultKeylines;
    }

    const qreal positiveScrollOffset = std::max(0.0, scrollOffset);

    // The scroll distance needed to move through each step list.
    qreal startShiftDistance = 0.0;
    if (!startSteps.isEmpty()) {
        startShiftDistance = std::max(startSteps.last().first().unadjustedOffset
                                          - startSteps.first().first().unadjustedOffset,
                                      0.0);
    }
    qreal endShiftDistance = 0.0;
    if (!endSteps.isEmpty()) {
        endShiftDistance = std::max(endSteps.first().last().unadjustedOffset
                                        - endSteps.last().last().unadjustedOffset,
                                    0.0);
    }
    const qreal endShiftOffset = std::max(0.0, maxScrollOffset - endShiftDistance);

    if (positiveScrollOffset >= startShiftDistance && positiveScrollOffset <= endShiftOffset) {
        return defaultKeylines;
    }

    QList<qreal> shiftPoints;
    const QList<QList<Keyline>> *steps = nullptr;
    qreal interpolation = 0.0;
    if (positiveScrollOffset < startShiftDistance) {
        // Interpolation runs 1 -> 0 across the start steps.
        interpolation = startShiftDistance == 0.0
            ? 1.0
            : 1.0 - positiveScrollOffset / startShiftDistance;
        steps = &startSteps;
        shiftPoints = startShiftPoints(startShiftDistance, startSteps);
    } else {
        interpolation = maxScrollOffset - endShiftOffset == 0.0
            ? 1.0
            : (positiveScrollOffset - endShiftOffset) / (maxScrollOffset - endShiftOffset);
        steps = &endSteps;
        shiftPoints = endShiftPoints(endShiftDistance, endSteps);
    }

    int fromStepIndex = 0;
    int toStepIndex = 0;
    qreal steppedInterpolation = 0.0;
    qreal lowerBounds = shiftPoints.value(0, 0.0);
    for (int i = 1; i < steps->size(); ++i) {
        const qreal upperBounds = shiftPoints.value(i, 1.0);
        if (interpolation <= upperBounds || i == steps->size() - 1) {
            fromStepIndex = i - 1;
            toStepIndex = i;
            steppedInterpolation = upperBounds == lowerBounds
                ? 0.0
                : std::clamp((interpolation - lowerBounds) / (upperBounds - lowerBounds), 0.0,
                             1.0);
            break;
        }
        lowerBounds = upperBounds;
    }

    return lerpKeylineLists((*steps)[fromStepIndex], (*steps)[toStepIndex], steppedInterpolation);
}

// --- painting --------------------------------------------------------------------

void MdCarouselStyle::paintItem(QPainter &painter, const QRectF &rect, qreal maskedWidth,
                                bool hovered, bool focused, bool pressed, bool enabled,
                                const MdCarouselTokens &tokens)
{
    Q_UNUSED(maskedWidth);
    const MdTheme &theme = MdTheme::instance();

    painter.setRenderHint(QPainter::Antialiasing, true);

    const QList<qreal> radii = {tokens.containerShapeRadius, tokens.containerShapeRadius,
                                tokens.containerShapeRadius, tokens.containerShapeRadius};
    const QPainterPath path = MdShape::roundedRect(rect, radii);

    // The disabled composite: the container at 0.38 with the outline at 0.12.
    const qreal containerOpacity = enabled ? 1.0 : tokens.disabledContainerOpacity;

    // 1. The elevation shadow — level 0 in the resting rows, level 1 on
    //    hover, back to level 0 pressed (the "Pressed (ripple)" row).
    const qreal elevation =
        !enabled ? tokens.containerElevation
                 : (pressed ? tokens.pressedContainerElevation
                            : (hovered ? tokens.hoverContainerElevation
                                       : tokens.containerElevation));
    MdElevation::drawShadowDp(&painter, rect, tokens.containerShapeRadius, elevation,
                              theme.color(tokens.containerShadowColor));

    // 2. The container.
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme.color(tokens.containerColor));
    painter.setOpacity(containerOpacity);
    painter.drawPath(path);
    painter.setOpacity(1.0);

    // 3. The state layer — hover / keyboard focus / pressed rows. The
    //    pressed response is the state layer row itself, same as the
    //    ripple-carrying families.
    QColor layer = theme.color(tokens.stateLayerColor);
    if (enabled && pressed) {
        layer.setAlphaF(tokens.pressedStateLayerOpacity);
        painter.setPen(Qt::NoPen);
        painter.setBrush(layer);
        painter.drawPath(path);
    } else if (enabled && hovered) {
        layer.setAlphaF(tokens.hoverStateLayerOpacity);
        painter.setPen(Qt::NoPen);
        painter.setBrush(layer);
        painter.drawPath(path);
    } else if (enabled && focused) {
        layer.setAlphaF(tokens.focusStateLayerOpacity);
        painter.setPen(Qt::NoPen);
        painter.setBrush(layer);
        painter.drawPath(path);
    }

    // 4. The with-outline variant: the 1 px outline, dimmed to 0.12 when
    //    disabled.
    if (tokens.withOutline) {
        QPen pen(theme.color(tokens.outlineColor));
        pen.setWidthF(tokens.outlineWidth);
        if (!enabled) {
            QColor dimmed = pen.color();
            dimmed.setAlphaF(tokens.disabledOutlineOpacity);
            pen.setColor(dimmed);
        }
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
}

void MdCarouselStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *carousel = qobject_cast<MdCarousel *>(widget);
    if (!carousel) {
        return;
    }
    carousel->paintItems(*painter);
}

} // namespace md
