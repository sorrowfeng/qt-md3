#ifndef MD_TIMELINE_H
#define MD_TIMELINE_H

// MdTimeline — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "List + 连线" — a vertical list of events
// joined by a connector line.
//
// MD3 has no timeline family. The control is the standard event log
// composition, assembled from the families that do publish numbers:
//
//   row       List one-line metrics (md.comp.list.list-item.*): 56 px floor,
//             TitleMedium headline + BodyMedium supporting
//   dot       8 px disc on the leading gutter (the progress track reading:
//             primary when the step is done / active, outline when upcoming)
//   line      2 px outline-variant connector (md.comp.divider.* thickness)
//   error     md.sys.color.error marks a failed item
//
// The widget is a value-model list (`MdTimelineItem`) that paints itself,
// matching the other Stage 2 value APIs. Non-interactive: no hover / press
// states, no focus ring — timeline rows are read, not clicked.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtGui/QColor>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

/// Visual state of one timeline node.
enum class MdTimelineStatus {
    /// Upcoming / neutral: outline-variant dot, outline-variant connector.
    Default,
    /// In progress: primary dot with an outer ring, primary connector above.
    Active,
    /// Completed: solid primary dot, primary connector above.
    Complete,
    /// Failed: solid error dot, error connector above.
    Error,
    Count,
};

/// One event row in the timeline.
struct MdTimelineItem
{
    QString title;
    QString supporting;
    QString timestamp;
    MdTimelineStatus status = MdTimelineStatus::Default;

    MdTimelineItem() = default;
    MdTimelineItem(const QString &title_, const QString &supporting_,
                   const QString &timestamp_ = QString(),
                   MdTimelineStatus status_ = MdTimelineStatus::Default)
        : title(title_)
        , supporting(supporting_)
        , timestamp(timestamp_)
        , status(status_)
    {
    }

    bool operator==(const MdTimelineItem &other) const
    {
        return title == other.title && supporting == other.supporting
                && timestamp == other.timestamp && status == other.status;
    }
};

class QT_MD3_EXPORT MdTimeline : public QWidget
{
    Q_OBJECT

    /// Event rows, top to bottom.
    Q_PROPERTY(QList<md::MdTimelineItem> items READ items WRITE setItems NOTIFY itemsChanged)
    /// Draw the connector line through the gutter. Off leaves only dots.
    Q_PROPERTY(bool connectorVisible READ isConnectorVisible WRITE setConnectorVisible
                   NOTIFY connectorVisibleChanged)
    /// Draw each item's timestamp on the trailing edge.
    Q_PROPERTY(bool timestampsVisible READ isTimestampsVisible WRITE setTimestampsVisible
                   NOTIFY timestampsVisibleChanged)

public:
    explicit MdTimeline(QWidget *parent = nullptr);
    ~MdTimeline() override;

    QList<MdTimelineItem> items() const { return m_items; }
    void setItems(const QList<MdTimelineItem> &items);

    void addItem(const MdTimelineItem &item);
    void clearItems();

    bool isConnectorVisible() const { return m_connectorVisible; }
    void setConnectorVisible(bool visible);

    bool isTimestampsVisible() const { return m_timestampsVisible; }
    void setTimestampsVisible(bool visible);

    /// Resolved title colour: on-surface.
    QColor resolvedTitleColor() const;
    /// Resolved supporting / timestamp colour: on-surface-variant.
    QColor resolvedSupportingColor() const;
    /// Resolved connector colour: outline-variant.
    QColor resolvedConnectorColor() const;
    /// Resolved dot colour for `status`.
    QColor resolvedDotColor(MdTimelineStatus status) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

signals:
    void itemsChanged(const QList<md::MdTimelineItem> &items);
    void connectorVisibleChanged(bool visible);
    void timestampsVisibleChanged(bool visible);

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    qreal itemHeight(int index, qreal textWidth) const;
    QSizeF measure(int width) const;

    QList<MdTimelineItem> m_items;
    bool m_connectorVisible = true;
    bool m_timestampsVisible = true;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdTimelineStatus)
Q_DECLARE_METATYPE(md::MdTimelineItem)
Q_DECLARE_METATYPE(QList<md::MdTimelineItem>)

#endif // MD_TIMELINE_H
