#include "MdSearchBar.h"

#include "styles/MdSearchBarStyle.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QProxyStyle>

#include <cmath>

namespace md {

namespace {

/// Qt 6's QLineEdit is a plain QWidget — its rectangular "frame" comes from
/// the style's PE_FrameLineEdit / PE_PanelLineEdit (and, on some platform
/// styles, a CC_EditBox / PE_Frame slab riding the same QStyleOptionFrame).
/// Those primitives paint a sunken rectangle over the pill: two horizontal
/// lines across the top and bottom, and the stadium caps cut off at the
/// corners. They are no-ops here: MdSearchBarStyle owns the chrome.
class SearchLineEditStyle : public QProxyStyle
{
public:
    void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                       QPainter *painter, const QWidget *widget) const override
    {
        switch (element) {
        case PE_FrameLineEdit:
        case PE_PanelLineEdit:
        case PE_Frame:
        case PE_Widget:
            return;
        default:
            break;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    void drawControl(ControlElement element, const QStyleOption *option,
                     QPainter *painter, const QWidget *widget) const override
    {
        if (element == CE_ShapedFrame) {
            return;
        }
        QProxyStyle::drawControl(element, option, painter, widget);
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
    // setFrame(false) as well as the proxy: initStyleOption takes its
    // lineWidth from hasFrame(), and a non-zero lineWidth is what asks the
    // style for a frame in the first place.
    setFrame(false);
    setStyle(searchLineEditStyle());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // The leading search icon owns the first 16+24 px; the input line starts
    // after it (and after the trailing slot on the other side). The focus-ring
    // margin is included so text sits inside the pill, not the widget edge.
    const MdSearchTokens &tokens = searchTokens();
    const qreal inset = MdSearchBarStyle::focusRingInset(tokens);
    setTextMargins(int(inset + tokens.leadingSpace + tokens.iconSize + 8.0), 0,
                   int(inset + tokens.trailingSpace), 0);
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
    const QRectF container = containerRect();
    const QRectF r(container.left() + tokens.leadingSpace,
                   container.top() + (container.height() - tokens.iconSize) / 2.0,
                   tokens.iconSize, tokens.iconSize);
    // The leading icon rides the *leading* edge — the right side under RTL.
    return layoutDirection() == Qt::RightToLeft
        ? QRectF(container.right() - r.right() + container.left(), r.y(), r.width(), r.height())
        : r;
}

QRectF MdSearchBar::containerRect() const
{
    // The widget is the container plus a transparent margin the outward focus
    // indicator paints into. Qt clips a child to its own rect, so without the
    // margin the indicator is cut off — see MdSearchBarStyle::focusRingInset().
    const qreal inset = MdSearchBarStyle::focusRingInset(searchTokens());
    return QRectF(rect()).adjusted(inset, inset, -inset, -inset);
}

QSize MdSearchBar::sizeHint() const
{
    const MdSearchTokens &tokens = searchTokens();
    const qreal inset = MdSearchBarStyle::focusRingInset(tokens);
    return QSize(int(std::ceil(360.0 + 2.0 * inset)),
                 int(std::ceil(tokens.containerHeight + 2.0 * inset)));
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
