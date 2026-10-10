#include "MdTextField.h"

#include "styles/MdTextFieldStyle.h"

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

MdTextField::MdTextField(QWidget *parent)
    : QLineEdit(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(this, &QLineEdit::textChanged, this, [this](const QString &) {
        emit populatedChanged(isPopulated());
        retargetAnimations();
    });

    QTimer *timer = new QTimer(this);
    timer->setInterval(kTickMs);
    connect(timer, &QTimer::timeout, this, &MdTextField::tickAnimations);
    timer->start();

    retargetAnimations();
    m_labelFloat = m_labelFloatTarget;
}

MdTextField::MdTextField(const QString &contents, QWidget *parent)
    : MdTextField(parent)
{
    setText(contents);
}

MdTextField::~MdTextField() = default;

void MdTextField::setLabelText(const QString &label)
{
    if (label == m_label) {
        return;
    }
    m_label = label;
    emit labelTextChanged(label);
    update();
}

void MdTextField::setSupportingText(const QString &text)
{
    if (text == m_supporting) {
        return;
    }
    m_supporting = text;
    emit supportingTextChanged(text);
    update();
}

void MdTextField::setPlaceholderText(const QString &text)
{
    if (text == m_placeholder) {
        return;
    }
    m_placeholder = text;
    emit placeholderTextChanged(text);
    update();
}

void MdTextField::setError(bool error)
{
    if (error == m_error) {
        return;
    }
    m_error = error;
    emit errorChanged(error);
    retargetAnimations();
    update();
}

void MdTextField::setVariant(MdTextFieldVariant variant)
{
    if (variant == m_variant) {
        return;
    }
    m_variant = variant;
    m_tokensDirty = true;
    emit variantChanged(variant);
    update();
}

bool MdTextField::isPopulated() const
{
    return hasFocus() || !text().isEmpty();
}

// --- geometry ---------------------------------------------------------------

QRectF MdTextField::containerRect() const
{
    return QRectF(rect());
}

QRectF MdTextField::labelRect() const
{
    const MdTextFieldTokens &tokens = textFieldTokens();
    // At rest the label sits in the input line; floated it rises to the top
    // edge on body-small.
    const qreal restY = (rect().height() - 24.0) / 2.0;
    const qreal floatedY = 8.0;
    const qreal y = restY + (floatedY - restY) * m_labelFloat;
    return QRectF(16.0, y, rect().width() - 32.0, 24.0);
}

QSize MdTextField::sizeHint() const
{
    const MdTextFieldTokens &tokens = textFieldTokens();
    return QSize(240, int(tokens.containerHeight + (m_supporting.isEmpty() ? 0.0 : 20.0)));
}

QSize MdTextField::minimumSizeHint() const
{
    return QSize(120, sizeHint().height());
}

// --- tokens -----------------------------------------------------------------

const MdTextFieldTokens &MdTextField::textFieldTokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdTextFieldTokens::resolve(m_variant);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdTextField::setTextFieldTokens(const MdTextFieldTokens &tokens)
{
    m_tokens = tokens;
    m_tokensDirty = false;
    update();
}

// --- animations ----------------------------------------------------------------

void MdTextField::retargetAnimations()
{
    m_labelFloatTarget = isPopulated() ? 1.0 : 0.0;
}

void MdTextField::tickAnimations()
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

void MdTextField::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);
    QPainter painter(this);
    MdTextFieldStyle::paintTextField(painter, *this, textFieldTokens());
}

void MdTextField::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);
    update();
}

void MdTextField::enterEvent(QEnterEvent *event)
{
    QLineEdit::enterEvent(event);
    m_hovered = true;
    retargetAnimations();
    update();
}

void MdTextField::leaveEvent(QEvent *event)
{
    QLineEdit::leaveEvent(event);
    m_hovered = false;
    retargetAnimations();
    update();
}

void MdTextField::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);
    m_focusIsKeyboard = event->reason() != Qt::MouseFocusReason
        && event->reason() != Qt::PopupFocusReason;
    retargetAnimations();
    update();
}

void MdTextField::focusOutEvent(QFocusEvent *event)
{
    QLineEdit::focusOutEvent(event);
    m_focusIsKeyboard = false;
    retargetAnimations();
    update();
}

} // namespace md
