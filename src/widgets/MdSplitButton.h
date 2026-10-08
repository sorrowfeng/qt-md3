#ifndef MD_SPLIT_BUTTON_H
#define MD_SPLIT_BUTTON_H

// MdSplitButton — MD3 Expressive split buttons.
//
// A pill cut into two halves with a 2 px `between-space`: a leading button
// (icon and/or label) and a trailing button (always the dropdown icon). Both
// halves are painted from the button family's colour rows — the spec page:
// "Split buttons use the same color schemes as standard buttons ... the split
// button color doesn't change when selected — only a state layer is applied."
//
// The family's one behaviour the buttons don't have is the *inner-corner
// morph*: the two corners the halves turn towards each other are almost
// square at rest (4 px at most sizes) and round off on hover / press
// (`inner-corner-hovered-corner-size`, `inner-corner-pressed-corner-size`),
// while the outer corners stay a full pill. The trailing half's facing
// corners go to 50% while it is selected (`trailing-button-inner-corner-
// selected-corner-size`), which is the open-menu state.
//
// Why a plain QWidget and not QAbstractButton: there are *two* press targets
// in one visual control, each with its own ripple and its own state layer,
// and no single checked/pressed state describes both. The cost is owning
// clicked()/keyboard activation by hand; the benefit is that each half can
// be exactly a button.
//
// Painting is not done through QStyle::drawControl. See MdSplitButtonStyle.

#include "core/MdRipple.h"
#include "core/MdSplitButtonTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QFocusEvent;
class QKeyEvent;
class QMouseEvent;

namespace md {

class MdFocusRingController;
class MdSplitButtonStyle;

class QT_MD3_EXPORT MdSplitButton : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(md::ButtonVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(md::SplitButtonSize splitSize READ splitSize WRITE setSplitSize
                   NOTIFY splitSizeChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString leadingIcon READ leadingIcon WRITE setLeadingIcon NOTIFY leadingIconChanged)
    Q_PROPERTY(QString trailingIcon READ trailingIcon WRITE setTrailingIcon
                   NOTIFY trailingIconChanged)
    /// True while the trailing button's menu is open: the trailing half's
    /// facing corners sit at 50% and its colours read as selected.
    Q_PROPERTY(bool trailingSelected READ isTrailingSelected WRITE setTrailingSelected
                   NOTIFY trailingSelectedChanged)
    /// Disabled for interaction but still keyboard-focusable (the button
    /// family's convention).
    Q_PROPERTY(bool softDisabled READ isSoftDisabled WRITE setSoftDisabled
                   NOTIFY softDisabledChanged)

public:
    /// The two interactive halves. `None` is the no-pointer/no-focus state.
    enum class Zone {
        None,
        Leading,
        Trailing,
    };
    Q_ENUM(Zone)

    explicit MdSplitButton(QWidget *parent = nullptr);
    explicit MdSplitButton(const QString &text, QWidget *parent = nullptr);
    ~MdSplitButton() override;

    // --- configuration ----------------------------------------------------
    ButtonVariant variant() const { return m_variant; }
    void setVariant(ButtonVariant variant);

    SplitButtonSize splitSize() const { return m_size; }
    void setSplitSize(SplitButtonSize size);

    QString text() const { return m_text; }
    void setText(const QString &text);

    /// Material Symbols name drawn before the label (after it in RTL).
    QString leadingIcon() const { return m_leadingIcon; }
    void setLeadingIcon(const QString &icon);

    /// Material Symbols name drawn on the trailing half. Defaults to
    /// `arrow_drop_down` — the spec: "The trailing button should always have
    /// a menu icon."
    QString trailingIcon() const { return m_trailingIcon; }
    void setTrailingIcon(const QString &icon);

    bool isTrailingSelected() const { return m_trailingSelected; }
    void setTrailingSelected(bool selected);

    bool isSoftDisabled() const { return m_softDisabled; }
    void setSoftDisabled(bool softDisabled);

    /// True when the control neither accepts input nor paints an interactive
    /// state: `!isEnabled() || isSoftDisabled()`.
    bool isEffectivelyDisabled() const;

    // --- interaction state ------------------------------------------------
    /// The half the pointer is over, or `None`. Tracked explicitly because
    /// QWidget::underMouse() is not dependable under the offscreen platform.
    Zone hoveredZone() const { return m_hoveredZone; }
    /// The half the pointer went down on, or `None`.
    Zone pressedZone() const { return m_pressedZone; }
    /// The half keyboard interaction would act on, or `None` while the
    /// control holds no keyboard focus.
    Zone focusedZone() const { return m_focusedZone; }

    /// The current inner (facing) corner radius of one half, in logical px —
    /// the animated value between the published rest / hovered / pressed /
    /// selected radii. Exposed so the style never recomputes the animation.
    qreal innerCornerRadius(Zone zone) const;

    /// True when the control holds focus *and* that focus is keyboard focus
    /// (`:focus-visible` semantics, shared with MdButton).
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    // --- tokens -----------------------------------------------------------
    /// The resolved `md.comp.split-button.*` set (with the button family's
    /// colour rows), after the application-wide and per-instance overrides.
    const MdSplitButtonTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this control alone. Mutating
    /// through this accessor drops the cached token set.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- controllers ------------------------------------------------------
    /// One per half; owned by the control, painted by the style.
    MdRippleController *rippleController(Zone zone) const;

    /// Owned by the control; paints around the whole split container.
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    // --- geometry ---------------------------------------------------------
    /// The whole split (both halves plus the between-space), in widget
    /// coordinates.
    QRectF containerRect() const;
    /// The two halves, in widget coordinates. Leading is the inline-start
    /// side; both swap under RTL.
    QRectF leadingRect() const;
    QRectF trailingRect() const;
    /// The half containing `position` (widget coordinates), or `None`.
    Zone zoneAt(const QPointF &position) const;

    /// The split plus the focus-indicator margin, on both axes.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void variantChanged(md::ButtonVariant variant);
    void splitSizeChanged(md::SplitButtonSize size);
    void textChanged(const QString &text);
    void leadingIconChanged(const QString &icon);
    void trailingIconChanged(const QString &icon);
    void trailingSelectedChanged(bool selected);
    void softDisabledChanged(bool softDisabled);

    /// The leading half was activated (pointer release on it, or keyboard).
    void leadingClicked();
    /// The trailing half was activated. Showing a menu is the host's job —
    /// set trailingSelected while it is open.
    void trailingClicked();

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
    void setHoveredZone(Zone zone);
    /// The radius the half's facing corners animate towards right now, from
    /// the published rows (selected > pressed > hovered > rest).
    qreal innerRadiusTarget(Zone zone) const;
    void animateMorphTo(Zone zone, qreal target);
    Zone nextZone(Zone zone, int delta) const;

    ButtonVariant m_variant = ButtonVariant::Filled;
    SplitButtonSize m_size = SplitButtonSize::Small;
    QString m_text;
    QString m_leadingIcon;
    QString m_trailingIcon = QStringLiteral("arrow_drop_down");
    bool m_trailingSelected = false;
    bool m_softDisabled = false;

    Zone m_hoveredZone = Zone::None;
    Zone m_pressedZone = Zone::None;
    Zone m_focusedZone = Zone::None;

    /// Per-half inner-corner radius, animated between the published rows
    /// (leading first, trailing second). Starts at the rest radius. Mutable
    /// because the token cache rebuild — a const path — is also what seeds
    /// the animation from a config change.
    mutable qreal m_innerRadius[2] = {0.0, 0.0};
    mutable qreal m_radiusTo[2] = {0.0, 0.0};
    qreal m_radiusFrom[2] = {0.0, 0.0};
    QElapsedTimer m_morphClock[2];
    class QTimer *m_morphTimer = nullptr;

    mutable MdSplitButtonTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripples[2] = {nullptr, nullptr};
    MdFocusRingController *m_focusRing = nullptr;
    bool m_focusIsKeyboard = false;
};

} // namespace md

#endif // MD_SPLIT_BUTTON_H
