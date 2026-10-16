#ifndef MD_STEPS_H
#define MD_STEPS_H

// MdSteps — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "用 shape 与连接线表达进度，参照 Progress
// indicator 语义" — shape + connecting lines that express progress, with
// Progress-indicator semantics.
//
// MD3 has no steps / stepper family. The control is the standard multi-step
// progress surface (checkout, wizard, install) built from three published
// families:
//
//   indicator   a shape-scale disc, the same identity mark MdAvatar and
//               MdEmptyState promote (ShapeCorner::Full). Completed steps
//               take `primary` / `on-primary`; the current step takes
//               `primary-container` / `on-primary-container` (the selected
//               reading); upcoming steps take `surface-container-highest` /
//               `on-surface-variant` (the skeleton / placeholder reading).
//   connector   a 2 px line in `outline-variant`, switching to `primary`
//               once the step before it is complete — the progress-indicator
//               track / active-indicator pair.
//   label       label-medium under the disc (or beside it, vertical),
//               `on-surface` for reached steps, `on-surface-variant` ahead.
//
// Orientation is horizontal or vertical. `currentStep` is 0-based and selects
// which step reads as "current": every index below it is complete, every
// index above is upcoming. Step count and labels are live.
//
// Clicking a step (when `readOnly` is false) emits `stepSelected`; keyboard
// Left/Right (horizontal) or Up/Down (vertical) walk the current step and
// Home/End jump to the ends. The widget is Tab-reachable and shows the
// standard focus ring on `:focus-visible`.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QStringList>
#include <QtWidgets/QWidget>

class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

namespace md {

enum class MdStepsOrientation {
    Horizontal,
    Vertical,
    Count,
};

class QT_MD3_EXPORT MdSteps : public QWidget
{
    Q_OBJECT

    /// Number of steps. Values below 1 are clamped to 1.
    Q_PROPERTY(int stepCount READ stepCount WRITE setStepCount NOTIFY stepCountChanged)
    /// 0-based current step. Clamped into [0, stepCount - 1].
    Q_PROPERTY(int currentStep READ currentStep WRITE setCurrentStep NOTIFY currentStepChanged)
    /// Horizontal (default) or vertical stacking.
    Q_PROPERTY(md::MdStepsOrientation orientation READ orientation WRITE setOrientation
                   NOTIFY orientationChanged)
    /// Per-step labels. Shorter lists leave the trailing steps unlabelled.
    Q_PROPERTY(QStringList labels READ labels WRITE setLabels NOTIFY labelsChanged)
    /// When true the steps are display-only (no click, no key walk).
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)

public:
    explicit MdSteps(QWidget *parent = nullptr);
    ~MdSteps() override;

    int stepCount() const { return m_stepCount; }
    void setStepCount(int count);

    int currentStep() const { return m_currentStep; }
    void setCurrentStep(int step);

    MdStepsOrientation orientation() const { return m_orientation; }
    void setOrientation(MdStepsOrientation orientation);

    QStringList labels() const { return m_labels; }
    void setLabels(const QStringList &labels);

    bool isReadOnly() const { return m_readOnly; }
    void setReadOnly(bool readOnly);

    /// The three progress states a step can be in.
    enum class StepState {
        Complete,
        Current,
        Upcoming,
        Count,
    };
    StepState stateAt(int index) const;

    // --- resolved colours (theme live) --------------------------------------
    QColor resolvedDiscColor(StepState state) const;
    QColor resolvedDiscContent(StepState state) const;
    QColor resolvedConnectorColor(bool beforeComplete) const;
    QColor resolvedLabelColor(StepState state) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void stepCountChanged(int stepCount);
    void currentStepChanged(int currentStep);
    void orientationChanged(md::MdStepsOrientation orientation);
    void labelsChanged(const QStringList &labels);
    void readOnlyChanged(bool readOnly);
    /// Emitted when a step is clicked or the keyboard walk lands on one.
    void stepSelected(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    /// Centre points of every disc, in widget coordinates.
    QVector<QPointF> discCentres() const;
    qreal discRadius() const;
    int stepAtPosition(const QPointF &position) const;
    void selectStep(int index);

    int m_stepCount = 3;
    int m_currentStep = 0;
    MdStepsOrientation m_orientation = MdStepsOrientation::Horizontal;
    QStringList m_labels;
    bool m_readOnly = false;

    /// Whether the focus currently held arrived by keyboard (`:focus-visible`).
    bool m_focusIsKeyboard = false;
};

/// Alias kept for the appendix D name pair (`MdSteps / MdStepper`).
using MdStepper = MdSteps;

} // namespace md

Q_DECLARE_METATYPE(md::MdStepsOrientation)
Q_DECLARE_METATYPE(md::MdSteps::StepState)

#endif // MD_STEPS_H
