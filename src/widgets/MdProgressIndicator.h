#ifndef MD_PROGRESS_INDICATOR_H
#define MD_PROGRESS_INDICATOR_H

// MdProgressIndicator — MD3 progress indicators, linear and circular, in the
// merged `md.comp.progress-indicator.*` family (export 34.0.21).
//
// One widget, two shapes (`ProgressIndicatorShape`), two modes:
//
//   determinate    value/max drives the active indicator; a value change
//                  transitions over the published durations (250 ms linear /
//                  500 ms circular, the material-web CSS transition rows)
//   indeterminate  the material-web animation, itself transplanted from MDC:
//                  linear = the 2 s two-bar translate+scale keyframes,
//                  circular = the three composed rotations (1.333 s arc
//                  expand, 4×1.333 s group rotate, linear spin)
//
// `fourColor` renders the deprecated per-shape sets' four-color cycle
// (primary → primary-container → tertiary → tertiary-container) — the only
// place those deprecated sets survive.
//
// A progress indicator is *not interactive*: the merged export publishes no
// state rows at all — no hover, no press, no focus indicator, no disabled
// row. Like MdBadge, the whole interaction contract is "nothing happens".
//
// One recorded gap: the export's wave rows (amplitude / wavelength and the
// with-wave container sizes) are published, non-deprecated Expressive tokens
// that neither material-web nor this port renders yet — see
// docs/porting-todo.md for the registered question.

#include "core/MdProgressIndicatorTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QWidget>

class QTimer;
class QVariantAnimation;

namespace md {

class MdProgressIndicatorStyle;

class QT_MD3_EXPORT MdProgressIndicator : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(md::ProgressIndicatorShape shape READ shape WRITE setShape NOTIFY shapeChanged)
    Q_PROPERTY(bool indeterminate READ isIndeterminate WRITE setIndeterminate NOTIFY
                   indeterminateChanged)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(qreal max READ max WRITE setMax NOTIFY maxChanged)
    Q_PROPERTY(qreal buffer READ buffer WRITE setBuffer NOTIFY bufferChanged)
    Q_PROPERTY(bool fourColor READ isFourColor WRITE setFourColor NOTIFY fourColorChanged)

public:
    explicit MdProgressIndicator(QWidget *parent = nullptr);
    explicit MdProgressIndicator(ProgressIndicatorShape shape, QWidget *parent = nullptr);
    ~MdProgressIndicator() override;

    // --- shape / mode -------------------------------------------------------
    ProgressIndicatorShape shape() const { return m_shape; }
    void setShape(ProgressIndicatorShape shape);

    bool isIndeterminate() const { return m_indeterminate; }
    void setIndeterminate(bool indeterminate);

    // --- value model ----------------------------------------------------------
    /// The displayed fraction, `value / max` clamped to 0..1 — Compose's
    /// contract; material-web's CSS would happily scale past 100 %.
    qreal fraction() const;
    qreal value() const { return m_value; }
    void setValue(qreal value);

    qreal max() const { return m_max; }
    void setMax(qreal max);

    /// Linear only: the buffer fraction's numerator. `0` (default) hides the
    /// buffer entirely — the material-web contract (`buffer <= 0` means off).
    qreal buffer() const { return m_buffer; }
    void setBuffer(qreal buffer);

    /// Render the indeterminate cycle in the deprecated four-color set
    /// instead of the single active-indicator colour.
    bool isFourColor() const { return m_fourColor; }
    void setFourColor(bool fourColor);

    /// The determinate fraction the *painting* currently shows — the animated
    /// display value, not `value / max`.
    qreal displayFraction() const { return m_displayFraction; }

    /// Milliseconds since this indicator was created — the clock the
    /// indeterminate cycles and the buffer dots run on.
    qint64 animationElapsedMs() const { return m_clock.elapsed(); }

    // --- tokens -------------------------------------------------------------
    const MdProgressIndicatorTokens &tokens() const;

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        updateAnimationState();
        update();
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -------------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void shapeChanged(md::ProgressIndicatorShape shape);
    void indeterminateChanged(bool indeterminate);
    void valueChanged(qreal value);
    void maxChanged(qreal max);
    void bufferChanged(qreal buffer);
    void fourColorChanged(bool fourColor);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void onThemeChanged();
    void onAnimationFrame();

private:
    void init();
    void invalidateTokens();
    /// Restart/stop the repaint timer: it runs while the widget has something
    /// animated to show (indeterminate, or the buffer dots region).
    void updateAnimationState();
    /// Snap the determinate display fraction to `value / max`.
    void snapDisplayFraction();
    /// Transition the determinate display fraction to `value / max` over the
    /// shape's published duration.
    void transitionDisplayFraction();

    ProgressIndicatorShape m_shape = ProgressIndicatorShape::Linear;
    bool m_indeterminate = false;
    qreal m_value = 0.0;
    qreal m_max = 1.0;
    qreal m_buffer = 0.0;
    bool m_fourColor = false;

    mutable MdProgressIndicatorTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    QElapsedTimer m_clock;
    QTimer *m_timer = nullptr;
    QVariantAnimation *m_transition = nullptr;
    qreal m_displayFraction = 0.0;
};

} // namespace md

#endif // MD_PROGRESS_INDICATOR_H
