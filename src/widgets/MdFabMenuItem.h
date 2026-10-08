#ifndef MD_FAB_MENU_ITEM_H
#define MD_FAB_MENU_ITEM_H

// MdFabMenuItem — one element of an MdFabMenu: either the close button or a
// list item. Created and owned by MdFabMenu; public so applications can
// restyle or query items, not intended to stand alone (a close button or a
// list item outside its menu has no spec meaning).
//
// Interaction shape, shared with the button families and recorded in
// docs/interaction-reference.md:
//
//   * hover and keyboard focus paint the flat state layer; press does not —
//     the press response is the ripple alone;
//   * the ripple colour is the pressed row's state-layer role;
//   * the focus indicator follows `:focus-visible` semantics;
//   * the container shape never morphs on press (no pressed shape token).

#include "core/MdFabMenuTokens.h"
#include "core/MdRipple.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QPointF>
#include <QtWidgets/QPushButton>

class QFocusEvent;
class QMouseEvent;

namespace md {

class MdFabMenuStyle;
class MdFocusRingController;

class QT_MD3_EXPORT MdFabMenuItem : public QPushButton
{
    Q_OBJECT

    /// The label text is QPushButton's own text — setText() *is* setLabel().
    Q_PROPERTY(md::FabMenuElement elementRole READ elementRole WRITE setElementRole NOTIFY
                   elementRoleChanged)
    Q_PROPERTY(md::FabMenuVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    /// Reveal progress for the menu's staggered open animation: 0 = hidden,
    /// 1 = fully shown. Painted as opacity plus a downward offset from the
    /// close button's anchor.
    Q_PROPERTY(qreal reveal READ reveal WRITE setReveal)

public:
    explicit MdFabMenuItem(FabMenuElement elementRole, FabMenuVariant variant,
                           QWidget *parent = nullptr);
    MdFabMenuItem(FabMenuElement elementRole, FabMenuVariant variant, const QString &iconName,
                  const QString &label, QWidget *parent = nullptr);
    ~MdFabMenuItem() override;

    FabMenuElement elementRole() const { return m_elementRole; }
    void setElementRole(FabMenuElement role);

    FabMenuVariant variant() const { return m_variant; }
    void setVariant(FabMenuVariant variant);

    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    MdIconSet iconSet() const { return m_iconSet; }
    void setIconSet(MdIconSet set);

    MdIconFamily iconFamily() const { return m_iconFamily; }
    void setIconFamily(MdIconFamily family);

    bool isEffectivelyDisabled() const { return !isEnabled(); }
    bool isHovered() const { return m_hovered; }
    qreal reveal() const { return m_reveal; }
    void setReveal(qreal reveal);

    const MdFabMenuTokens &menuTokens() const;

    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    QRectF containerRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void elementRoleChanged(md::FabMenuElement role);
    void variantChanged(md::FabMenuVariant variant);
    void iconNameChanged(const QString &iconName);
    void iconSetChanged(md::MdIconSet set);
    void iconFamilyChanged(md::MdIconFamily family);

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
    void onPressed();
    void onReleased();

private:
    void init();
    void invalidateTokens();

    FabMenuElement m_elementRole;
    FabMenuVariant m_variant;
    QString m_iconName;
    MdIconSet m_iconSet = MdIconSet::Auto;
    MdIconFamily m_iconFamily = MdIconFamily::Outlined;

    bool m_hovered = false;
    qreal m_reveal = 1.0;

    mutable MdFabMenuTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
    bool m_focusIsKeyboard = false;

    bool m_hasPressPosition = false;
    QPointF m_pressPosition;
};

} // namespace md

#endif // MD_FAB_MENU_ITEM_H
