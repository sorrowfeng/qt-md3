#ifndef MD_LIST_ITEM_H
#define MD_LIST_ITEM_H

// MdListItem — one MD3 list item.
//
//   style  md.comp.list.list-item.*
//
// The style is a visual choice, not a behaviour one [spec]: `Standard` is the
// baseline square-cornered item, `Expressive` is the recommended one whose
// corner radius morphs with the interaction state (extra-small at rest,
// medium on hover, large on focus / press / select / drag, animated on the
// FastSpatial spring [compose]).
//
// Content model, in the spec's slot vocabulary:
//
//   overline    a label-small line above the headline
//   headline    the label text (body-large), the required slot
//   supporting  a body-medium line (or wrapped paragraph) under the headline
//   leading     icon (24, or 20 in the expressive style) | avatar (40,
//               corner-full, primary-container) | image (56x56) | video
//               (100x56, 114x64 large)
//   trailing    icon (24 / 20) | trailing supporting text (label-small) |
//               any widget the caller hands over (checkbox, switch, icon
//               button)
//
// The line count is *derived*, never set: one line with only a headline, two
// with either an overline or a supporting text, three with both or with a
// supporting text that wraps [compose]. The container's height floor follows
// the count (56 / 72 / 88).
//
// Interaction, mirroring the card family:
//
//   * non-interactive (the default, material-web's `type="text"`): no state
//     layers, no ripple, no focus — the enabled row is the whole story.
//   * interactive (`setInteractive(true)`, material-web's `type="button"`):
//     hover / keyboard-focus state layers, a press ripple, Space/Enter
//     activation, and the *inward* focus ring material-web configures for
//     list items (the ring sits inside the item so it never collides with a
//     neighbouring item — unlike the card's outward ring).
//
// Selection is a state, not a variant: `setSelected(true)` paints the
// `selected.*` rows (secondary-container container, on-secondary-container
// content, the large corner) and the host MdList drives it from its selection
// model.

#include "core/MdListTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdRipple.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QFocusEvent;
class QKeyEvent;
class QMouseEvent;

namespace md {

class MdFocusRingController;
class MdListItemStyle;

class QT_MD3_EXPORT MdListItem : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString overline READ overline WRITE setOverline NOTIFY overlineChanged)
    Q_PROPERTY(QString headline READ headline WRITE setHeadline NOTIFY headlineChanged)
    Q_PROPERTY(QString supporting READ supporting WRITE setSupporting NOTIFY supportingChanged)
    Q_PROPERTY(QString trailingSupporting READ trailingSupporting WRITE setTrailingSupporting NOTIFY
                   trailingSupportingChanged)
    Q_PROPERTY(QString leadingIcon READ leadingIcon WRITE setLeadingIcon NOTIFY leadingIconChanged)
    Q_PROPERTY(QString trailingIcon READ trailingIcon WRITE setTrailingIcon NOTIFY
                   trailingIconChanged)
    Q_PROPERTY(md::MdListVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(bool interactive READ isInteractive WRITE setInteractive NOTIFY interactiveChanged)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)
    Q_PROPERTY(bool dragged READ isDragged WRITE setDragged NOTIFY draggedChanged)

public:
    /// What the leading slot holds. `None` (the default) means the slot is
    /// empty; `Icon` paints a Material Symbols glyph in the slot; `Avatar`
    /// paints the corner-full primary-container disc; `Image` and `Video`
    /// reserve the token geometry for a caller-supplied widget.
    enum class LeadingKind
    {
        None,
        Icon,
        Avatar,
        Image,
        Video,
    };
    Q_ENUM(LeadingKind)

    explicit MdListItem(QWidget *parent = nullptr);
    explicit MdListItem(const QString &headline, QWidget *parent = nullptr);
    ~MdListItem() override;

    // --- content ------------------------------------------------------------
    QString overline() const { return m_overline; }
    void setOverline(const QString &text);

    QString headline() const { return m_headline; }
    void setHeadline(const QString &text);

    QString supporting() const { return m_supporting; }
    void setSupporting(const QString &text);

    /// Label-small meta text at the trailing edge ("100+", "12 min").
    QString trailingSupporting() const { return m_trailingSupporting; }
    void setTrailingSupporting(const QString &text);

    /// Line count derived from the content — 1, 2 or 3. Exists for the API
    /// to be readable and for the tests to pin the rule.
    int lineCount() const;

    // --- leading slot --------------------------------------------------------
    LeadingKind leadingKind() const { return m_leadingKind; }
    void setLeadingKind(LeadingKind kind);

    QString leadingIcon() const { return m_leadingIcon; }
    /// Setting a name switches the slot to `LeadingKind::Icon`.
    void setLeadingIcon(const QString &name);

    /// The label painted inside the avatar disc (initials); ignored unless
    /// the leading kind is `Avatar`.
    QString avatarLabel() const { return m_avatarLabel; }
    void setAvatarLabel(const QString &text);

    /// A caller-owned widget for the leading slot (an image, a video
    /// thumbnail, a custom avatar). The item places it in the token slot and
    /// takes ownership through the Qt parent relationship; the slot's rounded
    /// clip is the caller's business (Qt widget masks).
    QWidget *leadingWidget() const { return m_leadingWidget; }
    void setLeadingWidget(QWidget *widget);

    /// The radii the leading slot paints with, so a caller can clip media the
    /// same way (empty when the slot is empty).
    QList<qreal> leadingSlotRadii() const;

    // --- trailing slot -------------------------------------------------------
    QString trailingIcon() const { return m_trailingIcon; }
    void setTrailingIcon(const QString &name);

    QWidget *trailingWidget() const { return m_trailingWidget; }
    void setTrailingWidget(QWidget *widget);

    // --- style / state -------------------------------------------------------
    MdListVariant variant() const { return m_variant; }
    void setVariant(MdListVariant variant);

    /// True when the item handles input (state layers, ripple, focus,
    /// `activated()`). Off by default, like material-web's `type="text"`.
    bool isInteractive() const { return m_interactive; }
    void setInteractive(bool interactive);

    bool isSelected() const { return m_selected; }
    void setSelected(bool selected);

    /// External drag frameworks set this while a drag that moves the item is
    /// in progress; it paints the dragged row (level-4 elevation, its own
    /// state layer, the pressed-equivalent shape).
    bool isDragged() const { return m_dragged; }
    void setDragged(bool dragged);

    /// Where the item sits in a segmented list [compose] `segmentedShapes`.
    /// `index < 0` or `count <= 0` means "not segmented"; a count of 1 is a
    /// one-item segment, whose four corners all take the list's shape.
    void setSegmentedPosition(int index, int count);
    int segmentedIndex() const { return m_segmentIndex; }
    int segmentedCount() const { return m_segmentCount; }

    // --- interaction state ----------------------------------------------------
    /// Hover is tracked explicitly — QWidget::underMouse() is not dependable
    /// before the first enter event and is never set under the offscreen
    /// platform plugin the tests run on.
    bool isHovered() const { return m_hovered; }
    /// True between a mouse press and its release, interactive only.
    bool isPressed() const { return m_pressed; }
    /// True when the item holds focus *and* that focus is keyboard focus —
    /// the `:focus-visible` rule, shared with MdButton / MdCard.
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    // --- tokens ----------------------------------------------------------------
    /// The resolved `md.comp.list.list-item.*` set, after the application-wide
    /// and per-instance `md.comp.*` overrides.
    const MdListTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this item alone. Mutating
    /// through this accessor drops the cached token set, so an override takes
    /// effect on the next repaint without any further call.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- controllers -------------------------------------------------------------
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    // --- geometry ----------------------------------------------------------------
    /// The painted container, in widget coordinates.
    QRectF containerRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

signals:
    void overlineChanged(const QString &text);
    void headlineChanged(const QString &text);
    void supportingChanged(const QString &text);
    void trailingSupportingChanged(const QString &text);
    void leadingIconChanged(const QString &name);
    void trailingIconChanged(const QString &name);
    void variantChanged(md::MdListVariant variant);
    void interactiveChanged(bool interactive);
    void selectedChanged(bool selected);
    void draggedChanged(bool dragged);
    /// A click (mouse release inside, or Space / Enter) — interactive items
    /// only. MdList listens to this to drive its selection model.
    void activated();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void invalidateTokens();
    void applyInteractivity();
    /// Keep the ripple clip + bounds in step with the container.
    void syncRippleGeometry();
    /// Place the caller's slot widgets into the measured slots.
    void placeSlotWidgets();

    QString m_overline;
    QString m_headline;
    QString m_supporting;
    QString m_trailingSupporting;
    LeadingKind m_leadingKind = LeadingKind::None;
    QString m_leadingIcon;
    QString m_avatarLabel;
    QString m_trailingIcon;
    QWidget *m_leadingWidget = nullptr;
    QWidget *m_trailingWidget = nullptr;

    MdListVariant m_variant = MdListVariant::Standard;
    bool m_interactive = false;
    bool m_selected = false;
    bool m_dragged = false;
    int m_segmentIndex = -1;
    int m_segmentCount = 0;

    bool m_hovered = false;
    bool m_pressed = false;
    bool m_focusIsKeyboard = false;

    mutable MdListTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
};

} // namespace md

#endif // MD_LIST_ITEM_H
