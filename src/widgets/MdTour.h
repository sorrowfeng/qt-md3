#ifndef MD_TOUR_H
#define MD_TOUR_H

// MdTour / MdCoachMark — Stage 2 Qt extension (Appendix D, P3).
//
// Semantic anchor (appendix D): "Scrim + shape 高亮 + Dialog".
//
// The product-tour coach mark: a scrim over the host window with a
// shape-scale *hole* punched around the target widget, plus a dialog-styled
// callout card that explains the step.
//
//   scrim    `md.sys.color.scrim` at the modal-drawer opacity (0.32)
//   hole     the target rect grown by a shape-scale margin, rounded with
//            ShapeCorner::Large — the shape-highlight reading
//   card     the dialog chrome (surface-container-high, corner extra-large)
//            carrying a headline-small title, body-medium body, and the
//            dialog's Text-button action pair
//
// `MdTour` is the controller (step list + next / skip); `MdCoachMark` is
// the single callout widget it places. The two ship together because a tour
// without a coach mark is just a step counter.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QPaintEvent;
class QKeyEvent;

namespace md {

class MdButton;

/// One coach mark: a target rect (in host coordinates) plus the copy.
struct QT_MD3_EXPORT MdTourStep
{
    QString title;
    QString body;
    QString targetName; ///< objectName of the target widget, optional
    QRect targetRect;   ///< explicit rect, used when targetName is empty

    MdTourStep() = default;
    MdTourStep(const QString &title_, const QString &body_, const QString &targetName_ = QString())
        : title(title_)
        , body(body_)
        , targetName(targetName_)
    {
    }

    bool operator==(const MdTourStep &other) const
    {
        return title == other.title && body == other.body && targetName == other.targetName
               && targetRect == other.targetRect;
    }
};

/// The callout card + scrim + shape hole. Parent it to the host window.
class QT_MD3_EXPORT MdCoachMark : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString body READ body WRITE setBody NOTIFY bodyChanged)
    Q_PROPERTY(QRect targetRect READ targetRect WRITE setTargetRect NOTIFY targetRectChanged)
    Q_PROPERTY(QString stepText READ stepText WRITE setStepText NOTIFY stepTextChanged)

public:
    explicit MdCoachMark(QWidget *parent = nullptr);
    ~MdCoachMark() override;

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString body() const { return m_body; }
    void setBody(const QString &body);

    QRect targetRect() const { return m_targetRect; }
    void setTargetRect(const QRect &rect);

    /// Small "2 / 5" style caption. Empty hides it.
    QString stepText() const { return m_stepText; }
    void setStepText(const QString &text);

    /// Next / skip buttons (always created).
    MdButton *nextButton() const { return m_nextButton; }
    MdButton *skipButton() const { return m_skipButton; }

    /// Painted card rect in widget coordinates.
    QRectF cardRect() const;
    /// The shape-hole rect (target grown by the margin).
    QRectF holeRect() const;

    QSize sizeHint() const override;
    int heightForWidth(int width) const override;

signals:
    void titleChanged(const QString &title);
    void bodyChanged(const QString &body);
    void targetRectChanged(const QRect &rect);
    void stepTextChanged(const QString &text);
    void nextRequested();
    void skipRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void layoutButtons();

    QString m_title;
    QString m_body;
    QString m_stepText;
    QRect m_targetRect;
    MdButton *m_nextButton = nullptr;
    MdButton *m_skipButton = nullptr;
};

/// The tour controller: a list of steps driven through one MdCoachMark.
class QT_MD3_EXPORT MdTour : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int currentStep READ currentStep NOTIFY currentStepChanged)
    Q_PROPERTY(int stepCount READ stepCount NOTIFY stepCountChanged)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)

public:
    explicit MdTour(QWidget *parent = nullptr);
    ~MdTour() override;

    void setSteps(const QList<MdTourStep> &steps);
    void addStep(const MdTourStep &step);
    void clearSteps();
    QList<MdTourStep> steps() const { return m_steps; }

    int currentStep() const { return m_currentStep; }
    int stepCount() const { return int(m_steps.size()); }
    bool isActive() const { return m_active; }

    /// The coach mark widget (created on first start).
    MdCoachMark *coachMark() const { return m_coach; }

public slots:
    void start();
    void stop();
    void next();
    void previous();

signals:
    void currentStepChanged(int step);
    void stepCountChanged(int count);
    void activeChanged(bool active);
    void finished();
    void skipped();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onNext();
    void onSkip();

private:
    void init();
    void applyStep();
    QRect resolveTarget(const MdTourStep &step) const;

    QList<MdTourStep> m_steps;
    int m_currentStep = 0;
    bool m_active = false;
    MdCoachMark *m_coach = nullptr;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdTourStep)
Q_DECLARE_METATYPE(QList<md::MdTourStep>)

#endif // MD_TOUR_H
