#ifndef MD_DIVIDER_H
#define MD_DIVIDER_H

// MdDivider — MD3 dividers.
//
// "A divider is a thin line that groups content in lists and containers."
// The token export publishes exactly two rows (thickness 1px, colour
// outline-variant); everything else about this family is arrangement:
//
//   orientation  horizontal (material-web's block line, Compose's
//                HorizontalDivider) and vertical (Compose's VerticalDivider —
//                no web counterpart, CSS layout does not need one [compose]).
//   inset        the spec's measurements name three placements — "inset"
//                (start margin 16dp, end 0), "middle-inset" (both 16dp) and
//                full-width — while material-web publishes the attributes
//                [inset-start], [inset-end] and [inset] = both. MdDivider's
//                InsetMode (None / Start / End / Both) covers every
//                combination, so both vocabularies are reachable. Start/end
//                are *logical* inline edges: an RTL layout mirrors them, the
//                same semantics material-web's padding-inline-* spells.
//   thickness    Compose exposes `thickness` as a parameter (its deprecated
//                Divider even accepts a density-independent hairline);
//                setThickness() overrides the token the same way [compose].
//
// A divider is *not interactive*: the export publishes no state rows at all.
// It takes no focus, paints no state layer, no ripple, and clicks pass
// through it. The line is painted device-pixel aligned, so a 1px thickness
// stays a crisp single physical pixel instead of an antialiased blur.

#include "core/MdDividerTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtGui/QColor>
#include <QtWidgets/QWidget>

namespace md {

class MdDividerStyle;

class QT_MD3_EXPORT MdDivider : public QWidget
{
    Q_OBJECT

    /// Horizontal (default) or vertical — Compose's HorizontalDivider /
    /// VerticalDivider pair [compose]; material-web has no vertical form.
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation
                   NOTIFY orientationChanged)
    /// Which inline edges the 16px inset pads. Start/end are logical: an RTL
    /// layout swaps them.
    Q_PROPERTY(InsetMode insetMode READ insetMode WRITE setInsetMode NOTIFY insetModeChanged)
    /// Cross-axis size in px. 0 means the `md.comp.divider.thickness` token.
    Q_PROPERTY(qreal thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
    /// Explicit line colour; an invalid colour (the default) means the
    /// `md.comp.divider.color` token [compose — Compose exposes `color`].
    Q_PROPERTY(QColor customColor READ customColor WRITE setCustomColor NOTIFY customColorChanged)

public:
    /// Which inline edges the inset pads.
    ///
    ///   None   the spec's full-width divider (100%).
    ///   Start  the spec's "inset" measurement — start margin 16dp, end 0.
    ///          material-web spells this attribute [inset-start].
    ///   End    material-web's [inset-end].
    ///   Both   the spec's "middle-inset"; material-web spells this attribute
    ///          [inset]. (The two sources disagree on what bare "inset"
    ///          means — recorded in docs/porting-todo.md.)
    enum class InsetMode
    {
        None,
        Start,
        End,
        Both,
    };
    Q_ENUM(InsetMode)

    explicit MdDivider(QWidget *parent = nullptr);
    /// Convenience: an orientation-first constructor.
    explicit MdDivider(Qt::Orientation orientation, QWidget *parent = nullptr);
    ~MdDivider() override;

    // --- arrangement --------------------------------------------------------
    Qt::Orientation orientation() const { return m_orientation; }
    void setOrientation(Qt::Orientation orientation);

    InsetMode insetMode() const { return m_insetMode; }
    void setInsetMode(InsetMode mode);

    qreal thickness() const;
    /// 0 restores the token thickness.
    void setThickness(qreal thickness);

    QColor customColor() const { return m_customColor; }
    /// An invalid colour restores the token colour.
    void setCustomColor(const QColor &color);

    /// The effective line colour — the explicit override when valid, the
    /// theme-resolved token otherwise.
    QColor effectiveColor() const;

    // --- tokens ------------------------------------------------------------
    /// The resolved `md.comp.divider.*` set, after the application-wide and
    /// per-instance `md.comp.*` overrides.
    const MdDividerTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this divider alone. Mutating
    /// through this accessor drops the cached token set, so an override takes
    /// effect on the next repaint without any further call.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -----------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void orientationChanged(Qt::Orientation orientation);
    void insetModeChanged(InsetMode mode);
    void thicknessChanged(qreal thickness);
    void customColorChanged(const QColor &color);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void updateSizePolicy();
    /// Drop the cached token set; the next tokens() call rebuilds it.
    void invalidateTokens() { m_tokensDirty = true; }

    Qt::Orientation m_orientation = Qt::Horizontal;
    InsetMode m_insetMode = InsetMode::None;
    /// 0 = the token thickness.
    qreal m_thicknessOverride = 0.0;
    QColor m_customColor;

    mutable MdDividerTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_DIVIDER_H
