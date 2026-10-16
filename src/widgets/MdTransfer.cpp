#include "MdTransfer.h"

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdButton.h"

#include <algorithm>

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtMath>

namespace md {

namespace {

/// List one-line row height (`md.comp.list.list-item.container.height` 56).
constexpr qreal kRowH = 56.0;
/// Horizontal inset inside a panel (`md.sys.spacing` 16).
constexpr qreal kInset = 16.0;
/// Title band above each list (`md.sys.spacing` 12 + label line).
constexpr qreal kTitleBand = 32.0;
/// Gutter between the two panels for the action buttons (`md.sys.spacing` 12).
constexpr qreal kGutter = 12.0;
/// Outline-variant hairline under each row (`md.comp.divider.thickness` 1).
constexpr qreal kDivider = 1.0;
/// Roundness of the panel chrome (`md.sys.shape.corner.medium` family).
constexpr qreal kPanelRadius = 12.0;

} // namespace

MdTransfer::MdTransfer(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdTransfer::~MdTransfer()
{
    // Disconnect children before members die.
    if (m_toTarget) {
        disconnect(m_toTarget, nullptr, this, nullptr);
    }
    if (m_toSource) {
        disconnect(m_toSource, nullptr, this, nullptr);
    }
}

void MdTransfer::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(QStringLiteral("transfer"));
    m_sourceTitle = QStringLiteral("Source");
    m_targetTitle = QStringLiteral("Target");

    // Text buttons with chevron glyphs, the icon-button reading of the
    // shuttle actions (md.comp.button.text + md.comp.icon.size 24).
    m_toTarget = new MdButton(this);
    m_toTarget->setVariant(ButtonVariant::Text);
    m_toTarget->setLeadingIcon(QStringLiteral("chevron_right"));
    m_toTarget->setAccessibleName(QStringLiteral("move to target"));
    m_toTarget->show();
    connect(m_toTarget, &MdButton::clicked, this, &MdTransfer::onMoveToTarget);

    m_toSource = new MdButton(this);
    m_toSource->setVariant(ButtonVariant::Text);
    m_toSource->setLeadingIcon(QStringLiteral("chevron_left"));
    m_toSource->setAccessibleName(QStringLiteral("move to source"));
    m_toSource->show();
    connect(m_toSource, &MdButton::clicked, this, &MdTransfer::onMoveToSource);

    layoutButtons();
    MdStyleBase::connectThemeUpdate(this, &MdTransfer::onThemeChanged);
}

void MdTransfer::setSourceTitle(const QString &title)
{
    if (m_sourceTitle == title) {
        return;
    }
    m_sourceTitle = title;
    update();
    emit sourceTitleChanged(m_sourceTitle);
}

void MdTransfer::setTargetTitle(const QString &title)
{
    if (m_targetTitle == title) {
        return;
    }
    m_targetTitle = title;
    update();
    emit targetTitleChanged(m_targetTitle);
}

void MdTransfer::setSourceItems(const QList<MdTransferItem> &items)
{
    if (m_sourceItems == items) {
        return;
    }
    m_sourceItems = items;
    const bool hadSelection = !m_sourceSelection.isEmpty();
    m_sourceSelection.clear();
    updateGeometry();
    update();
    emit sourceItemsChanged(m_sourceItems);
    if (hadSelection) {
        emit selectionChanged();
    }
}

void MdTransfer::setTargetItems(const QList<MdTransferItem> &items)
{
    if (m_targetItems == items) {
        return;
    }
    m_targetItems = items;
    const bool hadSelection = !m_targetSelection.isEmpty();
    m_targetSelection.clear();
    updateGeometry();
    update();
    emit targetItemsChanged(m_targetItems);
    if (hadSelection) {
        emit selectionChanged();
    }
}

void MdTransfer::setMultiSelect(bool multi)
{
    if (m_multiSelect == multi) {
        return;
    }
    m_multiSelect = multi;
    if (!m_multiSelect) {
        if (m_sourceSelection.size() > 1) {
            m_sourceSelection = QList<int>() << m_sourceSelection.last();
        }
        if (m_targetSelection.size() > 1) {
            m_targetSelection = QList<int>() << m_targetSelection.last();
        }
    }
    update();
    emit multiSelectChanged(m_multiSelect);
    emit selectionChanged();
}

void MdTransfer::selectSourceRow(int row, bool selected)
{
    if (row < 0 || row >= m_sourceItems.size()) {
        return;
    }
    const int idx = m_sourceSelection.indexOf(row);
    if (selected && idx < 0) {
        if (!m_multiSelect) {
            m_sourceSelection.clear();
        }
        m_sourceSelection.append(row);
    } else if (!selected && idx >= 0) {
        m_sourceSelection.removeAt(idx);
    } else {
        return;
    }
    update();
    emit selectionChanged();
}

void MdTransfer::selectTargetRow(int row, bool selected)
{
    if (row < 0 || row >= m_targetItems.size()) {
        return;
    }
    const int idx = m_targetSelection.indexOf(row);
    if (selected && idx < 0) {
        if (!m_multiSelect) {
            m_targetSelection.clear();
        }
        m_targetSelection.append(row);
    } else if (!selected && idx >= 0) {
        m_targetSelection.removeAt(idx);
    } else {
        return;
    }
    update();
    emit selectionChanged();
}

void MdTransfer::clearSelection()
{
    if (m_sourceSelection.isEmpty() && m_targetSelection.isEmpty()) {
        return;
    }
    m_sourceSelection.clear();
    m_targetSelection.clear();
    update();
    emit selectionChanged();
}

void MdTransfer::moveSelectedToTarget()
{
    if (m_sourceSelection.isEmpty()) {
        return;
    }
    QList<int> rows = m_sourceSelection;
    std::sort(rows.begin(), rows.end());
    QList<MdTransferItem> moved;
    for (int i = rows.size() - 1; i >= 0; --i) {
        const int row = rows.at(i);
        if (row < 0 || row >= m_sourceItems.size()) {
            continue;
        }
        moved.prepend(m_sourceItems.takeAt(row));
    }
    if (moved.isEmpty()) {
        return;
    }
    m_targetItems.append(moved);
    m_sourceSelection.clear();
    m_focusInSource = true;
    m_focusRow = 0;
    updateGeometry();
    update();
    emit sourceItemsChanged(m_sourceItems);
    emit targetItemsChanged(m_targetItems);
    emit selectionChanged();
    emit movedToTarget(moved);
}

void MdTransfer::moveSelectedToSource()
{
    if (m_targetSelection.isEmpty()) {
        return;
    }
    QList<int> rows = m_targetSelection;
    std::sort(rows.begin(), rows.end());
    QList<MdTransferItem> moved;
    for (int i = rows.size() - 1; i >= 0; --i) {
        const int row = rows.at(i);
        if (row < 0 || row >= m_targetItems.size()) {
            continue;
        }
        moved.prepend(m_targetItems.takeAt(row));
    }
    if (moved.isEmpty()) {
        return;
    }
    m_sourceItems.append(moved);
    m_targetSelection.clear();
    m_focusInSource = false;
    m_focusRow = 0;
    updateGeometry();
    update();
    emit sourceItemsChanged(m_sourceItems);
    emit targetItemsChanged(m_targetItems);
    emit selectionChanged();
    emit movedToSource(moved);
}

QColor MdTransfer::resolvedSelectedBackground() const
{
    // md.sys.color.secondary-container
    return MdTheme::instance().color(ColorRole::SecondaryContainer);
}

QColor MdTransfer::resolvedSelectedContent() const
{
    // md.sys.color.on-secondary-container
    return MdTheme::instance().color(ColorRole::OnSecondaryContainer);
}

QColor MdTransfer::resolvedPanelColor() const
{
    // md.sys.color.surface-container
    return MdTheme::instance().color(ColorRole::SurfaceContainer);
}

QSize MdTransfer::sizeHint() const
{
    const int rows = qMax(m_sourceItems.size(), m_targetItems.size());
    const qreal h = kTitleBand + qMax(3, rows) * kRowH + 24.0;
    return QSize(qCeil(560.0), qCeil(h));
}

QSize MdTransfer::minimumSizeHint() const
{
    return QSize(280, 160);
}

QRectF MdTransfer::sourcePanelRect() const
{
    const qreal half = (width() - kGutter * 2.0 - 48.0) * 0.5;
    return QRectF(0, 0, half, height());
}

QRectF MdTransfer::targetPanelRect() const
{
    const qreal half = (width() - kGutter * 2.0 - 48.0) * 0.5;
    return QRectF(width() - half, 0, half, height());
}

QRectF MdTransfer::rowRect(bool source, int row, const QRectF &panel) const
{
    const qreal top = panel.top() + kTitleBand + row * kRowH;
    return QRectF(panel.left(), top, panel.width(), kRowH);
}

int MdTransfer::rowAt(bool source, const QPointF &pos) const
{
    const QList<MdTransferItem> &items = source ? m_sourceItems : m_targetItems;
    const QRectF panel = source ? sourcePanelRect() : targetPanelRect();
    if (!panel.contains(pos)) {
        return -1;
    }
    const int row = int((pos.y() - panel.top() - kTitleBand) / kRowH);
    return (row >= 0 && row < items.size()) ? row : -1;
}

void MdTransfer::toggleRow(bool source, int row)
{
    if (source) {
        const bool on = m_sourceSelection.contains(row);
        selectSourceRow(row, !on);
    } else {
        const bool on = m_targetSelection.contains(row);
        selectTargetRow(row, !on);
    }
}

void MdTransfer::layoutButtons()
{
    const qreal cx = width() * 0.5;
    const qreal cy = height() * 0.5;
    if (m_toTarget) {
        const QSize hint = m_toTarget->sizeHint();
        m_toTarget->resize(hint);
        m_toTarget->move(int(cx - hint.width() * 0.5), int(cy - hint.height() - 4.0));
    }
    if (m_toSource) {
        const QSize hint = m_toSource->sizeHint();
        m_toSource->resize(hint);
        m_toSource->move(int(cx - hint.width() * 0.5), int(cy + 4.0));
    }
}

void MdTransfer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutButtons();
}

void MdTransfer::mousePressEvent(QMouseEvent *event)
{
    const QPointF pos = md::mousePosition(event);
    const int srcRow = rowAt(true, pos);
    const int tgtRow = rowAt(false, pos);
    if (srcRow >= 0) {
        m_focusInSource = true;
        m_focusRow = srcRow;
        toggleRow(true, srcRow);
        event->accept();
        return;
    }
    if (tgtRow >= 0) {
        m_focusInSource = false;
        m_focusRow = tgtRow;
        toggleRow(false, tgtRow);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MdTransfer::keyPressEvent(QKeyEvent *event)
{
    const int count = m_focusInSource ? m_sourceItems.size() : m_targetItems.size();
    switch (event->key()) {
    case Qt::Key_Up:
        m_focusRow = qMax(0, m_focusRow - 1);
        update();
        event->accept();
        return;
    case Qt::Key_Down:
        m_focusRow = qMin(qMax(0, count - 1), m_focusRow + 1);
        update();
        event->accept();
        return;
    case Qt::Key_Home:
        m_focusRow = 0;
        update();
        event->accept();
        return;
    case Qt::Key_End:
        m_focusRow = qMax(0, count - 1);
        update();
        event->accept();
        return;
    case Qt::Key_Left:
        m_focusInSource = true;
        m_focusRow = qBound(0, m_focusRow, qMax(0, m_sourceItems.size() - 1));
        update();
        event->accept();
        return;
    case Qt::Key_Right:
        m_focusInSource = false;
        m_focusRow = qBound(0, m_focusRow, qMax(0, m_targetItems.size() - 1));
        update();
        event->accept();
        return;
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (count > 0) {
            toggleRow(m_focusInSource, m_focusRow);
        }
        event->accept();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdTransfer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF sourcePanel = sourcePanelRect();
    const QRectF targetPanel = targetPanelRect();
    const QColor panelColor = resolvedPanelColor();
    const QColor selectedBg = resolvedSelectedBackground();
    const QColor selectedFg = resolvedSelectedContent();
    const QColor titleColor = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
    const QColor textColor = MdTheme::instance().color(ColorRole::OnSurface);
    const QColor supportingColor = titleColor;
    const QColor dividerColor = MdTheme::instance().color(ColorRole::OutlineVariant);
    const QColor outline = MdTheme::instance().color(ColorRole::Outline);
    const QFont titleFont = MdTypeScale::font(TypeStyle::LabelLarge);
    const QFont rowFont = MdTypeScale::font(TypeStyle::TitleMedium);
    const QFont bodyFont = MdTypeScale::font(TypeStyle::BodyMedium);

    auto paintPanel = [&](const QRectF &panel, const QString &title,
                          const QList<MdTransferItem> &items,
                          const QList<int> &selection, bool source) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(panelColor);
        painter.drawRoundedRect(panel, kPanelRadius, kPanelRadius);

        painter.setFont(titleFont);
        painter.setPen(titleColor);
        painter.drawText(QRectF(panel.left() + kInset, panel.top(),
                                panel.width() - kInset * 2.0, kTitleBand),
                         Qt::AlignLeft | Qt::AlignVCenter, title);

        for (int row = 0; row < items.size(); ++row) {
            const QRectF r = rowRect(source, row, panel);
            const bool selected = selection.contains(row);
            const bool focused = source == m_focusInSource && row == m_focusRow
                    && hasFocus();
            if (selected) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(selectedBg);
                painter.drawRect(r);
            }
            if (focused) {
                painter.setPen(outline);
                painter.setBrush(Qt::NoBrush);
                painter.drawRect(r.adjusted(1, 1, -1, -1));
            }

            painter.setFont(rowFont);
            painter.setPen(selected ? selectedFg : textColor);
            painter.drawText(QRectF(r.left() + kInset, r.top(),
                                    r.width() - kInset * 2.0, r.height() * 0.55),
                             Qt::AlignLeft | Qt::AlignBottom, items.at(row).title);

            if (!items.at(row).supporting.isEmpty()) {
                painter.setFont(bodyFont);
                painter.setPen(selected ? selectedFg : supportingColor);
                painter.drawText(QRectF(r.left() + kInset, r.top() + r.height() * 0.5,
                                        r.width() - kInset * 2.0, r.height() * 0.45),
                                 Qt::AlignLeft | Qt::AlignTop, items.at(row).supporting);
            }

            if (row + 1 < items.size()) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(dividerColor);
                painter.drawRect(QRectF(r.left() + kInset, r.bottom() - kDivider,
                                        r.width() - kInset * 2.0, kDivider));
            }
        }
    };

    paintPanel(sourcePanel, m_sourceTitle, m_sourceItems, m_sourceSelection, true);
    paintPanel(targetPanel, m_targetTitle, m_targetItems, m_targetSelection, false);
}

void MdTransfer::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
        layoutButtons();
    }
}

void MdTransfer::onThemeChanged()
{
    update();
}

void MdTransfer::onMoveToTarget()
{
    moveSelectedToTarget();
}

void MdTransfer::onMoveToSource()
{
    moveSelectedToSource();
}

} // namespace md
