#include "MdTextField.h"

#include "styles/MdTextFieldStyle.h"

#include "core/MdMotion.h"

#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QPainter>

namespace md {

namespace {

constexpr int kTickMs = 8;

} // namespace

MdTextField::MdTextField(QWidget *parent)
    : QLineEdit(parent)
{
    // The decoration paints on the widget itself. QLineEdit's own paintEvent
    // fills QPalette::Base first — in dark mode that default is still light,
    // and the disabled row's on-surface @ 0.04 cannot cover it (the field
    // came out a white slab). Keep the chrome transparent so only the style's
    // container shows. Same rule as MdTextArea's viewport.
    setAutoFillBackground(false);
    QPalette pal = palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    pal.setColor(QPalette::Window, Qt::transparent);
    setPalette(pal);
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
    m_labelFloatFrom = m_labelFloatTarget;
    m_labelClock.start();
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

void MdTextField::setLeadingIconName(const QString &iconName)
{
    if (iconName == m_leadingIconName) {
        return;
    }
    m_leadingIconName = iconName;
    emit leadingIconNameChanged(iconName);
    updateTextMargins();
    update();
}

void MdTextField::setTrailingIconName(const QString &iconName)
{
    if (iconName == m_trailingIconName) {
        return;
    }
    m_trailingIconName = iconName;
    emit trailingIconNameChanged(iconName);
    updateTextMargins();
    update();
}

void MdTextField::updateTextMargins()
{
    // Keep the edit text clear of the 24 px icon slots plus their 16 px
    // padding — the same inset the label already uses.
    const MdTextFieldTokens &tokens = textFieldTokens();
    const qreal lead = m_leadingIconName.isEmpty() ? 0.0 : tokens.iconSize + 8.0;
    const qreal trail = m_trailingIconName.isEmpty() ? 0.0 : tokens.iconSize + 8.0;
    setTextMargins(int(lead), 0, int(trail), 0);
}

bool MdTextField::isPopulated() const
{
    return hasFocus() || !text().isEmpty();
}

// --- geometry ---------------------------------------------------------------

QRectF MdTextField::containerRect() const
{
    // Same rule as MdTextArea: the supporting line is painted just below the
    // container, so the container must leave that band inside the widget.
    const qreal supporting = m_supporting.isEmpty() ? 0.0 : 20.0;
    return QRectF(0.0, 0.0, rect().width(), rect().height() - supporting);
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

QRectF MdTextField::leadingIconRect() const
{
    if (m_leadingIconName.isEmpty()) {
        return QRectF();
    }
    const MdTextFieldTokens &tokens = textFieldTokens();
    const qreal size = tokens.iconSize;
    const qreal x = layoutDirection() == Qt::RightToLeft
                        ? containerRect().width() - 16.0 - size
                        : 16.0;
    const qreal y = (containerRect().height() - size) / 2.0;
    return QRectF(x, y, size, size);
}

QRectF MdTextField::trailingIconRect() const
{
    if (m_trailingIconName.isEmpty()) {
        return QRectF();
    }
    const MdTextFieldTokens &tokens = textFieldTokens();
    const qreal size = tokens.iconSize;
    const qreal x = layoutDirection() == Qt::RightToLeft
                        ? 16.0
                        : containerRect().width() - 16.0 - size;
    const qreal y = (containerRect().height() - size) / 2.0;
    return QRectF(x, y, size, size);
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
    const qreal target = isPopulated() ? 1.0 : 0.0;
    if (!qFuzzyCompare(m_labelFloatTarget + 1.0, target + 1.0)) {
        // Target changed: continue from the float currently on screen.
        m_labelFloatFrom = m_labelFloat;
        m_labelClock.restart();
    }
    m_labelFloatTarget = target;
}

void MdTextField::tickAnimations()
{
    if (qFuzzyCompare(m_labelFloat + 1.0, m_labelFloatTarget + 1.0)) {
        return;
    }
    const MdSpring spring = MdMotion::spring(MotionSpring::EffectsDefault);
    const qreal seconds = qreal(m_labelClock.elapsed()) / 1000.0;
    const qreal progress = spring.valueAt(seconds);
    m_labelFloat = m_labelFloatFrom + (m_labelFloatTarget - m_labelFloatFrom) * progress;
    if (progress >= 1.0
        || qreal(m_labelClock.elapsed()) >= spring.settlingDurationMs()
        || qAbs(m_labelFloat - m_labelFloatTarget) < 0.01) {
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

void MdTextField::enterEvent(MdEnterEvent *event)
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
