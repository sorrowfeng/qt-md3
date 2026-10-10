#include "MdSearchBar.h"

#include "styles/MdSearchBarStyle.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

namespace md {

MdSearchBar::MdSearchBar(QWidget *parent)
    : QLineEdit(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

MdSearchBar::~MdSearchBar() = default;

void MdSearchBar::setPlaceholderText(const QString &text)
{
    if (text == m_placeholder) {
        return;
    }
    m_placeholder = text;
    emit placeholderTextChanged(text);
    update();
}

void MdSearchBar::setSurface(MdSearchSurface surface)
{
    if (surface == m_surface) {
        return;
    }
    m_surface = surface;
    m_tokensDirty = true;
    emit surfaceChanged(surface);
    update();
}

// --- geometry ---------------------------------------------------------------

QRectF MdSearchBar::leadingIconRect() const
{
    const MdSearchTokens &tokens = searchTokens();
    const QRectF r(tokens.leadingSpace, (rect().height() - tokens.iconSize) / 2.0,
                   tokens.iconSize, tokens.iconSize);
    // The leading icon rides the *leading* edge — the right side under RTL.
    return layoutDirection() == Qt::RightToLeft
        ? QRectF(rect().width() - r.right(), r.y(), r.width(), r.height())
        : r;
}

QRectF MdSearchBar::containerRect() const
{
    return QRectF(rect());
}

QSize MdSearchBar::sizeHint() const
{
    const MdSearchTokens &tokens = searchTokens();
    return QSize(360, int(tokens.containerHeight));
}

QSize MdSearchBar::minimumSizeHint() const
{
    return QSize(180, sizeHint().height());
}

// --- tokens -----------------------------------------------------------------

const MdSearchTokens &MdSearchBar::searchTokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdSearchTokens::resolve(m_surface);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdSearchBar::setSearchTokens(const MdSearchTokens &tokens)
{
    m_tokens = tokens;
    m_tokensDirty = false;
    update();
}

// --- events ---------------------------------------------------------------------

void MdSearchBar::paintEvent(QPaintEvent *event)
{
    QLineEdit::paintEvent(event);
    QPainter painter(this);
    MdSearchBarStyle::paintSearchBar(painter, *this, searchTokens());
}

void MdSearchBar::mousePressEvent(QMouseEvent *event)
{
    QLineEdit::mousePressEvent(event);
    emit activated();
}

void MdSearchBar::enterEvent(QEnterEvent *event)
{
    QLineEdit::enterEvent(event);
    m_hovered = true;
    update();
}

void MdSearchBar::leaveEvent(QEvent *event)
{
    QLineEdit::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdSearchBar::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit searchRequested(text());
        return;
    }
    QLineEdit::keyPressEvent(event);
}

} // namespace md
