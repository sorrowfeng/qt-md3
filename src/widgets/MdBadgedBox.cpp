#include "MdBadgedBox.h"

#include "MdBadge.h"

#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace md {

namespace {

// Compose BadgedBox anchoring facts (Badge.kt), in logical px:
//
//   BadgeOffset                        6 dp  — dot form, end edge and top edge
//   BadgeWithContentHorizontalOffset  12 dp  — pill start edge inside the end edge
//   BadgeWithContentVerticalOffset    14 dp  — pill bottom edge below the top edge
constexpr qreal kDotOffset = 6.0;
constexpr qreal kContentHorizontalOffset = 12.0;
constexpr qreal kContentVerticalOffset = 14.0;

/// How far the badge sticks out past the anchor's top-end corner, per form.
QSizeF badgeOverhang(const MdBadge &badge)
{
    const QSizeF size(badge.sizeHint());
    if (!badge.hasContent()) {
        // The dot's end/top edges sit *on* the anchor's: nothing overhangs.
        return QSizeF(qMax<qreal>(size.width() - kDotOffset, 0.0),
                      qMax<qreal>(size.height() - kDotOffset, 0.0));
    }
    // The pill's start edge is 12 px inside the end edge and its bottom edge
    // 14 px below the top edge — a 16 px pill overhangs 4 px / 2 px.
    return QSizeF(qMax<qreal>(size.width() - kContentHorizontalOffset, 0.0),
                  qMax<qreal>(size.height() - kContentVerticalOffset, 0.0));
}

} // namespace

MdBadgedBox::MdBadgedBox(QWidget *parent)
    : QWidget(parent)
{
    m_badge = new MdBadge(this);
    setFocusPolicy(Qt::NoFocus);
}

MdBadgedBox::~MdBadgedBox() = default;

void MdBadgedBox::setContentWidget(QWidget *content)
{
    if (m_content == content) {
        return;
    }
    if (m_content) {
        m_content->setParent(nullptr);
    }
    m_content = content;
    if (m_content) {
        m_content->setParent(this);
        m_content->show();
    }
    layoutChildren();
    updateGeometry();
}

QSize MdBadgedBox::sizeHint() const
{
    const QSizeF overhang = badgeOverhang(*m_badge);
    const QSize contentSize = m_content ? m_content->sizeHint() : QSize();
    return QSize(int(std::ceil(contentSize.width() + overhang.width())),
                 int(std::ceil(contentSize.height() + overhang.height())));
}

QSize MdBadgedBox::minimumSizeHint() const
{
    return sizeHint();
}

void MdBadgedBox::layoutChildren()
{
    if (m_badge == nullptr) {
        return;
    }

    const QSizeF overhang = badgeOverhang(*m_badge);
    const QSizeF badgeSize(m_badge->sizeHint());

    // The content fills the box minus the reserved overhang, mirroring
    // Compose's "the anchor fills the BadgedBox".
    if (m_content) {
        const QRectF contentRect(0.0, overhang.height(),
                                 qMax<qreal>(width() - overhang.width(), 0.0),
                                 qMax<qreal>(height() - overhang.height(), 0.0));
        const QRect target(contentRect.toRect());
        if (m_content->geometry() != target) {
            m_content->setGeometry(target);
        }
    }

    // The badge: end edge `offset` px inside the content's end edge, bottom
    // edge `overlap` px below the content's top edge. With the overhang
    // reserved, the badge's own top-left lands at the box's top-left.
    const qreal horizontalOffset =
        m_badge->hasContent() ? kContentHorizontalOffset : kDotOffset;
    const qreal verticalOffset =
        m_badge->hasContent() ? kContentVerticalOffset : kDotOffset;

    const qreal left = (m_content ? m_content->geometry().right() + 1 : qreal(width()))
                       - horizontalOffset;
    const qreal top = overhang.height() - (badgeSize.height() - verticalOffset);
    const QRect target(int(std::round(left)), int(std::round(top)), int(badgeSize.width()),
                       int(badgeSize.height()));
    if (m_badge->geometry() != target) {
        m_badge->setGeometry(target);
    }
    m_badge->raise();
}

void MdBadgedBox::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

void MdBadgedBox::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    layoutChildren();
}

} // namespace md
