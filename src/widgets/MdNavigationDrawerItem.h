#ifndef MD_NAVIGATION_DRAWER_ITEM_H
#define MD_NAVIGATION_DRAWER_ITEM_H

// MdNavigationDrawerItem — one destination inside an `MdNavigationDrawer`.
//
// The drawer's item is **not** the bar's shared expressive item, and that is
// the source's own decision: Compose's `NavigationDrawerItem` is an
// independent composable, because the geometry differs in kind — the pill *is
// the item*, a full-width row (`heightIn(min = 56)`, `fillMaxWidth`) whose
// container colour is the selected state, with no width animation, a `badge`
// slot at the trailing edge, and a left-aligned label in a `weight(1f)` box
// rather than the bar's centred column.
//
// So this widget is a plain checkable button whose whole rect is the pill:
//
//   * selected, the pill fills `secondary-container`; unselected, nothing;
//   * the state layer and the ripple clip to the whole pill — no
//     `MappedInteractionSource` remapping is needed, because there is no
//     smaller pill inside the item to remap into;
//   * the focus ring draws inside the pill on the export's `inner-offset`
//     (this family publishes its own `focus-indicator-*` rows);
//   * the content row insets are Compose's hard-coded `start = 16, end = 24`
//     with a 12 px icon-label gap — see the token header.

#include "core/MdNavigationDrawerTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtCore/QString>
#include <QtGui/QFont>
#include <QtWidgets/QPushButton>

namespace md {

class MdFocusRingController;
class MdRippleController;

class QT_MD3_EXPORT MdNavigationDrawerItem : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    Q_PROPERTY(md::MdIconSet iconSet READ iconSet WRITE setIconSet NOTIFY iconSetChanged)
    Q_PROPERTY(md::MdIconFamily iconFamily READ iconFamily WRITE setIconFamily NOTIFY
                   iconFamilyChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QString badge READ badge WRITE setBadge NOTIFY badgeChanged)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)

public:
    explicit MdNavigationDrawerItem(QWidget *parent = nullptr);
    explicit MdNavigationDrawerItem(const QString &label, const QString &iconName,
                                    QWidget *parent = nullptr);
    ~MdNavigationDrawerItem() override;

    /// Where the paint puts everything, in widget coordinates. The pill is the
    /// whole item, so there is no `indicator`/`indicatorRipple` pair — one
    /// rect, and no animation between them.
    struct Boxes
    {
        QRectF pill;
        QRectF icon;
        QRectF label;
        /// The badge text's box; empty when there is no badge.
        QRectF badge;
        /// The item's own size — what `sizeHint()` reports. The owning drawer
        /// stretches the width and keeps this height.
        QSizeF naturalSize;
        bool hasBadge = false;
    };

    Boxes boxes() const;

    /// The label's font — `label-large`, emphasized (prominent) when selected.
    /// Public because the paint path lives in the style.
    QFont labelFont() const;
    /// The badge's font — the `large-badge-label-*` rows, which are
    /// `label-large` at the baseline weight.
    QFont badgeFont() const;

    // --- content ------------------------------------------------------------
    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    MdIconSet iconSet() const { return m_iconSet; }
    void setIconSet(MdIconSet set);

    MdIconFamily iconFamily() const { return m_iconFamily; }
    void setIconFamily(MdIconFamily family);

    QString label() const { return m_label; }
    void setLabel(const QString &label);

    /// The trailing badge text. Compose's badge is an arbitrary composable
    /// slot; the export's `large-badge-label-*` rows describe a text, and a
    /// text is what this port paints. Recorded in the token header.
    QString badge() const { return m_badge; }
    void setBadge(const QString &badge);

    // --- state --------------------------------------------------------------
    bool isSelected() const { return isChecked(); }
    void setSelected(bool selected);

    // --- tokens ---------------------------------------------------------------
    const MdNavigationDrawerItemTokens &itemTokens() const;
    void setItemTokens(const MdNavigationDrawerItemTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- interaction state ------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    /// `:focus-visible` — keyboard focus only. The mouse must not paint a
    /// focus ring.
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- controllers ------------------------------------------------------------
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    // --- geometry -----------------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void iconNameChanged(const QString &iconName);
    void iconSetChanged(md::MdIconSet set);
    void iconFamilyChanged(md::MdIconFamily family);
    void labelChanged(const QString &label);
    void badgeChanged(const QString &badge);
    void selectedChanged(bool selected);

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

private:
    void init();
    void invalidateTokens();

    QString m_iconName;
    QString m_label;
    QString m_badge;
    MdIconSet m_iconSet = MdIconSet::Auto;
    MdIconFamily m_iconFamily = MdIconFamily::Outlined;

    bool m_hovered = false;
    bool m_focusIsKeyboard = false;

    mutable MdNavigationDrawerItemTokens m_itemTokens;
    mutable bool m_tokensDirty = true;
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
};

} // namespace md

#endif // MD_NAVIGATION_DRAWER_ITEM_H
