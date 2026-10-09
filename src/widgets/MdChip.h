#ifndef MD_CHIP_H
#define MD_CHIP_H

// MdChip — the four published chip families over one widget.
//
// Compose's shared `Chip` / `SelectableChip` are a `Surface` (shape, colour,
// border, shadow elevation) wrapping a three-slot content row — leading
// (icon or avatar), label (weighted), trailing — and this widget ports both:
//
//   * **the three-slot row with the two-spacing arrangement** — Compose's
//     `ChipArrangement` spaces element 0→1 with the *leading* spacing and
//     right-aligns the trailing slot, so the label absorbs the width and the
//     trailing icon never moves. Filter and input tighten the gap beside a
//     leading icon to the 4 px compact spacing; assist and suggestion keep
//     the 8 px everywhere.
//   * **selectable families read their second colour side** — filter and
//     input chips resolve the selected tables when checked (Compose's
//     `SelectableChipColors`), with the pressed state-layer swap: an
//     unselected filter press ripples `on-secondary-container`, a selected
//     one `on-surface-variant`. Colours change **instantly** — upstream
//     animates only the icons' presence, not the colours.
//   * **assist and suggestion are click-only** — not checkable; Space and
//     Enter click, nothing toggles.
//
// The icon-presence animation (a trailing icon fading/expanding in when it
// appears) is recorded in porting-todo.md as not ported; the drag state is a
// programmatic property carrying the export's dragged rows (level-4
// elevation, the dragged colour tables).

#include "core/MdChipTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QAbstractButton>

class QTimer;

namespace md {

class MdFocusRingController;
class MdRippleController;

class QT_MD3_EXPORT MdChip : public QAbstractButton
{
    Q_OBJECT

    Q_PROPERTY(md::MdChipVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(md::MdChipKind kind READ kind WRITE setKind NOTIFY kindChanged)
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    Q_PROPERTY(md::MdIconSet iconSet READ iconSet WRITE setIconSet NOTIFY iconSetChanged)
    Q_PROPERTY(md::MdIconFamily iconFamily READ iconFamily WRITE setIconFamily NOTIFY
                   iconFamilyChanged)
    Q_PROPERTY(QString trailingIconName READ trailingIconName WRITE setTrailingIconName NOTIFY
                   trailingIconNameChanged)
    Q_PROPERTY(QString avatarIconName READ avatarIconName WRITE setAvatarIconName NOTIFY
                   avatarIconNameChanged)
    Q_PROPERTY(bool dragged READ isDragged WRITE setDragged NOTIFY draggedChanged)

public:
    explicit MdChip(QWidget *parent = nullptr);
    explicit MdChip(const QString &label, QWidget *parent = nullptr);
    ~MdChip() override;

    // --- content ------------------------------------------------------------
    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    MdIconSet iconSet() const { return m_iconSet; }
    void setIconSet(MdIconSet set);

    MdIconFamily iconFamily() const { return m_iconFamily; }
    void setIconFamily(MdIconFamily family);

    QString trailingIconName() const { return m_trailingIconName; }
    void setTrailingIconName(const QString &iconName);

    /// The avatar slot: a 24 px corner-full circle carrying an icon glyph
    /// (upstream it carries a caller-supplied photo — see the header note).
    QString avatarIconName() const { return m_avatarIconName; }
    void setAvatarIconName(const QString &iconName);

    // --- state --------------------------------------------------------------
    MdChipVariant variant() const { return m_variant; }
    void setVariant(MdChipVariant variant);

    MdChipKind kind() const { return m_kind; }
    void setKind(MdChipKind kind);

    bool isSelected() const { return isChecked(); }
    void setSelected(bool selected);

    bool isDragged() const { return m_dragged; }
    void setDragged(bool dragged);

    // --- geometry -----------------------------------------------------------
    /// Where the paint puts everything, in widget coordinates. Exposed for
    /// the tests, as on the tab.
    struct Boxes
    {
        QRectF avatar;
        QRectF leadingIcon;
        QRectF label;
        QRectF trailingIcon;
        QSizeF naturalSize;
        bool hasAvatar = false;
        bool hasLeadingIcon = false;
        bool hasTrailingIcon = false;
    };

    Boxes boxes() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- tokens -----------------------------------------------------------------
    const MdChipVariantTokens &chipTokens() const;
    void setChipTokens(const MdChipVariantTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- interaction state ---------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- controllers -------------------------------------------------------------------
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

signals:
    void variantChanged(md::MdChipVariant variant);
    void kindChanged(md::MdChipKind kind);
    void iconNameChanged(const QString &iconName);
    void iconSetChanged(md::MdIconSet set);
    void iconFamilyChanged(md::MdIconFamily family);
    void trailingIconNameChanged(const QString &iconName);
    void avatarIconNameChanged(const QString &iconName);
    void draggedChanged(bool dragged);

protected:
    /// The shared paint filter draws the chip; this override exists only
    /// because `QAbstractButton` declares it pure.
    void paintEvent(QPaintEvent *event) override;
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
    QString m_trailingIconName;
    QString m_avatarIconName;
    MdIconSet m_iconSet = MdIconSet::Auto;
    MdIconFamily m_iconFamily = MdIconFamily::Outlined;

    MdChipVariant m_variant = MdChipVariant::Assist;
    MdChipKind m_kind = MdChipKind::Flat;
    bool m_dragged = false;
    bool m_hovered = false;
    bool m_focusIsKeyboard = false;

    mutable MdChipVariantTokens m_tokens;
    mutable bool m_tokensDirty = true;
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
};

} // namespace md

#endif // MD_CHIP_H
