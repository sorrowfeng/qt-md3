#include "MdTextArea.h"

#include "styles/MdTextAreaStyle.h"

#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QPainter>

namespace md {

namespace {

constexpr int kTickMs = 8;

qreal approach(qreal current, qreal target, qreal rate)
{
    return current + (target - current) * rate;
}

} // namespace

MdTextArea::MdTextArea(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    // The decoration paints on the scroll-area widget itself; the viewport
    // must stay transparent so the container fill shows through (the
    // QPlainTextEdit document paints on the viewport child).
    viewport()->setAutoFillBackground(false);
    QPalette pal = viewport()->palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    viewport()->setPalette(pal);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_TranslucentBackground, false);
    connect(this, &QPlainTextEdit::textChanged, this, [this]() {
        emit populatedChanged(isPopulated());
        retargetAnimations();
    });

    QTimer *timer = new QTimer(this);
    timer->setInterval(kTickMs);
    connect(timer, &QTimer::timeout, this, &MdTextArea::tickAnimations);
    timer->start();

    retargetAnimations();
    m_labelFloat = m_labelFloatTarget;
}

MdTextArea::~MdTextArea() = default;

void MdTextArea::setLabelText(const QString &label)
{
    if (label == m_label) {
        return;
    }
    m_label = label;
    emit labelTextChanged(label);
    update();
}

void MdTextArea::setSupportingText(const QString &text)
{
    if (text == m_supporting) {
        return;
    }
    m_supporting = text;
    emit supportingTextChanged(text);
    update();
}

void MdTextArea::setError(bool error)
{
    if (error == m_error) {
        return;
    }
    m_error = error;
    emit errorChanged(error);
    retargetAnimations();
    update();
}

void MdTextArea::setVariant(MdTextFieldVariant variant)
{
    if (variant == m_variant) {
        return;
    }
    m_variant = variant;
    m_tokensDirty = true;
    emit variantChanged(variant);
    update();
}

void MdTextArea::setRows(int rows)
{
    const int clamped = qMax(1, rows);
    if (clamped == m_rows) {
        return;
    }
    m_rows = clamped;
    emit rowsChanged(clamped);
    updateGeometry();
    update();
}

bool MdTextArea::isPopulated() const
{
    return hasFocus() || !toPlainText().isEmpty();
}

// --- geometry ---------------------------------------------------------------

QRectF MdTextArea::containerRect() const
{
    return QRectF(rect());
}

QRectF MdTextArea::labelRect() const
{
    // At rest the label sits on the first text line; floated it rises to the
    // container's top edge on body-small.
    const qreal restY = 8.0 + 8.0;
    const qreal floatedY = 4.0;
    const qreal y = restY + (floatedY - restY) * m_labelFloat;
    return QRectF(16.0, y, rect().width() - 32.0, 24.0);
}

QSize MdTextArea::sizeHint() const
{
    const MdTextFieldTokens &tokens = textFieldTokens();
    const QFontMetricsF fm(font());
    // The body is `rows` lines tall; the label band sits above it and the
    // supporting text below.
    const qreal body = m_rows * fm.height() + 16.0;
    const qreal supporting = m_supporting.isEmpty() ? 0.0 : 20.0;
    return QSize(280, int(tokens.containerHeight + body + supporting - 24.0));
}

QSize MdTextArea::minimumSizeHint() const
{
    return QSize(180, sizeHint().height());
}

// --- tokens -----------------------------------------------------------------

const MdTextFieldTokens &MdTextArea::textFieldTokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdTextFieldTokens::resolve(m_variant);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdTextArea::setTextFieldTokens(const MdTextFieldTokens &tokens)
{
    m_tokens = tokens;
    m_tokensDirty = false;
    update();
}

// --- animations ----------------------------------------------------------------

void MdTextArea::retargetAnimations()
{
    m_labelFloatTarget = isPopulated() ? 1.0 : 0.0;
}

void MdTextArea::tickAnimations()
{
    if (qFuzzyCompare(m_labelFloat, m_labelFloatTarget)) {
        return;
    }
    m_labelFloat = approach(m_labelFloat, m_labelFloatTarget, 0.35);
    if (qAbs(m_labelFloat - m_labelFloatTarget) < 0.01) {
        m_labelFloat = m_labelFloatTarget;
    }
    update();
}

// --- events ----------------------------------------------------------------------

void MdTextArea::paintEvent(QPaintEvent *event)
{
    // A QAbstractScrollArea forwards the *viewport's* paint to this handler —
    // the document paints there. The decoration (container, indicator, label,
    // supporting text) is painted by the Pattern A filter on the area's own
    // Paint event (MdPaintFilterHub → MdTextAreaStyle::drawWidget), under the
    // transparent viewport. Painting it here as well would hand a painter for
    // 	his to a pass that belongs to the viewport — the painter goes dead
    // mid-decorate (the render-smoke warnings this comment replaces).
    QPlainTextEdit::paintEvent(event);
}

void MdTextArea::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    update();
}

void MdTextArea::enterEvent(QEnterEvent *event)
{
    QPlainTextEdit::enterEvent(event);
    m_hovered = true;
    retargetAnimations();
    update();
}

void MdTextArea::leaveEvent(QEvent *event)
{
    QPlainTextEdit::leaveEvent(event);
    m_hovered = false;
    retargetAnimations();
    update();
}

void MdTextArea::focusInEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusInEvent(event);
    m_focusIsKeyboard = event->reason() != Qt::MouseFocusReason
        && event->reason() != Qt::PopupFocusReason;
    retargetAnimations();
    update();
}

void MdTextArea::focusOutEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusOutEvent(event);
    m_focusIsKeyboard = false;
    retargetAnimations();
    update();
}

} // namespace md
