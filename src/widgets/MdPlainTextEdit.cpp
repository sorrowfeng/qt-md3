#include "MdPlainTextEdit.h"

#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QTextCursor>
#include <QtGui/QTextDocument>
#include <QtGui/QWheelEvent>
#include <QtGui/QTextBlock>

namespace md {

namespace {

constexpr qreal kGutterPad = 8.0;
constexpr qreal kMinZoom = 0.5;
constexpr qreal kMaxZoom = 3.0;

} // namespace

MdPlainTextEdit::MdPlainTextEdit(QWidget *parent)
    : QPlainTextEdit(parent)
{
    init();
}

MdPlainTextEdit::~MdPlainTextEdit() = default;

void MdPlainTextEdit::init()
{
    setAccessibleName(QStringLiteral("plain text edit"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    setFrameShape(QFrame::NoFrame);
    setAttribute(Qt::WA_StyledBackground, false);

    applyZoomFont();
    updateGutterWidth();

    connect(this, &MdPlainTextEdit::blockCountChanged, this,
            &MdPlainTextEdit::updateGutterWidth);
    connect(this, &MdPlainTextEdit::updateRequest, this,
            [this](const QRect &rect, int dy) {
                Q_UNUSED(rect);
                Q_UNUSED(dy);
                viewport()->update();
            });

    MdStyleBase::connectThemeUpdate(this, &MdPlainTextEdit::onThemeChanged);
}

void MdPlainTextEdit::setLabelText(const QString &label)
{
    if (m_label == label) {
        return;
    }
    m_label = label;
    update();
    emit labelTextChanged(m_label);
}

void MdPlainTextEdit::setSupportingText(const QString &text)
{
    if (m_supporting == text) {
        return;
    }
    m_supporting = text;
    update();
    emit supportingTextChanged(m_supporting);
}

void MdPlainTextEdit::setError(bool error)
{
    if (m_error == error) {
        return;
    }
    m_error = error;
    update();
    emit errorChanged(m_error);
}

void MdPlainTextEdit::setVariant(MdTextFieldVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    update();
    emit variantChanged(m_variant);
}

void MdPlainTextEdit::setLineNumbersVisible(bool visible)
{
    if (m_lineNumbers == visible) {
        return;
    }
    m_lineNumbers = visible;
    updateGutterWidth();
    update();
    emit lineNumbersVisibleChanged(m_lineNumbers);
}

void MdPlainTextEdit::setCurrentLineHighlight(bool on)
{
    if (m_currentLineHighlight == on) {
        return;
    }
    m_currentLineHighlight = on;
    viewport()->update();
    emit currentLineHighlightChanged(m_currentLineHighlight);
}

void MdPlainTextEdit::setZoomFactor(qreal factor)
{
    const qreal clamped = qBound(kMinZoom, factor, kMaxZoom);
    if (qFuzzyCompare(m_zoom, clamped)) {
        return;
    }
    m_zoom = clamped;
    applyZoomFont();
    emit zoomFactorChanged(m_zoom);
}

void MdPlainTextEdit::applyZoomFont()
{
    QFont font = MdTypeScale::font(TypeStyle::BodyLarge);
    // The type scale publishes a pixel size; Qt5 has no static families() so
    // keep the zoom on the pixel size and pick a fixed-pitch face below.
    const int pixel = qMax(1, qRound(font.pixelSize() * m_zoom));
    font.setPixelSize(pixel);
    font.setFamily(QStringLiteral("Consolas"));
    setFont(font);
    updateGutterWidth();
}

int MdPlainTextEdit::gutterWidth() const
{
    if (!m_lineNumbers) {
        return 0;
    }
    const int digits = qMax(2, QString::number(qMax(1, blockCount())).size());
    const QFontMetrics fm(font());
    return int(digits * fm.horizontalAdvance(QLatin1Char('9')) + kGutterPad * 2);
}

void MdPlainTextEdit::updateGutterWidth()
{
    setViewportMargins(gutterWidth(), 0, 0, 0);
    viewport()->update();
}

QColor MdPlainTextEdit::resolvedIndicatorColor() const
{
    const MdTheme &theme = MdTheme::instance();
    if (m_error) {
        return theme.color(ColorRole::Error);
    }
    if (hasFocus()) {
        return theme.color(ColorRole::Primary);
    }
    return theme.color(ColorRole::Outline);
}

QColor MdPlainTextEdit::resolvedSupportingColor() const
{
    const MdTheme &theme = MdTheme::instance();
    return m_error ? theme.color(ColorRole::Error) : theme.color(ColorRole::OnSurfaceVariant);
}

QColor MdPlainTextEdit::resolvedGutterColor() const
{
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

bool MdPlainTextEdit::findNext(const QString &needle, bool caseSensitive)
{
    if (needle.isEmpty()) {
        return false;
    }
    const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QTextCursor cursor = textCursor();
    cursor = document()->find(needle, cursor, cs ? QTextDocument::FindCaseSensitively
                                                : QTextDocument::FindFlags());
    if (cursor.isNull()) {
        cursor = document()->find(needle, 0, cs ? QTextDocument::FindCaseSensitively
                                                : QTextDocument::FindFlags());
    }
    if (cursor.isNull()) {
        return false;
    }
    setTextCursor(cursor);
    return true;
}

bool MdPlainTextEdit::replace(const QString &from, const QString &to, bool caseSensitive)
{
    if (from.isEmpty()) {
        return false;
    }
    const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()
        && cursor.selectedText().compare(from, cs) == 0) {
        cursor.insertText(to);
    }
    return findNext(from, caseSensitive);
}

int MdPlainTextEdit::replaceAll(const QString &from, const QString &to, bool caseSensitive)
{
    if (from.isEmpty()) {
        return 0;
    }
    const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
    int count = 0;
    QTextCursor cursor(document());
    cursor.beginEditBlock();
    while (true) {
        cursor = document()->find(from, cursor,
                                  cs ? QTextDocument::FindCaseSensitively : QTextDocument::FindFlags());
        if (cursor.isNull()) {
            break;
        }
        cursor.insertText(to);
        ++count;
    }
    cursor.endEditBlock();
    return count;
}

void MdPlainTextEdit::paintGutter(QPaintEvent *event)
{
    QPainter painter(viewport());
    const int gw = gutterWidth();
    painter.fillRect(QRect(0, 0, gw, viewport()->height()),
                     MdTheme::instance().color(ColorRole::Surface));
    painter.setPen(resolvedGutterColor());
    painter.setFont(font());

    const QFontMetrics fm(font());
    QTextBlock block = firstVisibleBlock();
    int top = int(blockBoundingGeometry(block).translated(contentOffset()).top());
    while (block.isValid() && top <= viewport()->height()) {
        if (block.isVisible()) {
            const QString number = QString::number(block.blockNumber() + 1);
            painter.drawText(QRectF(kGutterPad, top, gw - 2 * kGutterPad, fm.height()),
                             Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, number);
        }
        block = block.next();
        top += int(blockBoundingRect(block).height());
        if (top > event->rect().bottom()) {
            break;
        }
    }
}

void MdPlainTextEdit::paintEvent(QPaintEvent *event)
{
    if (m_currentLineHighlight) {
        const QList<QTextEdit::ExtraSelection> extras;
        Q_UNUSED(extras);
        QTextEdit::ExtraSelection selection;
        QColor highlight = MdStateLayer::overlay(
            MdTheme::instance().color(ColorRole::OnSurface), StateLayerKind::Hover);
        selection.format.setBackground(highlight);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        QList<QTextEdit::ExtraSelection> list;
        list.append(selection);
        setExtraSelections(list);
    }

    QPlainTextEdit::paintEvent(event);

    if (m_lineNumbers) {
        paintGutter(event);
    }
}

void MdPlainTextEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    updateGutterWidth();
}

void MdPlainTextEdit::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta != 0) {
            setZoomFactor(m_zoom + (delta > 0 ? 0.1 : -0.1));
            event->accept();
            return;
        }
    }
    QPlainTextEdit::wheelEvent(event);
}

void MdPlainTextEdit::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGutterWidth();
    }
    QPlainTextEdit::changeEvent(event);
}

void MdPlainTextEdit::onThemeChanged()
{
    update();
    viewport()->update();
}

} // namespace md
