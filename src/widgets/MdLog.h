#ifndef MD_LOG_H
#define MD_LOG_H

// MdLog — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "surface container + label typography
// monospace variant".
//
// A scrollable console-like line view. MD3 has no log family; the widget is
// the surface-container reading of a monospace text view:
//
//   container surface-container (level 1 feel), ShapeCorner::Medium clip
//   lines     label-medium metrics at a monospaced family (the type scale's
//             plain family forced to a fixed-pitch face)
//   levels    Info (on-surface), Success (primary), Warning (tertiary),
//             Error (error)
//
// The view keeps a line model (not a QPlainTextEdit document) so levels stay
// first-class. Auto-scroll parks the viewport on the newest line until the
// user scrolls away.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QPaintEvent;
class QWheelEvent;

namespace md {

/// Severity of one log line. Drives the line's content colour.
enum class MdLogLevel {
    Info,
    Success,
    Warning,
    Error,
    Count,
};

/// One log line.
struct QT_MD3_EXPORT MdLogLine
{
    QString text;
    MdLogLevel level = MdLogLevel::Info;
    QString timestamp;

    MdLogLine() = default;
    MdLogLine(const QString &text_, MdLogLevel level_ = MdLogLevel::Info,
              const QString &timestamp_ = QString())
        : text(text_)
        , level(level_)
        , timestamp(timestamp_)
    {
    }

    bool operator==(const MdLogLine &other) const
    {
        return text == other.text && level == other.level && timestamp == other.timestamp;
    }
};

class QT_MD3_EXPORT MdLog : public QWidget
{
    Q_OBJECT

    /// The log's lines.
    Q_PROPERTY(QList<md::MdLogLine> lines READ lines WRITE setLines NOTIFY linesChanged)
    /// When true the viewport follows the newest appended line.
    Q_PROPERTY(bool autoScroll READ autoScroll WRITE setAutoScroll NOTIFY autoScrollChanged)
    /// When true a monospaced face is used; when false the label-medium
    /// brand/plain family.
    Q_PROPERTY(bool monospace READ isMonospace WRITE setMonospace NOTIFY monospaceChanged)
    /// When true each line's timestamp is drawn in the leading gutter.
    Q_PROPERTY(bool timestampsVisible READ timestampsVisible WRITE setTimestampsVisible NOTIFY
                   timestampsVisibleChanged)
    /// Maximum number of retained lines. Older lines are dropped. 0 = unbounded.
    Q_PROPERTY(int maxLines READ maxLines WRITE setMaxLines NOTIFY maxLinesChanged)

public:
    explicit MdLog(QWidget *parent = nullptr);
    ~MdLog() override;

    QList<MdLogLine> lines() const { return m_lines; }
    void setLines(const QList<MdLogLine> &lines);
    void appendLine(const QString &text, MdLogLevel level = MdLogLevel::Info,
                    const QString &timestamp = QString());
    void clearLines();

    int lineCount() const { return int(m_lines.size()); }
    MdLogLine lineAt(int index) const;

    bool autoScroll() const { return m_autoScroll; }
    void setAutoScroll(bool on);

    bool isMonospace() const { return m_monospace; }
    void setMonospace(bool on);

    bool timestampsVisible() const { return m_timestampsVisible; }
    void setTimestampsVisible(bool on);

    int maxLines() const { return m_maxLines; }
    void setMaxLines(int max);

    /// Content colour for a level.
    QColor resolvedLevelColor(MdLogLevel level) const;
    /// Resolved container background.
    QColor resolvedSurfaceColor() const;

    /// Current vertical scroll offset in lines (0 = top / oldest).
    int scrollOffset() const { return m_scrollOffset; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void linesChanged(const QList<md::MdLogLine> &lines);
    void autoScrollChanged(bool on);
    void monospaceChanged(bool on);
    void timestampsVisibleChanged(bool on);
    void maxLinesChanged(int max);
    /// A line was double-clicked.
    void lineActivated(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    qreal lineHeight() const;
    QFont lineFont() const;
    int visibleRows() const;
    void clampScroll();
    int lineAtPosition(const QPointF &pos) const;

    QList<MdLogLine> m_lines;
    bool m_autoScroll = true;
    bool m_monospace = true;
    bool m_timestampsVisible = true;
    int m_maxLines = 2000;
    int m_scrollOffset = 0;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdLogLevel)
Q_DECLARE_METATYPE(md::MdLogLine)
Q_DECLARE_METATYPE(QList<md::MdLogLine>)

#endif // MD_LOG_H
