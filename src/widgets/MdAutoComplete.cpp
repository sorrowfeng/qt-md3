#include "MdAutoComplete.h"

#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdMenu.h"
#include "widgets/MdTextField.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>

namespace md {

MdAutoComplete::MdAutoComplete(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdAutoComplete::~MdAutoComplete()
{
    // Children are destroyed in ~QWidget, which runs *before* ~QObject
    // auto-disconnects. A child emitting during its own destruction would
    // then invoke a slot on a half-destroyed MdAutoComplete. Cut the wires
    // first (the MdInputNumber pattern).
    if (m_field) {
        disconnect(m_field, nullptr, this, nullptr);
    }
    if (m_menu) {
        disconnect(m_menu, nullptr, this, nullptr);
    }
    for (MdMenuItem *item : m_menuItems) {
        if (item) {
            disconnect(item, nullptr, this, nullptr);
        }
    }
}

void MdAutoComplete::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("autocomplete"));

    m_field = new MdTextField(this);
    m_field->setVariant(MdTextFieldVariant::Outlined);
    connect(m_field, &QLineEdit::textChanged, this, &MdAutoComplete::onFieldTextChanged);

    // The menu is a Qt::Popup top-level; it is not parented into the layout
    // (the same construction MdSelect uses).
    m_menu = new MdMenu(this->window());
    connect(m_menu, &MdMenu::closed, this, &MdAutoComplete::onMenuClosed);

    m_field->installEventFilter(this);

    MdStyleBase::connectThemeUpdate(this, &MdAutoComplete::onThemeChanged);
}

QString MdAutoComplete::text() const
{
    return m_field ? m_field->text() : QString();
}

void MdAutoComplete::setText(const QString &text)
{
    if (!m_field || m_field->text() == text) {
        return;
    }
    m_field->setText(text);
    // onFieldTextChanged will fire and re-filter.
}

QString MdAutoComplete::labelText() const
{
    return m_field ? m_field->labelText() : QString();
}

void MdAutoComplete::setLabelText(const QString &label)
{
    if (m_field) {
        m_field->setLabelText(label);
    }
    emit labelTextChanged(label);
}

QString MdAutoComplete::placeholderText() const
{
    return m_field ? m_field->placeholderText() : QString();
}

void MdAutoComplete::setPlaceholderText(const QString &placeholder)
{
    if (m_field) {
        m_field->setPlaceholderText(placeholder);
    }
    emit placeholderTextChanged(placeholder);
}

void MdAutoComplete::setFilter(MdAutoCompleteFilter filter)
{
    if (filter >= MdAutoCompleteFilter::Count) {
        return;
    }
    if (m_filter == filter) {
        return;
    }
    m_filter = filter;
    recomputeFiltered();
    rebuildMenu();
    emit filterChanged(m_filter);
}

void MdAutoComplete::setMaxVisible(int maxVisible)
{
    const int clamped = qMax(0, maxVisible);
    if (m_maxVisible == clamped) {
        return;
    }
    m_maxVisible = clamped;
    rebuildMenu();
    emit maxVisibleChanged(m_maxVisible);
}

void MdAutoComplete::setItems(const QStringList &items)
{
    m_items = items;
    recomputeFiltered();
    rebuildMenu();
}

void MdAutoComplete::addItem(const QString &text)
{
    m_items.append(text);
    recomputeFiltered();
    rebuildMenu();
}

void MdAutoComplete::clearItems()
{
    m_items.clear();
    recomputeFiltered();
    rebuildMenu();
}

void MdAutoComplete::setFilteredItems(const QStringList &items)
{
    m_filtered = items;
    if (m_maxVisible > 0 && m_filtered.size() > m_maxVisible) {
        m_filtered = m_filtered.mid(0, m_maxVisible);
    }
    m_highlight = -1;
    rebuildMenu();
}

void MdAutoComplete::showPopup()
{
    if (m_filtered.isEmpty()) {
        return;
    }
    rebuildMenu();
    if (m_menuItems.isEmpty()) {
        return;
    }
    m_popupOpen = true;
    m_menu->popup(QRect(mapToGlobal(QPoint(0, 0)), size()));
    emit popupVisibleChanged(true);
}

void MdAutoComplete::hidePopup()
{
    if (!m_popupOpen) {
        return;
    }
    m_popupOpen = false;
    if (m_menu) {
        m_menu->hide();
    }
    emit popupVisibleChanged(false);
}

void MdAutoComplete::recomputeFiltered()
{
    const QString query = text();
    m_filtered.clear();
    m_highlight = -1;

    if (m_filter == MdAutoCompleteFilter::Custom) {
        emit filterRequested(query, m_items);
        return;
    }

    for (const QString &candidate : m_items) {
        if (query.isEmpty()) {
            m_filtered.append(candidate);
            continue;
        }
        if (m_filter == MdAutoCompleteFilter::StartsWith) {
            if (candidate.startsWith(query, Qt::CaseInsensitive)) {
                m_filtered.append(candidate);
            }
        } else {
            if (candidate.contains(query, Qt::CaseInsensitive)) {
                m_filtered.append(candidate);
            }
        }
    }

    if (m_maxVisible > 0 && m_filtered.size() > m_maxVisible) {
        m_filtered = m_filtered.mid(0, m_maxVisible);
    }
}

void MdAutoComplete::rebuildMenu()
{
    if (!m_menu) {
        return;
    }
    m_rebuilding = true;
    m_menu->clear();
    m_menuItems.clear();
    for (const QString &text : m_filtered) {
        MdMenuItem *item = m_menu->addItem(text);
        connect(item, &MdMenuItem::clicked, this, &MdAutoComplete::onMenuItemClicked);
        m_menuItems.append(item);
    }
    m_rebuilding = false;
}

void MdAutoComplete::commit(const QString &text)
{
    if (m_field && m_field->text() != text) {
        m_field->setText(text);
    }
    hidePopup();
    emit activated(text);
    emit textChanged(text);
}

void MdAutoComplete::moveHighlight(int delta)
{
    if (m_menuItems.isEmpty()) {
        return;
    }
    int next = m_highlight + delta;
    if (next < 0) {
        next = m_menuItems.size() - 1;
    } else if (next >= m_menuItems.size()) {
        next = 0;
    }
    m_highlight = next;
    for (int i = 0; i < m_menuItems.size(); ++i) {
        m_menuItems.at(i)->setSelected(i == m_highlight);
    }
}

void MdAutoComplete::onFieldTextChanged(const QString &text)
{
    recomputeFiltered();
    rebuildMenu();
    if (!m_popupOpen && !text.isEmpty() && !m_filtered.isEmpty()) {
        showPopup();
    } else if (m_popupOpen && m_filtered.isEmpty()) {
        hidePopup();
    } else if (m_popupOpen) {
        // Refresh the open popup in place.
        m_menu->popup(QRect(mapToGlobal(QPoint(0, 0)), size()));
    }
    emit textChanged(text);
}

void MdAutoComplete::onMenuClosed()
{
    if (!m_popupOpen) {
        return;
    }
    m_popupOpen = false;
    m_highlight = -1;
    emit popupVisibleChanged(false);
}

void MdAutoComplete::onMenuItemClicked()
{
    auto *item = qobject_cast<MdMenuItem *>(sender());
    if (!item) {
        return;
    }
    commit(item->text());
}

void MdAutoComplete::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_field) {
        m_field->setGeometry(rect());
    }
}

void MdAutoComplete::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Down:
        if (m_popupOpen) {
            moveHighlight(1);
        } else {
            showPopup();
        }
        return;
    case Qt::Key_Up:
        if (m_popupOpen) {
            moveHighlight(-1);
        }
        return;
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (m_popupOpen && m_highlight >= 0 && m_highlight < m_filtered.size()) {
            commit(m_filtered.at(m_highlight));
            return;
        }
        break;
    case Qt::Key_Escape:
        if (m_popupOpen) {
            hidePopup();
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdAutoComplete::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    if (m_field) {
        m_field->setFocus(event->reason());
    }
}

void MdAutoComplete::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange) {
        if (m_field) {
            m_field->setLayoutDirection(layoutDirection());
        }
    }
}

bool MdAutoComplete::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_field && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up
            || keyEvent->key() == Qt::Key_Escape) {
            keyPressEvent(keyEvent);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void MdAutoComplete::onThemeChanged()
{
    update();
}

} // namespace md
