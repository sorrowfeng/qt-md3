#ifndef MD_TAB_H
#define MD_TAB_H

// MdTab — one selectable page label inside an `MdTabs`.
//
// Compose's `Tab` is a selectable column (icon above label) or row (icon
// leading label, the `LeadingIconTab` overload) with three behaviours this
// widget ports:
//
//   * **The content colours cross-fade on selection.** Compose's
//     `TabTransition` animates `LocalContentColor` between the active and
//     inactive table rows — the fade-in on the *default* effects spring, the
//     fade-out on the *fast* one. Here that is one progress value driven by
//     the springs and an Oklab interpolation, which is what
//     `Color.VectorConverter` does upstream.
//   * **The ripple's colour is the selected colour.** Compose builds the
//     `ripple(bounded = true, color = selectedContentColor)` "because we want
//     to show the color before the item is considered selected" — so a press
//     on an inactive tab ripples in the colour it is about to earn.
//   * **The tab is its own focus ring's bounds.** The export's
//     `focus-indicator-outline-offset` is the system *inner* offset, so the
//     ring draws inside the tab and `sizeHint()` is the tab's own size — the
//     navigation item's rule, and the reason `MdTabs` lays out by `sizeHint()`
//     without reserving margins.
//
// The indicator itself is not this widget's: it belongs to the row, which
// animates it between tabs. What this widget contributes is the *content
// width* the primary indicator animates to — `indicatorContentWidth()` — and
// the geometry (`boxes()`) the style paints from.

#include "core/MdTabsTokens.h"
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

class QT_MD3_EXPORT MdTab : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    Q_PROPERTY(md::MdIconSet iconSet READ iconSet WRITE setIconSet NOTIFY iconSetChanged)
    Q_PROPERTY(md::MdIconFamily iconFamily READ iconFamily WRITE setIconFamily NOTIFY
                   iconFamilyChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)
    Q_PROPERTY(md::MdTabIconPosition iconPosition READ iconPosition WRITE setIconPosition NOTIFY
                   iconPositionChanged)

public:
    explicit MdTab(QWidget *parent = nullptr);
    explicit MdTab(const QString &label, QWidget *parent = nullptr);
    ~MdTab() override;

    /// Where the paint puts everything, in widget coordinates. Exposed for the
    /// tests, as on the navigation item.
    struct Boxes
    {
        QRectF icon;
        QRectF label;
        QSizeF naturalSize;
        bool hasLabel = false;
        bool hasIcon = false;
        bool iconAboveLabel = true;
    };

    Boxes boxes() const;

    /// The label's font — `title-small` in both families, always measured in
    /// the emphasized cut so a weight change cannot resize the tab mid-state.
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
    bool isSelected() const { return isChecked(); }
    void setSelected(bool selected);

    MdTabIconPosition iconPosition() const { return m_iconPosition; }
    void setIconPosition(MdTabIconPosition position);

    // --- tokens -------------------------------------------------------------
    /// 0 fully in the inactive colours, 1 fully in the active ones. Driven by
    /// the selection through the two effects springs (in on `EffectsDefault`,
    /// out on `EffectsFast` — Compose's `TabTransition`).
    qreal colourProgress() const { return m_colourProgress; }

    const MdTabsVariantTokens &variantTokens() const;
    void setVariantTokens(const MdTabsVariantTokens &tokens);

    /// Instance-level `md.comp.*` overrides, used only while no variant token
    /// set has been pushed in.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- the indicator's content width ----------------------------------------
    /// The width the primary indicator takes against this tab when the row is
    /// `availableWidth` wide: Compose's
    /// `max(min(maxIntrinsicWidth, tabWidth) - 2 * HorizontalTextPadding, 24)`.
    qreal indicatorContentWidth(qreal availableWidth) const;

    // --- interaction state ----------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- controllers ------------------------------------------------------------
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    // --- geometry ---------------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void iconNameChanged(const QString &iconName);
    void iconSetChanged(md::MdIconSet set);
    void iconFamilyChanged(md::MdIconFamily family);
    void labelChanged(const QString &label);
    void selectedChanged(bool selected);
    void iconPositionChanged(md::MdTabIconPosition position);

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
    void onColourTick();

private:
    void init();
    void invalidateTokens();
    void restartColourAnimation();

    QString m_iconName;
    QString m_label;
    MdIconSet m_iconSet = MdIconSet::Auto;
    MdIconFamily m_iconFamily = MdIconFamily::Outlined;
    MdTabIconPosition m_iconPosition = MdTabIconPosition::Top;

    bool m_hovered = false;
    bool m_focusIsKeyboard = false;

    qreal m_colourProgress = 0.0;
    qreal m_colourFrom = 0.0;
    qreal m_colourTo = 0.0;
    QElapsedTimer m_colourClock;

    mutable MdTabsVariantTokens m_variantTokens;
    mutable bool m_tokensDirty = true;
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;

    QTimer *m_colourTimer = nullptr;
    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
};

} // namespace md

#endif // MD_TAB_H
