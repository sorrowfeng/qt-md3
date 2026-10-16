#ifndef MD_FLEX_H
#define MD_FLEX_H

// MdFlex — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "MD3 layout 的 flex / grid 语义".
// MD3 does not publish a Flex component the way it publishes Buttons; what it
// publishes is a *layout language* — the 4 dp grid, the spacing scale, and the
// row / column arrangement rules every Compose `Row` / `Column` sample uses.
// MdFlex is that language as a Qt container.
//
//   direction  row (default) or column
//   justify    start / center / end / space-between / space-around / space-evenly
//   align      start / center / end / stretch
//   gap        the MD3 spacing scale (see `MdFlexSpacing`)
//   wrap       when on, a run that overflows the main axis wraps to the next
//              cross axis, leaving `gap` between wrapped lines too
//
// The spacing scale is transcribed from the material-web layout guidance and
// the 4 dp base unit Material Design 3 uses everywhere (`md.sys.layout.*` is
// not a published token family in the export, so the steps below are the
// named scale the design system's own samples use — recorded in
// docs/porting-todo.md rather than invented silently).
//
// MdFlex paints nothing of its own. It is a pure container: every pixel
// belongs to the caller's children, exactly like MdScaffold's body slot.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

namespace md {

/// Main-axis flow direction.
enum class MdFlexDirection {
    Row,
    Column,
    Count,
};

/// Main-axis distribution of free space.
enum class MdFlexJustify {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly,
    Count,
};

/// Cross-axis alignment of each line's children.
enum class MdFlexAlign {
    Start,
    Center,
    End,
    Stretch,
    Count,
};

/// The MD3 spacing scale (4 dp base unit).
///
/// Values are logical px. Transcribed from the Material Design 3 layout
/// guidance (4 dp grid) and the spacing steps the component exports use
/// (`0 / 4 / 8 / 12 / 16 / 24 / 32`). See docs/porting-todo.md.
enum class MdFlexSpacing {
    None,       ///< 0
    ExtraSmall, ///< 4
    Small,      ///< 8
    Medium,     ///< 12
    Large,      ///< 16
    ExtraLarge, ///< 24
    Huge,       ///< 32
    Count,
};

class QT_MD3_EXPORT MdFlex : public QWidget
{
    Q_OBJECT

    /// Main-axis flow direction.
    Q_PROPERTY(md::MdFlexDirection direction READ direction WRITE setDirection
                   NOTIFY directionChanged)
    /// Main-axis free-space distribution.
    Q_PROPERTY(md::MdFlexJustify justify READ justify WRITE setJustify NOTIFY justifyChanged)
    /// Cross-axis alignment.
    Q_PROPERTY(md::MdFlexAlign align READ align WRITE setAlign NOTIFY alignChanged)
    /// Spacing between children (and between wrapped lines).
    Q_PROPERTY(md::MdFlexSpacing gap READ gap WRITE setGap NOTIFY gapChanged)
    /// Allow the main run to wrap onto the next cross axis.
    Q_PROPERTY(bool wrap READ isWrap WRITE setWrap NOTIFY wrapChanged)

public:
    explicit MdFlex(QWidget *parent = nullptr);
    ~MdFlex() override;

    MdFlexDirection direction() const { return m_direction; }
    void setDirection(MdFlexDirection direction);

    MdFlexJustify justify() const { return m_justify; }
    void setJustify(MdFlexJustify justify);

    MdFlexAlign align() const { return m_align; }
    void setAlign(MdFlexAlign align);

    MdFlexSpacing gap() const { return m_gap; }
    void setGap(MdFlexSpacing gap);

    bool isWrap() const { return m_wrap; }
    void setWrap(bool wrap);

    /// Resolved gap in logical px for the current spacing step.
    qreal gapPx() const;
    /// The published spacing-scale value for one step, in logical px.
    static qreal spacingPx(MdFlexSpacing spacing);

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void directionChanged(md::MdFlexDirection direction);
    void justifyChanged(md::MdFlexJustify justify);
    void alignChanged(md::MdFlexAlign align);
    void gapChanged(md::MdFlexSpacing gap);
    void wrapChanged(bool wrap);

protected:
    void childEvent(QChildEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void relayout() const;

    MdFlexDirection m_direction = MdFlexDirection::Row;
    MdFlexJustify m_justify = MdFlexJustify::Start;
    MdFlexAlign m_align = MdFlexAlign::Start;
    MdFlexSpacing m_gap = MdFlexSpacing::None;
    bool m_wrap = false;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdFlexDirection)
Q_DECLARE_METATYPE(md::MdFlexJustify)
Q_DECLARE_METATYPE(md::MdFlexAlign)
Q_DECLARE_METATYPE(md::MdFlexSpacing)

#endif // MD_FLEX_H
