#ifndef MD_ICON_BUTTON_H
#define MD_ICON_BUTTON_H

// MdIconButton — MD3 icon buttons.
//
// Four colour styles, five Expressive sizes, two container shapes, three
// published padding tracks, and the toggle ("selected") form:
//
//   variant  md.comp.icon-button.standard | filled | tonal | outlined
//   size     md.comp.icon-button.xsmall | small | medium | large | xlarge
//   shape    md.comp.icon-button.container.shape.round | .square
//   track    md.comp.icon-button.<default|narrow|wide>-leading-space
//
// An icon button is icon-only by definition — the token arithmetic
// (`leading-space + icon + trailing-space == container-height` on the default
// track) leaves no room for a label, and the export publishes no text tokens
// at all.
//
// Why QPushButton and not a bare QWidget: exactly the argument MdButton makes.
// QAbstractButton already owns clicked/pressed/released/toggled, checkable,
// the keyboard shortcut, Space/Enter activation and the accessibility role;
// overriding the painting is the small surface.
//
// The toggle form is `toggleable` + Qt's own `checked`: an M3 *selected* icon
// button is a checkable one. The selected state changes both the colours
// (the `selected-*` / `unselected-*` token families) and the resting corner
// shape (the `selected-container.shape.*` tokens publish the *other* knob, so
// a selected round icon button has the square-ish corner — the token form of
// "selected shape changes between square and round").
//
// Painting is not done through QStyle::drawControl. See MdButtonStyle for why.

#include "core/MdIconButtonTokens.h"
#include "core/MdRipple.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtWidgets/QPushButton>

class QFocusEvent;
class QMouseEvent;

namespace md {

class MdFocusRingController;
class MdIconButtonStyle;

class QT_MD3_EXPORT MdIconButton : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(md::IconButtonVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(md::ButtonSize buttonSize READ buttonSize WRITE setButtonSize
                   NOTIFY buttonSizeChanged)
    Q_PROPERTY(md::ButtonShape buttonShape READ buttonShape WRITE setButtonShape
                   NOTIFY buttonShapeChanged)
    Q_PROPERTY(md::IconButtonSpaceTrack spaceTrack READ spaceTrack WRITE setSpaceTrack
                   NOTIFY spaceTrackChanged)
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    Q_PROPERTY(md::MdIconSet iconSet READ iconSet WRITE setIconSet NOTIFY iconSetChanged)
    Q_PROPERTY(md::MdIconFamily iconFamily READ iconFamily WRITE setIconFamily
                   NOTIFY iconFamilyChanged)
    Q_PROPERTY(bool toggleable READ isToggleable WRITE setToggleable NOTIFY toggleableChanged)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)

public:
    explicit MdIconButton(QWidget *parent = nullptr);
    /// `iconName` follows the Material Symbols / classic-SVG name convention
    /// the rest of the library uses (`"add"`, `"search"`, …).
    explicit MdIconButton(const QString &iconName, QWidget *parent = nullptr);
    ~MdIconButton() override;

    // --- variant / size / shape / track -----------------------------------
    IconButtonVariant variant() const { return m_variant; }
    void setVariant(IconButtonVariant variant);

    ButtonSize buttonSize() const { return m_size; }
    void setButtonSize(ButtonSize size);

    ButtonShape buttonShape() const { return m_shape; }
    void setButtonShape(ButtonShape shape);

    IconButtonSpaceTrack spaceTrack() const { return m_track; }
    void setSpaceTrack(IconButtonSpaceTrack track);

    // --- icon --------------------------------------------------------------
    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    MdIconSet iconSet() const { return m_iconSet; }
    void setIconSet(MdIconSet set);

    MdIconFamily iconFamily() const { return m_iconFamily; }
    void setIconFamily(MdIconFamily family);

    // --- toggle / selected -------------------------------------------------
    /// When true the button is checkable and `selected` follows `checked`.
    /// Setting this clears a selection that is no longer expressible.
    bool isToggleable() const { return m_toggleable; }
    void setToggleable(bool toggleable);

    bool isSelected() const { return isChecked(); }
    void setSelected(bool selected);

    // --- disabled ----------------------------------------------------------
    /// True when the button neither accepts input nor paints an interactive
    /// state: `!isEnabled()`.
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- interaction state -------------------------------------------------
    /// Hover is tracked explicitly — QWidget::underMouse() is not dependable
    /// before the first enter event and is never set under the offscreen
    /// platform plugin the tests run on.
    bool isHovered() const { return m_hovered; }

    /// 0 = resting shape, 1 = fully pressed shape. The paint path interpolates
    /// the container's corner radii with it.
    qreal pressMorph() const { return m_morph; }

    // --- tokens ------------------------------------------------------------
    /// The resolved `md.comp.icon-button.*` set for this button, after the
    /// application-wide and per-instance `md.comp.*` overrides.
    const MdIconButtonTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this button alone. Mutating
    /// through this accessor drops the cached token set, so an override takes
    /// effect on the next repaint without any further call.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- controllers -------------------------------------------------------
    /// Owned by the button; the style paints from them.
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    /// True when the icon button holds focus *and* that focus is keyboard
    /// focus — the `:focus-visible` rule the official components apply to the
    /// focus indicator and the focused state colours. See MdButton for the
    /// reason-classification.
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    /// The painted container, in widget coordinates.
    QRectF containerRect() const;

    // --- geometry -----------------------------------------------------------
    /// Container size plus the focus-indicator margin, on both axes.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void variantChanged(md::IconButtonVariant variant);
    void buttonSizeChanged(md::ButtonSize size);
    void buttonShapeChanged(md::ButtonShape shape);
    void spaceTrackChanged(md::IconButtonSpaceTrack track);
    void iconNameChanged(const QString &iconName);
    void iconSetChanged(md::MdIconSet set);
    void iconFamilyChanged(md::MdIconFamily family);
    void toggleableChanged(bool toggleable);
    void selectedChanged(bool selected);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
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

    IconButtonVariant m_variant = IconButtonVariant::Standard;
    ButtonSize m_size = ButtonSize::Small;
    ButtonShape m_shape = ButtonShape::Round;
    IconButtonSpaceTrack m_track = IconButtonSpaceTrack::Default;
    QString m_iconName;
    MdIconSet m_iconSet = MdIconSet::Auto;
    MdIconFamily m_iconFamily = MdIconFamily::Outlined;
    bool m_toggleable = false;

    bool m_hovered = false;

    /// Press shape morph: 0 = resting corner, 1 = fully pressed corner.
    qreal m_morph = 0.0;
    qreal m_morphFrom = 0.0;
    qreal m_morphTo = 0.0;
    QElapsedTimer m_morphClock;
    class QTimer *m_morphTimer = nullptr;

    mutable MdIconButtonTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
    /// Whether the focus currently held arrived by keyboard.
    bool m_focusIsKeyboard = false;

    bool m_hasPressPosition = false;
    QPointF m_pressPosition;
};

} // namespace md

#endif // MD_ICON_BUTTON_H
