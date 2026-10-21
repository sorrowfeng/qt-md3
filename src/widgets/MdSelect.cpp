#include "MdSelect.h"

#include "core/MdIcon.h"
#include "core/MdTheme.h"
#include "private/MdPopup.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdMenu.h"
#include "widgets/MdTextField.h"

#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>

namespace md {

namespace {

/// The trailing dropdown indicator the M3 exposed dropdown menu specifies.
/// MdSplitButton uses the same glyph for its menu trigger.
const QString kDropdownIcon = QStringLiteral("arrow_drop_down");

} // namespace

MdSelect::MdSelect(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdSelect::~MdSelect() = default;

void MdSelect::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_InputMethodEnabled, false);

    // The field is the whole chrome. It is read-only: MdSelect owns selection.
    m_field = new MdTextField(this);
    m_field->setReadOnly(true);
    m_field->setFocusPolicy(Qt::NoFocus);
    m_field->setTrailingIconName(kDropdownIcon);
    m_field->setCursor(Qt::PointingHandCursor);
    // Clicks on the field must reach MdSelect so they open the popup.
    m_field->installEventFilter(this);

    // Re-emit the field's NOTIFYs so MdSelect's Q_PROPERTYs stay live.
    connect(m_field, &MdTextField::variantChanged, this, &MdSelect::variantChanged);
    connect(m_field, &MdTextField::labelTextChanged, this, &MdSelect::labelTextChanged);
    connect(m_field, &MdTextField::supportingTextChanged, this, &MdSelect::supportingTextChanged);
    connect(m_field, &MdTextField::placeholderTextChanged, this, &MdSelect::placeholderTextChanged);
    connect(m_field, &MdTextField::errorChanged, this, &MdSelect::errorChanged);
    connect(m_field, &MdTextField::leadingIconNameChanged, this, &MdSelect::leadingIconNameChanged);

    // The menu is a Qt::Popup top-level; it is not parented into the layout.
    m_menu = new MdMenu(this->window());
    connect(m_menu, &MdMenu::closed, this, &MdSelect::onMenuClosed);

    // Theme switches must re-sync the field: a rebuild or palette change can
    // leave the value blank / invisible even though currentIndex is intact.
    MdStyleBase::connectThemeUpdate(this, &MdSelect::onThemeChanged);
}

// --- field appearance ---------------------------------------------------------

MdTextFieldVariant MdSelect::variant() const
{
    return m_field->variant();
}

void MdSelect::setVariant(MdTextFieldVariant variant)
{
    m_field->setVariant(variant);
    emit variantChanged(variant);
}

QString MdSelect::labelText() const
{
    return m_field->labelText();
}

void MdSelect::setLabelText(const QString &label)
{
    m_field->setLabelText(label);
}

QString MdSelect::supportingText() const
{
    return m_field->supportingText();
}

void MdSelect::setSupportingText(const QString &text)
{
    m_field->setSupportingText(text);
}

QString MdSelect::placeholderText() const
{
    return m_field->placeholderText();
}

void MdSelect::setPlaceholderText(const QString &text)
{
    m_field->setPlaceholderText(text);
}

bool MdSelect::hasError() const
{
    return m_field->hasError();
}

void MdSelect::setError(bool error)
{
    m_field->setError(error);
}

QString MdSelect::leadingIconName() const
{
    return m_field->leadingIconName();
}

void MdSelect::setLeadingIconName(const QString &iconName)
{
    m_field->setLeadingIconName(iconName);
}

// --- model --------------------------------------------------------------------

int MdSelect::count() const
{
    return m_items.size();
}

int MdSelect::currentIndex() const
{
    return m_currentIndex;
}

QString MdSelect::currentText() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_items.size()) {
        return QString();
    }
    return m_items.at(m_currentIndex);
}

void MdSelect::setCurrentIndex(int index)
{
    if (m_items.isEmpty()) {
        if (m_currentIndex == -1) {
            return;
        }
        m_currentIndex = -1;
        applyCurrentText();
        emit currentIndexChanged(-1);
        emit currentTextChanged(QString());
        return;
    }
    const int clamped = qBound(0, index, m_items.size() - 1);
    if (m_currentIndex == clamped) {
        return;
    }
    m_currentIndex = clamped;
    applyCurrentText();
    emit currentIndexChanged(m_currentIndex);
    emit currentTextChanged(currentText());
}

int MdSelect::addItem(const QString &text)
{
    const int index = m_items.size();
    m_items.append(text);
    rebuildMenu();
    emit countChanged(m_items.size());
    if (m_currentIndex < 0) {
        setCurrentIndex(0);
    }
    return index;
}

void MdSelect::addItems(const QStringList &texts)
{
    if (texts.isEmpty()) {
        return;
    }
    m_items.append(texts);
    rebuildMenu();
    emit countChanged(m_items.size());
    if (m_currentIndex < 0) {
        setCurrentIndex(0);
    }
}

void MdSelect::insertItem(int index, const QString &text)
{
    const int clamped = qBound(0, index, m_items.size());
    m_items.insert(clamped, text);
    if (m_currentIndex >= clamped) {
        ++m_currentIndex;
    }
    rebuildMenu();
    emit countChanged(m_items.size());
    if (m_currentIndex < 0) {
        setCurrentIndex(0);
    }
}

void MdSelect::removeItem(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return;
    }
    m_items.removeAt(index);
    if (m_currentIndex == index) {
        m_currentIndex = m_items.isEmpty() ? -1 : qMin(index, m_items.size() - 1);
        applyCurrentText();
        emit currentIndexChanged(m_currentIndex);
        emit currentTextChanged(currentText());
    } else if (m_currentIndex > index) {
        --m_currentIndex;
        emit currentIndexChanged(m_currentIndex);
    }
    rebuildMenu();
    emit countChanged(m_items.size());
}

void MdSelect::clear()
{
    if (m_items.isEmpty()) {
        return;
    }
    m_items.clear();
    m_currentIndex = -1;
    applyCurrentText();
    rebuildMenu();
    emit countChanged(0);
    emit currentIndexChanged(-1);
    emit currentTextChanged(QString());
}

QString MdSelect::itemText(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return QString();
    }
    return m_items.at(index);
}

// --- popup -------------------------------------------------------------------

void MdSelect::showPopup()
{
    if (m_items.isEmpty()) {
        return;
    }
    if (m_popupOpen || (m_menu && m_menu->isVisible())) {
        hidePopup();
        return;
    }
    // Keep the field's focused chrome on while the menu is open: the menu
    // steals the popup focus and the select itself may see FocusOut.
    if (m_field) {
        m_field->setForceFocusVisual(true);
    }
    // Rebuild only when the model changed since the last build. Rebuilding
    // every open used to clear() the previous rows (orphan top-level windows)
    // and re-add them just to show the same list.
    if (m_menuDirty) {
        rebuildMenu();
    }
    // Highlight the current selection so the open menu lands on it.
    for (int i = 0; i < m_menuItems.size(); ++i) {
        m_menuItems.at(i)->setSelected(i == m_currentIndex);
    }
    // Anchored under the field, left-aligned — the exposed dropdown's
    // only published anchor position.
    // Optional frame dump (QT_MD3_POPUP_FRAMES=<dir>) for the open flight.
    if (!qEnvironmentVariableIsEmpty("QT_MD3_POPUP_FRAMES")) {
        new PopupFrameDumper(m_menu, QStringLiteral("select"), m_menu);
    }
    m_menu->popup(QRect(mapToGlobal(QPoint(0, 0)), size()));
    m_popupOpen = true;
}

void MdSelect::hidePopup()
{
    if (!m_popupOpen && !(m_menu && m_menu->isVisible())) {
        return;
    }
    // Reverse the menu open springs; a bare hide() pops the surface out
    // in one frame.
    if (m_menu) {
        m_menu->requestClose();
    }
    m_popupOpen = false;
}

bool MdSelect::isPopupVisible() const
{
    return m_popupOpen || (m_menu && m_menu->isVisible());
}

// --- geometry ----------------------------------------------------------------

QSize MdSelect::sizeHint() const
{
    return m_field->sizeHint();
}

QSize MdSelect::minimumSizeHint() const
{
    return m_field->minimumSizeHint();
}

// --- events -------------------------------------------------------------------

void MdSelect::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_field->setGeometry(rect());
}

void MdSelect::keyPressEvent(QKeyEvent *event)
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
        if (!m_items.isEmpty()) {
            setCurrentIndex(qBound(0, m_currentIndex + 1, m_items.size() - 1));
            emit activated(m_currentIndex);
            emit textActivated(currentText());
            event->accept();
            return;
        }
        break;
    case Qt::Key_Up:
        if (!m_items.isEmpty()) {
            setCurrentIndex(qBound(0, m_currentIndex - 1, m_items.size() - 1));
            emit activated(m_currentIndex);
            emit textActivated(currentText());
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

void MdSelect::focusInEvent(QFocusEvent *event)
{
    // The field is a NoFocus child so Tab stays one stop on the select, which
    // means the field cannot take focus and paint the focused chrome itself.
    // Drive the focus visual explicitly instead of relying on setFocus().
    if (m_field && event->reason() != Qt::NoFocusReason) {
        m_field->setForceFocusVisual(true);
    }
    QWidget::focusInEvent(event);
}

void MdSelect::focusOutEvent(QFocusEvent *event)
{
    if (m_field && event->reason() != Qt::PopupFocusReason) {
        m_field->setForceFocusVisual(false);
    }
    QWidget::focusOutEvent(event);
}

void MdSelect::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange && m_field) {
        m_field->setEnabled(isEnabled());
    }
}

bool MdSelect::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_field) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            showPopup();
            return true;
        case QEvent::FocusIn:
            // Keep MdSelect's focus proxy in sync with the field.
            break;
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// --- internal ----------------------------------------------------------------

void MdSelect::onMenuClosed()
{
    m_popupOpen = false;
    // Return focus to the select so the user can re-open with the keyboard.
    // Skip when the select is already gone (menu outlives a page teardown).
    if (isVisible()) {
        setFocus(Qt::PopupFocusReason);
    }
}

void MdSelect::onMenuItemClicked()
{
    auto *item = qobject_cast<MdMenuItem *>(sender());
    if (!item) {
        return;
    }
    const int index = m_menuItems.indexOf(item);
    if (index < 0) {
        return;
    }
    // Dismiss first, then apply. A mapped Qt::Popup holds the application
    // mouse grab, and the theme fan-out from currentIndexChanged must not run
    // while that grab is live (the avatar menu closes via closeAndAccept before
    // onThemeAction for the same reason). hidePopup() is idempotent when the
    // MdMenu close slot already ran.
    hidePopup();
    setIndexFromUser(index);
}

void MdSelect::setIndexFromUser(int index)
{
    const int previous = m_currentIndex;
    setCurrentIndex(index);
    if (m_currentIndex != previous || m_currentIndex >= 0) {
        emit activated(m_currentIndex);
        emit textActivated(currentText());
    }
}

void MdSelect::rebuildMenu()
{
    m_rebuilding = true;
    m_menu->clear();
    m_menuItems.clear();
    for (const QString &text : m_items) {
        MdMenuItem *item = m_menu->addItem(text);
        connect(item, &MdMenuItem::clicked, this, &MdSelect::onMenuItemClicked);
        m_menuItems.append(item);
    }
    m_rebuilding = false;
    m_menuDirty = false;
}

void MdSelect::applyCurrentText()
{
    m_field->setText(currentText());
}

void MdSelect::onThemeChanged()
{
    // Re-push the current item text so the field never comes back blank after
    // a theme switch, and let the field repaint with the new palette.
    applyCurrentText();
    update();
}

} // namespace md
