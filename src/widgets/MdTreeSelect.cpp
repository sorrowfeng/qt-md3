#include "MdTreeSelect.h"

#include "MdTree.h"
#include "widgets/MdTextField.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QVBoxLayout>

namespace md {

namespace {

/// The trailing dropdown indicator the M3 exposed dropdown menu specifies.
const QString kDropdownIcon = QStringLiteral("arrow_drop_down");

/// Popup height floor: five list rows before the tree scrolls.
constexpr int kPopupMinRows = 3;
constexpr int kPopupMaxRows = 8;

/// Menu surface: `surface-container` at elevation level 2 (md.comp.menu).
/// The popup paints this itself — MdTree is a plain surface painter.

} // namespace

MdTreeSelect::MdTreeSelect(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdTreeSelect::~MdTreeSelect()
{
    // m_popup is parented to this->window(), which is `this` when MdTreeSelect
    // is top-level. ~QWidget's deleteChildren() would then destroy the popup
    // after this class's members are already dead, and the destroyed handler
    // would write m_popup/m_popupOpen on a half-destroyed object (UBSAN
    // type-confusion: dynamic type is only QWidget at that point). Tear the
    // popup down first so destroyed fires while `this` is still MdTreeSelect.
    if (m_popup != nullptr) {
        disconnect(m_popup, nullptr, this, nullptr);
        delete m_popup;
        m_popup = nullptr;
        m_tree = nullptr;
        m_popupOpen = false;
    }
    // Cut field -> parent wires before ~QWidget destroys the children;
    // otherwise a child emitting during its own destruction would call a
    // slot on a half-destroyed MdTreeSelect.
    if (m_field != nullptr) {
        disconnect(m_field, nullptr, this, nullptr);
    }
}

void MdTreeSelect::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_InputMethodEnabled, false);

    // The field is the chrome. Read-only: the user picks from the tree.
    m_field = new MdTextField(this);
    m_field->setReadOnly(true);
    m_field->setFocusPolicy(Qt::NoFocus);
    m_field->setTrailingIconName(kDropdownIcon);
    m_field->setCursor(Qt::PointingHandCursor);
    m_field->installEventFilter(this);

    connect(m_field, &MdTextField::variantChanged, this, &MdTreeSelect::variantChanged);
    connect(m_field, &MdTextField::labelTextChanged, this, &MdTreeSelect::labelTextChanged);
    connect(m_field, &MdTextField::supportingTextChanged, this,
            &MdTreeSelect::supportingTextChanged);
    connect(m_field, &MdTextField::placeholderTextChanged, this,
            &MdTreeSelect::placeholderTextChanged);

    // The popup is a Qt::Popup top-level hosting the tree, the same shape
    // MdSelect uses for its menu — not parented into the field's layout.
    m_popup = new QWidget(this->window(), Qt::Popup | Qt::FramelessWindowHint);
    m_popup->setObjectName(QStringLiteral("md-tree-select-popup"));
    m_popup->setAttribute(Qt::WA_DeleteOnClose, false);

    m_tree = new MdTree(m_popup);
    m_tree->setIndent(16);
    m_tree->setRowHeight(56);
    auto *layout = new QVBoxLayout(m_popup);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_tree);

    connect(m_tree, &MdTree::itemActivated, this, &MdTreeSelect::onTreeActivated);
    connect(m_popup, &QObject::destroyed, this, [this]() {
        m_popup = nullptr;
        m_tree = nullptr;
        m_popupOpen = false;
    });
}

// --- field appearance ---------------------------------------------------------

MdTextFieldVariant MdTreeSelect::variant() const
{
    return m_field->variant();
}

void MdTreeSelect::setVariant(MdTextFieldVariant variant)
{
    m_field->setVariant(variant);
    emit variantChanged(variant);
}

QString MdTreeSelect::labelText() const
{
    return m_field->labelText();
}

void MdTreeSelect::setLabelText(const QString &label)
{
    m_field->setLabelText(label);
}

QString MdTreeSelect::supportingText() const
{
    return m_field->supportingText();
}

void MdTreeSelect::setSupportingText(const QString &text)
{
    m_field->setSupportingText(text);
}

QString MdTreeSelect::placeholderText() const
{
    return m_field->placeholderText();
}

void MdTreeSelect::setPlaceholderText(const QString &text)
{
    m_field->setPlaceholderText(text);
}

QString MdTreeSelect::currentText() const
{
    const MdTreeItem *item =
        m_tree != nullptr ? m_tree->itemAt(m_currentPath) : nullptr;
    return item != nullptr ? item->text() : QString();
}

MdTreePath MdTreeSelect::currentPath() const
{
    return m_currentPath;
}

// --- tree model ---------------------------------------------------------------

void MdTreeSelect::addTopLevelItem(MdTreeItem *item)
{
    if (m_tree != nullptr) {
        m_tree->addTopLevelItem(item);
    }
}

void MdTreeSelect::clear()
{
    if (m_tree != nullptr) {
        m_tree->clear();
    }
    m_currentPath.clear();
    syncField();
    emit currentPathChanged(m_currentPath);
    emit currentTextChanged(currentText());
}

void MdTreeSelect::setSelectedPath(const MdTreePath &path)
{
    if (m_tree == nullptr) {
        return;
    }
    m_tree->setSelectedPath(path);
    m_currentPath = path;
    syncField();
    emit currentPathChanged(m_currentPath);
    emit currentTextChanged(currentText());
}

void MdTreeSelect::clearSelection()
{
    setSelectedPath(MdTreePath());
}

// --- popup ---------------------------------------------------------------------

void MdTreeSelect::showPopup()
{
    if (m_popup == nullptr || m_tree == nullptr) {
        return;
    }
    if (m_popupOpen) {
        hidePopup();
        return;
    }
    // Size the popup to the visible rows, clamped to the published menu
    // max-height reading (a scroll surface, never an unbounded list).
    const int rows = m_tree->visibleRowCount();
    const int shown = qBound(kPopupMinRows, rows == 0 ? kPopupMinRows : rows, kPopupMaxRows);
    const int popupWidth = qMax(width(), 200);
    const int popupHeight = shown * m_tree->rowHeight();
    m_popup->resize(popupWidth, popupHeight);
    m_tree->setGeometry(m_popup->rect());

    // Anchored under the field, left-aligned — the exposed dropdown's only
    // published anchor position.
    m_popup->move(mapToGlobal(QPoint(0, height())));
    m_popup->show();
    m_popupOpen = true;
    m_tree->setFocus(Qt::PopupFocusReason);
}

void MdTreeSelect::hidePopup()
{
    if (!m_popupOpen || m_popup == nullptr) {
        return;
    }
    m_popup->hide();
    m_popupOpen = false;
}

bool MdTreeSelect::isPopupVisible() const
{
    return m_popupOpen;
}

// --- geometry ------------------------------------------------------------------

QSize MdTreeSelect::sizeHint() const
{
    return m_field->sizeHint();
}

QSize MdTreeSelect::minimumSizeHint() const
{
    return m_field->minimumSizeHint();
}

int MdTreeSelect::heightForWidth(int width) const
{
    Q_UNUSED(width);
    return m_field != nullptr ? m_field->sizeHint().height() : 56;
}

// --- events --------------------------------------------------------------------

void MdTreeSelect::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MdTreeSelect::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Space:
    case Qt::Key_F4:
        showPopup();
        event->accept();
        return;
    case Qt::Key_Down:
        if (event->modifiers() & Qt::AltModifier) {
            showPopup();
            event->accept();
            return;
        }
        break;
    case Qt::Key_Escape:
        if (m_popupOpen) {
            hidePopup();
            event->accept();
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdTreeSelect::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange && m_field != nullptr) {
        m_field->setEnabled(isEnabled());
    }
}

bool MdTreeSelect::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_field) {
        if (event->type() == QEvent::MouseButtonPress) {
            showPopup();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// --- internal ------------------------------------------------------------------

void MdTreeSelect::onFieldClicked()
{
    showPopup();
}

void MdTreeSelect::onTreeActivated(const MdTreePath &path)
{
    if (m_tree == nullptr) {
        return;
    }
    const MdTreeItem *item = m_tree->itemAt(path);
    // Branch activation toggles expansion only; a leaf commits.
    if (item != nullptr && item->childCount() > 0) {
        return;
    }
    m_currentPath = path;
    if (m_tree != nullptr) {
        m_tree->setSelectedPath(path);
    }
    syncField();
    hidePopup();
    setFocus(Qt::PopupFocusReason);
    emit currentPathChanged(m_currentPath);
    emit currentTextChanged(currentText());
}

void MdTreeSelect::onPopupClosed()
{
    m_popupOpen = false;
    setFocus(Qt::PopupFocusReason);
}

void MdTreeSelect::relayout()
{
    if (m_field != nullptr) {
        m_field->setGeometry(rect());
    }
}

void MdTreeSelect::syncField()
{
    if (m_field == nullptr) {
        return;
    }
    m_field->setText(currentText());
}

} // namespace md
