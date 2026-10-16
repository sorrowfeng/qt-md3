#include "MdMentions.h"

#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdMenu.h"
#include "widgets/MdTextField.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>

namespace md {

MdMentions::MdMentions(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdMentions::~MdMentions()
{
    // Children are destroyed in ~QWidget, which runs *before* ~QObject
    // auto-disconnects. Cut the wires first (the MdAutoComplete pattern).
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

void MdMentions::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("mentions"));

    m_field = new MdTextField(this);
    m_field->setVariant(MdTextFieldVariant::Outlined);
    connect(m_field, &QLineEdit::textChanged,
            this, &MdMentions::onFieldTextChanged);

    // Qt::Popup top-level, same construction MdSelect / MdAutoComplete use.
    m_menu = new MdMenu(this->window());
    connect(m_menu, &MdMenu::closed, this, [this] {
        if (m_popupVisible) {
            m_popupVisible = false;
            m_highlight = 0;
            emit popupVisibleChanged(false);
        }
    });

    m_field->installEventFilter(this);
    MdStyleBase::connectThemeUpdate(this, &MdMentions::onThemeChanged);
}

QString MdMentions::text() const
{
    return m_field ? m_field->text() : QString();
}

void MdMentions::setText(const QString &text)
{
    if (!m_field || m_field->text() == text) {
        return;
    }
    m_field->setText(text);
}

QString MdMentions::labelText() const
{
    return m_field ? m_field->labelText() : QString();
}

void MdMentions::setLabelText(const QString &label)
{
    if (m_field) {
        m_field->setLabelText(label);
    }
    emit labelTextChanged(label);
}

QString MdMentions::placeholderText() const
{
    return m_field ? m_field->placeholderText() : QString();
}

void MdMentions::setPlaceholderText(const QString &placeholder)
{
    if (m_field) {
        m_field->setPlaceholderText(placeholder);
    }
    emit placeholderTextChanged(placeholder);
}

void MdMentions::setTrigger(QChar trigger)
{
    if (m_trigger == trigger) {
        return;
    }
    m_trigger = trigger;
    emit triggerChanged(m_trigger);
}

void MdMentions::setItems(const QStringList &items)
{
    if (m_items == items) {
        return;
    }
    m_items = items;
    emit itemsChanged(m_items);
}

void MdMentions::setPopupVisible(bool visible)
{
    if (visible) {
        showPopup();
    } else {
        hidePopup();
    }
}

void MdMentions::setMaxVisible(int count)
{
    const int clamped = qMax(1, count);
    if (m_maxVisible == clamped) {
        return;
    }
    m_maxVisible = clamped;
    emit maxVisibleChanged(m_maxVisible);
}

QString MdMentions::currentQuery() const
{
    const QString text = this->text();
    const int cursor = m_field ? m_field->cursorPosition() : text.size();
    const int start = text.lastIndexOf(m_trigger, cursor - 1);
    if (start < 0) {
        return QString();
    }
    // The trigger must begin a token: start of text or whitespace before it.
    if (start > 0) {
        const QChar before = text.at(start - 1);
        if (!before.isSpace()) {
            return QString();
        }
    }
    const QString mid = text.mid(start + 1, cursor - start - 1);
    if (mid.contains(QLatin1Char(' ')) || mid.contains(QLatin1Char('\n'))) {
        return QString();
    }
    return mid;
}

QStringList MdMentions::filteredItems() const
{
    const QString query = currentQuery();
    QStringList out;
    for (const QString &candidate : m_items) {
        if (query.isEmpty() || candidate.contains(query, Qt::CaseInsensitive)) {
            out.append(candidate);
        }
        if (m_maxVisible > 0 && out.size() >= m_maxVisible) {
            break;
        }
    }
    return out;
}

QSize MdMentions::sizeHint() const
{
    return m_field ? m_field->sizeHint() : QSize(240, 56);
}

QSize MdMentions::minimumSizeHint() const
{
    return m_field ? m_field->minimumSizeHint() : QSize(120, 56);
}

void MdMentions::commit(const QString &name)
{
    if (!m_field) {
        return;
    }
    QString text = m_field->text();
    const int cursor = m_field->cursorPosition();
    const int start = text.lastIndexOf(m_trigger, cursor - 1);
    if (start >= 0) {
        text.replace(start, cursor - start, m_trigger + name + QLatin1Char(' '));
        m_field->setText(text);
        m_field->setCursorPosition(start + 1 + name.size() + 1);
    }
    hidePopup();
    emit mentioned(name);
}

void MdMentions::hidePopup()
{
    if (!m_popupVisible) {
        return;
    }
    m_popupVisible = false;
    if (m_menu) {
        m_menu->hide();
    }
    emit popupVisibleChanged(false);
}

void MdMentions::showPopup()
{
    m_filtered = filteredItems();
    if (m_filtered.isEmpty()) {
        hidePopup();
        return;
    }
    rebuildPopup();
    if (m_menuItems.isEmpty()) {
        return;
    }
    m_popupVisible = true;
    m_highlight = 0;
    for (int i = 0; i < m_menuItems.size(); ++i) {
        m_menuItems.at(i)->setSelected(i == 0);
    }
    anchorPopup();
    emit popupVisibleChanged(true);
}

void MdMentions::anchorPopup()
{
    if (!m_menu) {
        return;
    }
    m_menu->popup(QRect(mapToGlobal(QPoint(0, 0)), size()));
}

void MdMentions::rebuildPopup()
{
    if (!m_menu) {
        return;
    }
    m_menu->clear();
    for (MdMenuItem *item : m_menuItems) {
        disconnect(item, nullptr, this, nullptr);
    }
    m_menuItems.clear();
    for (const QString &name : m_filtered) {
        MdMenuItem *item = m_menu->addItem(name);
        connect(item, &MdMenuItem::clicked, this, &MdMentions::onItemTriggered);
        m_menuItems.append(item);
    }
}

void MdMentions::moveHighlight(int delta)
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

void MdMentions::onFieldTextChanged(const QString &text)
{
    emit textChanged(text);
    const QString query = currentQuery();
    if (query.isNull()) {
        // Not inside a trigger token.
        hidePopup();
        return;
    }
    m_filtered = filteredItems();
    if (m_filtered.isEmpty()) {
        hidePopup();
        return;
    }
    if (!m_popupVisible) {
        showPopup();
    } else {
        rebuildPopup();
        m_highlight = 0;
        for (int i = 0; i < m_menuItems.size(); ++i) {
            m_menuItems.at(i)->setSelected(i == 0);
        }
        anchorPopup();
    }
}

void MdMentions::onItemTriggered()
{
    auto *item = qobject_cast<MdMenuItem *>(sender());
    if (!item) {
        return;
    }
    commit(item->text());
}

void MdMentions::onThemeChanged()
{
    update();
}

void MdMentions::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_field) {
        m_field->setGeometry(rect());
    }
}

void MdMentions::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange && m_field) {
        m_field->setLayoutDirection(layoutDirection());
    }
}

bool MdMentions::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_field && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Down:
            if (m_popupVisible) {
                moveHighlight(1);
            } else {
                onFieldTextChanged(text());
            }
            return true;
        case Qt::Key_Up:
            if (m_popupVisible) {
                moveHighlight(-1);
            }
            return true;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (m_popupVisible && m_highlight >= 0 && m_highlight < m_filtered.size()) {
                commit(m_filtered.at(m_highlight));
                return true;
            }
            break;
        case Qt::Key_Escape:
            if (m_popupVisible) {
                hidePopup();
                return true;
            }
            break;
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace md
