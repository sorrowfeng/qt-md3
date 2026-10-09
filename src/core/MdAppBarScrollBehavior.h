#ifndef MD_APP_BAR_SCROLL_BEHAVIOR_H
#define MD_APP_BAR_SCROLL_BEHAVIOR_H

// MdAppBarScrollBehavior — how a top app bar reacts to the content under it
// scrolling away.
//
// Transcribed from Compose, because this is behaviour and behaviour is not
// published as tokens:
//
//   androidx/androidx
//     compose/material3/.../AppBar.kt
//       TopAppBarState                 (the three offsets and the two fractions)
//       TopAppBarDefaults.pinnedScrollBehavior            (mode 1)
//       TopAppBarDefaults.enterAlwaysScrollBehavior       (mode 2)
//       TopAppBarDefaults.exitUntilCollapsedScrollBehavior(mode 3)
//       settleAppBar()                                    (the snap)
//
// Four facts worth keeping visible:
//
//   * **`collapsedFraction` is the whole state machine.** `heightOffset` runs
//     from 0 (fully expanded) down to `heightOffsetLimit` (fully collapsed) and
//     `collapsedFraction = heightOffset / heightOffsetLimit` is what every
//     painter reads. The limit is set from the *collapsible* row's height: a
//     two-row bar's icon/action row never collapses, so only the text row is
//     counted — `adjustHeightOffsetLimit` runs on that row alone.
//   * **`overlappedFraction` is not the same as `collapsedFraction`.** It is
//     derived from `contentOffset`, i.e. how far the *content* has scrolled
//     under the bar, and it is what a single-row bar's container colour reads.
//     That is why a pinned bar (which never changes height) still turns
//     `surface-container` as its content scrolls: `collapsedFraction` stays 0
//     while `overlappedFraction` reaches 1. Compose's threshold is
//     `> 0.01f`.
//   * **The port's bridge is explicit, not invented as protocol.** Compose
//     hooks the bar into `NestedScrollConnection`: an event is offered to the
//     bar (`onPreScroll`), then the scrollable consumes what is left, then the
//     bar sees the result (`onPostScroll`). Qt has no nested-scroll protocol,
//     so the two hooks collapse into one call — `consumeScroll(dy, atStart)`
//     returns the part of the delta the bar took, and the caller applies the
//     remainder to its viewport. All three modes are implemented in terms of
//     that single hook; the reduction is recorded in docs/porting-todo.md.
//   * **`followScrollBar` is deliberately not interception.** Mirroring a
//     `QScrollBar` into `contentOffset` is exact for the `Pinned` mode (the
//     scroll position *is* the offset) and it keeps `contentAtStart` honest,
//     but a bar that absorbs wheel delta needs a scroll area that is willing
//     to hand it over, which stock Qt does not have. The gap is recorded
//     rather than papered over with a guessed pixels-per-notch constant.

#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QObject>

class QScrollBar;
class QTimer;

namespace md {

/// The three published scroll behaviours.
///
/// m3.material.io describes them in prose rather than as tokens; Compose is
/// where the arithmetic lives.
enum class MdAppBarScrollMode
{
    /// `TopAppBarDefaults.pinnedScrollBehavior` — the bar holds its height and
    /// only its container colour reacts. Compose's `isPinned` branch also
    /// disables the drag-to-resize gesture.
    Pinned,
    /// `enterAlwaysScrollBehavior` — the bar takes the whole delta whenever
    /// its height can move, so the bar appears as soon as the user scrolls
    /// back even if the content is nowhere near its top.
    EnterAlways,
    /// `exitUntilCollapsedScrollBehavior` — the bar collapses on the first
    /// upward delta but only expands once the content has returned to its
    /// start. This is the behaviour the medium and large bars are documented
    /// with.
    ExitUntilCollapsed,
    Count,
};

class QT_MD3_EXPORT MdAppBarScrollBehavior : public QObject
{
    Q_OBJECT

    Q_PROPERTY(MdAppBarScrollMode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(qreal heightOffset READ heightOffset WRITE setHeightOffset NOTIFY changed)
    Q_PROPERTY(qreal heightOffsetLimit READ heightOffsetLimit WRITE setHeightOffsetLimit NOTIFY
                   changed)
    Q_PROPERTY(qreal contentOffset READ contentOffset WRITE setContentOffset NOTIFY changed)

public:
    explicit MdAppBarScrollBehavior(QObject *parent = nullptr);
    explicit MdAppBarScrollBehavior(MdAppBarScrollMode mode, QObject *parent = nullptr);
    ~MdAppBarScrollBehavior() override;

    MdAppBarScrollMode mode() const { return m_mode; }
    void setMode(MdAppBarScrollMode mode);

    /// `TopAppBarState.heightOffset` — 0 when fully expanded, down to
    /// `heightOffsetLimit` when fully collapsed. Coerced into that range on
    /// every write, exactly as Compose's setter does.
    qreal heightOffset() const { return m_heightOffset; }
    void setHeightOffset(qreal offset);

    /// `TopAppBarState.heightOffsetLimit` — the negative pixel limit the bar's
    /// height may shrink by. The bar itself sets this from the height of its
    /// collapsible row.
    qreal heightOffsetLimit() const { return m_heightOffsetLimit; }
    void setHeightOffsetLimit(qreal limit);

    /// `TopAppBarState.contentOffset` — the running total of the delta the
    /// *content* has scrolled (negative as the content moves up). Only
    /// `overlappedFraction` reads it.
    qreal contentOffset() const { return m_contentOffset; }
    void setContentOffset(qreal offset);

    /// `canScroll` — a caller-supplied veto, e.g. while the bar is not the
    /// one that owns the scroll.
    bool canScroll() const { return m_canScroll; }
    void setCanScroll(bool canScroll);

    /// `TopAppBarState.isScrollingContentAtStart`. Also maintained by
    /// `followScrollBar`. Only `ExitUntilCollapsed` consults it directly;
    /// `overlappedFraction` uses it for its special case.
    bool isScrollingContentAtStart() const { return m_contentAtStart; }
    void setScrollingContentAtStart(bool atStart);

    /// `heightOffset / heightOffsetLimit`, 0..1. 0 means fully expanded.
    qreal collapsedFraction() const;

    /// How much of the bar's area overlaps scrolled content, 0..1.
    qreal overlappedFraction() const;

    /// True when the bar is at one end and there is nothing to settle.
    /// Compose's guard is `collapsedFraction < 0.01f || collapsedFraction == 1f`.
    bool isSettled() const;

    /// Offer a scroll delta to the bar.
    ///
    /// `dy` follows Compose's sign convention: **negative** when the content
    /// moves up (the user drags up / scrolls down through the content), which
    /// is the collapsing direction; positive when it moves back down.
    ///
    /// Returns the part of `dy` the bar consumed, so the caller applies
    /// `dy - consumed` to its viewport. This single hook replaces Compose's
    /// `onPreScroll` + `onPostScroll` pair; see the header comment.
    qreal consumeScroll(qreal dy, bool contentAtStart = true);

    /// `consumeScroll`, discarding the return value.
    void scrollBy(qreal dy, bool contentAtStart = true);

    /// Jump straight to the nearer end, no animation.
    void snapNow();

    /// Animate to the nearer end. `settleAppBar` flings first and snaps
    /// after; the port has no velocity source, so it runs the snap half only,
    /// on the published spec — `TopAppBarDefaults.snapAnimationSpec` is
    /// `MotionSchemeKeyTokens.DefaultEffects`, the same spring the container
    /// colour rides.
    void settle();

    /// True while `settle()` is running.
    bool isSettling() const;

    /// Mirror a scroll bar into `contentOffset` and `contentAtStart`.
    ///
    /// Exact for `Pinned` — the scroll position *is* the offset — and honest
    /// for the other modes, whose height still needs a caller that hands the
    /// delta over via `consumeScroll()`. See the header comment.
    void followScrollBar(QScrollBar *bar);

signals:
    void modeChanged(MdAppBarScrollMode mode);
    /// Any of the offsets, a fraction, or the settling state changed.
    void changed();

private slots:
    void onSettleTick();

private:
    void onScrollBarValueChanged(int value);

    MdAppBarScrollMode m_mode = MdAppBarScrollMode::Pinned;
    qreal m_heightOffset = 0.0;
    qreal m_heightOffsetLimit = 0.0;
    qreal m_contentOffset = 0.0;
    bool m_canScroll = true;
    bool m_contentAtStart = true;

    QTimer *m_settleTimer = nullptr;
    /// Set while `settle()` is animating, so `changed()` can report it.
    bool m_settling = false;
    qreal m_settleFrom = 0.0;
    qreal m_settleTo = 0.0;
    qint64 m_settleElapsedMs = 0;
    QScrollBar *m_scrollBar = nullptr;
};

} // namespace md

#endif // MD_APP_BAR_SCROLL_BEHAVIOR_H
