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

void MdExpressiveMenuItem::setOrientation(MdExpressiveMenuOrientation orientation)
{
    if (m_orientation == orientation) {
        return;
    }
    m_orientation = orientation;
    updateGeometry();
    update();
}

void MdExpressiveMenuItem::setIconOnly(bool iconOnly)
{
    if (m_iconOnly == iconOnly) {
        return;
    }
    m_iconOnly = iconOnly;
    updateGeometry();
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
    const MdExpressiveMenuLayoutMetrics m = t.layoutMetricsFor(m_orientation, m_iconOnly);
    const QFontMetricsF labelMeasure(MdTypeScale::font(
        t.labelTextType, TypeEmphasis::Baseline, MdTheme::instance().scriptCategory()));

    // The icon-only variant is a square icon button: the published
    // `horizontal.icon-only.menu-item.*-space` around the icon, no label gap.
    if (m_iconOnly && m_orientation == MdExpressiveMenuOrientation::Horizontal) {
        const qreal iconExtent = !m_leadingIconName.isEmpty()
            ? t.leadingIconSize
            : (!m_trailingIconName.isEmpty() ? t.trailingIconSize : t.leadingIconSize);
        return QSize(int(std::ceil(m.itemLeadingSpace + iconExtent + m.itemTrailingSpace)),
                     int(std::ceil(m.itemTopSpace + iconExtent + m.itemBottomSpace)));
    }

    const qreal textWidth = text().isEmpty() ? 0.0 : std::ceil(labelMeasure.horizontalAdvance(text()));
    qreal content = m.itemLeadingSpace + textWidth + m.itemTrailingSpace;
    if (!m_leadingIconName.isEmpty()) {
        content += t.leadingIconSize + m.itemIconTextSpacing;
    }
    if (!m_trailingIconName.isEmpty()) {
        content += t.trailingIconSize + m.itemIconTextSpacing;
    }
    if (!m_trailingSupportingText.isEmpty()) {
        content += std::ceil(labelMeasure.horizontalAdvance(m_trailingSupportingText))
            + m.itemIconTextSpacing;
    }

    if (m_orientation == MdExpressiveMenuOrientation::Horizontal) {
        // No `horizontal.menu-item.height` row: the cross extent is the
        // content's own height plus the published top/bottom spaces.
        qreal contentHeight = 0.0;
        if (!text().isEmpty()) {
            contentHeight = qMax<qreal>(
                contentHeight,
                MdTypeScale::lineHeight(t.labelTextType, TypeEmphasis::Baseline,
                                        MdTheme::instance().scriptCategory()));
        }
        if (!m_supportingText.isEmpty()) {
            contentHeight = qMax<qreal>(
                contentHeight,
                MdTypeScale::lineHeight(t.supportingTextType, TypeEmphasis::Baseline,
                                        MdTheme::instance().scriptCategory()));
        }
        if (!m_leadingIconName.isEmpty()) {
            contentHeight = qMax(contentHeight, t.leadingIconSize);
        }
        if (!m_trailingIconName.isEmpty()) {
            contentHeight = qMax(contentHeight, t.trailingIconSize);
        }
        if (contentHeight <= 0.0) {
            // An empty row still needs an extent; the icon box is the
            // smallest published one.
            contentHeight = t.leadingIconSize;
        }
        return QSize(int(std::ceil(content)),
                     int(std::ceil(m.itemTopSpace + contentHeight + m.itemBottomSpace)));
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
        // Touch is delayed by TOUCH_DELAY_MS so a scroll can cancel it; mouse
        // presses stay immediate (material-web's ripple handler).
        if (event->source() == Qt::MouseEventSynthesizedByQt) {
            m_ripple->pressTouch(QPointF(event->pos()));
        } else {
            m_ripple->press(QPointF(event->pos()));
        }
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

void MdExpressiveMenu::setOrientation(MdExpressiveMenuOrientation orientation)
{
    if (orientation == m_orientation) {
        return;
    }
    m_orientation = orientation;
    pushTokensToItems();
    relayout();
    update();
    emit orientationChanged(m_orientation);
}

void MdExpressiveMenu::setIconOnly(bool iconOnly)
{
    if (iconOnly == m_iconOnly) {
        return;
    }
    m_iconOnly = iconOnly;
    pushTokensToItems();
    relayout();
    update();
    emit iconOnlyChanged(m_iconOnly);
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
    item->setOrientation(m_orientation);
    item->setIconOnly(m_iconOnly);
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
        item->setOrientation(m_orientation);
        item->setIconOnly(m_iconOnly);
        item->applyVariant(m_variant);
    }
}

void MdExpressiveMenu::relayout()
{
    const MdExpressiveMenuTokens &t = menuTokens();
    const MdExpressiveMenuLayoutMetrics m = t.layoutMetricsFor(m_orientation, m_iconOnly);

    if (m_orientation == MdExpressiveMenuOrientation::Horizontal) {
        // Items left-to-right. `horizontal.container.top/bottom-space` is the
        // cross inset; the main axis has no published container row and sits
        // flush, which nests the first/last item's corner-full cap exactly
        // inside the container's.
        qreal rowCross = 0.0;
        for (QWidget *row : m_rows) {
            if (auto *item = qobject_cast<MdExpressiveMenuItem *>(row)) {
                rowCross = qMax(rowCross, qreal(item->sizeHint().height()));
            }
        }
        qreal x = m.containerLeadingSpace;
        bool firstRow = true;
        for (QWidget *row : m_rows) {
            if (!firstRow) {
                x += m.itemGap;
            }
            firstRow = false;
            if (auto *item = qobject_cast<MdExpressiveMenuItem *>(row)) {
                const QSize hint = item->sizeHint();
                item->setGeometry(int(std::ceil(x)), int(std::ceil(m.containerTopSpace)),
                                  hint.width(), hint.height());
                x += hint.width();
            } else {
                // A divider in the bar is a 1 px vertical rule spanning the
                // row's cross extent.
                row->setGeometry(int(std::ceil(x)), int(std::ceil(m.containerTopSpace)), 1,
                                 int(std::ceil(rowCross)));
                x += 1.0;
            }
        }
        x += m.containerTrailingSpace;
        resize(int(std::ceil(x)),
               int(std::ceil(m.containerTopSpace + rowCross + m.containerBottomSpace)));
        return;
    }

    qreal contentWidth = 0.0;
    for (MdExpressiveMenuItem *item : m_items) {
        contentWidth = qMax(contentWidth, qreal(item->sizeHint().width()));
    }
    if (contentWidth <= 0.0) {
        contentWidth = 160.0;
    }

    // `group.padding` around the whole stack, `gap` between rows.
    qreal y = m.containerTopSpace;
    bool firstRow = true;
    for (QWidget *row : m_rows) {
        if (!firstRow) {
            y += m.itemGap;
        }
        firstRow = false;
        if (auto *item = qobject_cast<MdExpressiveMenuItem *>(row)) {
            item->setGeometry(int(m.containerLeadingSpace), int(y),
                              int(std::ceil(contentWidth)), int(t.itemHeight));
            y += t.itemHeight;
        } else {
            row->setGeometry(int(m.containerLeadingSpace + m.itemLeadingSpace), int(y),
                             int(std::ceil(contentWidth - 2.0 * m.itemLeadingSpace)), 1);
            y += 1.0;
        }
    }
    y += m.containerBottomSpace;
    resize(int(std::ceil(contentWidth + m.containerLeadingSpace + m.containerTrailingSpace)),
           int(std::ceil(y)));
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

    // The arrow pair matches the bar's axis: Up/Down in a column, Left/Right
    // in a row. Both still wrap.
    int step = 0;
    switch (event->key()) {
    case Qt::Key_Down:
        step = m_orientation == MdExpressiveMenuOrientation::Vertical ? 1 : 0;
        break;
    case Qt::Key_Up:
        step = m_orientation == MdExpressiveMenuOrientation::Vertical ? -1 : 0;
        break;
    case Qt::Key_Right:
        step = m_orientation == MdExpressiveMenuOrientation::Horizontal ? 1 : 0;
        break;
    case Qt::Key_Left:
        step = m_orientation == MdExpressiveMenuOrientation::Horizontal ? -1 : 0;
        break;
    case Qt::Key_Escape:
        closeAndAccept();
        event->accept();
        return;
    default:
        break;
    }
    if (step != 0 && !rows.isEmpty()) {
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
    QWidget::keyPressEvent(event);
}

void MdExpressiveMenu::mousePressEvent(QMouseEvent *event)
{
    const MdExpressiveMenuTokens &t = menuTokens();
    const MdExpressiveMenuLayoutMetrics m = t.layoutMetricsFor(m_orientation, m_iconOnly);
    if (!rect().adjusted(int(m.containerLeadingSpace), int(m.containerTopSpace),
                         -int(m.containerTrailingSpace), -int(m.containerBottomSpace))
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
