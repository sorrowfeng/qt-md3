#include "MdMessageBox.h"

#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdButton.h"

#include <QtCore/QEventLoop>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QShowEvent>

namespace md {

namespace {

/// Dialog shadow margin — MdDialog grows the widget rect by this much so the
/// level-3 shadow has headroom. Matched so a message box and an MdDialog
/// sit on the same grid.
constexpr qreal kShadowMargin = 12.0;

/// The default Material Symbols glyph per variant.
QString defaultIconName(MdMessageBoxVariant variant)
{
    switch (variant) {
    case MdMessageBoxVariant::Information:
    case MdMessageBoxVariant::Question:
        return QStringLiteral("info");
    case MdMessageBoxVariant::Warning:
        return QStringLiteral("warning");
    case MdMessageBoxVariant::Error:
        return QStringLiteral("error");
    case MdMessageBoxVariant::None:
    case MdMessageBoxVariant::Count:
        break;
    }
    return QString();
}

/// Information / Question -> primary, Warning -> tertiary, Error -> error.
ColorRole semanticRole(MdMessageBoxVariant variant)
{
    switch (variant) {
    case MdMessageBoxVariant::Warning:
        return ColorRole::Tertiary;
    case MdMessageBoxVariant::Error:
        return ColorRole::Error;
    case MdMessageBoxVariant::Information:
    case MdMessageBoxVariant::Question:
        return ColorRole::Primary;
    case MdMessageBoxVariant::None:
    case MdMessageBoxVariant::Count:
        break;
    }
    return ColorRole::Primary;
}

/// The dialog's icon slot is any widget; this one paints a tinted glyph.
class MessageBoxIcon : public QWidget
{
public:
    explicit MessageBoxIcon(const QString &name, const QColor &color, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_name(name)
        , m_color(color)
    {
        setFixedSize(24, 24);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    void setGlyph(const QString &name, const QColor &color)
    {
        m_name = name;
        m_color = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        if (m_name.isEmpty()) {
            return;
        }
        QPainter painter(this);
        MdIcon::paint(&painter, QRectF(rect()), m_name, m_color);
    }

private:
    QString m_name;
    QColor m_color;
};

bool roleIsAffirmative(MdMessageBoxButtonRole role)
{
    switch (role) {
    case MdMessageBoxButtonRole::AcceptRole:
    case MdMessageBoxButtonRole::YesRole:
    case MdMessageBoxButtonRole::RetryRole:
        return true;
    case MdMessageBoxButtonRole::Invalid:
    case MdMessageBoxButtonRole::RejectRole:
    case MdMessageBoxButtonRole::NoRole:
    case MdMessageBoxButtonRole::CloseRole:
    case MdMessageBoxButtonRole::ActionRole:
    case MdMessageBoxButtonRole::Count:
        break;
    }
    return false;
}

int roleResult(MdMessageBoxButtonRole role)
{
    switch (role) {
    case MdMessageBoxButtonRole::AcceptRole:
        return int(MdMessageBoxStandardButton::Ok);
    case MdMessageBoxButtonRole::RejectRole:
    case MdMessageBoxButtonRole::CloseRole:
        return int(MdMessageBoxStandardButton::Cancel);
    case MdMessageBoxButtonRole::YesRole:
        return int(MdMessageBoxStandardButton::Yes);
    case MdMessageBoxButtonRole::NoRole:
        return int(MdMessageBoxStandardButton::No);
    case MdMessageBoxButtonRole::RetryRole:
        return int(MdMessageBoxStandardButton::Retry);
    case MdMessageBoxButtonRole::Invalid:
    case MdMessageBoxButtonRole::ActionRole:
    case MdMessageBoxButtonRole::Count:
        break;
    }
    return 0;
}

QString standardLabel(MdMessageBoxStandardButton which)
{
    switch (which) {
    case MdMessageBoxStandardButton::Ok:
        return QStringLiteral("OK");
    case MdMessageBoxStandardButton::Cancel:
        return QStringLiteral("Cancel");
    case MdMessageBoxStandardButton::Yes:
        return QStringLiteral("Yes");
    case MdMessageBoxStandardButton::No:
        return QStringLiteral("No");
    case MdMessageBoxStandardButton::Retry:
        return QStringLiteral("Retry");
    case MdMessageBoxStandardButton::Close:
        return QStringLiteral("Close");
    case MdMessageBoxStandardButton::None:
    case MdMessageBoxStandardButton::Count:
        break;
    }
    return QString();
}

MdMessageBoxButtonRole standardRole(MdMessageBoxStandardButton which)
{
    switch (which) {
    case MdMessageBoxStandardButton::Ok:
        return MdMessageBoxButtonRole::AcceptRole;
    case MdMessageBoxStandardButton::Cancel:
        return MdMessageBoxButtonRole::RejectRole;
    case MdMessageBoxStandardButton::Yes:
        return MdMessageBoxButtonRole::YesRole;
    case MdMessageBoxStandardButton::No:
        return MdMessageBoxButtonRole::NoRole;
    case MdMessageBoxStandardButton::Retry:
        return MdMessageBoxButtonRole::RetryRole;
    case MdMessageBoxStandardButton::Close:
        return MdMessageBoxButtonRole::CloseRole;
    case MdMessageBoxStandardButton::None:
    case MdMessageBoxStandardButton::Count:
        break;
    }
    return MdMessageBoxButtonRole::Invalid;
}

} // namespace

MdMessageBox::MdMessageBox(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdMessageBox::MdMessageBox(MdMessageBoxVariant variant,
                           const QString &title,
                           const QString &text,
                           QWidget *parent)
    : QWidget(parent)
    , m_variant(variant)
    , m_title(title)
    , m_text(text)
{
    init();
}

MdMessageBox::~MdMessageBox()
{
    if (m_loop != nullptr) {
        m_loop->quit();
    }
}

void MdMessageBox::init()
{
    // The dialog container is a passive surface but must take pointer
    // events so a press is not read as an outside click by any host.
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_tokens = MdDialogTokens::resolve();

    MdStyleBase::connectThemeUpdate(this, &MdMessageBox::onThemeChanged);

    if (m_standardButtons == MdMessageBoxStandardButton::None) {
        setStandardButtons(MdMessageBoxStandardButton::Ok);
    }
    updateIcon();
}

QColor MdMessageBox::semanticColor() const
{
    return MdTheme::instance().color(semanticRole(m_variant));
}

void MdMessageBox::setVariant(MdMessageBoxVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    updateIcon();
    emit variantChanged(m_variant);
}

void MdMessageBox::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateGeometry();
    layoutChildren();
    update();
    emit titleChanged(m_title);
}

void MdMessageBox::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    updateGeometry();
    layoutChildren();
    update();
    emit textChanged(m_text);
}

void MdMessageBox::setInformativeText(const QString &text)
{
    if (m_informativeText == text) {
        return;
    }
    m_informativeText = text;
    updateGeometry();
    layoutChildren();
    update();
    emit informativeTextChanged(m_informativeText);
}

QString MdMessageBox::bodyText() const
{
    if (m_informativeText.isEmpty()) {
        return m_text;
    }
    if (m_text.isEmpty()) {
        return m_informativeText;
    }
    return m_text + QStringLiteral("\n\n") + m_informativeText;
}

void MdMessageBox::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    updateIcon();
    emit iconNameChanged(m_iconName);
}

void MdMessageBox::setIconWidget(QWidget *icon)
{
    if (m_customIcon != nullptr && m_customIcon != icon) {
        // Only delete icons we created or that were handed over earlier.
        if (m_iconWidget == m_customIcon) {
            delete m_customIcon;
        }
    }
    m_customIcon = icon;
    m_iconWidget = icon;
    if (m_iconWidget != nullptr) {
        m_iconWidget->setParent(this);
        m_iconWidget->show();
    }
    updateGeometry();
    layoutChildren();
    update();
}

void MdMessageBox::updateIcon()
{
    if (m_customIcon != nullptr) {
        return;
    }
    const QString name = m_iconName.isEmpty() ? defaultIconName(m_variant) : m_iconName;
    if (name.isEmpty()) {
        if (m_iconWidget != nullptr) {
            m_iconWidget->hide();
            m_iconWidget->deleteLater();
            m_iconWidget = nullptr;
        }
        updateGeometry();
        layoutChildren();
        update();
        return;
    }
    // Recreate the glyph widget whenever the name or colour may have moved.
    if (m_iconWidget != nullptr) {
        m_iconWidget->hide();
        m_iconWidget->deleteLater();
        m_iconWidget = nullptr;
    }
    m_iconWidget = new MessageBoxIcon(name, semanticColor(), this);
    m_iconWidget->show();
    updateGeometry();
    layoutChildren();
    update();
}

MdButton *MdMessageBox::addButton(const QString &text, MdMessageBoxButtonRole role)
{
    auto *button = new MdButton(text, this);
    button->setVariant(roleIsAffirmative(role) ? ButtonVariant::Filled : ButtonVariant::Text);
    button->setProperty("mdMessageRole", int(role));
    button->setProperty("mdMessageWhich", roleResult(role));
    connect(button, &MdButton::clicked, this, &MdMessageBox::onButtonClicked);
    m_ownedButtons.append(button);
    button->show();
    updateGeometry();
    layoutChildren();
    return button;
}

void MdMessageBox::clearButtons()
{
    qDeleteAll(m_ownedButtons);
    m_ownedButtons.clear();
    m_buttonWidths.clear();
    m_defaultButton = nullptr;
    m_escapeButton = nullptr;
    m_standardButtons = MdMessageBoxStandardButton::None;
    updateGeometry();
    layoutChildren();
}

QList<MdButton *> MdMessageBox::buttons() const
{
    return m_ownedButtons;
}

void MdMessageBox::setStandardButtons(MdMessageBoxStandardButtons buttons)
{
    clearButtons();
    m_standardButtons = buttons;
    const MdMessageBoxStandardButton order[] = {
        MdMessageBoxStandardButton::Yes,
        MdMessageBoxStandardButton::Retry,
        MdMessageBoxStandardButton::Ok,
        MdMessageBoxStandardButton::No,
        MdMessageBoxStandardButton::Cancel,
        MdMessageBoxStandardButton::Close,
    };
    MdButton *lastAffirmative = nullptr;
    MdButton *firstDismissive = nullptr;
    for (MdMessageBoxStandardButton which : order) {
        if (!buttons.testFlag(which)) {
            continue;
        }
        MdButton *button = addButton(standardLabel(which), standardRole(which));
        button->setProperty("mdMessageWhich", int(which));
        if (button->variant() == ButtonVariant::Filled) {
            lastAffirmative = button;
        } else if (firstDismissive == nullptr) {
            firstDismissive = button;
        }
    }
    m_defaultButton = lastAffirmative != nullptr ? lastAffirmative : firstDismissive;
    // Escape only maps to a dismissive button. With no dismissive action
    // (an OK-only box) Escape dismisses with result 0 — the QMessageBox
    // convention — rather than accidentally confirming.
    m_escapeButton = firstDismissive;
    if (m_defaultButton != nullptr) {
        m_defaultButton->setDefault(true);
    }
}

void MdMessageBox::setDefaultButton(MdButton *button)
{
    if (m_defaultButton != nullptr) {
        m_defaultButton->setDefault(false);
    }
    m_defaultButton = button;
    if (m_defaultButton != nullptr) {
        m_defaultButton->setDefault(true);
    }
}

void MdMessageBox::setEscapeButton(MdButton *button)
{
    m_escapeButton = button;
}

int MdMessageBox::standardButtonValue(const MdButton *button) const
{
    if (button == nullptr) {
        return 0;
    }
    return button->property("mdMessageWhich").toInt();
}

QRectF MdMessageBox::containerRect() const
{
    return QRectF(rect()).adjusted(kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
}

QSize MdMessageBox::sizeHint() const
{
    const int width = qBound(int(MdDialogTokens::kMinWidth), 320, int(MdDialogTokens::kMaxWidth));
    return QSize(width + int(kShadowMargin * 2), heightForWidth(width + int(kShadowMargin * 2)));
}

int MdMessageBox::heightForWidth(int width) const
{
    const QRectF container = QRectF(0, 0, width, 1000).adjusted(
        kShadowMargin, kShadowMargin, -kShadowMargin, -kShadowMargin);
    const qreal contentWidth = container.width() - MdDialogTokens::kContainerPadding * 2;

    QFont bodyFont = MdTypeScale::font(TypeStyle::BodyMedium);
    QFont titleFont = MdTypeScale::font(TypeStyle::HeadlineSmall);
    QFontMetrics bodyFm(bodyFont);
    QFontMetrics titleFm(titleFont);

    qreal height = MdDialogTokens::kContainerPadding; // top
    if (m_iconWidget != nullptr) {
        height += m_iconWidget->sizeHint().height() + MdDialogTokens::kIconBottomPadding;
    }
    if (!m_title.isEmpty()) {
        height += titleFm.boundingRect(QRectF(0, 0, contentWidth, 1000).toRect(),
                                       Qt::TextWordWrap, m_title)
                      .height();
        height += MdDialogTokens::kTitleBottomPadding;
    }
    const QString body = bodyText();
    if (!body.isEmpty()) {
        height += bodyFm.boundingRect(QRectF(0, 0, contentWidth, 1000).toRect(),
                                      Qt::TextWordWrap, body)
                      .height();
        height += MdDialogTokens::kTextBottomPadding;
    }
    // Action row: one row of buttons is enough for the preferred height.
    qreal actionHeight = 0.0;
    for (MdButton *button : m_ownedButtons) {
        actionHeight = qMax(actionHeight, qreal(button->sizeHint().height()));
    }
    if (actionHeight > 0.0) {
        height += actionHeight;
    }
    height += MdDialogTokens::kContainerPadding; // bottom
    return int(height + kShadowMargin * 2);
}

QList<QList<QRectF>> MdMessageBox::actionRows(const QRectF &content,
                                             const QList<qreal> &widths) const
{
    QList<QList<QRectF>> rows;
    if (widths.isEmpty()) {
        return rows;
    }
    // End-aligned, wrapping FlowRow — the Compose AlertDialog action row.
    QList<QRectF> current;
    qreal x = content.right(); // build right-to-left, then reverse
    const qreal spacing = MdDialogTokens::kActionsSpacing;
    const qreal rowHeight = 40.0;
    qreal y = content.bottom() - rowHeight;

    for (int i = widths.size() - 1; i >= 0; --i) {
        const qreal w = widths.at(i);
        if (!current.isEmpty() && x - spacing - w < content.left()) {
            rows.append(current);
            current.clear();
            x = content.right();
            y -= rowHeight + spacing;
        }
        x -= w;
        current.append(QRectF(x, y, w, rowHeight));
        x -= spacing;
    }
    if (!current.isEmpty()) {
        rows.append(current);
    }
    // Row 0 is the rightmost group (confirm side); keep that order.
    return rows;
}

void MdMessageBox::layoutChildren()
{
    if (width() <= 0 || height() <= 0) {
        return;
    }
    const QRectF container = containerRect();
    const qreal pad = MdDialogTokens::kContainerPadding;
    const QRectF content = container.adjusted(pad, pad, -pad, -pad);

    qreal y = content.top();

    if (m_iconWidget != nullptr && !m_iconWidget->isHidden()) {
        const QSize iconSize = m_iconWidget->sizeHint();
        m_iconWidget->setGeometry(
            QRectF(content.center().x() - iconSize.width() / 2.0, y, iconSize.width(),
                   iconSize.height())
                .toRect());
        y += iconSize.height() + MdDialogTokens::kIconBottomPadding;
    }

    QFont titleFont = MdTypeScale::font(TypeStyle::HeadlineSmall);
    QFont bodyFont = MdTypeScale::font(TypeStyle::BodyMedium);
    QFontMetrics titleFm(titleFont);
    QFontMetrics bodyFm(bodyFont);

    if (!m_title.isEmpty()) {
        const int titleHeight = titleFm.boundingRect(QRectF(0, 0, content.width(), 1000).toRect(),
                                                     Qt::TextWordWrap, m_title)
                                    .height();
        y += titleHeight + MdDialogTokens::kTitleBottomPadding;
    }
    const QString body = bodyText();
    if (!body.isEmpty()) {
        const int bodyHeight = bodyFm.boundingRect(QRectF(0, 0, content.width(), 1000).toRect(),
                                                   Qt::TextWordWrap, body)
                                   .height();
        y += bodyHeight + MdDialogTokens::kTextBottomPadding;
    }

    // Action row, end-aligned at the content bottom.
    m_buttonWidths.clear();
    for (MdButton *button : m_ownedButtons) {
        m_buttonWidths.append(qMax(48.0, qreal(button->sizeHint().width())));
    }
    const QList<QList<QRectF>> rows = actionRows(
        QRectF(content.left(), y, content.width(), qMax(0.0, content.bottom() - y)),
        m_buttonWidths);

    int buttonIndex = m_ownedButtons.size() - 1; // rows are built right-to-left
    for (const QList<QRectF> &row : rows) {
        for (const QRectF &cell : row) {
            if (buttonIndex >= 0 && buttonIndex < m_ownedButtons.size()) {
                m_ownedButtons.at(buttonIndex)->setGeometry(cell.toRect());
            }
            --buttonIndex;
        }
    }
}

void MdMessageBox::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

void MdMessageBox::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    layoutChildren();
}

void MdMessageBox::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::EnabledChange) {
        updateGeometry();
        layoutChildren();
        update();
    }
}

void MdMessageBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF container = containerRect();
    const QList<qreal> radii =
        MdShape::resolvedRadii(m_tokens.containerShape, container.size());
    const QPainterPath shape = MdShape::roundedRect(container, radii);

    // Tonal elevation: surface-container-high at level 3 is the dialog's
    // own container colour — not a drop shadow.
    painter.fillPath(shape, MdTheme::instance().color(m_tokens.containerColor));

    // Icon is a child widget; title and body are painted here (the
    // snackbar / dialog idiom: the box owns the strings and wraps them).
    const qreal pad = MdDialogTokens::kContainerPadding;
    const QRectF content = container.adjusted(pad, pad, -pad, -pad);
    qreal y = content.top();

    if (m_iconWidget != nullptr && !m_iconWidget->isHidden()) {
        y += m_iconWidget->sizeHint().height() + MdDialogTokens::kIconBottomPadding;
    }

    painter.setClipPath(shape);

    if (!m_title.isEmpty()) {
        QFont titleFont = MdTypeScale::font(TypeStyle::HeadlineSmall);
        painter.setFont(titleFont);
        painter.setPen(MdTheme::instance().color(m_tokens.headlineColor));
        QFontMetrics titleFm(titleFont);
        const int titleHeight = titleFm.boundingRect(QRectF(0, 0, content.width(), 1000).toRect(),
                                                     Qt::TextWordWrap, m_title)
                                    .height();
        painter.drawText(QRectF(content.left(), y, content.width(), titleHeight),
                         Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, m_title);
        y += titleHeight + MdDialogTokens::kTitleBottomPadding;
    }

    const QString body = bodyText();
    if (!body.isEmpty()) {
        QFont bodyFont = MdTypeScale::font(TypeStyle::BodyMedium);
        painter.setFont(bodyFont);
        painter.setPen(MdTheme::instance().color(m_tokens.supportingTextColor));
        QFontMetrics bodyFm(bodyFont);
        const int bodyHeight = bodyFm.boundingRect(QRectF(0, 0, content.width(), 1000).toRect(),
                                                   Qt::TextWordWrap, body)
                                   .height();
        painter.drawText(QRectF(content.left(), y, content.width(), bodyHeight),
                         Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, body);
    }
}

void MdMessageBox::onButtonClicked()
{
    auto *button = qobject_cast<MdButton *>(sender());
    if (button == nullptr) {
        return;
    }
    const int which = standardButtonValue(button);
    emit buttonClicked(which);
    const auto role = MdMessageBoxButtonRole(button->property("mdMessageRole").toInt());
    if (role == MdMessageBoxButtonRole::ActionRole) {
        return; // does not close
    }
    applyResult(which);
}

void MdMessageBox::applyResult(int result)
{
    if (m_finished) {
        return;
    }
    m_finished = true;
    m_result = result;
    hide();
    emit finished(m_result);
    if (m_loop != nullptr) {
        m_loop->quit();
    }
}

int MdMessageBox::exec()
{
    m_result = 0;
    m_finished = false;
    layoutChildren();
    show();
    QEventLoop loop;
    m_loop = &loop;
    connect(this, &MdMessageBox::finished, &loop, &QEventLoop::quit);
    loop.exec();
    m_loop = nullptr;
    return m_result;
}

void MdMessageBox::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (m_escapeButton != nullptr) {
            m_escapeButton->click();
        } else {
            applyResult(0);
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_defaultButton != nullptr) {
            m_defaultButton->click();
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void MdMessageBox::onThemeChanged()
{
    updateIcon();
    update();
}

// --- static conveniences ------------------------------------------------------

int MdMessageBox::information(QWidget *parent,
                              const QString &title,
                              const QString &text,
                              MdMessageBoxStandardButtons buttons)
{
    MdMessageBox box(MdMessageBoxVariant::Information, title, text, parent);
    box.setStandardButtons(buttons);
    return box.exec();
}

int MdMessageBox::warning(QWidget *parent,
                          const QString &title,
                          const QString &text,
                          MdMessageBoxStandardButtons buttons)
{
    MdMessageBox box(MdMessageBoxVariant::Warning, title, text, parent);
    box.setStandardButtons(buttons);
    return box.exec();
}

int MdMessageBox::critical(QWidget *parent,
                           const QString &title,
                           const QString &text,
                           MdMessageBoxStandardButtons buttons)
{
    MdMessageBox box(MdMessageBoxVariant::Error, title, text, parent);
    box.setStandardButtons(buttons);
    return box.exec();
}

int MdMessageBox::question(QWidget *parent,
                           const QString &title,
                           const QString &text,
                           MdMessageBoxStandardButtons buttons)
{
    MdMessageBox box(MdMessageBoxVariant::Question, title, text, parent);
    if (buttons == 0) {
        buttons = MdMessageBoxStandardButton::Yes | MdMessageBoxStandardButton::No;
    }
    box.setStandardButtons(buttons);
    return box.exec();
}

} // namespace md
