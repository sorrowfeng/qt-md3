#ifndef MD_NAVIGATION_BAR_ITEM_H
#define MD_NAVIGATION_BAR_ITEM_H

// MdNavigationBarItem — one destination inside an `MdNavigationBar`.
//
// A navigation item is a selectable pill: the indicator sits *behind* the icon,
// and the label sits under it (the baseline arrangement) or beside it (the
// flexible family's horizontal arrangement). Compose's expressive
// `NavigationItem` is the shared behaviour for both — `NavigationBarItem` (the
// baseline) and `ShortNavigationBarItem` (the flexible one) are thin wrappers
// that differ only in their padding constants, and it is that one composable
// this item ports.
//
// Four behaviours are worth naming because none of them is what a plain
// checkable button does:
//
//   * **The whole item is selectable, but only the indicator ripples.** Compose
//     re-maps the item's `InteractionSource` into the indicator's coordinates
//     with a `MappedInteractionSource` so that a press anywhere produces a
//     ripple *inside the pill*. In Qt the same thing is one clip path, and this
//     widget is the family where the library gets it for free — see
//     `MdNavigationBarItemStyle`.
//   * **The indicator's width animates, its height does not.** It grows from
//     `0` to `indicatorRipple.width()` on a spatial spring
//     (`MotionSpring::SpatialDefault`), centred in place, so the pill appears
//     to open out of the icon.
//   * **The label's visibility is a function of the selection.** Compose paints
//     it when `alwaysShowLabel` or when the indicator has progress; a
//     collapsed-to-icon bar is built out of `alwaysShowLabel = false` items,
//     and in that mode the icon is centred in the item while the indicator is
//     away and rises to leave room for the label as it arrives.
//   * **The item's height always reserves the label**, even when the label is
//     not painted. That is what Compose measures, and it is why a
//     `alwaysShowLabel = false` bar is the same height as one that shows them.
//
// The focus indicator is drawn on the **indicator**, with the export's
// `inner-offset`, so nothing has to be reserved outside the item's rect and
// `sizeHint()` is the item's own size. The baseline publishes the ring rows;
// the flexible family publishes none and inherits them.

#include "core/MdNavigationBarTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtWidgets/QPushButton>

class QTimer;

namespace md {

class MdFocusRingController;
class MdRippleController;

class QT_MD3_EXPORT MdNavigationBarItem : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    Q_PROPERTY(md::MdIconSet iconSet READ iconSet WRITE setIconSet NOTIFY iconSetChanged)
    Q_PROPERTY(md::MdIconFamily iconFamily READ iconFamily WRITE setIconFamily NOTIFY
                   iconFamilyChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)
    Q_PROPERTY(bool alwaysShowLabel READ alwaysShowLabel WRITE setAlwaysShowLabel NOTIFY
                   alwaysShowLabelChanged)
    Q_PROPERTY(md::MdNavigationBarVariant variant READ variant WRITE setVariant NOTIFY
                   variantChanged)
    Q_PROPERTY(md::MdNavigationItemIconPosition iconPosition READ iconPosition WRITE
                   setIconPosition NOTIFY iconPositionChanged)

public:
    explicit MdNavigationBarItem(QWidget *parent = nullptr);
    explicit MdNavigationBarItem(const QString &label, const QString &iconName,
                                 QWidget *parent = nullptr);
    ~MdNavigationBarItem() override;

    /// Where the paint puts everything, in widget coordinates.
    ///
    /// Exposed rather than kept private because it is the item's whole geometry
    /// and the test suite asserts it directly; the widgets are not children of
    /// the item, so there is no `Layout` on the style to read it back from.
    struct Boxes
    {
        /// The pill as it is painted right now — animated width, fixed height.
        QRectF indicator;
        /// The pill at full width. This is the ripple's clip path and the focus
        /// ring's bounds.
        QRectF indicatorRipple;
        QRectF icon;
        /// Empty when the item has no label or the label is not painted.
        QRectF label;
        /// The item's own size, i.e. what `sizeHint()` reports. Independent of
        /// the widget's rect: a `EqualWeight` bar gives an item more width than
        /// this and the paint centres in it, which is what Compose's
        /// `contentAlignment = Center` does.
        QSizeF naturalSize;
        bool hasLabel = false;
        bool labelVisible = false;
        qreal labelOpacity = 1.0;
        bool iconAboveLabel = true;
    };

    Boxes boxes() const;

    /// The font the label is painted in — the export's
    /// `active-label-text-weight` is `label-medium-weight-prominent`, so a
    /// selected label is the emphasized cut of the same size. Public because
    /// the paint path lives in the style, not here.
    QFont labelFont() const;

    // --- content ------------------------------------------------------------
    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    MdIconSet iconSet() const { return m_iconSet; }
    void setIconSet(MdIconSet set);

    MdIconFamily iconFamily() const { return m_iconFamily; }
    void setIconFamily(MdIconFamily family);

    QString label() const { return m_label; }
    void setLabel(const QString &label);

    // --- state --------------------------------------------------------------
    /// The selection, which is Qt's checked state: a click, Space or Enter all
    /// report it through `QAbstractButton::toggled`.
    bool isSelected() const { return isChecked(); }
    void setSelected(bool selected);

    /// When false the label is painted only while the item is selected, and the
    /// icon centres in the item while it is not. Default true, as in Compose.
    bool alwaysShowLabel() const { return m_alwaysShowLabel; }
    void setAlwaysShowLabel(bool alwaysShowLabel);

    MdNavigationBarVariant variant() const { return m_variant; }
    void setVariant(MdNavigationBarVariant variant);

    MdNavigationItemIconPosition iconPosition() const { return m_iconPosition; }
    void setIconPosition(MdNavigationItemIconPosition position);

    // --- tokens -------------------------------------------------------------
    /// 0 while the indicator is away, 1 when it is fully open. Driven by the
    /// selection; animating between the two is the pill growing.
    qreal indicatorProgress() const { return m_indicatorProgress; }

    /// The variant rows this item paints from. An owning `MdNavigationBar`
    /// pushes its own resolved set here, so the bar's per-instance overrides
    /// reach its items; a stand-alone item resolves from the application-wide
    /// store instead.
    const MdNavigationBarVariantTokens &variantTokens() const;
    void setVariantTokens(const MdNavigationBarVariantTokens &tokens);

    /// Instance-level `md.comp.*` overrides, used only while no variant token
    /// set has been pushed in.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- interaction state --------------------------------------------------
    bool isHovered() const { return m_hovered; }

    /// `:focus-visible` — keyboard focus only. See MdButton for the reason
    /// classification; the mouse must not paint a focus ring.
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- controllers --------------------------------------------------------
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    // --- geometry -----------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void iconNameChanged(const QString &iconName);
    void iconSetChanged(md::MdIconSet set);
    void iconFamilyChanged(md::MdIconFamily family);
    void labelChanged(const QString &label);
    void selectedChanged(bool selected);
    void alwaysShowLabelChanged(bool alwaysShowLabel);
    void variantChanged(md::MdNavigationBarVariant variant);
    void iconPositionChanged(md::MdNavigationItemIconPosition position);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onIndicatorTick();

private:
    void init();
    void invalidateTokens();
    void restartIndicatorAnimation();

    QString m_iconName;
    QString m_label;
    MdIconSet m_iconSet = MdIconSet::Auto;
    MdIconFamily m_iconFamily = MdIconFamily::Outlined;
    bool m_alwaysShowLabel = true;
    MdNavigationBarVariant m_variant = MdNavigationBarVariant::Baseline;
    MdNavigationItemIconPosition m_iconPosition = MdNavigationItemIconPosition::Top;

    bool m_hovered = false;
    bool m_focusIsKeyboard = false;

    qreal m_indicatorProgress = 0.0;
    /// The spring runs from `m_indicatorFrom` to `m_indicatorTo`; both are 0/1
    /// for a plain selection change, but keeping the pair means an interrupted
    /// animation continues from where it is rather than snapping.
    qreal m_indicatorFrom = 0.0;
    qreal m_indicatorTo = 0.0;
    QElapsedTimer m_indicatorClock;

    mutable MdNavigationBarVariantTokens m_variantTokens;
    mutable bool m_tokensDirty = true;
    /// True once an owner has pushed a set in, so `variantTokens()` stops
    /// re-resolving over the top of it.
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;

    QTimer *m_indicatorTimer = nullptr;
    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
};

} // namespace md

#endif // MD_NAVIGATION_BAR_ITEM_H
