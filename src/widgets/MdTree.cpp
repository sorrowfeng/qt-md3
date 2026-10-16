#include "MdTree.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <algorithm>
#include <functional>

namespace md {

namespace {

/// Trailing text inset, matching the list's label spacing.
constexpr qreal kTextInset = 16.0;
/// Leading chevron gutter width (24 px icon + 12 px air, the list's leading
/// element box for a disclosure indicator).
constexpr qreal kChevronBox = 48.0;
constexpr qreal kChevronSize = 24.0;
/// Inward focus-ring margin, matching MdListItem.
constexpr qreal kFocusMargin = 2.0;

} // namespace

// ---------------------------------------------------------------------------
// MdTreeItem
// ---------------------------------------------------------------------------

MdTreeItem::MdTreeItem(const QString &text)
    : m_text(text)
{
}

void MdTreeItem::setText(const QString &text)
{
    m_text = text;
}

void MdTreeItem::setSupportingText(const QString &text)
{
    m_supporting = text;
}

void MdTreeItem::setIconName(const QString &name)
{
    m_iconName = name;
}

void MdTreeItem::setExpanded(bool expanded)
{
    m_expanded = expanded;
}

void MdTreeItem::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

MdTreeItem *MdTreeItem::childAt(int index) const
{
    if (index < 0 || index >= m_children.size()) {
        return nullptr;
    }
    return m_children.at(index);
}

void MdTreeItem::appendChild(MdTreeItem *child)
{
    if (child == nullptr) {
        return;
    }
    child->m_parent = this;
    m_children.append(child);
}

void MdTreeItem::insertChild(int index, MdTreeItem *child)
{
    if (child == nullptr) {
        return;
    }
    child->m_parent = this;
    m_children.insert(qBound(0, index, m_children.size()), child);
}

void MdTreeItem::removeChild(MdTreeItem *child)
{
    if (child == nullptr) {
        return;
    }
    m_children.removeAll(child);
    if (child->m_parent == this) {
        child->m_parent = nullptr;
    }
}

void MdTreeItem::clearChildren()
{
    qDeleteAll(m_children);
    m_children.clear();
}

// ---------------------------------------------------------------------------
// MdTree
// ---------------------------------------------------------------------------

MdTree::MdTree(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdTree::~MdTree()
{
    qDeleteAll(m_roots);
}

void MdTree::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAccessibleName(QStringLiteral("tree"));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&MdTree::update));

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this,
            qOverload<>(&MdTree::update));

    MdStyleBase::connectThemeUpdate(this, &MdTree::onThemeChanged);
}

void MdTree::addTopLevelItem(MdTreeItem *item)
{
    insertTopLevelItem(m_roots.size(), item);
}

void MdTree::insertTopLevelItem(int index, MdTreeItem *item)
{
    if (item == nullptr) {
        return;
    }
    item->m_parent = nullptr;
    m_roots.insert(qBound(0, index, m_roots.size()), item);
    updateGeometry();
    update();
}

void MdTree::removeTopLevelItem(MdTreeItem *item)
{
    if (item == nullptr) {
        return;
    }
    m_roots.removeAll(item);
    if (item->m_parent == nullptr) {
        delete item;
    }
    updateGeometry();
    update();
}

void MdTree::clear()
{
    qDeleteAll(m_roots);
    m_roots.clear();
    m_selectedPath.clear();
    m_hoveredRow = -1;
    m_pressedRow = -1;
    m_focusRow = 0;
    updateGeometry();
    update();
    emit selectedPathChanged(m_selectedPath);
}

MdTreeItem *MdTree::itemAt(const MdTreePath &path) const
{
    if (path.isEmpty()) {
        return nullptr;
    }
    if (path.first() < 0 || path.first() >= m_roots.size()) {
        return nullptr;
    }
    MdTreeItem *node = m_roots.at(path.first());
    for (int level = 1; level < path.size(); ++level) {
        const int index = path.at(level);
        if (index < 0 || index >= node->childCount()) {
            return nullptr;
        }
        node = node->childAt(index);
    }
    return node;
}

MdTreePath MdTree::pathOf(const MdTreeItem *item) const
{
    if (item == nullptr) {
        return MdTreePath();
    }
    // Walk up to the root, then reverse.
    MdTreePath path;
    const MdTreeItem *node = item;
    while (node != nullptr) {
        const MdTreeItem *parent = node->parentItem();
        if (parent == nullptr) {
            const int index = m_roots.indexOf(const_cast<MdTreeItem *>(node));
            if (index < 0) {
                return MdTreePath();
            }
            path.prepend(index);
            break;
        }
        const int index = parent->children().indexOf(const_cast<MdTreeItem *>(node));
        if (index < 0) {
            return MdTreePath();
        }
        path.prepend(index);
        node = parent;
    }
    return path;
}

void MdTree::setSelectedPath(const MdTreePath &path)
{
    if (m_selectedPath == path) {
        return;
    }
    m_selectedPath = path;
    // Keep the roving focus row on the selection when possible.
    const QList<Row> rows = visibleRows();
    for (int i = 0; i < rows.size(); ++i) {
        if (rows.at(i).path == m_selectedPath) {
            m_focusRow = i;
            break;
        }
    }
    update();
    emit selectedPathChanged(m_selectedPath);
}

void MdTree::clearSelection()
{
    setSelectedPath(MdTreePath());
}

bool MdTree::isExpanded(const MdTreePath &path) const
{
    const MdTreeItem *item = itemAt(path);
    return item != nullptr && item->isExpanded();
}

void MdTree::setExpanded(const MdTreePath &path, bool expanded)
{
    MdTreeItem *item = itemAt(path);
    if (item == nullptr || item->isExpanded() == expanded) {
        return;
    }
    item->setExpanded(expanded);
    updateGeometry();
    update();
    emit itemExpanded(path, expanded);
}

void MdTree::expandAll()
{
    const QList<Row> rows = visibleRows();
    // Walk every node, not just the currently visible ones.
    std::function<void(MdTreeItem *)> walk = [&](MdTreeItem *node) {
        if (node == nullptr) {
            return;
        }
        if (node->childCount() > 0) {
            node->setExpanded(true);
        }
        for (MdTreeItem *child : node->children()) {
            walk(child);
        }
    };
    for (MdTreeItem *root : m_roots) {
        walk(root);
    }
    Q_UNUSED(rows);
    updateGeometry();
    update();
}

void MdTree::collapseAll()
{
    std::function<void(MdTreeItem *)> walk = [&](MdTreeItem *node) {
        if (node == nullptr) {
            return;
        }
        node->setExpanded(false);
        for (MdTreeItem *child : node->children()) {
            walk(child);
        }
    };
    for (MdTreeItem *root : m_roots) {
        walk(root);
    }
    updateGeometry();
    update();
}

void MdTree::setIndent(int indent)
{
    const int clamped = qMax(0, indent);
    if (m_indent == clamped) {
        return;
    }
    m_indent = clamped;
    update();
    emit indentChanged(m_indent);
}

void MdTree::setRowHeight(int height)
{
    const int clamped = qMax(1, height);
    if (m_rowHeight == clamped) {
        return;
    }
    m_rowHeight = clamped;
    updateGeometry();
    update();
    emit rowHeightChanged(m_rowHeight);
}

void MdTree::setWrapNavigation(bool wrap)
{
    if (m_wrap == wrap) {
        return;
    }
    m_wrap = wrap;
    emit wrapNavigationChanged(m_wrap);
}

void MdTree::collectRows(const MdTreeItem *item, const MdTreePath &base, int depth,
                         QList<Row> *out) const
{
    if (item == nullptr || out == nullptr) {
        return;
    }
    Row row;
    row.item = const_cast<MdTreeItem *>(item);
    row.path = base;
    row.depth = depth;
    row.hasChildren = item->childCount() > 0;
    out->append(row);
    if (!item->isExpanded()) {
        return;
    }
    for (int i = 0; i < item->childCount(); ++i) {
        MdTreePath childPath = base;
        childPath.append(i);
        collectRows(item->childAt(i), childPath, depth + 1, out);
    }
}

QList<MdTree::Row> MdTree::visibleRows() const
{
    QList<Row> rows;
    for (int i = 0; i < m_roots.size(); ++i) {
        MdTreePath path;
        path.append(i);
        collectRows(m_roots.at(i), path, 0, &rows);
    }
    return rows;
}

QSize MdTree::sizeHint() const
{
    return QSize(240, m_rowHeight * qMax(1, visibleRowCount()));
}

QSize MdTree::minimumSizeHint() const
{
    return QSize(0, m_rowHeight);
}

QRectF MdTree::chevronRectForRow(int row) const
{
    const QList<Row> rows = visibleRows();
    if (row < 0 || row >= rows.size()) {
        return QRectF();
    }
    const Row &r = rows.at(row);
    const qreal x = r.depth * m_indent;
    return QRectF(x, row * m_rowHeight, kChevronBox, m_rowHeight);
}

int MdTree::rowAt(const QPointF &position) const
{
    const int row = int(position.y()) / m_rowHeight;
    const QList<Row> rows = visibleRows();
    if (row < 0 || row >= rows.size()) {
        return -1;
    }
    return row;
}

void MdTree::setHoveredRow(int row)
{
    if (m_hoveredRow == row) {
        return;
    }
    m_hoveredRow = row;
    update();
}

void MdTree::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdTheme &theme = MdTheme::instance();
    const QColor surface = theme.color(ColorRole::Surface);
    const QColor onSurface = theme.color(ColorRole::OnSurface);
    const QColor onSurfaceVariant = theme.color(ColorRole::OnSurfaceVariant);
    const QColor secondaryContainer = theme.color(ColorRole::SecondaryContainer);
    const QColor onSecondaryContainer = theme.color(ColorRole::OnSecondaryContainer);

    const QList<Row> rows = visibleRows();
    const QFont headlineFont = MdTypeScale::font(TypeStyle::BodyLarge);
    const QFont supportFont = MdTypeScale::font(TypeStyle::BodyMedium);
    const QFontMetrics headlineMetrics(headlineFont);

    painter.fillRect(rect(), surface);

    for (int i = 0; i < rows.size(); ++i) {
        const Row &row = rows.at(i);
        if (!row.item->isEnabled()) {
            continue;
        }
        const QRectF rowRect(0.0, i * m_rowHeight, width(), m_rowHeight);
        const bool selected = (row.path == m_selectedPath);

        // Container: selected takes the list's secondary-container side.
        if (selected) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(secondaryContainer);
            painter.drawRect(rowRect);
        }

        // State layer: hover 0.08 / focus 0.12 / press 0.12 on on-surface,
        // or on on-secondary-container when selected (the list rule).
        const bool focused = (i == m_focusRow) && hasFocus() && m_focusIsKeyboard;
        if (m_hoveredRow == i || m_pressedRow == i || focused) {
            const QColor layerColour = selected ? onSecondaryContainer : onSurface;
            QColor layer;
            if (m_pressedRow == i) {
                layer = MdStateLayer::overlay(layerColour, StateLayerKind::Pressed);
            } else if (focused) {
                layer = MdStateLayer::overlay(layerColour, StateLayerKind::Focus);
            } else {
                layer = MdStateLayer::overlay(layerColour, StateLayerKind::Hover);
            }
            painter.setPen(Qt::NoPen);
            painter.setBrush(layer);
            painter.drawRect(rowRect);
        }

        // Leading disclosure chevron for nodes with children.
        const qreal indentX = row.depth * m_indent;
        if (row.hasChildren) {
            const QRectF chevron(indentX + (kChevronBox - kChevronSize) / 2.0,
                                 rowRect.center().y() - kChevronSize / 2.0,
                                 kChevronSize, kChevronSize);
            const QString glyph = row.item->isExpanded()
                ? QStringLiteral("expand_less")
                : QStringLiteral("chevron_right");
            const QColor chevronColour = selected ? onSecondaryContainer
                                                  : onSurfaceVariant;
            if (!MdIcon::paint(&painter, chevron, glyph, chevronColour)) {
                QPen pen(chevronColour);
                pen.setWidthF(2.0);
                pen.setCapStyle(Qt::RoundCap);
                pen.setJoinStyle(Qt::RoundJoin);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);
                QPainterPath path;
                const QPointF c = chevron.center();
                if (row.item->isExpanded()) {
                    path.moveTo(c.x() - 5.0, c.y() + 2.5);
                    path.lineTo(c.x(), c.y() - 2.5);
                    path.lineTo(c.x() + 5.0, c.y() + 2.5);
                } else {
                    path.moveTo(c.x() - 2.5, c.y() - 5.0);
                    path.lineTo(c.x() + 2.5, c.y());
                    path.lineTo(c.x() - 2.5, c.y() + 5.0);
                }
                painter.drawPath(path);
            }
        }

        // Headline + optional supporting text.
        const qreal textLeft = indentX + kChevronBox + kTextInset / 2.0;
        const qreal textRight = width() - kTextInset;
        const QRectF textRect(textLeft, rowRect.top(),
                              qMax(0.0, textRight - textLeft), rowRect.height());
        const QColor textColour = selected ? onSecondaryContainer : onSurface;
        if (row.item->supportingText().isEmpty()) {
            painter.setFont(headlineFont);
            painter.setPen(textColour);
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                             row.item->text());
        } else {
            const qreal line = qMin(rowRect.height() / 2.0, 28.0);
            QRectF top(textRect.left(), rowRect.center().y() - line,
                       textRect.width(), line);
            QRectF bottom(textRect.left(), rowRect.center().y(),
                          textRect.width(), line);
            painter.setFont(headlineFont);
            painter.setPen(textColour);
            painter.drawText(top, Qt::AlignLeft | Qt::AlignVCenter,
                             row.item->text());
            painter.setFont(supportFont);
            painter.setPen(selected ? onSecondaryContainer : onSurfaceVariant);
            painter.drawText(bottom, Qt::AlignLeft | Qt::AlignVCenter,
                             row.item->supportingText());
        }

        // :focus-visible ring on the focused row — keyboard only, inward.
        if (focused && m_focusRing != nullptr) {
            const QRectF bounds = rowRect.adjusted(kFocusMargin, kFocusMargin,
                                                   -kFocusMargin, -kFocusMargin);
            MdFocusRingSpec spec;
            spec.inward = true;
            const QList<qreal> radii = MdShape::radii(ShapeCorner::None);
            MdFocusRing::paint(&painter, bounds, radii,
                               theme.color(ColorRole::Secondary), spec,
                               m_focusRing->isAnimating() ? m_focusRing->elapsedMs()
                                                          : -1);
        }
    }
    Q_UNUSED(headlineMetrics);
}

void MdTree::mousePressEvent(QMouseEvent *event)
{
    const int row = rowAt(mousePosition(event));
    if (row < 0) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressedRow = row;
    m_focusRow = row;
    if (m_ripple != nullptr) {
        const QList<Row> rows = visibleRows();
        const bool selected = rows.at(row).path == m_selectedPath;
        m_ripple->setBounds(QSizeF(width(), m_rowHeight));
        m_ripple->setContentColor(MdTheme::instance().color(
            selected ? ColorRole::OnSecondaryContainer : ColorRole::OnSurface));
        m_ripple->press(QPointF(mousePosition(event).x(), mousePosition(event).y()
                                                        - row * m_rowHeight));
    }
    update();
    QWidget::mousePressEvent(event);
}

void MdTree::mouseReleaseEvent(QMouseEvent *event)
{
    const bool wasPressed = m_pressedRow >= 0;
    const int row = rowAt(mousePosition(event));
    m_pressedRow = -1;
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
    if (wasPressed && row >= 0 && row == rowAt(mousePosition(event))) {
        const QList<Row> rows = visibleRows();
        if (row < rows.size()) {
            // Click on the chevron toggles; on the row selects + activates.
            const QRectF chevron = chevronRectForRow(row);
            if (rows.at(row).hasChildren && chevron.contains(mousePosition(event))) {
                toggleRow(row);
            } else {
                setSelectedPath(rows.at(row).path);
                activateRow(row);
            }
        }
    }
    update();
    QWidget::mouseReleaseEvent(event);
}

void MdTree::mouseDoubleClickEvent(QMouseEvent *event)
{
    const int row = rowAt(mousePosition(event));
    if (row >= 0) {
        const QList<Row> rows = visibleRows();
        if (row < rows.size() && rows.at(row).hasChildren) {
            toggleRow(row);
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void MdTree::enterEvent(MdEnterEvent *event)
{
    setHoveredRow(rowAt(enterPosition(this, event)));
    QWidget::enterEvent(event);
}

void MdTree::leaveEvent(QEvent *event)
{
    setHoveredRow(-1);
    QWidget::leaveEvent(event);
}

void MdTree::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    switch (event->reason()) {
    case Qt::MouseFocusReason:
    case Qt::PopupFocusReason:
    case Qt::ActiveWindowFocusReason:
        m_focusIsKeyboard = false;
        break;
    default:
        m_focusIsKeyboard = true;
        break;
    }
    if (m_focusRing != nullptr) {
        if (m_focusIsKeyboard) {
            m_focusRing->start();
        } else {
            m_focusRing->stop();
        }
    }
    update();
}

void MdTree::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing != nullptr) {
        m_focusRing->stop();
    }
    update();
}

void MdTree::keyPressEvent(QKeyEvent *event)
{
    const QList<Row> rows = visibleRows();
    switch (event->key()) {
    case Qt::Key_Down:
        moveSelection(1);
        event->accept();
        return;
    case Qt::Key_Up:
        moveSelection(-1);
        event->accept();
        return;
    case Qt::Key_Home:
        if (!rows.isEmpty()) {
            m_focusRow = 0;
            setSelectedPath(rows.first().path);
        }
        event->accept();
        return;
    case Qt::Key_End:
        if (!rows.isEmpty()) {
            m_focusRow = rows.size() - 1;
            setSelectedPath(rows.last().path);
        }
        event->accept();
        return;
    case Qt::Key_Right:
        if (m_focusRow >= 0 && m_focusRow < rows.size()) {
            const Row &row = rows.at(m_focusRow);
            if (row.hasChildren && !row.item->isExpanded()) {
                setExpanded(row.path, true);
            } else if (row.hasChildren && m_focusRow + 1 < rows.size()) {
                m_focusRow += 1;
                setSelectedPath(rows.at(m_focusRow).path);
            }
        }
        event->accept();
        return;
    case Qt::Key_Left:
        if (m_focusRow >= 0 && m_focusRow < rows.size()) {
            const Row &row = rows.at(m_focusRow);
            if (row.hasChildren && row.item->isExpanded()) {
                setExpanded(row.path, false);
            } else if (!row.path.isEmpty()) {
                MdTreePath parentPath = row.path;
                parentPath.removeLast();
                setSelectedPath(parentPath);
                for (int i = 0; i < rows.size(); ++i) {
                    if (rows.at(i).path == parentPath) {
                        m_focusRow = i;
                        break;
                    }
                }
            }
        }
        event->accept();
        return;
    case Qt::Key_Space:
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (m_focusRow >= 0 && m_focusRow < rows.size()) {
            activateRow(m_focusRow);
        }
        event->accept();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdTree::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange) {
        update();
    }
}

void MdTree::onThemeChanged()
{
    update();
}

void MdTree::moveSelection(int delta)
{
    const QList<Row> rows = visibleRows();
    if (rows.isEmpty()) {
        return;
    }
    // First move from "nothing selected" lands on the focus row, not the
    // one after it — the list's roving-tabindex entry behaviour.
    if (m_selectedPath.isEmpty()) {
        m_focusRow = qBound(0, m_focusRow, rows.size() - 1);
        setSelectedPath(rows.at(m_focusRow).path);
        return;
    }
    int next = m_focusRow + delta;
    if (next < 0) {
        next = m_wrap ? rows.size() - 1 : 0;
    } else if (next >= rows.size()) {
        next = m_wrap ? 0 : rows.size() - 1;
    }
    m_focusRow = next;
    setSelectedPath(rows.at(next).path);
}

void MdTree::activateRow(int row)
{
    const QList<Row> rows = visibleRows();
    if (row < 0 || row >= rows.size()) {
        return;
    }
    setSelectedPath(rows.at(row).path);
    emit itemActivated(rows.at(row).path);
}

void MdTree::toggleRow(int row)
{
    const QList<Row> rows = visibleRows();
    if (row < 0 || row >= rows.size()) {
        return;
    }
    const Row &r = rows.at(row);
    if (!r.hasChildren) {
        return;
    }
    setExpanded(r.path, !r.item->isExpanded());
}

} // namespace md
