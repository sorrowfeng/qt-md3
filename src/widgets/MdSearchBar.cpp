#include "MdSearchBar.h"

#include "styles/MdSearchBarStyle.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QProxyStyle>

namespace md {

namespace {

/// Qt 6's QLineEdit is a plain QWidget — its rectangular "frame" comes from
/// the style's PE_FrameLineEdit / PE_PanelLineEdit. Those two primitives paint
/// a sunken slab over the pill (two horizontal lines, corners cut off), so
/// they are no-ops here: MdSearchBarStyle owns the chrome.
class SearchLineEditStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override
    {
        if (element == PE_FrameLineEdit || element == PE_PanelLineEdit) {
            return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

SearchLineEditStyle *searchLineEditStyle()
{
    // Never destroyed: setStyle() does not take ownership, and every bar
    // shares one filter.
    static SearchLineEditStyle *style = new SearchLineEditStyle;
    return style;
}

} // namespace

MdSearchBar::MdSearchBar(QWidget *parent)
    : QLineEdit(parent)
{
    // Same rule as MdTextField: the style paints the pill on this widget, so
    // QLineEdit's default Base fill must not show through (it is light even
    // in dark mode).
    setAutoFillBackground(false);
    QPalette pal = palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    pal.setColor(QPalette::Window, Qt::transparent);
    setPalette(pal);
    // Kill the style-drawn rectangular slab; the pill is the only chrome.
    setStyle(searchLineEditStyle());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // The leading search icon owns the first 16+24 px; the input line starts
    // after it (and after the trailing slot on the other side).
    const MdSearchTokens &tokens = searchTokens();
    setTextMargins(int(tokens.leadingSpace + tokens.iconSize + 8.0), 0,
                   int(tokens.trailingSpace), 0);
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
    // Chrome first, then QLineEdit's text / cursor / selection on top.
    // Painting the container *after* QLineEdit laid an opaque slab over the
    // input — typing worked but the glyphs were invisible.
    {
        QPainter painter(this);
        MdSearchBarStyle::paintSearchBar(painter, *this, searchTokens());
    }
    QLineEdit::paintEvent(event);
}

void MdSearchBar::mousePressEvent(QMouseEvent *event)
{
    QLineEdit::mousePressEvent(event);
    emit activated();
}

void MdSearchBar::enterEvent(MdEnterEvent *event)
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
