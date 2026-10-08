#include "MdDialog.h"

#include "styles/MdDialogStyle.h"
#include "MdButton.h"
#include "core/MdTheme.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QWidget>

namespace md {

MdDialog::MdDialog(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdDialog::~MdDialog() = default;

void MdDialog::init()
{
    setFocusPolicy(Qt::NoFocus);
    MdDialogStyle::shared();
    MdStyleBase::connectThemeUpdate(this, &MdDialog::onThemeChanged);
}

void MdDialog::onThemeChanged()
{
    invalidateTokens();
    update();
}

void MdDialog::invalidateTokens()
{
    m_tokensDirty = true;
    update();
}

void MdDialog::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    emit titleChanged(m_title);
    updateGeometry();
    syncChildGeometry();
    update();
}

void MdDialog::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    emit textChanged(m_text);
    updateGeometry();
    syncChildGeometry();
    update();
}

void MdDialog::setIconWidget(QWidget *icon)
{
    if (m_icon == icon) {
        return;
    }
    delete m_icon;
    m_icon = icon;
    if (m_icon) {
        m_icon->setParent(this);
        m_icon->show();
    }
    updateGeometry();
    syncChildGeometry();
    update();
}

void MdDialog::setConfirmButton(MdButton *button)
{
    if (m_confirmButton == button) {
        return;
    }
    delete m_confirmButton;
    m_confirmButton = button;
    if (m_confirmButton) {
        m_confirmButton->setParent(this);
        m_confirmButton->show();
    }
    updateGeometry();
    syncChildGeometry();
    update();
}

void MdDialog::setDismissButton(MdButton *button)
{
    if (m_dismissButton == button) {
        return;
    }
    delete m_dismissButton;
    m_dismissButton = button;
    if (m_dismissButton) {
        m_dismissButton->setParent(this);
        m_dismissButton->show();
    }
    updateGeometry();
    syncChildGeometry();
    update();
}

const MdDialogTokens &MdDialog::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdDialogTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

QRectF MdDialog::containerRect() const
{
    return QRectF(rect()).adjusted(MdDialogTokens::kShadowMargin,
                                   MdDialogTokens::kShadowMargin,
                                   -MdDialogTokens::kShadowMargin,
                                   -MdDialogTokens::kShadowMargin);
}

MdDialogStyle::Layout MdDialog::currentLayout() const
{
    MdDialogStyle::ContentSpec content;
    content.hasIcon = m_icon != nullptr;
    content.title = m_title;
    content.text = m_text;
    if (m_confirmButton) {
        content.actionWidths.append(qreal(m_confirmButton->sizeHint().width()));
    }
    if (m_dismissButton) {
        content.actionWidths.append(qreal(m_dismissButton->sizeHint().width()));
    }
    return MdDialogStyle::layoutFor(qreal(width()), content, tokens());
}

QSize MdDialog::sizeHint() const
{
    // Compose bounds the dialog width to [280, 560] through the platform
    // default; the natural size here is the cap, and hosts clamp down.
    return QSize(int(MdDialogTokens::kMaxWidth), heightForWidth(int(MdDialogTokens::kMaxWidth)));
}

int MdDialog::heightForWidth(int width) const
{
    MdDialogStyle::ContentSpec content;
    content.hasIcon = m_icon != nullptr;
    content.title = m_title;
    content.text = m_text;
    if (m_confirmButton) {
        content.actionWidths.append(qreal(m_confirmButton->sizeHint().width()));
    }
    if (m_dismissButton) {
        content.actionWidths.append(qreal(m_dismissButton->sizeHint().width()));
    }
    return int(MdDialogStyle::heightForWidth(qreal(width), content, tokens()));
}

void MdDialog::setPaintOpacity(qreal opacity)
{
    if (qFuzzyCompare(m_paintOpacity, opacity)) {
        return;
    }
    m_paintOpacity = opacity;
    update();
}

void MdDialog::keyPressEvent(QKeyEvent *event)
{
    // Compose: back press requests dismissal (onDismissRequest).
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        emit dismissed();
        return;
    }
    QWidget::keyPressEvent(event);
}

void MdDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    syncChildGeometry();
}

void MdDialog::syncChildGeometry()
{
    const MdDialogStyle::Layout layout = currentLayout();
    if (m_icon) {
        m_icon->setGeometry(layout.iconRect.toRect());
    }
    // Buttons: rows laid out end-aligned; row 0 carries the confirm first
    // when wrapped, or the single row holds [dismiss, confirm].
    if (m_confirmButton || m_dismissButton) {
        QList<QRectF> confirmRects;
        QList<QRectF> dismissRects;
        if (layout.actionRows.size() == 2) {
            confirmRects = layout.actionRows.at(0);
            dismissRects = layout.actionRows.at(1);
        } else if (layout.actionRows.size() == 1) {
            const QList<QRectF> &row = layout.actionRows.first();
            // Left-to-right: dismiss, then confirm.
            if (row.size() == 2) {
                dismissRects.append(row.first());
                confirmRects.append(row.last());
            } else if (!row.isEmpty()) {
                confirmRects.append(row.first());
            }
        }
        if (m_confirmButton && !confirmRects.isEmpty()) {
            m_confirmButton->setGeometry(confirmRects.first().toRect());
        }
        if (m_dismissButton && !dismissRects.isEmpty()) {
            m_dismissButton->setGeometry(dismissRects.first().toRect());
        }
    }
}

} // namespace md
