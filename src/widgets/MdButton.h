#ifndef MD_BUTTON_H
#define MD_BUTTON_H

// MdButton — MD3 common buttons.
//
// Five colour styles, five Expressive sizes, two container shapes:
//
//   variant  md.comp.button.elevated | filled | tonal | outlined | text
//   size     md.comp.button.xsmall | small | medium | large | xlarge
//   shape    md.comp.button.container.shape.round | .square
//
// so 5 x 5 x 2 = 50 combinations, every one of them a published token set.
//
// Why QPushButton and not a bare QWidget: a button's *behaviour* is most of
// what a button is. QAbstractButton already provides clicked/pressed/released/
// toggled, checkable and tri-state, a keyboard shortcut, Space/Enter
// activation, autoDefault/default handling for dialogs, the QAction
// integration and the accessibility role. Re-implementing that on a QWidget
// would be a large amount of surface to get subtly wrong; overriding the
// painting is a small amount of surface to get exactly right.
//
// Painting is not done through QStyle::drawControl. See MdButtonStyle for why.
//
// Two M3 Expressive behaviours are implemented here rather than in the style,
// because both are state machines rather than geometry:
//
//   * the container's corners morph on press — resting -> pressed shape,
//     driven by md.comp.button.pressed.container.corner-size.motion.spring
//     (spring-fast-spatial: stiffness 1400, damping ratio 0.9)
//   * the outward focus indicator grows and settles on focus

#include "core/MdButtonTokens.h"
#include "core/MdRipple.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtWidgets/QPushButton>

class QFocusEvent;
class QKeyEvent;
class QMouseEvent;

namespace md {

class MdFocusRingController;
class MdButtonStyle;

class QT_MD3_EXPORT MdButton : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(md::ButtonVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(md::ButtonSize buttonSize READ buttonSize WRITE setButtonSize NOTIFY buttonSizeChanged)
    Q_PROPERTY(md::ButtonShape buttonShape READ buttonShape WRITE setButtonShape
                   NOTIFY buttonShapeChanged)
    Q_PROPERTY(QString leadingIcon READ leadingIcon WRITE setLeadingIcon NOTIFY leadingIconChanged)
    Q_PROPERTY(QString trailingIcon READ trailingIcon WRITE setTrailingIcon NOTIFY trailingIconChanged)
    Q_PROPERTY(bool softDisabled READ isSoftDisabled WRITE setSoftDisabled NOTIFY softDisabledChanged)

public:
    explicit MdButton(QWidget *parent = nullptr);
    explicit MdButton(const QString &text, QWidget *parent = nullptr);
    ~MdButton() override;

    // --- variant / size / shape ------------------------------------------
    ButtonVariant variant() const { return m_variant; }
    void setVariant(ButtonVariant variant);

    ButtonSize buttonSize() const { return m_size; }
    void setButtonSize(ButtonSize size);

    ButtonShape buttonShape() const { return m_shape; }
    void setButtonShape(ButtonShape shape);

    // --- icons ------------------------------------------------------------
    /// Material Symbols name drawn before the label (after it in RTL).
    QString leadingIcon() const { return m_leadingIcon; }
    void setLeadingIcon(const QString &icon);

    /// Material Symbols name drawn after the label.
    QString trailingIcon() const { return m_trailingIcon; }
    void setTrailingIcon(const QString &icon);

    // --- disabled / soft-disabled ----------------------------------------
    /// Disabled for interaction but still keyboard-focusable.
    ///
    /// Upstream: "By default, disabled buttons are not focusable with the
    /// keyboard, while 'soft-disabled' buttons are." Used for toolbar items
    /// that should stay discoverable.
    bool isSoftDisabled() const { return m_softDisabled; }
    void setSoftDisabled(bool softDisabled);

    /// True when the button neither accepts input nor paints an interactive
    /// state: `!isEnabled() || isSoftDisabled()`.
    bool isEffectivelyDisabled() const;

    // --- interaction state ------------------------------------------------
    /// Hover is tracked explicitly rather than through QWidget::underMouse(),
    /// which is not dependable before the first enter event and is never set
    /// under the offscreen platform plugin the tests run on.
    bool isHovered() const { return m_hovered; }

    // --- per-corner shape override ----------------------------------------
    /// Corner radii in TL / TR / BR / BL order, in logical px.
    ///
    /// Empty — the default — means "use the shape tokens", which is every
    /// button that stands on its own. A *button group* sets these instead: an
    /// item shows a different corner towards each neighbour, and that is the
    /// only thing a connected button group contributes, because the spec gives
    /// groups no colours at all ("Button groups have no color properties").
    ///
    /// The values are absolute radii, not shape-token names, because the
    /// corner a group needs is a function of the item's position *and* of the
    /// group's cross-axis extent (the selected inner corner is the literal
    /// `50%`), so it cannot be expressed as a `ShapeCorner`.
    ///
    /// Passing an empty `resting` clears the override. A `pressed` list that is
    /// empty while `resting` is not means "keep the resting radii while
    /// pressed", which is a legitimate state and not a fallback to the tokens.
    void setCornerRadii(const QList<qreal> &resting, const QList<qreal> &pressed = QList<qreal>());
    QList<qreal> restingCornerRadii() const { return m_restingRadii; }
    QList<qreal> pressedCornerRadii() const { return m_pressedRadii; }

    /// The radii the container is painted with right now, override or token.
    /// Exposed so a host (the button group) can compare and skip a no-op set.
    QList<qreal> currentCornerRadii() const;

    /// 0 = resting shape, 1 = fully pressed shape. Exposed because the paint
    /// path interpolates the container's corner radii with it.
    qreal pressMorph() const { return m_morph; }

    /// The token row that painting should use for the current state.
    MdButtonState paintState() const;

    // --- tokens -----------------------------------------------------------
    /// The resolved `md.comp.button.*` set for this button, after the
    /// application-wide and per-instance `md.comp.*` overrides.
    const MdButtonTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this button alone. Falls back
    /// to MdComponentTokens::global() for anything not set here.
    ///
    /// Mutating through this accessor drops the cached token set, so
    /// `button.componentTokens().setValue(...)` takes effect on the next
    /// repaint without any further call.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- controllers ------------------------------------------------------
    /// Owned by the button; the style paints from them.
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    /// True when the button holds focus *and* that focus is keyboard focus.
    ///
    /// The official web components show the focus indicator and the focused
    /// state colours under `:focus-visible` semantics — a Tab into the control
    /// shows them, a mouse click does not. Qt has no such distinction, so the
    /// focus *reason* is recorded when focus arrives: Tab / Backtab / shortcut
    /// (and the ambiguous "other") count as keyboard, mouse and popup focus do
    /// not. Styles read this instead of `hasFocus()`.
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    /// The label as painted: QPushButton mnemonics ("&File", "&&") removed,
    /// because MD3 has no mnemonic underline and the ampersand would otherwise
    /// be drawn literally. The shortcut itself still works — QAbstractButton
    /// owns that, not the painter.
    QString displayText() const;

    /// The painted container, in widget coordinates. Exposed because the
    /// gallery's screenshot check and the tests both need to know where the
    /// token-sized container actually landed inside the padded widget.
    QRectF containerRect() const;

    // --- geometry ---------------------------------------------------------
    /// Container size plus the focus-indicator margin, on both axes.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void variantChanged(md::ButtonVariant variant);
    void buttonSizeChanged(md::ButtonSize size);
    void buttonShapeChanged(md::ButtonShape shape);
    void leadingIconChanged(const QString &icon);
    void trailingIconChanged(const QString &icon);
    void softDisabledChanged(bool softDisabled);

protected:
    void mousePressEvent(QMouseEvent *event) override;
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
    void onPressed();
    void onReleased();

private:
    void init();
    /// Drop the cached token set; the next tokens() call rebuilds it.
    void invalidateTokens();
    void animateMorphTo(qreal target);

    ButtonVariant m_variant = ButtonVariant::Filled;
    ButtonSize m_size = ButtonSize::Small;
    ButtonShape m_shape = ButtonShape::Round;
    QString m_leadingIcon;
    QString m_trailingIcon;
    bool m_softDisabled = false;

    bool m_hovered = false;

    /// Per-corner radius override; see setCornerRadii().
    QList<qreal> m_restingRadii;
    QList<qreal> m_pressedRadii;

    /// Press shape morph: 0 = resting corner, 1 = fully pressed corner. Driven
    /// by the spring the resolved tokens carry, so a token override changes
    /// both the corner and the motion.
    qreal m_morph = 0.0;
    qreal m_morphFrom = 0.0;
    qreal m_morphTo = 0.0;
    QElapsedTimer m_morphClock;
    class QTimer *m_morphTimer = nullptr;

    mutable MdButtonTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
    /// Whether the focus currently held arrived by keyboard (see
    /// hasKeyboardFocus()). Reset on any focus loss.
    bool m_focusIsKeyboard = false;

    bool m_hasPressPosition = false;
    QPointF m_pressPosition;
};

} // namespace md

#endif // MD_BUTTON_H
