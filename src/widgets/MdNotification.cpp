#include "MdNotification.h"

#include "MdSnackbar.h"

#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace md {

MdNotification::MdNotification(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdNotification::MdNotification(const QString &message, QWidget *parent)
    : QWidget(parent)
{
    init();
    setMessage(message);
}

MdNotification::~MdNotification() = default;

void MdNotification::init()
{
    // The notification is a chrome-free host: the snackbar child paints
    // everything. The host itself takes no focus — Tab lands on the
    // snackbar's own action / dismiss regions.
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAccessibleName(QStringLiteral("notification"));

    m_snackbar = new MdSnackbar(this);
    // Persistent by contract: the close icon is on unless the caller turns it
    // off explicitly (the snackbar export recommends it whenever the surface
    // does not self-dismiss).
    m_snackbar->setHasDismissAction(true);
    m_snackbar->show();

    connect(m_snackbar, &MdSnackbar::actionClicked,
            this, &MdNotification::onSnackbarAction);
    connect(m_snackbar, &MdSnackbar::dismissClicked,
            this, &MdNotification::onSnackbarDismiss);
    connect(m_snackbar, &MdSnackbar::messageChanged,
            this, &MdNotification::messageChanged);
    connect(m_snackbar, &MdSnackbar::actionLabelChanged,
            this, &MdNotification::actionLabelChanged);
    connect(m_snackbar, &MdSnackbar::hasDismissActionChanged,
            this, &MdNotification::hasDismissActionChanged);
    connect(m_snackbar, &MdSnackbar::actionOnNewLineChanged,
            this, &MdNotification::actionOnNewLineChanged);

    relayout();
}

QString MdNotification::message() const
{
    return m_snackbar->message();
}

void MdNotification::setMessage(const QString &message)
{
    m_snackbar->setMessage(message);
    updateGeometry();
    relayout();
}

QString MdNotification::actionLabel() const
{
    return m_snackbar->actionLabel();
}

void MdNotification::setActionLabel(const QString &actionLabel)
{
    m_snackbar->setActionLabel(actionLabel);
    updateGeometry();
    relayout();
}

bool MdNotification::hasDismissAction() const
{
    return m_snackbar->hasDismissAction();
}

void MdNotification::setHasDismissAction(bool show)
{
    m_snackbar->setHasDismissAction(show);
}

bool MdNotification::isActionOnNewLine() const
{
    return m_snackbar->isActionOnNewLine();
}

void MdNotification::setActionOnNewLine(bool onNewLine)
{
    m_snackbar->setActionOnNewLine(onNewLine);
    updateGeometry();
    relayout();
}

void MdNotification::dismiss()
{
    if (m_dismissed) {
        return;
    }
    m_dismissed = true;
    hide();
    emit dismissed();
}

void MdNotification::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MdNotification::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_dismissed = false;
    relayout();
}

void MdNotification::onSnackbarAction()
{
    emit actionClicked();
}

void MdNotification::onSnackbarDismiss()
{
    dismiss();
}

void MdNotification::relayout()
{
    if (m_snackbar == nullptr) {
        return;
    }
    // The snackbar reports heightForWidth; give it the full host rect.
    const int h = m_snackbar->heightForWidth(width() > 0 ? width() : sizeHint().width());
    m_snackbar->setGeometry(0, 0, width() > 0 ? width() : sizeHint().width(), h);
}

QSize MdNotification::sizeHint() const
{
    const int w = 400;
    return QSize(w, m_snackbar != nullptr ? m_snackbar->heightForWidth(w) : 48);
}

QSize MdNotification::minimumSizeHint() const
{
    return QSize(0, 48);
}

int MdNotification::heightForWidth(int width) const
{
    return m_snackbar != nullptr ? m_snackbar->heightForWidth(width) : 48;
}

} // namespace md
