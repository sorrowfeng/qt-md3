#include "MdExpressiveMenu.h"

#include "styles/MdExpressiveMenuStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <cmath>
#include <QtCore/QEventLoop>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

namespace md {

// ---------------------------------------------------------------------------
// MdExpressiveMenuItem
// ---------------------------------------------------------------------------

MdExpressiveMenuItem::MdExpressiveMenuItem(QWidget *parent)
    : QAbstractButton(parent)
{
    init();
}

MdExpressiveMenuItem::MdExpressiveMenuItem(const QString &text, QWidget *parent)
    : QAbstractButton(parent)
{
    setText(text);
    init();
}

MdExpressiveMenuItem::~MdExpressiveMenuItem() = default;

void MdExpressiveMenuItem::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_Hover, true);

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    MdStyleBase::connectThemeUpdate(this, &MdExpressiveMenuItem::onThemeChanged);
}

void MdExpressiveMenuItem::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    update();
}

void MdExpressiveMenuItem::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    update();
}

void MdExpressiveMenuItem::setSelected(bool selected)
{
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;
    update();
    emit selectedChanged(m_selected);
}

void MdExpressiveMenuItem::setLeadingIconName(const QString &iconName)
{
    if (m_leadingIconName == iconName) {
        return;
    }
    m_leadingIconName = iconName;
    update();
    emit leadingIconNameChanged(m_leadingIconName);
}

void MdExpressiveMenuItem::setTrailingIconName(const QString &iconName)
{
    if (m_trailingIconName == iconName) {
        return;
    }
    m_trailingIconName = iconName;
    update();
    emit trailingIconNameChanged(m_trailingIconName);
}

void MdExpressiveMenuItem::setSupportingText(const QString &text)
{
    if (m_supportingText == text) {
        return;
    }
    m_supportingText = text;
    updateGeometry();
    update();
    emit supportingTextChanged(m_supportingText);
}

void MdExpressiveMenuItem::setTrailingSupportingText(const QString &text)
{
    if (m_trailingSupportingText == text) {
        return;
    }
    m_trailingSupportingText = text;
    updateGeometry();
    update();
    emit trailingSupportingTextChanged(m_trailingSupportingText);
}

QSize MdExpressiveMenuItem::sizeHint() const
{
    const MdExpressiveMenuTokens &t = menuTokens();
    const QFontMetricsF labelMeasure(MdTypeScale::font(
        t.labelTextType, TypeEmphasis::Baseline, MdTheme::instance().scriptCategory()));
    const qreal textWidth = text().isEmpty() ? 0.0 : std::ceil(labelMeasure.horizontalAdvance(text()));
    qreal content = t.itemLeadingSpace + textWidth + t.itemTrailingSpace;
    if (!m_leadingIconName.isEmpty()) {
        content += t.leadingIconSize + t.itemIconTextSpacing;
    }
    if (!m_trailingIconName.isEmpty()) {
        content += t.trailingIconSize + t.itemIconTextSpacing;
    }
    if (!m_trailingSupportingText.isEmpty()) {
        content += std::ceil(labelMeasure.horizontalAdvance(m_trailingSupportingText))
            + t.itemIconTextSpacing;
    }
    return QSize(int(std::ceil(content)), int(std::ceil(t.itemHeight)));
}

QSize MdExpressiveMenuItem::minimumSizeHint() const
{
    return sizeHint();
}

const MdExpressiveMenuTokens &MdExpressiveMenuItem::menuTokens() const
{
    if (m_hasPushedTokens) {
        return m_tokens;
    }
    if (m_tokensDirty) {
        m_tokens = MdExpressiveMenuTokens::resolve(MdExpressiveMenuVariant::Standard,
                                                   &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdExpressiveMenuItem::setMenuTokens(const MdExpressiveMenuTokens &tokens)
{
    m_tokens = tokens;
    m_hasPushedTokens = true;
    updateGeometry();
    update();
}

void MdExpressiveMenuItem::applyVariant(MdExpressiveMenuVariant variant)
{
    setMenuTokens(MdExpressiveMenuTokens::resolve(variant, &m_componentTokens));
}

ShapeCorner MdExpressiveMenuItem::shapeInGroup(int index, int count) const
{
    return menuTokens().itemShapeFor(index, count, isSelected(), isDown());
}

bool MdExpressiveMenuItem::hitButton(const QPoint &pos) const
{
    return rect().contains(pos);
}

void MdExpressiveMenuItem::mousePressEvent(QMouseEvent *event)
{
    if (m_ripple != nullptr) {
        m_ripple->setBounds(QSizeF(qreal(width()), qreal(height())));
        m_ripple->press(QPointF(event->pos()));
    }
    QAbstractButton::mousePressEvent(event);
}

void MdExpressiveMenuItem::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
}

void MdExpressiveMenuItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // The paint filter (MdExpressiveMenuStyle) paints the item.
}

void MdExpressiveMenuItem::enterEvent(md::MdEnterEvent *event)
{
    QAbstractButton::enterEvent(event);
    m_hovered = true;
    update();
}

void MdExpressiveMenuItem::leaveEvent(QEvent *event)
{
    QAbstractButton::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdExpressiveMenuItem::focusInEvent(QFocusEvent *event)
{
    QAbstractButton::focusInEvent(event);
    // `:focus-visible`: keyboard navigation reasons only.
    const Qt::FocusReason reason = event->reason();
    m_focusIsKeyboard = reason == Qt::TabFocusReason || reason == Qt::BacktabFocusReason
                        || reason == Qt::ShortcutFocusReason;
    if (m_focusRing != nullptr && m_focusIsKeyboard) {
        m_focusRing->start();
    }
    update();
}

void MdExpressiveMenuItem::focusOutEvent(QFocusEvent *event)
{
    QAbstractButton::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing != nullptr) {
        m_focusRing->stop();
    }
    update();
}

void MdExpressiveMenuItem::keyPressEvent(QKeyEvent *event)
{
    if (isEffectivelyDisabled()) {
        switch (event->key()) {
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            event->accept();
            return;
        default:
            break;
        }
    }
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        click();
        event->accept();
        return;
    default:
        break;
    }
    QAbstractButton::keyPressEvent(event);
}

void MdExpressiveMenuItem::changeEvent(QEvent *event)
{
    QAbstractButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        update();
    }
}

void MdExpressiveMenuItem::onThemeChanged()
{
    if (!m_hasPushedTokens) {
        m_tokensDirty = true;
    }
    updateGeometry();
    update();
}

// ---------------------------------------------------------------------------
// MdExpressiveMenu
// ---------------------------------------------------------------------------

MdExpressiveMenu::MdExpressiveMenu(QWidget *parent)
    : QWidget(parent, Qt::Popup)
{
    init();
}

MdExpressiveMenu::~MdExpressiveMenu() = default;

void MdExpressiveMenu::init()
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    setFocusPolicy(Qt::StrongFocus);

    MdStyleBase::connectThemeUpdate(this, &MdExpressiveMenu::onThemeChanged);
    MdExpressiveMenuStyle::shared();
}

void MdExpressiveMenu::setVariant(MdExpressiveMenuVariant variant)
{
    if (variant == m_variant) {
        return;
    }
    m_variant = variant;
    m_tokensDirty = true;
    pushTokensToItems();
    relayout();
    update();
    emit variantChanged(m_variant);
}

MdExpressiveMenuItem *MdExpressiveMenu::addItem(const QString &text)
{
    auto *item = new MdExpressiveMenuItem(this);
    item->setText(text);
    addItem(item);
    return item;
}

void MdExpressiveMenu::addItem(MdExpressiveMenuItem *item)
{
    item->setParent(this);
    item->applyVariant(m_variant);
    item->show();
    m_items.append(item);
    m_rows.append(item);
    relayout();
}

void MdExpressiveMenu::addDivider()
{
    auto *divider = new QWidget(this);
    divider->setFixedHeight(1);
    divider->show();
    m_dividers.append(divider);
    m_rows.append(divider);
    relayout();
}

void MdExpressiveMenu::pushTokensToItems()
{
    for (MdExpressiveMenuItem *item : m_items) {
        item->applyVariant(m_variant);
    }
}

void MdExpressiveMenu::relayout()
{
    const MdExpressiveMenuTokens &t = menuTokens();
    qreal contentWidth = 0.0;
    for (MdExpressiveMenuItem *item : m_items) {
        contentWidth = qMax(contentWidth, qreal(item->sizeHint().width()));
    }
    if (contentWidth <= 0.0) {
        contentWidth = 160.0;
    }

    // `group.padding: 2px` around the whole stack, `gap: 2px` between rows.
    qreal y = t.groupPadding;
    bool firstRow = true;
    for (QWidget *row : m_rows) {
        if (!firstRow) {
            y += t.itemGap;
        }
        firstRow = false;
        if (auto *item = qobject_cast<MdExpressiveMenuItem *>(row)) {
            item->setGeometry(int(t.groupPadding), int(y),
                              int(std::ceil(contentWidth)), int(t.itemHeight));
            y += t.itemHeight;
        } else {
            row->setGeometry(int(t.groupPadding + t.itemLeadingSpace), int(y),
                             int(std::ceil(contentWidth - 2.0 * t.itemLeadingSpace)), 1);
            y += 1.0;
        }
    }
    y += t.groupPadding;
    resize(int(std::ceil(contentWidth + 2.0 * t.groupPadding)), int(std::ceil(y)));
}

void MdExpressiveMenu::popup(const QRect &anchorRect)
{
    relayout();
    pushTokensToItems();
    // Below the anchor's bottom edge, left-aligned with its left edge —
    // `MenuAnchorPosition.Below`, the classic menu's placement.
    move(anchorRect.left(), anchorRect.bottom() + 1);
    show();
    raise();
    setFocus();

    for (MdExpressiveMenuItem *item : m_items) {
        if (item->isEnabled()) {
            item->setFocus(Qt::PopupFocusReason);
            break;
        }
    }
}

void MdExpressiveMenu::exec(const QRect &anchorRect)
{
    popup(anchorRect);
    QEventLoop loop;
    connect(this, &MdExpressiveMenu::closed, &loop, &QEventLoop::quit);
    loop.exec();
}

void MdExpressiveMenu::closeAndAccept()
{
    close();
    emit closed();
}

void MdExpressiveMenu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    MdExpressiveMenuStyle::paintMenu(painter, *this, menuTokens());
}

void MdExpressiveMenu::keyPressEvent(QKeyEvent *event)
{
    const QList<MdExpressiveMenuItem *> rows = m_items;
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

void MdExpressiveMenu::mousePressEvent(QMouseEvent *event)
{
    const MdExpressiveMenuTokens &t = menuTokens();
    if (!rect().adjusted(int(t.groupPadding), int(t.groupPadding), -int(t.groupPadding),
                         -int(t.groupPadding))
             .contains(event->pos())) {
        closeAndAccept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MdExpressiveMenu::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

const MdExpressiveMenuTokens &MdExpressiveMenu::menuTokens() const
{
    if (m_hasPushedTokens) {
        return m_tokens;
    }
    if (m_tokensDirty) {
        m_tokens = MdExpressiveMenuTokens::resolve(m_variant, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdExpressiveMenu::setMenuTokens(const MdExpressiveMenuTokens &tokens)
{
    m_tokens = tokens;
    m_hasPushedTokens = true;
    pushTokensToItems();
    relayout();
    update();
}

void MdExpressiveMenu::onThemeChanged()
{
    if (!m_hasPushedTokens) {
        m_tokensDirty = true;
    }
    pushTokensToItems();
    relayout();
    update();
}

} // namespace md
