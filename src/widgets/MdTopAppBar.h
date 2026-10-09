#ifndef MD_TOP_APP_BAR_H
#define MD_TOP_APP_BAR_H

// MdTopAppBar — the MD3 top app bar.
//
// "Top app bars display information and actions at the top of a screen."
// The official component is *App bars*; this widget is its top half (the
// bottom half is MdBottomAppBar).
//
// The variants are the five the export publishes. m3.material.io's variant
// table lists seven entries, but two are not layouts of their own: "Center
// aligned" is merged into the small bar — "Use centered-text configuration" —
// and so is `MdAppBarAlignment` here, while the search app bar is a
// configuration in which the bar's centre is a search field, which is what
// the centre slot is for.
//
// The baseline medium and large bars are *deprecated as designs* — the spec
// says so outright and points at their flexible replacements — but they are
// still published token sets and still render here, because a port that could
// not draw a 2024 app bar would not be a port.
//
// Interaction is the scroll behaviour, not a state layer: the export
// publishes no hover, press or focus rows for this component at all. What
// changes with scrolling is the container colour (surface -> surface
// container, level0 -> level2 on two rows) and, for a two-row bar, the height
// and which title is visible. `MdAppBarScrollBehavior` owns that state; the
// bar reads it and sizes its own collapsible row.
//
// Slot widgets (`navigationWidget`, `centerWidget`, the action widgets) are
// reparented into the bar and placed by the style's layout. They keep their
// own painting — a navigation slot conventionally holds an `MdIconButton`,
// which is why the 4 px container padding plus that button's own 12 px lands
// the glyph 16 px from the edge.

#include "core/MdAppBarScrollBehavior.h"
#include "core/MdAppBarTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdTopAppBarStyle;

class QT_MD3_EXPORT MdTopAppBar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(MdAppBarVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(MdAppBarAlignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString subtitle READ subtitle WRITE setSubtitle NOTIFY subtitleChanged)

public:
    explicit MdTopAppBar(QWidget *parent = nullptr);
    explicit MdTopAppBar(const QString &title, QWidget *parent = nullptr);
    ~MdTopAppBar() override;

    // --- arrangement --------------------------------------------------------
    MdAppBarVariant variant() const { return m_variant; }
    void setVariant(MdAppBarVariant variant);

    MdAppBarAlignment alignment() const { return m_alignment; }
    void setAlignment(MdAppBarAlignment alignment);

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    /// A second line under the title. Only the small and the two flexible
    /// variants can show one: the baseline medium and large sets publish their
    /// `subtitle.font` rows under a deprecation that says so ("No subtitle
    /// support on the legacy app bar").
    QString subtitle() const { return m_subtitle; }
    void setSubtitle(const QString &subtitle);

    // --- slots --------------------------------------------------------------
    /// The leading element, conventionally an `MdIconButton`.
    QWidget *navigationWidget() const { return m_navigation; }
    /// Reparents `widget` into the bar. Passing null clears the slot.
    void setNavigationWidget(QWidget *widget);

    /// The centre element — where a search app bar's search field goes. When
    /// set, it takes the space the title would have used and the title and
    /// subtitle are not drawn.
    QWidget *centerWidget() const { return m_center; }
    void setCenterWidget(QWidget *widget);

    /// Trailing actions, laid out in order from the leading side. Null and
    /// duplicate widgets are ignored.
    void addActionWidget(QWidget *widget);
    void insertActionWidget(int index, QWidget *widget);
    void removeActionWidget(QWidget *widget);
    void clearActionWidgets();
    QList<QWidget *> actionWidgets() const { return m_actions; }

    // --- scrolling ----------------------------------------------------------
    /// The behaviour this bar reads. The bar owns half of the arrangement: it
    /// writes `heightOffsetLimit` from its own collapsible row, exactly as
    /// Compose's `adjustHeightOffsetLimit` does, so a variant change re-scales
    /// the same behaviour instead of stranding it.
    MdAppBarScrollBehavior *scrollBehavior() const { return m_behavior; }
    void setScrollBehavior(MdAppBarScrollBehavior *behavior);

    /// 0 fully expanded .. 1 fully collapsed. 0 without a behaviour.
    qreal collapsedFraction() const;
    /// How much of the bar overlaps scrolled content, 0..1. 0 without a
    /// behaviour.
    qreal overlappedFraction() const;

    // --- paint values the tests and the gallery read ------------------------
    /// The container colour *now* — the two roles interpolated on the eased
    /// transition fraction, which the scroll behaviour feeds.
    QColor containerColor() const;
    /// The first row's small-title opacity.
    qreal leadingTitleAlpha() const;
    /// The expanded title's opacity.
    qreal titleAlpha() const;

    // --- tokens -------------------------------------------------------------
    /// The resolved `md.comp.app-bar.*` set.
    const MdAppBarTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides. Mutating through this accessor
    /// drops the cache, so an override takes effect on the next repaint.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -----------------------------------------------------------
    /// The expanded height for the current variant and subtitle.
    qreal expandedHeight() const;
    /// The height the bar occupies right now.
    qreal currentHeight() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void variantChanged(MdAppBarVariant variant);
    void alignmentChanged(MdAppBarAlignment alignment);
    void titleChanged(const QString &title);
    void subtitleChanged(const QString &subtitle);
    void navigationWidgetChanged();
    void centerWidgetChanged();
    void actionWidgetsChanged();
    void scrollBehaviorChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onScrollChanged();

private:
    void init();
    void invalidateTokens();
    /// Re-apply the height and re-place the slot widgets.
    void syncGeometry();
    /// Place the slot widgets into the style's slots.
    void placeSlots();
    /// Push the collapsible row's height into the behaviour.
    void syncHeightOffsetLimit();
    /// Animate `m_scrollAmount` towards the behaviour's target.
    void retargetContainerColor();

    MdAppBarVariant m_variant = MdAppBarVariant::Small;
    MdAppBarAlignment m_alignment = MdAppBarAlignment::Leading;
    QString m_title;
    QString m_subtitle;

    QWidget *m_navigation = nullptr;
    QWidget *m_center = nullptr;
    QList<QWidget *> m_actions;

    MdAppBarScrollBehavior *m_behavior = nullptr;

    /// The eased colour transition, 0..1. Runs on the DefaultEffects spring —
    /// Compose's `animateColorAsState(target, DefaultEffects)` — because the
    /// single-row target is a step, not a ramp.
    qreal m_scrollAmount = 0.0;
    qreal m_scrollFrom = 0.0;
    qreal m_scrollTo = 0.0;
    qint64 m_scrollElapsedMs = 0;
    bool m_scrollAnimating = false;
    QTimer *m_scrollTimer = nullptr;

    mutable MdAppBarTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_TOP_APP_BAR_H
