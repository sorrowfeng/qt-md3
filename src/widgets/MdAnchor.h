#ifndef MD_ANCHOR_H
#define MD_ANCHOR_H

// MdAnchor / MdAffix — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "滚动吸附与锚点" — scroll pinning and
// anchor navigation.
//
// Two halves of the same scroll-affinity pair:
//
//   MdAffix    pins a child widget to an edge of a scroll area's viewport
//              once the content scrolls past an offset (the "sticky
//              header / toolbar" reading). Uses list-row height and the
//              surface-container background so it reads as chrome.
//   MdAnchor   a vertical list of destination links that scrolls a target
//              QAbstractScrollArea to named anchors (Y offsets registered
//              by the host). The current anchor tracks the scroll offset.
//
// Neither invents a token: MdAffix is `surface-container` + label-large;
// MdAnchor is list one-line rows (56 px) with secondary-container
// selection — the navigation-drawer item language reused, exactly as
// appendix D asks.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QAbstractScrollArea;
class QPaintEvent;

Q_DECLARE_OPAQUE_POINTER(QAbstractScrollArea *)

namespace md {

class MdAffixPrivate;

/// A sticky panel pinned to the top or bottom of a scroll viewport.
class QT_MD3_EXPORT MdAffix : public QWidget
{
    Q_OBJECT

    /// The watched scroll area. nullptr detaches.
    Q_PROPERTY(QAbstractScrollArea *scrollArea READ scrollArea WRITE setScrollArea NOTIFY
                   scrollAreaChanged)
    /// Offset (in content pixels) past which the affix sticks. Default 0.
    Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY offsetChanged)
    /// Whether the affix is currently pinned (scrolled past `offset`).
    Q_PROPERTY(bool pinned READ isPinned NOTIFY pinnedChanged)

public:
    explicit MdAffix(QWidget *parent = nullptr);
    ~MdAffix() override;

    QAbstractScrollArea *scrollArea() const { return m_scrollArea; }
    void setScrollArea(QAbstractScrollArea *area);

    int offset() const { return m_offset; }
    void setOffset(int offset);

    bool isPinned() const { return m_pinned; }

    /// Body widget filling the affix. Pass nullptr to clear.
    void setContentWidget(QWidget *content);
    QWidget *contentWidget() const { return m_content; }

    /// Resolved background: surface-container when pinned, transparent
    /// (so the page shows through) when free-floating.
    QColor resolvedBackgroundColor() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void scrollAreaChanged(QAbstractScrollArea *area);
    void offsetChanged(int offset);
    void pinnedChanged(bool pinned);
    void contentWidgetChanged(QWidget *content);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onScrollChanged();

private:
    void init();
    void hookScrollArea();
    void unhookScrollArea();
    void refreshPinned();

    QAbstractScrollArea *m_scrollArea = nullptr;
    QWidget *m_content = nullptr;
    int m_offset = 0;
    bool m_pinned = false;
};

/// One anchor entry.
struct QT_MD3_EXPORT MdAnchorItem
{
    QString text;
    int contentY = 0;

    MdAnchorItem() = default;
    MdAnchorItem(const QString &text_, int contentY_)
        : text(text_)
        , contentY(contentY_)
    {
    }

    bool operator==(const MdAnchorItem &other) const
    {
        return text == other.text && contentY == other.contentY;
    }
};

/// The anchor link list that drives a scroll area.
class QT_MD3_EXPORT MdAnchor : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QList<md::MdAnchorItem> items READ items WRITE setItems NOTIFY itemsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QAbstractScrollArea *scrollArea READ scrollArea WRITE setScrollArea NOTIFY
                   scrollAreaChanged)

public:
    explicit MdAnchor(QWidget *parent = nullptr);
    ~MdAnchor() override;

    QList<MdAnchorItem> items() const { return m_items; }
    void setItems(const QList<MdAnchorItem> &items);
    void addItem(const QString &text, int contentY);
    void clearItems();

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    QAbstractScrollArea *scrollArea() const { return m_scrollArea; }
    void setScrollArea(QAbstractScrollArea *area);

    int count() const { return int(m_items.size()); }
    MdAnchorItem itemAt(int index) const;

    /// Hit-test the painted rows. -1 when not on a row.
    int itemAtPosition(const QPointF &pos) const;

    /// Resolved content colour for a row.
    QColor resolvedItemColor() const;
    /// Resolved selected-row background.
    QColor resolvedSelectedBackground() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void itemsChanged(const QList<md::MdAnchorItem> &items);
    void currentIndexChanged(int index);
    void scrollAreaChanged(QAbstractScrollArea *area);
    void itemClicked(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onScrollChanged();

private:
    void init();
    void hookScrollArea();
    void unhookScrollArea();
    void syncToScroll();
    qreal rowHeight() const;

    QList<MdAnchorItem> m_items;
    int m_currentIndex = -1;
    QAbstractScrollArea *m_scrollArea = nullptr;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdAnchorItem)
Q_DECLARE_METATYPE(QList<md::MdAnchorItem>)

#endif // MD_ANCHOR_H
