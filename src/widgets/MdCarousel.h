#ifndef MD_CAROUSEL_H
#define MD_CAROUSEL_H

#include "styles/MdCarouselStyle.h"
#include "core/MdCarouselTokens.h"
#include "core/MdMotion.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QRectF>
#include <QtWidgets/QWidget>

class QTimer;

namespace md
{

/// The `md.comp.carousel-item.*` family: a horizontally scrolling row of
/// items whose widths follow the Compose M3 multi-browse keyline model — a
/// large item at the front, a medium and a small behind it, resizing as they
/// scroll through the slots.
///
/// Items are caller-supplied widgets wrapped by the carousel's own surface
/// containers (the container colour, the corner-extra-large clip, the state
/// layer and the optional outline come from the tokens; the content paints
/// itself inside the wrapper). The drag-scroll settles to the nearest item
/// boundary on release, Compose Pager's page-snap semantics.
///
/// Geometry management follows the sheet families: the carousel lays its item
/// wrappers out from the keyline math on every resize and scroll tick — the
/// content widget inside a wrapper fills it.
class QT_MD3_EXPORT MdCarousel : public QWidget
{
    Q_OBJECT

public:
    explicit MdCarousel(QWidget *parent = nullptr);

    /// Adds a content widget as the next carousel item, wrapping it in the
    /// token-painted surface container. Returns the item's index.
    int addItem(QWidget *content);

    int itemCount() const { return m_items.size(); }

    /// The preferred large-item width; 0 means "the carousel's own width"
    /// (Compose's default when no preferred item size is given).
    void setPreferredItemSize(qreal width);
    qreal preferredItemSize() const { return m_preferredItemSize; }

    /// The with-outline variant: a 1 px outline around every item container.
    void setWithOutline(bool on);
    bool withOutline() const { return m_tokens.withOutline; }

    /// The large item's width for the current container size (the strategy's
    /// `itemMainAxisSize`).
    qreal largeItemSize() const;

    /// The current and maximum scroll offsets, in the end-to-end model's px.
    qreal scrollOffset() const { return m_scrollOffset; }
    qreal maxScrollOffset() const;

    /// Programmatic scroll — clamped, no snap. Emits scrollOffsetChanged.
    void scrollTo(qreal offset);

    /// Whether a scroll animation is running. Exists for the test suite.
    bool isAnimating() const { return m_phase == Phase::Animating; }

    /// The keyline model the items are currently painted with. Exists for
    /// the test suite and the gallery.
    QList<MdCarouselStyle::Keyline> currentKeylines() const { return m_currentKeylines; }

    /// Per-item paint, called by the style's paint hub.
    void paintItems(QPainter &painter);

signals:
    void scrollOffsetChanged(qreal offset);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    enum class Phase
    {
        Idle,
        Animating,
    };

    struct ItemRecord
    {
        QWidget *wrapper = nullptr;
        QWidget *content = nullptr;
    };

    void init();
    void rebuildStrategy();
    void relayoutItems();
    void animateTo(qreal targetOffset);
    void applyScrollOffset(qreal offset);
    int itemAt(const QPointF &pos) const;
    void updateHover(const QPointF &pos);

    MdCarouselTokens m_tokens;
    QList<ItemRecord> m_items;
    qreal m_scrollOffset = 0.0;
    qreal m_preferredItemSize = 0.0;
    int m_hoveredItem = -1;
    int m_pressedItem = -1;
    bool m_dragging = false;
    qreal m_dragStartOffset = 0.0;
    qreal m_dragStartPos = 0.0;

    QList<MdCarouselStyle::Keyline> m_defaultKeylines;
    QList<QList<MdCarouselStyle::Keyline>> m_startSteps;
    QList<QList<MdCarouselStyle::Keyline>> m_endSteps;
    QList<MdCarouselStyle::Keyline> m_currentKeylines;

    Phase m_phase = Phase::Idle;
    QTimer *m_animationTimer = nullptr;
    QElapsedTimer *m_animationClock = nullptr;
    qreal m_animationFrom = 0.0;
    qreal m_animationTo = 0.0;
    MdSpring m_animationSpring;
};

} // namespace md

#endif // MD_CAROUSEL_H
