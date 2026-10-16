#include "MdScaffold.h"

#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

namespace md {

namespace {

/// `md.comp.app-bar.small.container.height` — 64 px, the default when the
/// caller has not sized the app bar yet.
constexpr int kDefaultAppBarHeight = 64;
/// `md.comp.navigation-bar.container.height` — 80 px.
constexpr int kDefaultBottomNavHeight = 80;
/// `md.comp.navigation-rail.container.width` — 80 px.
constexpr int kDefaultRailWidth = 80;
/// FAB inset from the body edge. Matches the 16 px layout grid the FAB
/// family documents (`fab-margin`).
constexpr int kDefaultFabMargin = 16;

} // namespace

MdScaffold::MdScaffold(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdScaffold::~MdScaffold() = default;

void MdScaffold::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_fabMargin = kDefaultFabMargin;
    MdStyleBase::connectThemeUpdate(this, &MdScaffold::onThemeChanged);
}

void MdScaffold::adopt(QWidget *widget)
{
    if (!widget) {
        return;
    }
    widget->setParent(this);
    widget->show();
}

void MdScaffold::release(QWidget *widget)
{
    Q_UNUSED(widget);
    // Ownership stays with the caller once the slot is replaced or cleared;
    // the old child is simply re-parented away from the scaffold.
}

void MdScaffold::setAppBar(QWidget *widget)
{
    if (m_appBar == widget) {
        return;
    }
    QWidget *previous = m_appBar;
    m_appBar = widget;
    adopt(m_appBar);
    if (previous && previous->parent() == this && previous != m_appBar) {
        previous->hide();
    }
    layoutSlots();
    emit appBarChanged(m_appBar);
}

void MdScaffold::setNavigation(QWidget *widget, MdNavPlacement placement)
{
    const bool widgetChanged = m_navigation != widget;
    const bool placeChanged = m_navPlacement != placement;
    if (!widgetChanged && !placeChanged) {
        return;
    }
    QWidget *previous = m_navigation;
    m_navigation = widget;
    m_navPlacement = placement;
    adopt(m_navigation);
    if (previous && previous->parent() == this && previous != m_navigation) {
        previous->hide();
    }
    layoutSlots();
    if (widgetChanged) {
        emit navigationChanged(m_navigation);
    }
    if (placeChanged) {
        emit navPlacementChanged(m_navPlacement);
    }
}

void MdScaffold::setNavPlacement(MdNavPlacement placement)
{
    if (m_navPlacement == placement) {
        return;
    }
    m_navPlacement = placement;
    layoutSlots();
    emit navPlacementChanged(m_navPlacement);
}

void MdScaffold::setEndPane(QWidget *widget)
{
    if (m_endPane == widget) {
        return;
    }
    QWidget *previous = m_endPane;
    m_endPane = widget;
    adopt(m_endPane);
    if (previous && previous->parent() == this && previous != m_endPane) {
        previous->hide();
    }
    layoutSlots();
    emit endPaneChanged(m_endPane);
}

void MdScaffold::setBody(QWidget *widget)
{
    if (m_body == widget) {
        return;
    }
    QWidget *previous = m_body;
    m_body = widget;
    adopt(m_body);
    if (previous && previous->parent() == this && previous != m_body) {
        previous->hide();
    }
    layoutSlots();
    emit bodyChanged(m_body);
}

void MdScaffold::setFab(QWidget *widget)
{
    if (m_fab == widget) {
        return;
    }
    QWidget *previous = m_fab;
    m_fab = widget;
    adopt(m_fab);
    if (previous && previous->parent() == this && previous != m_fab) {
        previous->hide();
    }
    layoutSlots();
    emit fabChanged(m_fab);
}

void MdScaffold::setFabPlacement(MdFabPlacement placement)
{
    if (m_fabPlacement == placement) {
        return;
    }
    m_fabPlacement = placement;
    layoutSlots();
    emit fabPlacementChanged(m_fabPlacement);
}

void MdScaffold::setFabMargin(int margin)
{
    const int clamped = qMax(0, margin);
    if (m_fabMargin == clamped) {
        return;
    }
    m_fabMargin = clamped;
    layoutSlots();
    emit fabMarginChanged(m_fabMargin);
}

QRect MdScaffold::bodyRect() const
{
    const bool rtl = MdStyleBase::isRtl(this);
    QRect body = rect();

    if (m_appBar && !m_appBar->isHidden()) {
        const int h = m_appBar->height() > 0 ? m_appBar->height() : kDefaultAppBarHeight;
        body.setTop(body.top() + h);
    }
    if (m_navigation && !m_navigation->isHidden()) {
        switch (m_navPlacement) {
        case MdNavPlacement::Start: {
            const int w = m_navigation->width() > 0 ? m_navigation->width() : kDefaultRailWidth;
            if (rtl) {
                body.setRight(body.right() - w);
            } else {
                body.setLeft(body.left() + w);
            }
            break;
        }
        case MdNavPlacement::End: {
            const int w = m_navigation->width() > 0 ? m_navigation->width() : kDefaultRailWidth;
            if (rtl) {
                body.setLeft(body.left() + w);
            } else {
                body.setRight(body.right() - w);
            }
            break;
        }
        case MdNavPlacement::Bottom: {
            const int h = m_navigation->height() > 0 ? m_navigation->height()
                                                    : kDefaultBottomNavHeight;
            body.setBottom(body.bottom() - h);
            break;
        }
        case MdNavPlacement::None:
        case MdNavPlacement::Count:
            break;
        }
    }
    if (m_endPane && !m_endPane->isHidden()) {
        const int w = m_endPane->width() > 0 ? m_endPane->width() : kDefaultRailWidth;
        if (rtl) {
            body.setLeft(body.left() + w);
        } else {
            body.setRight(body.right() - w);
        }
    }
    return body;
}

QSize MdScaffold::sizeHint() const
{
    int w = 800;
    int h = 600;
    if (m_body) {
        const QSize bodyHint = m_body->sizeHint();
        if (bodyHint.isValid()) {
            w = bodyHint.width();
            h = bodyHint.height();
        }
    }
    if (m_appBar) {
        h += m_appBar->sizeHint().height();
    }
    return QSize(w, h);
}

QSize MdScaffold::minimumSizeHint() const
{
    return QSize(200, 200);
}

void MdScaffold::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutSlots();
}

void MdScaffold::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    layoutSlots();
}

void MdScaffold::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    // `md.sys.color.surface` — the page skeleton's only token.
    painter.fillRect(rect(), MdTheme::instance().color(ColorRole::Surface));
}

void MdScaffold::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange) {
        layoutSlots();
    }
}

void MdScaffold::onThemeChanged()
{
    update();
}

void MdScaffold::layoutSlots()
{
    const bool rtl = MdStyleBase::isRtl(this);
    const QRect box = rect();
    const QRect body = bodyRect();

    if (m_appBar) {
        const int h = m_appBar->height() > 0 ? m_appBar->sizeHint().height() : kDefaultAppBarHeight;
        m_appBar->setGeometry(box.left(), box.top(), box.width(), h);
        m_appBar->raise();
    }

    if (m_navigation) {
        switch (m_navPlacement) {
        case MdNavPlacement::Start: {
            const int w = m_navigation->sizeHint().width() > 0 ? m_navigation->sizeHint().width()
                                                               : kDefaultRailWidth;
            const int x = rtl ? box.right() - w + 1 : box.left();
            const int y = body.top();
            const int h = box.bottom() - y + 1;
            m_navigation->setGeometry(x, y, w, h);
            break;
        }
        case MdNavPlacement::End: {
            const int w = m_navigation->sizeHint().width() > 0 ? m_navigation->sizeHint().width()
                                                               : kDefaultRailWidth;
            const int x = rtl ? box.left() : box.right() - w + 1;
            const int y = body.top();
            const int h = box.bottom() - y + 1;
            m_navigation->setGeometry(x, y, w, h);
            break;
        }
        case MdNavPlacement::Bottom: {
            const int h = m_navigation->sizeHint().height() > 0
                ? m_navigation->sizeHint().height()
                : kDefaultBottomNavHeight;
            m_navigation->setGeometry(box.left(), box.bottom() - h + 1, box.width(), h);
            break;
        }
        case MdNavPlacement::None:
        case MdNavPlacement::Count:
            m_navigation->hide();
            break;
        }
        if (m_navPlacement != MdNavPlacement::None) {
            m_navigation->show();
        }
        m_navigation->raise();
    }

    if (m_endPane) {
        const int w = m_endPane->sizeHint().width() > 0 ? m_endPane->sizeHint().width()
                                                        : kDefaultRailWidth;
        const int x = rtl ? box.left() : box.right() - w + 1;
        const int y = body.top();
        const int h = body.height();
        m_endPane->setGeometry(x, y, w, h);
        m_endPane->show();
        m_endPane->raise();
    }

    if (m_body) {
        m_body->setGeometry(body);
        m_body->show();
        m_body->lower();
    }

    if (m_fab) {
        const QSize fabSize = m_fab->sizeHint().isValid() && m_fab->sizeHint().width() > 0
            ? m_fab->sizeHint()
            : QSize(56, 56);
        int x = body.left() + m_fabMargin;
        int y = body.bottom() - m_fabMargin - fabSize.height() + 1;
        switch (m_fabPlacement) {
        case MdFabPlacement::EndFloat:
            x = body.right() - m_fabMargin - fabSize.width() + 1;
            break;
        case MdFabPlacement::StartFloat:
            x = body.left() + m_fabMargin;
            break;
        case MdFabPlacement::BottomCenter:
            x = body.left() + (body.width() - fabSize.width()) / 2;
            break;
        case MdFabPlacement::EndDocked:
            x = body.right() - m_fabMargin - fabSize.width() / 2 + 1;
            y = body.bottom() - fabSize.height() / 2 + 1;
            break;
        case MdFabPlacement::Count:
            break;
        }
        if (rtl && (m_fabPlacement == MdFabPlacement::EndFloat
                    || m_fabPlacement == MdFabPlacement::StartFloat)) {
            // Mirror the leading/trailing pair; centre and docked stay put.
            if (m_fabPlacement == MdFabPlacement::EndFloat) {
                x = body.left() + m_fabMargin;
            } else {
                x = body.right() - m_fabMargin - fabSize.width() + 1;
            }
        }
        m_fab->setGeometry(x, y, fabSize.width(), fabSize.height());
        m_fab->show();
        m_fab->raise();
    }

    update();
}

} // namespace md
