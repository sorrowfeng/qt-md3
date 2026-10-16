#ifndef MD_SPACE_H
#define MD_SPACE_H

// MdSpace — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "MD3 spacing scale".
//
// MD3 does not publish a Spacer component; what it publishes is the 4 dp
// spacing scale every layout sample uses. MdSpace is one step of that scale
// as a widget, so a caller can place a token gap without inventing a magic
// number (the same reason MdFlex has `gap`).
//
//   direction  horizontal (default) or vertical
//   size       one MdFlexSpacing step: 0 / 4 / 8 / 12 / 16 / 24 / 32
//
// The values are the same ladder MdFlex::spacingPx() publishes (material-web
// layout guidance and the 4 dp base unit; `md.sys.layout.*` is not a
// published token family in the export). MdSpace reuses `MdFlexSpacing`
// rather than forking a second spacing enum, so one ladder covers both.
//
// MdSpace paints nothing and takes no focus. It is a pure size hint.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include "MdFlex.h"

#include <QtWidgets/QWidget>

namespace md {

/// Main-axis orientation of the reserved gap.
enum class MdSpaceDirection {
    Horizontal,
    Vertical,
    Count,
};

class QT_MD3_EXPORT MdSpace : public QWidget
{
    Q_OBJECT

    /// Horizontal reserves width; vertical reserves height.
    Q_PROPERTY(md::MdSpaceDirection direction READ direction WRITE setDirection
                   NOTIFY directionChanged)
    /// The MD3 spacing-scale step reserved along `direction`.
    Q_PROPERTY(md::MdFlexSpacing size READ size WRITE setSize NOTIFY sizeChanged)
    /// Resolved extent in logical px (derived from `size`; writable so a
    /// layout can pin an exact gap while still reading the token ladder).
    Q_PROPERTY(qreal extent READ extent WRITE setExtent NOTIFY extentChanged)

public:
    explicit MdSpace(QWidget *parent = nullptr);
    ~MdSpace() override;

    MdSpaceDirection direction() const { return m_direction; }
    void setDirection(MdSpaceDirection direction);

    /// The spacing-scale step. When `extent` was overridden the step still
    /// reports the nearest ladder value; `extent()` reports the override.
    MdFlexSpacing size() const { return m_size; }
    void setSize(MdFlexSpacing size);

    /// Current reserved extent in logical px.
    qreal extent() const { return m_extent; }
    void setExtent(qreal extent);

    /// The published spacing-scale value for one step, in logical px.
    /// Identical to MdFlex::spacingPx so the ladder has one source.
    static qreal spacingPx(MdFlexSpacing spacing);

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void directionChanged(md::MdSpaceDirection direction);
    void sizeChanged(md::MdFlexSpacing size);
    void extentChanged(qreal extent);

private:
    void init();

    MdSpaceDirection m_direction = MdSpaceDirection::Horizontal;
    MdFlexSpacing m_size = MdFlexSpacing::None;
    qreal m_extent = 0.0;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdSpaceDirection)

#endif // MD_SPACE_H
