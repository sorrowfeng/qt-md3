#ifndef MD_SEGMENTED_BUTTON_H
#define MD_SEGMENTED_BUTTON_H

// MdSegmentedButton — MD3 segmented buttons (the outlined family).
//
// A row of segments sharing one 40 px pill outline: the first segment rounds
// its inline-start corners, the last rounds its inline-end corners and the
// middle ones are rectangles, and neighbours overlap by exactly the
// 1 px outline width so the shared edge is a single stroke — that shared
// stroke *is* the divider between segments (Compose: the row lays the items
// out with `Arrangement.spacedBy(-BorderWidth)`).
//
// Selection paints the segment's container in secondary-container with
// on-secondary-container content, and the check icon scales in on a reserved
// 18 px icon slot so the label never moves — the slot is reserved whether or
// not any segment has a custom icon (Compose measure policy). Single-choice
// by default; multi-choice is the same geometry with per-segment toggles.
//
// One published set only: `md.comp.outlined-segmented-button.*` has no size
// scale and no colour variants. Rows the export lacks (icon spacing, content
// padding, motion) come from Compose and are labelled as such.
//
// Why a plain QWidget: the segments are one control with N press targets and
// a shared outline — same reasoning as MdSplitButton, generalised from two
// halves to N segments.

#include "core/MdRipple.h"
#include "core/MdSegmentedButtonTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QPointF>
#include <QtCore/QStringList>
#include <QtWidgets/QWidget>

class QFocusEvent;
class QKeyEvent;
class QMouseEvent;

namespace md {

class MdFocusRingController;
class MdSegmentedButtonStyle;

class QT_MD3_EXPORT MdSegmentedButton : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QStringList segments READ segments WRITE setSegments NOTIFY segmentsChanged)
    Q_PROPERTY(QStringList leadingIcons READ leadingIcons WRITE setLeadingIcons
                   NOTIFY leadingIconsChanged)
    /// True (default) = radio semantics — checking one segment clears the
    /// others. False = independent toggles.
    Q_PROPERTY(bool singleChoice READ isSingleChoice WRITE setSingleChoice
                   NOTIFY singleChoiceChanged)

public:
    explicit MdSegmentedButton(QWidget *parent = nullptr);
    explicit MdSegmentedButton(const QStringList &segments, QWidget *parent = nullptr);
    ~MdSegmentedButton() override;

    // --- configuration ----------------------------------------------------
    QStringList segments() const { return m_segments; }
    void setSegments(const QStringList &segments);

    /// Optional Material Symbols name per segment; an empty string means the
    /// segment has no custom icon (the reserved slot holds the check when
    /// selected).
    QStringList leadingIcons() const { return m_leadingIcons; }
    void setLeadingIcons(const QStringList &leadingIcons);

    bool isSingleChoice() const { return m_singleChoice; }
    void setSingleChoice(bool singleChoice);

    // --- selection ----------------------------------------------------------
    /// Set one segment's checked state. In single-choice mode checking a
    /// segment clears the others.
    void setChecked(int index, bool checked);
    bool isChecked(int index) const;
    /// Every checked index, ascending.
    QList<int> checkedIndexes() const;
    /// Single-choice convenience: the first checked index or -1.
    int checkedIndex() const;

    int segmentCount() const { return m_segments.size(); }

    /// Disabled for interaction but still keyboard-focusable (the button
    /// families' convention).
    bool isSoftDisabled() const { return m_softDisabled; }
    void setSoftDisabled(bool softDisabled);

    /// True when the control neither accepts input nor paints an interactive
    /// state.
    bool isEffectivelyDisabled() const { return !isEnabled() || m_softDisabled; }

    // --- interaction state ------------------------------------------------
    int hoveredSegment() const { return m_hovered; }
    int pressedSegment() const { return m_pressed; }
    /// The segment keyboard interaction would act on, or -1.
    int focusedSegment() const { return m_focused; }

    /// True when the control holds focus *and* that focus is keyboard focus
    /// (`:focus-visible` semantics, shared with the button families).
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    /// The check icon's scale for one segment: 0 = absent, 1 = fully shown.
    /// Driven by spring-fast-spatial while selection animates.
    qreal checkMorph(int index) const;

    // --- tokens -----------------------------------------------------------
    const MdSegmentedButtonTokens &tokens() const;
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- controllers ------------------------------------------------------
    /// One ripple per segment; owned by the control, painted by the style.
    MdRippleController *rippleController(int index) const;
    /// Owned by the control; paints around the focused segment.
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    // --- geometry ---------------------------------------------------------
    /// Segment `index`, in widget coordinates (already overlap-adjusted).
    QRectF segmentRect(int index) const;
    /// Segments plus the focus-indicator margin, on both axes.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void segmentsChanged(const QStringList &segments);
    void leadingIconsChanged(const QStringList &leadingIcons);
    void singleChoiceChanged(bool singleChoice);
    /// One segment's checked state changed (by pointer, keyboard or code).
    void segmentChecked(int index, bool checked);
    /// A segment was activated, checked or not.
    void segmentActivated(int index);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onMorphTick();

private:
    void init();
    void invalidateTokens();
    void rebuildControllers();
    void setHoveredSegment(int index);
    int segmentAt(const QPointF &position) const;
    void activate(int index);
    void animateCheckTo(int index, qreal target);

    QStringList m_segments;
    QStringList m_leadingIcons;
    bool m_singleChoice = true;
    QVector<bool> m_checked;
    bool m_softDisabled = false;

    int m_hovered = -1;
    int m_pressed = -1;
    int m_focused = -1;

    /// Per-segment check scale, 0..1, spring-driven.
    QVector<qreal> m_checkMorph;
    QVector<qreal> m_morphFrom;
    QVector<qreal> m_morphTo;
    QElapsedTimer m_morphClock;
    class QTimer *m_morphTimer = nullptr;

    mutable MdSegmentedButtonTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    QVector<MdRippleController *> m_ripples;
    MdFocusRingController *m_focusRing = nullptr;
    bool m_focusIsKeyboard = false;
};

} // namespace md

#endif // MD_SEGMENTED_BUTTON_H
