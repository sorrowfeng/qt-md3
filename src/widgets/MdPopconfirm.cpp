#include "MdPopconfirm.h"

#include "core/MdTypeScale.h"
#include "widgets/MdButton.h"

#include <QtGui/QKeyEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

namespace md {

namespace {

/// Gap between the question and the action row (Dialog's action gap).
constexpr qreal kActionGap = 16.0;
/// Gap between the two buttons.
constexpr qreal kButtonGap = 8.0;

} // namespace

MdPopconfirm::MdPopconfirm(QWidget *parent)
    : MdPopover(parent)
{
    init();
}

MdPopconfirm::~MdPopconfirm()
{
    // Children are destroyed in ~QWidget, which runs *before* ~QObject
    // auto-disconnects. Cut the wires first (the MdInputNumber pattern).
    if (m_confirm) {
        disconnect(m_confirm, nullptr, this, nullptr);
    }
    if (m_cancel) {
        disconnect(m_cancel, nullptr, this, nullptr);
    }
}

void MdPopconfirm::init()
{
    setAccessibleName(QStringLiteral("popconfirm"));

    m_confirm = new MdButton(QStringLiteral("OK"), this);
    m_confirm->setVariant(ButtonVariant::Text);
    m_cancel = new MdButton(QStringLiteral("Cancel"), this);
    m_cancel->setVariant(ButtonVariant::Text);

    connect(m_confirm, &MdButton::clicked, this, &MdPopconfirm::onConfirmClicked);
    connect(m_cancel, &MdButton::clicked, this, &MdPopconfirm::onCancelClicked);
    connect(this, &MdPopover::closed, this, &MdPopconfirm::onClosed);

    rebuildBody();
}

void MdPopconfirm::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    rebuildBody();
    emit textChanged(m_text);
}

void MdPopconfirm::setConfirmText(const QString &text)
{
    if (m_confirmText == text) {
        return;
    }
    m_confirmText = text;
    if (m_confirm) {
        m_confirm->setText(text.isEmpty() ? QStringLiteral("OK") : text);
    }
    emit confirmTextChanged(m_confirmText);
}

void MdPopconfirm::setCancelText(const QString &text)
{
    if (m_cancelText == text) {
        return;
    }
    m_cancelText = text;
    if (m_cancel) {
        m_cancel->setText(text.isEmpty() ? QStringLiteral("Cancel") : text);
    }
    emit cancelTextChanged(m_cancelText);
}

void MdPopconfirm::open(const QRect &anchorRect)
{
    rebuildBody();
    popup(anchorRect);
}

void MdPopconfirm::rebuildBody()
{
    // Body widget: question + trailing action row, both reading Dialog tokens.
    auto *body = new QWidget(this);
    auto *layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *label = new QLabel(m_text, body);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    label->setFont(MdTypeScale::font(TypeStyle::BodyMedium));
    QPalette pal = label->palette();
    pal.setColor(QPalette::WindowText, resolvedTitleColor());
    label->setPalette(pal);
    layout->addWidget(label);

    layout->addSpacing(qRound(kActionGap));

    auto *row = new QWidget(body);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(qRound(kButtonGap));
    rowLayout->addStretch(1);
    if (m_cancel) {
        m_cancel->setParent(row);
        rowLayout->addWidget(m_cancel);
    }
    if (m_confirm) {
        m_confirm->setParent(row);
        rowLayout->addWidget(m_confirm);
    }
    layout->addWidget(row);

    setContentWidget(body);
}

void MdPopconfirm::onConfirmClicked()
{
    hide();
    emit confirmed();
}

void MdPopconfirm::onCancelClicked()
{
    hide();
    emit cancelled();
}

void MdPopconfirm::onClosed()
{
    // Outside click / Esc is a cancel.
    emit cancelled();
}

void MdPopconfirm::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        onConfirmClicked();
        return;
    }
    MdPopover::keyPressEvent(event);
}

} // namespace md
