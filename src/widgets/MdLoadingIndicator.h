#ifndef MD_LOADING_INDICATOR_H
#define MD_LOADING_INDICATOR_H

// MdLoadingIndicator — the M3 Expressive loading indicator, the
// `md.comp.loading-indicator.*` family (export 34.0.21).
//
// The export is token-only — material-web ships the token rows but no web
// component — so the behaviour port is Compose M3 Expressive's
// LoadingIndicator (androidx-main), like Badge and SegmentedButton before
// it. One widget, two variants (`LoadingIndicatorVariant`) and two modes:
//
//   plain      the morphing shape alone, in active-indicator.color
//   contained  the shape inside the corner-full container disc, in
//              contained.active-indicator.color on
//              contained.container.color
//
//   indeterminate  the shape morphs through the seven Material shapes
//                  (soft-burst → 9-cookie → pentagon → pill → sunny →
//                  4-cookie → oval → soft-burst) on the 650 ms grid, each
//                  morph on the published spring, while the whole thing
//                  spins: quarter-turn steps per morph on top of the
//                  4666 ms linear global rotation
//   determinate    progress walks the morph sequence (circle → soft-burst)
//                  and sweeps a counter-clockwise half turn
//
// A loading indicator is *not interactive*: the export publishes no state
// rows at all — no hover, no press, no focus indicator, no disabled row.
// Like MdBadge and MdProgressIndicator, the whole interaction contract is
// "nothing happens".
//
// The deprecated `container.color` row (secondary-container) is transcribed
// in the tokens but never read — the contained variant's distinct colour
// mapping superseded it.

#include "core/MdLoadingIndicatorTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdLoadingIndicatorStyle;

class QT_MD3_EXPORT MdLoadingIndicator : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(md::LoadingIndicatorVariant variant READ variant WRITE setVariant NOTIFY
                   variantChanged)
    Q_PROPERTY(bool indeterminate READ isIndeterminate WRITE setIndeterminate NOTIFY
                   indeterminateChanged)
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)

public:
    explicit MdLoadingIndicator(QWidget *parent = nullptr);
    explicit MdLoadingIndicator(LoadingIndicatorVariant variant, QWidget *parent = nullptr);
    ~MdLoadingIndicator() override;

    // --- variant / mode -----------------------------------------------------
    LoadingIndicatorVariant variant() const { return m_variant; }
    void setVariant(LoadingIndicatorVariant variant);

    bool isIndeterminate() const { return m_indeterminate; }
    void setIndeterminate(bool indeterminate);

    /// The determinate progress, clamped to 0..1 (Compose coerces too).
    qreal progress() const { return m_progress; }
    void setProgress(qreal progress);

    /// Master switch for the indeterminate cycle; determinate painting is
    /// stateless and ignores this.
    bool isRunning() const { return m_running; }
    void setRunning(bool running);

    /// True while the indeterminate repaint cycle is active — running, in
    /// indeterminate mode, and visible. The observable of the animation
    /// state (offscreen platforms never deliver the paint events, so tests
    /// cannot count repaints).
    bool isAnimating() const { return m_timer && m_timer->isActive(); }

    /// Milliseconds since this indicator was created — the clock the
    /// indeterminate morph/spin cycle runs on.
    qint64 animationElapsedMs() const { return m_clock.elapsed(); }

    // --- tokens -------------------------------------------------------------
    const MdLoadingIndicatorTokens &tokens() const;

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        updateAnimationState();
        update();
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -----------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void variantChanged(md::LoadingIndicatorVariant variant);
    void indeterminateChanged(bool indeterminate);
    void progressChanged(qreal progress);
    void runningChanged(bool running);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void onThemeChanged();
    void onAnimationFrame();

private:
    void init();
    void invalidateTokens();
    /// Run the repaint timer only while an indeterminate cycle is showing.
    void updateAnimationState();

    LoadingIndicatorVariant m_variant = LoadingIndicatorVariant::Plain;
    bool m_indeterminate = true;
    qreal m_progress = 0.0;
    bool m_running = true;

    mutable MdLoadingIndicatorTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    QElapsedTimer m_clock;
    QTimer *m_timer = nullptr;
};

} // namespace md

#endif // MD_LOADING_INDICATOR_H
