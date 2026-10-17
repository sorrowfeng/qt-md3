#include "MdLog.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontDatabase>
#include <QtGui/QFontMetrics>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QWheelEvent>

namespace md {

namespace {

constexpr int kLineHPadding = 12;
constexpr int kLineVPadding = 6;
constexpr int kTimestampWidth = 72;

QStringList monospaceFamilies()
{
    // Qt 5 has no static QFontDatabase::families(); always use an instance.
    const QFontDatabase db;
    QStringList preferred;
    preferred << QStringLiteral("Consolas") << QStringLiteral("Cascadia Mono")
              << QStringLiteral("Menlo") << QStringLiteral("DejaVu Sans Mono")
              << QStringLiteral("Courier New");
    const QStringList available = db.families();
    QStringList result;
    for (const QString &name : preferred) {
        if (available.contains(name)) {
            result.append(name);
        }
    }
    return result;
}

} // namespace

MdLog::MdLog(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdLog::~MdLog() = default;

void MdLog::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("log"));
    MdStyleBase::connectThemeUpdate(this, &MdLog::onThemeChanged);
}

void MdLog::setLines(const QList<MdLogLine> &lines)
{
    m_lines = lines;
    if (m_maxLines > 0 && int(m_lines.size()) > m_maxLines) {
        m_lines = m_lines.mid(int(m_lines.size()) - m_maxLines);
    }
    if (m_autoScroll) {
        m_scrollOffset = qMax(0, int(m_lines.size()) - visibleRows());
    }
    clampScroll();
    update();
    emit linesChanged(m_lines);
}

void MdLog::appendLine(const QString &text, MdLogLevel level, const QString &timestamp)
{
    if (level >= MdLogLevel::Count) {
        level = MdLogLevel::Info;
    }
    m_lines.append(MdLogLine(text, level, timestamp));
    if (m_maxLines > 0 && int(m_lines.size()) > m_maxLines) {
        m_lines.removeFirst();
        if (m_scrollOffset > 0) {
            --m_scrollOffset;
        }
    }
    if (m_autoScroll) {
        m_scrollOffset = qMax(0, int(m_lines.size()) - visibleRows());
    }
    clampScroll();
    update();
    emit linesChanged(m_lines);
}

void MdLog::clearLines()
{
    if (m_lines.isEmpty()) {
        return;
    }
    m_lines.clear();
    m_scrollOffset = 0;
    update();
    emit linesChanged(m_lines);
}

MdLogLine MdLog::lineAt(int index) const
{
    if (index < 0 || index >= int(m_lines.size())) {
        return MdLogLine();
    }
    return m_lines.at(index);
}

void MdLog::setAutoScroll(bool on)
{
    if (m_autoScroll == on) {
        return;
    }
    m_autoScroll = on;
    if (m_autoScroll) {
        m_scrollOffset = qMax(0, int(m_lines.size()) - visibleRows());
        clampScroll();
        update();
    }
    emit autoScrollChanged(m_autoScroll);
}

void MdLog::setMonospace(bool on)
{
    if (m_monospace == on) {
        return;
    }
    m_monospace = on;
    updateGeometry();
    update();
    emit monospaceChanged(m_monospace);
}

void MdLog::setTimestampsVisible(bool on)
{
    if (m_timestampsVisible == on) {
        return;
    }
    m_timestampsVisible = on;
    update();
    emit timestampsVisibleChanged(m_timestampsVisible);
}

void MdLog::setMaxLines(int max)
{
    const int clamped = qMax(0, max);
    if (m_maxLines == clamped) {
        return;
    }
    m_maxLines = clamped;
    if (m_maxLines > 0 && int(m_lines.size()) > m_maxLines) {
        m_lines = m_lines.mid(int(m_lines.size()) - m_maxLines);
        clampScroll();
        emit linesChanged(m_lines);
    }
    emit maxLinesChanged(m_maxLines);
    update();
}

QColor MdLog::resolvedLevelColor(MdLogLevel level) const
{
    const MdTheme &theme = MdTheme::instance();
    switch (level) {
    case MdLogLevel::Info:
        return theme.color(ColorRole::OnSurface);
    case MdLogLevel::Success:
        return theme.color(ColorRole::Primary);
    case MdLogLevel::Warning:
        return theme.color(ColorRole::Tertiary);
    case MdLogLevel::Error:
        return theme.color(ColorRole::Error);
    case MdLogLevel::Count:
        break;
    }
    return theme.color(ColorRole::OnSurface);
}

QColor MdLog::resolvedSurfaceColor() const
{
    return MdTheme::instance().color(ColorRole::SurfaceContainer);
}

QFont MdLog::lineFont() const
{
    if (!m_monospace) {
        return MdTypeScale::font(TypeStyle::LabelMedium);
    }
    QFont font = MdTypeScale::font(TypeStyle::LabelMedium);
    const QStringList families = monospaceFamilies();
    if (!families.isEmpty()) {
        font.setFamily(families.first());
        font.setFixedPitch(true);
    }
    return font;
}

qreal MdLog::lineHeight() const
{
    return QFontMetrics(lineFont()).height() + kLineVPadding * 2;
}

int MdLog::visibleRows() const
{
    const qreal lh = lineHeight();
    if (lh <= 0) {
        return 1;
    }
    return qMax(1, int(height() / lh));
}

void MdLog::clampScroll()
{
    const int maxOffset = qMax(0, int(m_lines.size()) - visibleRows());
    if (m_scrollOffset > maxOffset) {
        m_scrollOffset = maxOffset;
    }
    if (m_scrollOffset < 0) {
        m_scrollOffset = 0;
    }
}

int MdLog::lineAtPosition(const QPointF &pos) const
{
    const qreal lh = lineHeight();
    if (lh <= 0) {
        return -1;
    }
    const int row = int(pos.y() / lh);
    const int index = m_scrollOffset + row;
    if (row < 0 || index < 0 || index >= int(m_lines.size())) {
        return -1;
    }
    return index;
}

QSize MdLog::sizeHint() const
{
    return QSize(320, int(lineHeight() * 8));
}

QSize MdLog::minimumSizeHint() const
{
    return QSize(160, int(lineHeight() * 3));
}

void MdLog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds(0, 0, width(), height());
    const QPainterPath path = MdShape::roundedRect(bounds, ShapeCorner::Medium);
    painter.fillPath(path, resolvedSurfaceColor());

    painter.setClipPath(path);

    const qreal lh = lineHeight();
    const QFont font = lineFont();
    const QFontMetrics fm(font);
    painter.setFont(font);

    const int rows = visibleRows() + 1;
    for (int row = 0; row < rows; ++row) {
        const int index = m_scrollOffset + row;
        if (index < 0 || index >= int(m_lines.size())) {
            break;
        }
        const MdLogLine &line = m_lines.at(index);
        const qreal y = row * lh;
        qreal x = kLineHPadding;
        qreal textWidth = width() - kLineHPadding * 2;

        if (m_timestampsVisible && !line.timestamp.isEmpty()) {
            painter.setPen(MdTheme::instance().color(ColorRole::OnSurfaceVariant));
            painter.drawText(QRectF(x, y, kTimestampWidth, lh),
                             Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                             line.timestamp);
            x += kTimestampWidth;
            textWidth -= kTimestampWidth;
        }

        painter.setPen(resolvedLevelColor(line.level));
        painter.drawText(QRectF(x, y, textWidth, lh),
                         Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, line.text);
    }
    Q_UNUSED(fm);
}

void MdLog::wheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();
    if (delta != 0) {
        m_scrollOffset -= (delta > 0 ? 3 : -3);
        m_autoScroll = m_scrollOffset >= qMax(0, int(m_lines.size()) - visibleRows());
        clampScroll();
        update();
        emit autoScrollChanged(m_autoScroll);
    }
    QWidget::wheelEvent(event);
}

void MdLog::mouseDoubleClickEvent(QMouseEvent *event)
{
    const int index = lineAtPosition(md::mousePosition(event));
    if (index >= 0) {
        emit lineActivated(index);
    }
    QWidget::mouseDoubleClickEvent(event);
}

void MdLog::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
        m_scrollOffset -= 1;
        m_autoScroll = false;
        emit autoScrollChanged(m_autoScroll);
        clampScroll();
        update();
        return;
    case Qt::Key_Down:
        m_scrollOffset += 1;
        clampScroll();
        m_autoScroll = m_scrollOffset >= qMax(0, int(m_lines.size()) - visibleRows());
        emit autoScrollChanged(m_autoScroll);
        update();
        return;
    case Qt::Key_PageUp:
        m_scrollOffset -= visibleRows();
        m_autoScroll = false;
        emit autoScrollChanged(m_autoScroll);
        clampScroll();
        update();
        return;
    case Qt::Key_PageDown:
        m_scrollOffset += visibleRows();
        clampScroll();
        m_autoScroll = m_scrollOffset >= qMax(0, int(m_lines.size()) - visibleRows());
        emit autoScrollChanged(m_autoScroll);
        update();
        return;
    case Qt::Key_Home:
        m_scrollOffset = 0;
        m_autoScroll = false;
        emit autoScrollChanged(m_autoScroll);
        update();
        return;
    case Qt::Key_End:
        m_scrollOffset = qMax(0, int(m_lines.size()) - visibleRows());
        m_autoScroll = true;
        emit autoScrollChanged(m_autoScroll);
        update();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdLog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    clampScroll();
    if (m_autoScroll) {
        m_scrollOffset = qMax(0, int(m_lines.size()) - visibleRows());
    }
}

void MdLog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
    }
    QWidget::changeEvent(event);
}

void MdLog::onThemeChanged()
{
    update();
}

} // namespace md
