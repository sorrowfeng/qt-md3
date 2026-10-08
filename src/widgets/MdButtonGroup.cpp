#include "MdButtonGroup.h"

#include "core/MdMotion.h"
#include "core/MdTheme.h"
#include "styles/MdButtonGroupStyle.h"
#include "widgets/MdButton.h"

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cmath>

namespace md {

namespace {

/// The group's round/square property mapped onto the token export's
/// `container.shape` enum.
///
/// The two enums carry the same two values on purpose — a button group's shape
/// vocabulary is the same round/square pair a button's is — so the property is
/// a `ButtonShape` and this is the one place the two meet.
ButtonGroupShape toGroupShape(ButtonShape shape)
{
    return shape == ButtonShape::Square ? ButtonGroupShape::Square : ButtonGroupShape::Round;
}

/// A readable accessible name for an icon-only item.
///
/// The icon is a Material Symbols ligature name — `arrow_forward`,
/// `format_bold`, `add` — which is not a name anybody wants read aloud. The
/// group separates the words and capitalises the first one, which is as far as
/// a ligature name can be turned into prose without a translation table. A
/// caller who has real words passes them to addItem() instead.
QString accessibleNameForIcon(const QString &icon)
{
    QString text = icon;
    text.replace(QLatin1Char('_'), QLatin1Char(' '));
    text = text.simplified();
    if (!text.isEmpty()) {
        text[0] = text.at(0).toUpper();
    }
    return text;
}

} // namespace

MdButtonGroup::MdButtonGroup(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdButtonGroup::~MdButtonGroup() = default;

void MdButtonGroup::init()
{
    // A button group is an invisible container, so it must not be a tab stop:
    // a focus indicator drawn on it would be a ring around nothing. The items
    // are what takes focus, and the group only routes the arrow keys between
    // them (see eventFilter).
    setFocusPolicy(Qt::NoFocus);

    // The style owns the geometry arithmetic and nothing else; creating it here
    // means the group is never measured by a code path that has not been asked
    // for its opinion. It deliberately installs no paint filter — see
    // MdButtonGroupStyle's header — so this is the whole of its involvement.
    MdButtonGroupStyle::shared();

    m_growthTimer = new QTimer(this);
    m_growthTimer->setInterval(16);
    m_growthTimer->setTimerType(Qt::PreciseTimer);
    connect(m_growthTimer, &QTimer::timeout, this, &MdButtonGroup::onGrowthTick);

    MdStyleBase::connectThemeUpdate(this, &MdButtonGroup::onThemeChanged);
}

// ---------------------------------------------------------------------------
// form
// ---------------------------------------------------------------------------

void MdButtonGroup::setVariant(ButtonGroupVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;

    if (m_variant == ButtonGroupVariant::Connected) {
        // A form change can land mid-press, and the connected form publishes no
        // width multiplier at all: leaving a standard group's growth applied
        // would leave the connected form with a gap nothing explains.
        m_pressedIndex = -1;
        m_growth = 0.0;
        m_growthFrom = 0.0;
        m_growthTo = 0.0;
        if (m_growthTimer != nullptr) {
            m_growthTimer->stop();
        }
    }

    refresh();
    emit variantChanged(m_variant);
}

void MdButtonGroup::setGroupSize(ButtonSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    refresh();
    emit groupSizeChanged(m_size);
}

void MdButtonGroup::setOrientation(ButtonGroupOrientation orientation)
{
    if (m_orientation == orientation) {
        return;
    }
    m_orientation = orientation;
    refresh();
    emit orientationChanged(m_orientation);
}

void MdButtonGroup::setGroupShape(ButtonShape shape)
{
    if (m_shape == shape) {
        return;
    }
    m_shape = shape;
    refresh();
    emit groupShapeChanged(m_shape);
}

void MdButtonGroup::setItemVariant(ButtonVariant variant)
{
    if (m_itemVariant == variant) {
        return;
    }
    m_itemVariant = variant;
    applyItemAppearance();
    update();
    emit itemVariantChanged(m_itemVariant);
}

void MdButtonGroup::setSelectedItemVariant(ButtonVariant variant)
{
    if (m_selectedItemVariant == variant) {
        return;
    }
    m_selectedItemVariant = variant;
    applyItemAppearance();
    update();
    emit selectedItemVariantChanged(m_selectedItemVariant);
}

// ---------------------------------------------------------------------------
// selection
// ---------------------------------------------------------------------------

void MdButtonGroup::setSelectionMode(ButtonGroupSelection selection)
{
    if (m_selection == selection) {
        return;
    }
    m_selection = selection;
    // refresh() reconciles: it makes the items checkable or not, prunes a
    // selection the new mode cannot hold, and fills an empty "required"
    // selection.
    refresh();
    emit selectionModeChanged(m_selection);
}

void MdButtonGroup::setCurrentIndex(int index)
{
    if (index < -1 || index >= m_items.size()) {
        return;
    }
    if (m_selection == ButtonGroupSelection::None || m_selection == ButtonGroupSelection::Count) {
        // With no selection model there is nothing for the index to name; it is
        // pinned to -1 by reconcileSelection().
        return;
    }
    if (index == -1) {
        clearSelection();
        return;
    }

    if (m_selection == ButtonGroupSelection::Multiple) {
        // Multiple is additive by definition, so "make this the current item"
        // cannot mean "and drop the others". It means: make sure it is in the
        // selection, and make it the one currentIndex() names.
        if (!m_selected.contains(index)) {
            applySelection(index, true);
        } else if (m_currentIndex != index) {
            m_currentIndex = index;
            emit currentIndexChanged(m_currentIndex);
        }
    } else {
        selectOnly(index);
    }
    refresh();
}

void MdButtonGroup::setSelected(int index, bool selected)
{
    if (m_selection == ButtonGroupSelection::None || m_selection == ButtonGroupSelection::Count) {
        // No model to write through. Ignoring rather than asserting is
        // deliberate: a caller that flips the mode and the selection in the
        // same breath should not have to order the two calls.
        return;
    }
    if (index < 0 || index >= m_items.size()) {
        return;
    }

    if (!selected) {
        if (m_selection == ButtonGroupSelection::Required) {
            // "Selection required" means exactly one: the last one cannot go.
            return;
        }
        applySelection(index, false);
        refresh();
        return;
    }

    if (m_selection == ButtonGroupSelection::Multiple) {
        applySelection(index, true);
    } else {
        selectOnly(index);
    }
    refresh();
}

void MdButtonGroup::clearSelection()
{
    if (m_selection == ButtonGroupSelection::Required) {
        // The configuration forbids an empty selection, so this is a no-op
        // rather than a state the group cannot represent.
        return;
    }
    if (m_selected.isEmpty() && m_currentIndex == -1) {
        return;
    }
    selectOnly(-1);
    refresh();
}

bool MdButtonGroup::isSelected(int index) const
{
    return m_selected.contains(index);
}

void MdButtonGroup::selectOnly(int index)
{
    const int previousCurrent = m_currentIndex;
    const bool valid = index >= 0 && index < m_items.size();

    for (int other : QList<int>(m_selected)) {
        if (valid && other == index) {
            continue;
        }
        m_selected.removeAll(other);
        if (MdButton *item = itemAt(other)) {
            item->setChecked(false);
        }
        emit itemDeselected(other);
    }

    if (valid && !m_selected.contains(index)) {
        m_selected.append(index);
        std::sort(m_selected.begin(), m_selected.end());
        emit itemSelected(index);
    }

    // Make the buttons agree with the model rather than trusting them. The
    // clicked button has already toggled itself, and in "selection required"
    // that toggle has to be undone — the group is the authority on which item
    // is selected, and this is what makes the two agree without a second path
    // through the click handling.
    for (int i = 0; i < m_items.size(); ++i) {
        MdButton *item = m_items.at(i);
        if (item == nullptr) {
            continue;
        }
        const bool shouldBeChecked = valid && i == index;
        if (item->isChecked() != shouldBeChecked) {
            item->setChecked(shouldBeChecked);
        }
    }

    m_currentIndex = valid ? index : -1;
    if (m_currentIndex != previousCurrent) {
        emit currentIndexChanged(m_currentIndex);
    }
}

void MdButtonGroup::applySelection(int index, bool selected)
{
    MdButton *item = itemAt(index);
    if (item == nullptr) {
        return;
    }

    const bool was = m_selected.contains(index);
    if (was == selected) {
        // Already agreed. The button is still put in step: it is the half of
        // the state a caller can toggle directly, and a group whose model and
        // whose items disagree paints the wrong thing.
        if (item->isChecked() != selected) {
            item->setChecked(selected);
        }
        return;
    }

    const int previousCurrent = m_currentIndex;

    if (selected) {
        m_selected.append(index);
        std::sort(m_selected.begin(), m_selected.end());
        m_currentIndex = index;
    } else {
        m_selected.removeAll(index);
        if (m_currentIndex == index) {
            // Falls back to the most recently selected survivor, which is what
            // currentIndex() documents for the multi-select case.
            m_currentIndex = m_selected.isEmpty() ? -1 : m_selected.last();
        }
    }
    item->setChecked(selected);

    if (selected) {
        emit itemSelected(index);
    } else {
        emit itemDeselected(index);
    }

    if (m_currentIndex != previousCurrent) {
        emit currentIndexChanged(m_currentIndex);
    }
}

void MdButtonGroup::reconcileSelection()
{
    const bool selectable = m_selection != ButtonGroupSelection::None
                            && m_selection != ButtonGroupSelection::Count;

    for (MdButton *item : m_items) {
        if (item != nullptr && item->isCheckable() != selectable) {
            // A group with no selection model is a row of plain actions; a
            // toggle state on them would be a state nothing owns or clears.
            item->setCheckable(selectable);
        }
    }

    if (!selectable) {
        if (!m_selected.isEmpty() || m_currentIndex != -1) {
            selectOnly(-1);
        }
        return;
    }

    const bool exclusive = m_selection == ButtonGroupSelection::Single
                           || m_selection == ButtonGroupSelection::Required;
    if (!exclusive) {
        return;
    }

    if (m_selected.isEmpty()) {
        if (m_selection == ButtonGroupSelection::Required && !m_items.isEmpty()) {
            // Exactly one, so an empty selection is a state this configuration
            // forbids. The first item takes it rather than the group sitting in
            // a state its own mode says cannot exist.
            selectOnly(0);
        }
        return;
    }

    if (m_selected.size() > 1) {
        // The current index is the most recent selection when it survived, so
        // it is the one a mid-interaction mode flip should keep.
        const int keep = m_selected.contains(m_currentIndex) ? m_currentIndex : m_selected.last();
        selectOnly(keep);
    }
}

// ---------------------------------------------------------------------------
// items
// ---------------------------------------------------------------------------

MdButton *MdButtonGroup::itemAt(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return nullptr;
    }
    return m_items.at(index);
}

int MdButtonGroup::indexOf(const MdButton *item) const
{
    if (item == nullptr) {
        return -1;
    }
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i) == item) {
            return i;
        }
    }
    return -1;
}

MdButton *MdButtonGroup::addItem(const QString &text,
                                 const QString &icon,
                                 const QString &accessibleName)
{
    return insertItem(m_items.size(), text, icon, accessibleName);
}

MdButton *MdButtonGroup::insertItem(int index,
                                    const QString &text,
                                    const QString &icon,
                                    const QString &accessibleName)
{
    index = qBound(0, index, m_items.size());

    auto *item = new MdButton(text, this);
    if (!icon.isEmpty()) {
        item->setLeadingIcon(icon);
    }

    // A button group exists mostly to hold icon buttons, and an icon-only
    // control with no accessible name is unusable with a screen reader — the
    // ligature name is not something to leave it with.
    if (!accessibleName.isEmpty()) {
        item->setAccessibleName(accessibleName);
    } else if (text.isEmpty() && !icon.isEmpty()) {
        item->setAccessibleName(accessibleNameForIcon(icon));
    }

    m_items.insert(index, item);
    connectItem(item);

    // Every index at or after the insertion point shifted by one.
    for (int i = 0; i < m_selected.size(); ++i) {
        if (m_selected.at(i) >= index) {
            m_selected[i] += 1;
        }
    }
    if (m_currentIndex >= index) {
        m_currentIndex += 1;
    }
    if (m_pressedIndex >= index) {
        m_pressedIndex += 1;
    }

    refresh();
    return item;
}

void MdButtonGroup::addButton(MdButton *button)
{
    if (button == nullptr || m_items.contains(button)) {
        return;
    }

    if (button->parentWidget() != this) {
        button->setParent(this);
        // setParent() hides the widget unconditionally, so the item would never
        // appear if this were left out. The group takes parenting and the item
        // shows with the group; a caller that wants one item hidden hides it
        // after adding it.
        button->show();
    }

    m_items.append(button);
    connectItem(button);
    refresh();
}

void MdButtonGroup::removeItem(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return;
    }

    MdButton *item = m_items.takeAt(index);
    if (item != nullptr) {
        item->removeEventFilter(this);
        item->disconnect(this);
        item->hide();
        // Unparented before deleteLater() so it leaves the group immediately
        // rather than lingering as a child until the event loop runs.
        item->setParent(nullptr);
        // deleteLater() rather than delete: this is reachable from
        // `itemClicked`, which is the item's own signal being emitted, and
        // destroying a sender mid-emit is undefined.
        item->deleteLater();
    }

    m_selected.removeAll(index);
    for (int i = 0; i < m_selected.size(); ++i) {
        if (m_selected.at(i) > index) {
            m_selected[i] -= 1;
        }
    }

    if (m_currentIndex == index) {
        m_currentIndex = m_selected.isEmpty() ? -1 : m_selected.last();
        emit currentIndexChanged(m_currentIndex);
    } else if (m_currentIndex > index) {
        m_currentIndex -= 1;
    }

    if (m_pressedIndex == index) {
        m_pressedIndex = -1;
    } else if (m_pressedIndex > index) {
        m_pressedIndex -= 1;
    }

    refresh();
}

void MdButtonGroup::clear()
{
    if (m_items.isEmpty()) {
        return;
    }

    const QVector<MdButton *> doomed = m_items;
    const bool currentChanged = m_currentIndex != -1;

    m_items.clear();
    m_selected.clear();
    m_currentIndex = -1;
    m_pressedIndex = -1;
    m_growth = 0.0;
    m_growthFrom = 0.0;
    m_growthTo = 0.0;
    if (m_growthTimer != nullptr) {
        m_growthTimer->stop();
    }

    for (MdButton *item : doomed) {
        if (item == nullptr) {
            continue;
        }
        item->removeEventFilter(this);
        item->disconnect(this);
        item->hide();
        item->setParent(nullptr);
        item->deleteLater();
    }

    if (currentChanged) {
        emit currentIndexChanged(-1);
    }
    refresh();
}

void MdButtonGroup::connectItem(MdButton *item)
{
    // clicked() rather than toggled(): a click is a user action with an index,
    // and toggled() also fires when the group itself sets the state, which
    // would make every programmatic selection look like a user edit.
    connect(item, &QAbstractButton::clicked, this, [this, item] { handleItemClicked(item); });
    connect(item, &QAbstractButton::pressed, this, [this, item] { handleItemPressed(item); });
    connect(item, &QAbstractButton::released, this, [this, item] { handleItemReleased(item); });

    // Arrow-key navigation is a property of the group, not of a button, so the
    // group watches the items rather than each item knowing about its siblings.
    item->installEventFilter(this);
}

// ---------------------------------------------------------------------------
// interaction
// ---------------------------------------------------------------------------

void MdButtonGroup::handleItemClicked(MdButton *item)
{
    const int index = indexOf(item);
    if (index < 0) {
        return;
    }

    emit itemClicked(index);

    if (m_selection == ButtonGroupSelection::None || m_selection == ButtonGroupSelection::Count) {
        return;
    }

    // QAbstractButton toggled the button *before* emitting clicked(), and that
    // toggle is the user's intent — so the group reads it rather than deriving
    // it a second time from the previous state.
    const bool wanted = item->isChecked();

    switch (m_selection) {
    case ButtonGroupSelection::Single:
        // Clicking the selected item clears the selection. That is precisely
        // what separates "single-select" from "selection-required" in the
        // spec's three configurations.
        if (wanted) {
            selectOnly(index);
        } else {
            applySelection(index, false);
        }
        break;
    case ButtonGroupSelection::Multiple:
        applySelection(index, wanted);
        break;
    case ButtonGroupSelection::Required:
        // Exactly one, so clicking the selected item is a no-op — which
        // selectOnly() enforces by putting the button's own toggle back.
        selectOnly(index);
        break;
    case ButtonGroupSelection::None:
    case ButtonGroupSelection::Count:
        return;
    }

    refresh();
}

void MdButtonGroup::handleItemPressed(MdButton *item)
{
    const int index = indexOf(item);
    if (index < 0) {
        return;
    }
    m_pressedIndex = index;
    animateGrowthTo(tokens().pressedWidthMultiplier);
}

void MdButtonGroup::handleItemReleased(MdButton *item)
{
    const int index = indexOf(item);
    if (index < 0) {
        return;
    }
    if (m_pressedIndex == index) {
        m_pressedIndex = -1;
    }
    animateGrowthTo(0.0);
}

// ---------------------------------------------------------------------------
// geometry
// ---------------------------------------------------------------------------

QVector<QRectF> MdButtonGroup::itemRects() const
{
    return MdButtonGroupStyle::layoutFor(*this, tokens(), growthVector()).items;
}

QSize MdButtonGroup::sizeHint() const
{
    const QSizeF preferred = MdButtonGroupStyle::layoutFor(*this, tokens()).preferredSize;
    return QSize(int(std::ceil(preferred.width())), int(std::ceil(preferred.height())));
}

QSize MdButtonGroup::minimumSizeHint() const
{
    // Same as sizeHint(), for the same reason MdButton's is: the layout reserves
    // room for the press growth and for the outward focus indicator, and a group
    // squeezed below it would clip both. md.comp.button-group's `between-space`
    // is already sized so the target area clears 48 dp, so the content size is
    // the floor rather than a preference.
    return sizeHint();
}

QVector<qreal> MdButtonGroup::growthVector() const
{
    QVector<qreal> growth(m_items.size(), 0.0);
    if (m_pressedIndex >= 0 && m_pressedIndex < growth.size() && m_growth > 0.0) {
        growth[m_pressedIndex] = m_growth;
    }
    return growth;
}

void MdButtonGroup::layoutItems()
{
    const MdButtonGroupStyle::Layout layout =
        MdButtonGroupStyle::layoutFor(*this, tokens(), growthVector());

    for (int i = 0; i < m_items.size(); ++i) {
        MdButton *item = m_items.at(i);
        if (item == nullptr || i >= layout.items.size()) {
            continue;
        }
        item->setGeometry(layout.items.at(i).toAlignedRect());
    }
}

// ---------------------------------------------------------------------------
// appearance
// ---------------------------------------------------------------------------

const MdButtonGroupTokens &MdButtonGroup::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdButtonGroupTokens::resolve(m_variant, m_size, toGroupShape(m_shape),
                                               &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdButtonGroup::invalidateTokens()
{
    m_tokensDirty = true;
}

void MdButtonGroup::applyItemAppearance()
{
    const MdButtonGroupTokens &resolved = tokens();
    const int count = m_items.size();
    const bool rightToLeft = m_orientation == ButtonGroupOrientation::Horizontal
                             && MdTheme::instance().isRightToLeft();

    for (int i = 0; i < count; ++i) {
        MdButton *item = m_items.at(i);
        if (item == nullptr) {
            continue;
        }

        const bool selected = isSelected(i);

        // The group sets the item's size and colour style, then the shape. The
        // shape swap is the whole of what a *standard* group does to an item on
        // selection: "when a toggle button is selected in a standard button
        // group, its shape should change between square and round."
        item->setButtonSize(m_size);
        item->setVariant(selected ? m_selectedItemVariant : m_itemVariant);
        item->setButtonShape(MdButtonGroupStyle::itemShape(*this, selected));

        // The connected form publishes a corner per side instead. Both ends of
        // the press are handed over — the resting corner and the pressed one —
        // so MdButton's own press morph animates between them and the group
        // needs no second animation of its own for the shape.
        //
        // For the standard form itemRadii() returns two empty lists, which is
        // how "no override, keep your own shape tokens" is spelled. It also
        // means MdButton::setCornerRadii() drops any override a previous mode
        // left behind.
        const QSizeF container =
            MdButtonGroupStyle::naturalContainerSize(*item, resolved, m_orientation);
        item->setCornerRadii(
            MdButtonGroupStyle::itemRadii(resolved, i, count, m_orientation, selected,
                                          /*pressed=*/false, container, rightToLeft),
            MdButtonGroupStyle::itemRadii(resolved, i, count, m_orientation, selected,
                                          /*pressed=*/true, container, rightToLeft));
    }
}

void MdButtonGroup::refresh()
{
    invalidateTokens();
    reconcileSelection();
    applyItemAppearance();
    updateGeometry();
    layoutItems();
    update();
}

// ---------------------------------------------------------------------------
// state machines
// ---------------------------------------------------------------------------

void MdButtonGroup::onThemeChanged()
{
    // The group's own token set carries no colours, but the items' do, and a
    // direction flip moves which end takes the outer corner — so the whole
    // thing is rebuilt rather than merely repainted.
    refresh();
}

void MdButtonGroup::animateGrowthTo(qreal multiplier)
{
    const MdButtonGroupTokens &resolved = tokens();
    if (resolved.pressedWidthMultiplier <= 0.0 || !MdSpring(resolved.springStiffness,
                                                           resolved.springDampingRatio)
                                                       .isValid()) {
        // The connected form publishes no width multiplier and no spring,
        // because a connected item's press changes only its own shape — and the
        // shape is animated by the button itself. Nothing to do here, but any
        // growth a previous form left applied still has to come off.
        if (m_growthTimer != nullptr) {
            m_growthTimer->stop();
        }
        if (!qFuzzyIsNull(m_growth)) {
            m_growth = 0.0;
            m_growthFrom = 0.0;
            m_growthTo = 0.0;
            layoutItems();
        }
        return;
    }

    if (m_growthTimer->isActive() && qFuzzyCompare(m_growthTo, multiplier)) {
        return;
    }
    m_growthFrom = m_growth;
    m_growthTo = multiplier;

    if (qFuzzyCompare(m_growthFrom, m_growthTo)) {
        m_growth = multiplier;
        m_growthTimer->stop();
        layoutItems();
        return;
    }

    m_growthClock.restart();
    m_growthTimer->start();
}

void MdButtonGroup::onGrowthTick()
{
    const MdButtonGroupTokens &resolved = tokens();
    const MdSpring spring(resolved.springStiffness, resolved.springDampingRatio);

    const qreal seconds = qreal(m_growthClock.elapsed()) / 1000.0;
    const qreal progress = spring.valueAt(seconds);
    m_growth = m_growthFrom + (m_growthTo - m_growthFrom) * progress;

    // A damped spring approaches its target asymptotically, so the elapsed-time
    // bound is what guarantees the timer stops even when `progress` never quite
    // reaches 1.0 — the same guard MdButton's press morph uses.
    const qreal settleMs = spring.settlingDurationMs();
    if (progress >= 1.0 || qreal(m_growthClock.elapsed()) >= settleMs) {
        m_growth = m_growthTo;
        m_growthTimer->stop();
    }
    layoutItems();
}

// ---------------------------------------------------------------------------
// events
// ---------------------------------------------------------------------------

int MdButtonGroup::neighbourIndex(int index, int delta) const
{
    const int count = m_items.size();
    if (count <= 0 || index < 0 || index >= count || delta == 0) {
        return -1;
    }

    int target = index + delta;
    const bool wrap = m_selection == ButtonGroupSelection::Single
                      || m_selection == ButtonGroupSelection::Required;
    if (wrap) {
        // The two exclusive modes are radiogroups, and a radiogroup is a cycle:
        // the arrow keys that walk it do not dead-end at the last item.
        target = ((target % count) + count) % count;
    } else if (target < 0 || target >= count) {
        // A tool bar or a multi-select group is not a cycle. The key is still
        // the group's to consume — see eventFilter — there is simply nowhere
        // for it to move.
        return -1;
    }
    return target;
}

bool MdButtonGroup::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        // The current item is the one that holds the focus, whichever way the
        // focus got there — Tab, setFocus(), or an arrow key. Naming it here
        // deliberately does not touch the selection: focus and selection are
        // separate things. And in a mode with no selection model at all the
        // index has nothing to name, so it stays pinned at -1 — the same rule
        // setCurrentIndex() follows.
        if (m_selection != ButtonGroupSelection::None
            && m_selection != ButtonGroupSelection::Count) {
            auto *focused = qobject_cast<MdButton *>(watched);
            const int index = focused == nullptr ? -1 : indexOf(focused);
            if (index >= 0 && m_currentIndex != index) {
                m_currentIndex = index;
                emit currentIndexChanged(m_currentIndex);
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() != QEvent::KeyPress) {
        return QWidget::eventFilter(watched, event);
    }

    auto *item = qobject_cast<MdButton *>(watched);
    if (item == nullptr) {
        return QWidget::eventFilter(watched, event);
    }

    const int index = indexOf(item);
    if (index < 0) {
        return QWidget::eventFilter(watched, event);
    }

    auto *key = static_cast<QKeyEvent *>(event);
    const bool horizontal = m_orientation == ButtonGroupOrientation::Horizontal;
    const bool rightToLeft = horizontal && MdTheme::instance().isRightToLeft();

    // Inline start is the left when the layout runs left to right and the right
    // when it does not, so the two horizontal arrows swap meaning under RTL
    // rather than the group moving the wrong way.
    const int inlineStart = rightToLeft ? 1 : -1;
    const int inlineEnd = -inlineStart;

    int target = -1;
    switch (key->key()) {
    case Qt::Key_Left:
    case Qt::Key_Right:
        if (!horizontal) {
            return true; // see the note below — cross-axis keys stay with the group
        }
        target = neighbourIndex(index,
                                key->key() == Qt::Key_Left ? inlineStart : inlineEnd);
        break;
    case Qt::Key_Up:
    case Qt::Key_Down:
        if (horizontal) {
            return true;
        }
        target = neighbourIndex(index, key->key() == Qt::Key_Up ? -1 : 1);
        break;
    case Qt::Key_Home:
        target = m_items.isEmpty() ? -1 : 0;
        break;
    case Qt::Key_End:
        target = m_items.size() - 1;
        break;
    default:
        return QWidget::eventFilter(watched, event);
    }

    // The group's own axis keys belong to the group even when there is nowhere
    // left to go. Letting one fall through at the last item would hand the
    // group's navigation to whatever encloses it, and the outcome would then
    // depend on the surrounding widget rather than on the group.
    //
    // The same reasoning covers the *cross*-axis arrows: they are consumed as
    // no-ops. QAbstractButton::keyPressEvent would otherwise treat an unhandled
    // arrow as "next/previous in tab order" via focusNextPrevChild(), which
    // silently wraps focus to the first item instead of leaving the group the
    // way the caller's arrow key asked it to.
    if (MdButton *next = itemAt(target)) {
        // Selection follows focus in the two exclusive modes, which is the
        // radiogroup behaviour those modes describe. It deliberately does not in
        // the other two: a tool bar's items are separate actions, and a
        // multi-select group's items are independent toggles, so walking the row
        // must not rewrite what is chosen.
        if (m_selection == ButtonGroupSelection::Single
            || m_selection == ButtonGroupSelection::Required) {
            setCurrentIndex(target);
        }
        next->setFocus(Qt::OtherFocusReason);
    }
    return true;
}

void MdButtonGroup::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutItems();
}

void MdButtonGroup::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    switch (event->type()) {
    case QEvent::EnabledChange:
        // Qt does not push a disabled state down to children, and a group whose
        // items stayed live while the group said it was disabled would be a
        // group that lies. Propagating is what QGroupBox does and what a caller
        // means by `group->setEnabled(false)`.
        for (MdButton *item : m_items) {
            if (item != nullptr) {
                item->setEnabled(isEnabled());
            }
        }
        update();
        break;
    case QEvent::FontChange:
    case QEvent::StyleChange:
        // Both change the label metrics, and therefore every item's width.
        refresh();
        break;
    case QEvent::LayoutDirectionChange:
    case QEvent::ApplicationLayoutDirectionChange:
        // Moves which end takes the outer corner and which arrow key means
        // "next", so the layout is rebuilt rather than merely repainted.
        refresh();
        break;
    default:
        break;
    }
}

} // namespace md
