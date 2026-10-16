#include "MdInputNumber.h"

#include "core/MdIconButtonTokens.h"
#include "core/MdIcon.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdTextField.h"

#include <QtCore/QEvent>
#include <QtCore/QLocale>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QHBoxLayout>

namespace md {

namespace {

/// `md.comp.icon-button.xsmall.container.height` — 32 px.
constexpr int kStepperSide = 32;
/// The gap between the two stacked steppers.
constexpr int kStepperGap = 0;
/// The trailing inset the stepper pair sits at inside the field box.
constexpr int kStepperInset = 8;
/// Keep the editable text clear of the steppers.
constexpr int kTextTrailingReserve = kStepperSide + kStepperInset * 2;

} // namespace

MdInputNumber::MdInputNumber(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdInputNumber::~MdInputNumber()
{
    // Children are destroyed in ~QWidget, which runs *before* ~QObject
    // auto-disconnects. A child emitting during its own destruction would
    // then invoke a slot on a half-destroyed MdInputNumber (the
    // "class destructor may have already run" assert). Cut the wires first.
    if (m_field) {
        disconnect(m_field, nullptr, this, nullptr);
    }
    if (m_up) {
        disconnect(m_up, nullptr, this, nullptr);
    }
    if (m_down) {
        disconnect(m_down, nullptr, this, nullptr);
    }
}

void MdInputNumber::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_field = new MdTextField(this);
    m_field->setVariant(MdTextFieldVariant::Filled);
    // The steppers own the trailing slot; the field must not paint its own
    // trailing icon on top of them.
    m_field->setTrailingIconName(QString());
    m_field->setContentsMargins(0, 0, kTextTrailingReserve, 0);

    m_up = new MdIconButton(QStringLiteral("arrow_drop_up"), this);
    m_up->setVariant(IconButtonVariant::Standard);
    m_up->setButtonSize(ButtonSize::XSmall);
    m_up->setFocusPolicy(Qt::NoFocus);
    m_up->setAccessibleName(QStringLiteral("Increase"));

    m_down = new MdIconButton(QStringLiteral("arrow_drop_down"), this);
    m_down->setVariant(IconButtonVariant::Standard);
    m_down->setButtonSize(ButtonSize::XSmall);
    m_down->setFocusPolicy(Qt::NoFocus);
    m_down->setAccessibleName(QStringLiteral("Decrease"));

    connect(m_field, &QLineEdit::editingFinished, this, &MdInputNumber::onFieldEditingFinished);
    connect(m_field, &QLineEdit::textChanged, this, &MdInputNumber::onFieldTextChanged);
    connect(m_up, &QAbstractButton::clicked, this, &MdInputNumber::onUpClicked);
    connect(m_down, &QAbstractButton::clicked, this, &MdInputNumber::onDownClicked);

    // Up/Down on the field is the spin-box keyboard contract.
    m_field->installEventFilter(this);

    m_lastGoodText = valueText();
    syncFieldText();
    layoutChildren();

    MdStyleBase::connectThemeUpdate(this, &MdInputNumber::onThemeChanged);
}

void MdInputNumber::setValue(double value)
{
    applyClamped(value, false);
}

void MdInputNumber::applyClamped(double value, bool emitStepped)
{
    const double lo = qMin(m_minimum, m_maximum);
    const double hi = qMax(m_minimum, m_maximum);
    const double clamped = qBound(lo, value, hi);
    if (qFuzzyCompare(m_value, clamped)) {
        // Still refresh the field so a prefix/decimals change lands.
        syncFieldText();
        return;
    }
    m_value = clamped;
    syncFieldText();
    emit valueChanged(m_value);
    if (emitStepped) {
        emit stepped(m_value);
    }
}

void MdInputNumber::setMinimum(double minimum)
{
    if (qFuzzyCompare(m_minimum, minimum)) {
        return;
    }
    m_minimum = minimum;
    if (m_value < qMin(m_minimum, m_maximum)) {
        applyClamped(m_value, false);
    }
    emit minimumChanged(m_minimum);
}

void MdInputNumber::setMaximum(double maximum)
{
    if (qFuzzyCompare(m_maximum, maximum)) {
        return;
    }
    m_maximum = maximum;
    if (m_value > qMax(m_minimum, m_maximum)) {
        applyClamped(m_value, false);
    }
    emit maximumChanged(m_maximum);
}

void MdInputNumber::setSingleStep(double step)
{
    const double sanitized = step < 0.0 ? -step : step;
    if (qFuzzyCompare(m_singleStep, sanitized) || (m_singleStep == 0.0 && sanitized == 0.0)) {
        return;
    }
    m_singleStep = sanitized;
    emit singleStepChanged(m_singleStep);
}

void MdInputNumber::setDecimals(int decimals)
{
    const int clamped = qBound(0, decimals, 10);
    if (m_decimals == clamped) {
        return;
    }
    m_decimals = clamped;
    syncFieldText();
    emit decimalsChanged(m_decimals);
}

void MdInputNumber::setRange(double minimum, double maximum)
{
    const bool minChanged = !qFuzzyCompare(m_minimum, minimum);
    const bool maxChanged = !qFuzzyCompare(m_maximum, maximum);
    if (!minChanged && !maxChanged) {
        return;
    }
    m_minimum = minimum;
    m_maximum = maximum;
    if (minChanged) {
        emit minimumChanged(m_minimum);
    }
    if (maxChanged) {
        emit maximumChanged(m_maximum);
    }
    applyClamped(m_value, false);
}

void MdInputNumber::setPrefix(const QString &prefix)
{
    if (m_prefix == prefix) {
        return;
    }
    m_prefix = prefix;
    syncFieldText();
    emit prefixChanged(m_prefix);
}

void MdInputNumber::setSuffix(const QString &suffix)
{
    if (m_suffix == suffix) {
        return;
    }
    m_suffix = suffix;
    syncFieldText();
    emit suffixChanged(m_suffix);
}

QString MdInputNumber::valueText() const
{
    // C locale for the number itself so the decimal point is a '.'; the
    // prefix/suffix carry any localised unit. QSpinBox formats the same way.
    QString number = QString::number(m_value, 'f', m_decimals);
    return number;
}

bool MdInputNumber::isReadOnly() const
{
    return m_field && m_field->isReadOnly();
}

void MdInputNumber::setReadOnly(bool readOnly)
{
    if (!m_field || m_field->isReadOnly() == readOnly) {
        return;
    }
    m_field->setReadOnly(readOnly);
    if (m_up) {
        m_up->setEnabled(!readOnly && isStepperEnabled(true));
    }
    if (m_down) {
        m_down->setEnabled(!readOnly && isStepperEnabled(false));
    }
    emit readOnlyChanged(readOnly);
}

MdTextFieldVariant MdInputNumber::variant() const
{
    return m_field ? m_field->variant() : MdTextFieldVariant::Filled;
}

void MdInputNumber::setVariant(MdTextFieldVariant variant)
{
    if (!m_field || m_field->variant() == variant) {
        return;
    }
    m_field->setVariant(variant);
    emit variantChanged(variant);
}

QString MdInputNumber::labelText() const
{
    return m_field ? m_field->labelText() : QString();
}

void MdInputNumber::setLabelText(const QString &label)
{
    if (m_field) {
        m_field->setLabelText(label);
    }
}

QString MdInputNumber::supportingText() const
{
    return m_field ? m_field->supportingText() : QString();
}

void MdInputNumber::setSupportingText(const QString &text)
{
    if (m_field) {
        m_field->setSupportingText(text);
    }
}

bool MdInputNumber::hasError() const
{
    return m_field && m_field->hasError();
}

void MdInputNumber::setError(bool error)
{
    if (m_field) {
        m_field->setError(error);
    }
}

void MdInputNumber::setSteppersVisible(bool visible)
{
    if (m_steppersVisible == visible) {
        return;
    }
    m_steppersVisible = visible;
    if (m_up) {
        m_up->setVisible(visible);
    }
    if (m_down) {
        m_down->setVisible(visible);
    }
    if (m_field) {
        m_field->setContentsMargins(0, 0, visible ? kTextTrailingReserve : 0, 0);
    }
    layoutChildren();
    emit steppersVisibleChanged(visible);
}

double MdInputNumber::stepUp()
{
    if (isReadOnly() || !isEnabled()) {
        return m_value;
    }
    // Always move by at least one step even at the bound when the user
    // presses: clamping then reports the bound as the new value.
    applyClamped(m_value + m_singleStep, true);
    return m_value;
}

double MdInputNumber::stepDown()
{
    if (isReadOnly() || !isEnabled()) {
        return m_value;
    }
    applyClamped(m_value - m_singleStep, true);
    return m_value;
}

QSize MdInputNumber::sizeHint() const
{
    const int height = m_field ? int(m_field->sizeHint().height()) : 56;
    int width = m_field ? m_field->sizeHint().width() : 120;
    if (m_steppersVisible) {
        width += kTextTrailingReserve / 2;
    }
    return QSize(qMax(width, 120), height);
}

QSize MdInputNumber::minimumSizeHint() const
{
    return QSize(80, m_field ? int(m_field->minimumSizeHint().height()) : 56);
}

bool MdInputNumber::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_field && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Up) {
            stepUp();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Down) {
            stepDown();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void MdInputNumber::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

void MdInputNumber::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        if (m_up) {
            m_up->setEnabled(isEnabled() && !isReadOnly() && isStepperEnabled(true));
        }
        if (m_down) {
            m_down->setEnabled(isEnabled() && !isReadOnly() && isStepperEnabled(false));
        }
        update();
    }
}

void MdInputNumber::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    layoutChildren();
}

void MdInputNumber::onFieldEditingFinished()
{
    if (!commitFromText()) {
        syncFieldText();
    }
}

void MdInputNumber::onFieldTextChanged(const QString &text)
{
    if (m_syncingText) {
        return;
    }
    Q_UNUSED(text);
    // Live typing does not clamp; that happens on commit so "12|" can become
    // "123" without the caret jumping to "99".
}

void MdInputNumber::onUpClicked()
{
    stepUp();
}

void MdInputNumber::onDownClicked()
{
    stepDown();
}

void MdInputNumber::onThemeChanged()
{
    update();
}

void MdInputNumber::layoutChildren()
{
    if (!m_field || !m_up || !m_down) {
        return;
    }
    const QRect box = rect();
    m_field->setGeometry(box);

    // RTL: the stepper pair sits at the trailing edge, which mirrors.
    const bool rtl = MdStyleBase::isRtl(this);
    const int pairX = rtl ? kStepperInset : box.width() - kStepperInset - kStepperSide;
    const int pairHeight = kStepperSide * 2 + kStepperGap;
    const int pairY = (box.height() - pairHeight) / 2;

    m_up->setGeometry(pairX, pairY, kStepperSide, kStepperSide);
    m_down->setGeometry(pairX, pairY + kStepperSide + kStepperGap, kStepperSide, kStepperSide);
    m_up->setVisible(m_steppersVisible);
    m_down->setVisible(m_steppersVisible);
    m_up->raise();
    m_down->raise();
}

void MdInputNumber::syncFieldText()
{
    if (!m_field) {
        return;
    }
    const QString text = m_prefix + valueText() + m_suffix;
    m_syncingText = true;
    if (m_field->text() != text) {
        m_field->setText(text);
    }
    m_lastGoodText = text;
    m_syncingText = false;
}

bool MdInputNumber::commitFromText()
{
    if (!m_field) {
        return true;
    }
    const QString raw = m_field->text();
    QString numeric = raw;
    if (!m_prefix.isEmpty() && numeric.startsWith(m_prefix)) {
        numeric = numeric.mid(m_prefix.size());
    }
    if (!m_suffix.isEmpty() && numeric.endsWith(m_suffix)) {
        numeric = numeric.left(numeric.size() - m_suffix.size());
    }
    numeric = numeric.trimmed();

    bool ok = false;
    const double parsed = numeric.toDouble(&ok);
    if (!ok) {
        // Reject the edit: restore the last good string and raise the error
        // overlay so the user can see the field refused the input.
        m_field->setError(true);
        syncFieldText();
        return false;
    }
    m_field->setError(false);
    applyClamped(parsed, false);
    return true;
}

bool MdInputNumber::isStepperEnabled(bool up) const
{
    if (up) {
        return m_value < qMax(m_minimum, m_maximum) - 1e-12 || m_singleStep == 0.0;
    }
    return m_value > qMin(m_minimum, m_maximum) + 1e-12 || m_singleStep == 0.0;
}

} // namespace md
