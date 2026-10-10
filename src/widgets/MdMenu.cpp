#include "MdMenu.h"

#include "styles/MdMenuStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdMotion.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <cmath>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

// ---------------------------------------------------------------------------
// MdMenuItem
// ---------------------------------------------------------------------------

MdMenuItem::MdMenuItem(QWidget *parent)
    : QAbstractButton(parent)
{
    init();
}

MdMenuItem::MdMenuItem(const QString &text, QWidget *parent)
    : QAbstractButton(parent)
{
    setText(text);
    init();
}

MdMenuItem::~MdMenuItem() = default;

void MdMenuItem::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_Hover, true);

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    MdStyleBase::connectThemeUpdate(this, &MdMenuItem::onThemeChanged);
}

void MdMenuItem::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    update();
}

void MdMenuItem::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    update();
}

void MdMenuItem::setSelected(bool selected)
{
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;
    update();
    emit selectedChanged(m_selected);
}

void MdMenuItem::setLeadingIconName(const QString &iconName)
{
    if (m_leadingIconName == iconName) {
        return;
    }
    m_leadingIconName = iconName;
    update();
    emit leadingIconNameChanged(m_leadingIconName);
}

void MdMenuItem::setTrailingIconName(const QString &iconName)
{
    if (m_trailingIconName == iconName) {
        return;
    }
    m_trailingIconName = iconName;
    update();
    emit trailingIconNameChanged(m_trailingIconName);
}

QSize MdMenuItem::sizeHint() const
{
    const MdMenuTokens &t = menuTokens();
    // Compose: `sizeIn(minWidth 112, maxWidth 280, minHeight 48)` around a
    // content row padded 12 px horizontally, icons spaced 8 px from the text.
    const QFontMetricsF measure(MdTypeScale::font(t.labelTextType, TypeEmphasis::Baseline,
                                                  MdTheme::instance().scriptCategory()));
    const qreal textWidth = text().isEmpty() ? 0.0 : std::ceil(measure.horizontalAdvance(text()));
    qreal content = 2.0 * t.itemHorizontalPadding + textWidth;
    if (!m_leadingIconName.isEmpty()) {
        content += t.iconSize + t.itemIconTextSpacing;
    }
    if (!m_trailingIconName.isEmpty()) {
        content += t.iconSize + t.itemIconTextSpacing;
    }
    const qreal w = qBound(t.itemMinWidth, qMax(t.itemMinWidth, content), t.itemMaxWidth);
    return QSize(int(std::ceil(w)), int(std::ceil(t.itemHeight)));
}

QSize MdMenuItem::minimumSizeHint() const
{
    return QSize(int(menuTokens().itemMinWidth), int(menuTokens().itemHeight));
}

const MdMenuTokens &MdMenuItem::menuTokens() const
{
    if (m_hasPushedTokens) {
        return m_tokens;
    }
    if (m_tokensDirty) {
        m_tokens = MdMenuTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdMenuItem::setMenuTokens(const MdMenuTokens &tokens)
{
    m_tokens = tokens;
    m_hasPushedTokens = true;
    updateGeometry();
    update();
}

bool MdMenuItem::hitButton(const QPoint &pos) const
{
    // The item row is the whole widget.
    return rect().contains(pos);
}

void MdMenuItem::mousePressEvent(QMouseEvent *event)
{
    if (m_ripple != nullptr) {
        m_ripple->setBounds(QSizeF(qreal(width()), qreal(height())));
        m_ripple->press(QPointF(event->pos()));
    }
    QAbstractButton::mousePressEvent(event);
}

void MdMenuItem::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
}

void MdMenuItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // The paint filter (MdMenuStyle) paints the item; an empty body keeps the
    // native QAbstractButton painting out.
}

void MdMenuItem::enterEvent(md::MdEnterEvent *event)
{
    QAbstractButton::enterEvent(event);
    m_hovered = true;
    update();
}

void MdMenuItem::leaveEvent(QEvent *event)
{
    QAbstractButton::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdMenuItem::focusInEvent(QFocusEvent *event)
{
    QAbstractButton::focusInEvent(event);
    // `:focus-visible`: only the keyboard navigation reasons paint the ring —
    // Tab / Backtab / shortcuts. Everything else (mouse, activation, the
    // popup's own initial focus, Qt's OtherFocusReason catch-all) stays
    // ring-free.
    const Qt::FocusReason reason = event->reason();
    m_focusIsKeyboard = reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason
                        || reason == Qt::ShortcutFocusReason;
    if (m_focusRing != nullptr && m_focusIsKeyboard) {
        m_focusRing->start();
    }
    update();
}

void MdMenuItem::focusOutEvent(QFocusEvent *event)
{
    QAbstractButton::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing != nullptr) {
        m_focusRing->stop();
    }
    update();
}

void MdMenuItem::changeEvent(QEvent *event)
{
    QAbstractButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        update();
    }
}

void MdMenuItem::onThemeChanged()
{
    m_tokensDirty = true;
    updateGeometry();
    update();
}

// ---------------------------------------------------------------------------
// MdMenu
// ---------------------------------------------------------------------------

MdMenu::MdMenu(QWidget *parent)
    : QWidget(parent, Qt::Popup)
{
    init();
}

MdMenu::~MdMenu() = default;

void MdMenu::init()
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFocusPolicy(Qt::StrongFocus);

    m_openTimer = new QTimer(this);
    m_openTimer->setInterval(8);
    m_openTimer->setTimerType(Qt::PreciseTimer);
    connect(m_openTimer, &QTimer::timeout, this, &MdMenu::onOpenTick);

    MdStyleBase::connectThemeUpdate(this, &MdMenu::onThemeChanged);
    MdMenuStyle::shared();
}

MdMenuItem *MdMenu::addItem(const QString &text)
{
    auto *item = new MdMenuItem(this);
    item->setText(text);
    addItem(item);
    return item;
}

void MdMenu::addItem(MdMenuItem *item)
{
    item->setParent(this);
    item->show();
    m_items.append(item);
    m_rows.append(item);
    relayout();
}

void MdMenu::addDivider()
{
    auto *divider = new QWidget(this);
    divider->setFixedHeight(int(menuTokens().dividerHeight));
    divider->show();
    m_dividers.append(divider);
    m_rows.append(divider);
    relayout();
}

void MdMenu::relayout()
{
    const MdMenuTokens &t = menuTokens();
    qreal contentWidth = t.itemMinWidth;
    for (MdMenuItem *item : m_items) {
        contentWidth = qMax(contentWidth, qreal(item->sizeHint().width()));
    }
    // Rows in insertion order — items and dividers interleave as added — with
    // the 8 px vertical padding above the first row and below the last.
    qreal y = t.containerVerticalPadding;
    for (QWidget *row : m_rows) {
        if (auto *item = qobject_cast<MdMenuItem *>(row)) {
            item->setGeometry(0, int(y), int(std::ceil(contentWidth)), int(t.itemHeight));
            y += t.itemHeight;
        } else {
            row->setGeometry(int(t.dividerHorizontalPadding), int(y),
                             int(std::ceil(contentWidth - 2.0 * t.dividerHorizontalPadding)),
                             int(t.dividerHeight));
            y += t.dividerHeight + 2.0 * t.dividerVerticalPadding;
        }
    }
    y += t.containerVerticalPadding;
    resize(int(std::ceil(contentWidth)), int(std::ceil(y)));
}

void MdMenu::setChildrenPaintedByMenu(bool painted)
{
    // While the open/close animation runs, the surface paints the children
    // itself — through Compose's scale+alpha transform. Real child widgets
    // composite independently of the parent's painter transform, so they hide
    // for the flight and come back when the springs settle.
    for (MdMenuItem *item : m_items) {
        item->setVisible(!painted);
    }
    for (QWidget *divider : m_dividers) {
        divider->setVisible(!painted);
    }
}

void MdMenu::popup(const QRect &anchorRect)
{
    relayout();
    // Compose's `MenuAnchorPosition.Below` with the 8 px horizontal margin:
    // below the anchor's bottom edge, left-aligned with the anchor's left
    // edge (clamped into the screen by the window system).
    QPoint pos(anchorRect.left(), anchorRect.bottom() + 1);
    move(pos);
    // Restart the open animation from closed.
    m_opening = true;
    m_openFrom = 0.0;
    m_openProgress = 0.0;
    m_openAlpha = 0.0;
    m_openClock.restart();
    m_openTimer->start();
    setChildrenPaintedByMenu(true);
    show();
    raise();
    setFocus();

    // Keyboard focus lands on the first enabled item (arrow keys walk on).
    for (MdMenuItem *item : m_items) {
        if (item->isEnabled()) {
            item->setFocus(Qt::PopupFocusReason);
            break;
        }
    }
}

void MdMenu::exec(const QRect &anchorRect)
{
    popup(anchorRect);
    QEventLoop loop;
    connect(this, &MdMenu::closed, &loop, &QEventLoop::quit);
    loop.exec();
}

void MdMenu::closeAndAccept()
{
    close();
    emit closed();
}

void MdMenu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    MdMenuStyle::paintMenu(painter, *this, menuTokens());
}

void MdMenu::keyPressEvent(QKeyEvent *event)
{
    const QList<MdMenuItem *> rows = m_items;
    const int focusedIndex = [&] {
        for (int i = 0; i < rows.size(); ++i) {
            if (rows.at(i)->hasFocus()) {
                return i;
            }
        }
        return -1;
    }();

    switch (event->key()) {
    case Qt::Key_Down:
    case Qt::Key_Up: {
        if (rows.isEmpty()) {
            break;
        }
        const int step = event->key() == Qt::Key_Down ? 1 : -1;
        int next = focusedIndex;
        for (int n = 0; n < rows.size(); ++n) {
            next = (next + step + rows.size()) % rows.size();
            if (rows.at(next)->isEnabled()) {
                break;
            }
        }
        rows.at(next)->setFocus(Qt::TabFocusReason);
        event->accept();
        return;
    }
    case Qt::Key_Escape:
        closeAndAccept();
        event->accept();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdMenu::mousePressEvent(QMouseEvent *event)
{
    // A click on the translucent padding closes, like the popup's outside
    // click. Clicks on the items never reach here.
    if (!rect().adjusted(0, int(menuTokens().containerVerticalPadding), 0,
                         -int(menuTokens().containerVerticalPadding))
             .contains(event->pos())) {
        closeAndAccept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MdMenu::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MdMenu::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void MdMenu::onOpenTick()
{
    // Compose's pair: scale on the fast spatial spring, alpha on fast
    // effects. Closing reverses both from wherever they stand.
    const MdSpring scaleSpring = MdMotion::spring(MotionSpring::SpatialFast);
    const MdSpring alphaSpring = MdMotion::spring(MotionSpring::EffectsFast);
    const qreal seconds = qreal(m_openClock.elapsed()) / 1000.0;
    const qreal scaleTravel = scaleSpring.valueAt(seconds);
    const qreal alphaTravel = alphaSpring.valueAt(seconds);
    const qreal scaleTarget = m_opening ? 1.0 : 0.0;
    const qreal alphaTarget = m_opening ? 1.0 : 0.0;
    const qreal scaleProgress = m_openFrom + (scaleTarget - m_openFrom) * scaleTravel;
    const qreal alphaProgress = m_openFrom + (alphaTarget - m_openFrom) * alphaTravel;

    const bool scaleDone = qFuzzyCompare(scaleProgress + 1.0, scaleTarget + 1.0)
                           || m_openClock.elapsed() >= scaleSpring.settlingDurationMs();
    const bool alphaDone = qFuzzyCompare(alphaProgress + 1.0, alphaTarget + 1.0)
                           || m_openClock.elapsed() >= alphaSpring.settlingDurationMs();

    // One progress value drives the paint's scale; alpha rides alongside. The
    // scale is the slower spring, so it wins.
    m_openProgress = qMin(scaleProgress, alphaProgress);
    // The paint separates the two curves; store the alpha's own progress.
    m_openAlpha = alphaProgress;

    if (scaleDone && alphaDone) {
        m_openProgress = scaleTarget;
        m_openAlpha = alphaTarget;
        m_openTimer->stop();
        if (m_opening) {
            setChildrenPaintedByMenu(false);
            update();
        } else {
            hide();
            emit closed();
        }
    }
    update();
}

const MdMenuTokens &MdMenu::menuTokens() const
{
    if (m_hasPushedTokens) {
        return m_tokens;
    }
    if (m_tokensDirty) {
        m_tokens = MdMenuTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdMenu::setMenuTokens(const MdMenuTokens &tokens)
{
    m_tokens = tokens;
    m_hasPushedTokens = true;
    relayout();
    update();
}

void MdMenu::onThemeChanged()
{
    m_tokensDirty = true;
    relayout();
    update();
}

} // namespace md
