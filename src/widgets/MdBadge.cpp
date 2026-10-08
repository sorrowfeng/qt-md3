#include "MdBadge.h"

#include "styles/MdBadgeStyle.h"

#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QEvent>

namespace md {

MdBadge::MdBadge(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdBadge::MdBadge(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
{
    init();
}

MdBadge::~MdBadge() = default;

void MdBadge::init()
{
    // A badge is not interactive — the export publishes no state rows at all.
    // No focus, and (when anchored over content by MdBadgedBox) clicks pass
    // straight through to whatever sits underneath.
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // The accessible content is the text; a dot badge has none, so it at
    // least carries a name instead of presenting as an anonymous widget.
    setAccessibleName(m_text.isEmpty() ? QStringLiteral("badge") : m_text);

    MdStyleBase::connectThemeUpdate(this, &MdBadge::onThemeChanged);

    // Create and register the shared style as soon as the first badge exists,
    // so a badge is never momentarily painted by the platform style.
    MdBadgeStyle::shared();
}

void MdBadge::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    // The text decides the form (dot vs content), so the cached token set and
    // the geometry both have to go.
    invalidateTokens();
    setAccessibleName(m_text.isEmpty() ? QStringLiteral("badge") : m_text);
    updateGeometry();
    update();
    emit textChanged(m_text);
}

const MdBadgeTokens &MdBadge::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdBadgeTokens::resolve(hasContent(), &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

QSize MdBadge::sizeHint() const
{
    const MdBadgeTokens &resolved = tokens();
    const MdBadgeStyle::Layout layout = MdBadgeStyle::layoutFor(*this, resolved);
    return QSize(int(std::ceil(layout.preferredSize.width())),
                 int(std::ceil(layout.preferredSize.height())));
}

QSize MdBadge::minimumSizeHint() const
{
    return sizeHint();
}

void MdBadge::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    switch (event->type()) {
    case QEvent::EnabledChange:
        // A badge paints identically enabled or disabled — there is no
        // disabled row. Repaint in case something embedded cares.
        update();
        break;
    default:
        break;
    }
}

void MdBadge::onThemeChanged()
{
    invalidateTokens();
    update();
}

void MdBadge::invalidateTokens()
{
    m_tokensDirty = true;
}

} // namespace md
