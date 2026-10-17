#include "MdInputDialog.h"

#include "MdButton.h"
#include "MdTextField.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtCore/QEventLoop>
#include <QtGui/QFontMetrics>
#include <QtGui/QKeyEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// The dialog chrome's extra-large corner, from the dialog token set.
constexpr qreal kShadowMargin = 24.0;
constexpr qreal kContentPadding = 24.0;
constexpr qreal kGapTitle = 16.0;
constexpr qreal kGapField = 24.0;
constexpr qreal kGapActions = 24.0;
constexpr qreal kActionSpacing = 8.0;
constexpr qreal kFieldHeight = 56.0;
constexpr qreal kActionHeight = 40.0;
constexpr qreal kMinContainerWidth = 280.0;

} // namespace

MdInputDialog::MdInputDialog(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdInputDialog::MdInputDialog(const QString &title, const QString &label, const QString &text,
                             QWidget *parent)
    : QWidget(parent)
    , m_title(title)
    , m_label(label)
{
    init();
    if (m_field) {
        m_field->setText(text);
    }
}

MdInputDialog::~MdInputDialog()
{
    // Cut wires before members die (the MdInputNumber pattern).
    disconnect(this, nullptr, nullptr, nullptr);
    if (m_loop) {
        m_loop->quit();
    }
}

void MdInputDialog::init()
{
    setAttribute(Qt::WA_StyledBackground, false);
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("input dialog"));

    m_tokens = MdDialogTokens::resolve();
    m_acceptText = QStringLiteral("OK");
    m_rejectText = QStringLiteral("Cancel");

    m_field = new MdTextField(this);
    m_field->setVariant(MdTextFieldVariant::Filled);
    m_field->setObjectName(QStringLiteral("md-input-dialog-field"));
    if (!m_label.isEmpty()) {
        m_field->setLabelText(m_label);
    }

    m_acceptButton = new MdButton(this);
    m_acceptButton->setVariant(ButtonVariant::Filled);
    m_acceptButton->setButtonSize(ButtonSize::XSmall);
    m_acceptButton->setText(m_acceptText);
    m_acceptButton->setObjectName(QStringLiteral("md-input-dialog-accept"));
    connect(m_acceptButton, &MdButton::clicked, this, &MdInputDialog::onAcceptClicked);

    m_rejectButton = new MdButton(this);
    m_rejectButton->setVariant(ButtonVariant::Text);
    m_rejectButton->setButtonSize(ButtonSize::XSmall);
    m_rejectButton->setText(m_rejectText);
    m_rejectButton->setObjectName(QStringLiteral("md-input-dialog-reject"));
    connect(m_rejectButton, &MdButton::clicked, this, &MdInputDialog::onRejectClicked);

    MdStyleBase::connectThemeUpdate(this, &MdInputDialog::onThemeChanged);
}

QString MdInputDialog::text() const
{
    return m_field ? m_field->text() : QString();
}

void MdInputDialog::setText(const QString &text)
{
    if (!m_field || m_field->text() == text) {
        return;
    }
    m_field->setText(text);
    emit textChanged(text);
}

QString MdInputDialog::placeholderText() const
{
    return m_field ? m_field->placeholderText() : QString();
}

void MdInputDialog::setPlaceholderText(const QString &placeholder)
{
    if (m_field) {
        m_field->setPlaceholderText(placeholder);
    }
    emit placeholderTextChanged(placeholder);
}

void MdInputDialog::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateGeometry();
    update();
    emit titleChanged(m_title);
}

void MdInputDialog::setLabelText(const QString &label)
{
    if (m_label == label) {
        return;
    }
    m_label = label;
    if (m_field) {
        m_field->setLabelText(label);
    }
    update();
    emit labelTextChanged(m_label);
}

void MdInputDialog::setMode(MdInputDialogMode mode)
{
    if (mode >= MdInputDialogMode::Count || m_mode == mode) {
        return;
    }
    m_mode = mode;
    if (m_field) {
        m_field->setEchoMode(m_mode == MdInputDialogMode::PasswordInput ? QLineEdit::Password
                                                                       : QLineEdit::Normal);
    }
    emit modeChanged(m_mode);
}

void MdInputDialog::setAcceptText(const QString &text)
{
    if (m_acceptText == text) {
        return;
    }
    m_acceptText = text;
    if (m_acceptButton) {
        m_acceptButton->setText(text);
    }
    layoutChildren();
    emit acceptTextChanged(m_acceptText);
}

void MdInputDialog::setRejectText(const QString &text)
{
    if (m_rejectText == text) {
        return;
    }
    m_rejectText = text;
    if (m_rejectButton) {
        m_rejectButton->setText(text);
    }
    layoutChildren();
    emit rejectTextChanged(m_rejectText);
}

int MdInputDialog::intValue() const
{
    return text().toInt();
}

void MdInputDialog::setIntValue(int value)
{
    setText(QString::number(value));
}

double MdInputDialog::doubleValue() const
{
    return text().toDouble();
}

void MdInputDialog::setDoubleValue(double value)
{
    setText(QString::number(value, 'g', 12));
}

QRectF MdInputDialog::containerRect() const
{
    return QRectF(kShadowMargin, kShadowMargin, width() - 2 * kShadowMargin,
                  height() - 2 * kShadowMargin);
}

QSize MdInputDialog::sizeHint() const
{
    return QSize(400, heightForWidth(400));
}

int MdInputDialog::heightForWidth(int width) const
{
    const qreal inner = width - 2 * kShadowMargin - 2 * kContentPadding;
    qreal height = kContentPadding;
    if (!m_title.isEmpty()) {
        height += QFontMetrics(MdTypeScale::font(TypeStyle::HeadlineSmall)).height() + kGapTitle;
    }
    height += kFieldHeight + kGapField;
    height += kActionHeight + kGapActions;
    height += kContentPadding;
    Q_UNUSED(inner);
    return int(height + 2 * kShadowMargin);
}

bool MdInputDialog::exec()
{
    m_finished = false;
    show();
    if (m_field) {
        m_field->setFocus();
    }
    QEventLoop loop;
    m_loop = &loop;
    const QMetaObject::Connection okConn =
        connect(this, &MdInputDialog::finished, &loop, &QEventLoop::quit);
    loop.exec();
    disconnect(okConn);
    m_loop = nullptr;
    return !m_finished ? false : !m_acceptText.isEmpty();
}

void MdInputDialog::onAcceptClicked()
{
    applyResult(true);
}

void MdInputDialog::onRejectClicked()
{
    applyResult(false);
}

void MdInputDialog::applyResult(bool ok)
{
    if (m_finished) {
        return;
    }
    m_finished = true;
    if (ok) {
        emit accepted(text());
    } else {
        emit rejected();
    }
    emit finished(ok);
    if (m_loop) {
        m_loop->quit();
    }
}

void MdInputDialog::layoutChildren()
{
    const QRectF container = containerRect();
    const qreal innerX = container.left() + kContentPadding;
    const qreal innerW = container.width() - 2 * kContentPadding;

    qreal y = container.top() + kContentPadding;
    if (!m_title.isEmpty()) {
        y += QFontMetrics(MdTypeScale::font(TypeStyle::HeadlineSmall)).height() + kGapTitle;
    }

    if (m_field) {
        m_field->setGeometry(QRectF(innerX, y, innerW, kFieldHeight).toRect());
        y += kFieldHeight + kGapField;
    }

    // End-aligned action row (the dialog's FlowRow reading).
    qreal actionY = container.bottom() - kContentPadding - kActionHeight;
    qreal x = container.right() - kContentPadding;
    if (m_rejectButton) {
        const QSize hint = m_rejectButton->sizeHint();
        x -= hint.width();
        m_rejectButton->setGeometry(qRound(x), qRound(actionY + (kActionHeight - hint.height()) * 0.5),
                                    hint.width(), hint.height());
        x -= kActionSpacing;
    }
    if (m_acceptButton) {
        const QSize hint = m_acceptButton->sizeHint();
        x -= hint.width();
        m_acceptButton->setGeometry(qRound(x), qRound(actionY + (kActionHeight - hint.height()) * 0.5),
                                    hint.width(), hint.height());
    }
}

void MdInputDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF container = containerRect();
    const QPainterPath path = MdShape::roundedRect(container, ShapeCorner::ExtraLarge);
    painter.fillPath(path, MdTheme::instance().color(ColorRole::SurfaceContainerHigh));

    if (!m_title.isEmpty()) {
        painter.setFont(MdTypeScale::font(TypeStyle::HeadlineSmall));
        painter.setPen(MdTheme::instance().color(ColorRole::OnSurface));
        const QRectF titleBox(container.left() + kContentPadding, container.top() + kContentPadding,
                              container.width() - 2 * kContentPadding,
                              QFontMetrics(MdTypeScale::font(TypeStyle::HeadlineSmall)).height());
        painter.drawText(titleBox, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, m_title);
    }
}

void MdInputDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        applyResult(false);
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        applyResult(true);
        return;
    }
    QWidget::keyPressEvent(event);
}

void MdInputDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

void MdInputDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        layoutChildren();
        update();
    }
    QWidget::changeEvent(event);
}

void MdInputDialog::onThemeChanged()
{
    m_tokens = MdDialogTokens::resolve();
    update();
}

bool MdInputDialog::getText(QWidget *parent, const QString &title, const QString &label,
                            QString *text, bool *ok)
{
    MdInputDialog dialog(title, label, text ? *text : QString(), parent);
    dialog.setMode(MdInputDialogMode::TextInput);
    const bool accepted = dialog.exec();
    if (ok) {
        *ok = accepted;
    }
    if (text && accepted) {
        *text = dialog.text();
    }
    return accepted;
}

bool MdInputDialog::getInt(QWidget *parent, const QString &title, const QString &label,
                           int *value, bool *ok)
{
    MdInputDialog dialog(title, label, value ? QString::number(*value) : QString(), parent);
    dialog.setMode(MdInputDialogMode::IntInput);
    const bool accepted = dialog.exec();
    if (ok) {
        *ok = accepted;
    }
    if (value && accepted) {
        *value = dialog.intValue();
    }
    return accepted;
}

} // namespace md
