#include "MdFileDialog.h"

#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdButton.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdTextField.h"

#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

// ---------------------------------------------------------------------------
// MdFileEntry
// ---------------------------------------------------------------------------

QString MdFileEntry::iconName() const
{
    if (isDir) {
        return QStringLiteral("folder");
    }
    const QString suffix = QFileInfo(name).suffix().toLower();
    if (suffix == QLatin1String("txt") || suffix == QLatin1String("md")
        || suffix == QLatin1String("log")) {
        return QStringLiteral("description");
    }
    return QStringLiteral("insert_drive_file");
}

// ---------------------------------------------------------------------------
// MdFileDialog
// ---------------------------------------------------------------------------

namespace {

/// md.comp.list.list-item-one-line.container.height — 56 px.
constexpr qreal kRowHeight = 56.0;
/// md.comp.list.list-item-one-line.leading-icon.size — 24 px.
constexpr qreal kRowIconSize = 24.0;
/// md.comp.dialog.container.padding — 24 px.
constexpr qreal kDialogPadding = 24.0;
/// md.comp.dialog actions spacing — 8 px.
constexpr qreal kActionSpacing = 8.0;
/// md.comp.filled-text-field.container.height — 56 px.
constexpr qreal kFieldHeight = 56.0;
/// The left location column width (navigation-drawer collapsed width).
constexpr qreal kLocationWidth = 168.0;
/// Gap between the location column and the file list.
constexpr qreal kLocationGap = 12.0;
/// Height of the title row inside the dialog chrome.
constexpr qreal kTitleRow = 32.0;
/// State-layer opacity for a hovered list row (md.comp.list state-layer).
constexpr qreal kHoverOpacity = 0.08;

/// Home / Desktop / Documents / Downloads / Root, the standard side list.
QList<MdFileLocation> defaultLocations()
{
    QList<MdFileLocation> list;
    auto add = [&list](const QString &label, const QString &icon, const QString &path) {
        if (!path.isEmpty()) {
            MdFileLocation loc;
            loc.label = label;
            loc.iconName = icon;
            loc.path = path;
            list.append(loc);
        }
    };
    add(QStringLiteral("Home"), QStringLiteral("home"),
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    add(QStringLiteral("Desktop"), QStringLiteral("computer"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    add(QStringLiteral("Documents"), QStringLiteral("description"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    add(QStringLiteral("Downloads"), QStringLiteral("download"),
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    add(QStringLiteral("Root"), QStringLiteral("folder"),
        QDir::rootPath());
    return list;
}

} // namespace

MdFileDialog::MdFileDialog(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdFileDialog::~MdFileDialog()
{
    // Cut child -> parent wires before ~QWidget destroys the children;
    // otherwise a child emitting during its own destruction would call a
    // slot on a half-destroyed MdFileDialog.
    if (m_pathField) {
        disconnect(m_pathField, nullptr, this, nullptr);
    }
    if (m_nameField) {
        disconnect(m_nameField, nullptr, this, nullptr);
    }
    if (m_upButton) {
        disconnect(m_upButton, nullptr, this, nullptr);
    }
    if (m_newFolderButton) {
        disconnect(m_newFolderButton, nullptr, this, nullptr);
    }
    if (m_acceptButton) {
        disconnect(m_acceptButton, nullptr, this, nullptr);
    }
    if (m_rejectButton) {
        disconnect(m_rejectButton, nullptr, this, nullptr);
    }
}

void MdFileDialog::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(560, 400);

    m_title = QStringLiteral("Open file");
    m_directory = QDir::currentPath();
    m_locations = defaultLocations();
    m_tokens = MdDialogTokens();

    m_pathField = new MdTextField(this);
    m_pathField->setVariant(MdTextFieldVariant::Filled);
    m_pathField->setLabelText(QStringLiteral("Path"));
    m_pathField->setText(m_directory);

    m_nameField = new MdTextField(this);
    m_nameField->setVariant(MdTextFieldVariant::Filled);
    m_nameField->setLabelText(QStringLiteral("Name"));

    m_upButton = new MdIconButton(QStringLiteral("arrow_upward"), this);
    m_upButton->setVariant(IconButtonVariant::Standard);
    m_upButton->setButtonSize(ButtonSize::Small);
    m_upButton->setAccessibleName(QStringLiteral("Up"));

    m_newFolderButton = new MdIconButton(QStringLiteral("create_new_folder"), this);
    m_newFolderButton->setVariant(IconButtonVariant::Standard);
    m_newFolderButton->setButtonSize(ButtonSize::Small);
    m_newFolderButton->setAccessibleName(QStringLiteral("New folder"));

    m_acceptButton = new MdButton(QStringLiteral("Open"), this);
    m_acceptButton->setVariant(ButtonVariant::Filled);
    m_rejectButton = new MdButton(QStringLiteral("Cancel"), this);
    m_rejectButton->setVariant(ButtonVariant::Text);

    connect(m_acceptButton, &MdButton::clicked, this, &MdFileDialog::onAcceptClicked);
    connect(m_rejectButton, &MdButton::clicked, this, &MdFileDialog::onRejectClicked);
    connect(m_upButton, &MdIconButton::clicked, this, &MdFileDialog::onUpClicked);
    connect(m_newFolderButton, &MdIconButton::clicked, this, &MdFileDialog::onNewFolderClicked);
    connect(m_pathField, &MdTextField::editingFinished, this, &MdFileDialog::onPathEditingFinished);
    connect(m_nameField, &MdTextField::editingFinished, this, &MdFileDialog::onNameEditingFinished);

    refresh();
    layoutChildren();
    MdStyleBase::connectThemeUpdate(this, &MdFileDialog::onThemeChanged);
}

void MdFileDialog::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    update();
    emit titleChanged(m_title);
}

void MdFileDialog::setDirectory(const QString &path)
{
    QFileInfo info(path);
    QString resolved = path;
    if (info.exists() && info.isDir()) {
        resolved = info.absoluteFilePath();
    } else {
        resolved = QDir(path).absolutePath();
    }
    if (m_directory == resolved) {
        return;
    }
    m_directory = resolved;
    if (m_pathField) {
        m_pathField->setText(m_directory);
    }
    m_currentIndex = -1;
    refresh();
    emit directoryChanged(m_directory);
    emit currentIndexChanged(m_currentIndex);
}

void MdFileDialog::setSelectedName(const QString &name)
{
    if (m_selectedName == name) {
        return;
    }
    m_selectedName = name;
    if (m_nameField) {
        m_nameField->setText(name);
    }
    emit selectedNameChanged(m_selectedName);
}

QString MdFileDialog::selectedFilePath() const
{
    if (!m_selectedName.isEmpty()) {
        return QDir(m_directory).filePath(m_selectedName);
    }
    if (m_currentIndex >= 0 && m_currentIndex < m_entries.size()) {
        return m_entries.at(m_currentIndex).absolutePath;
    }
    return m_directory;
}

void MdFileDialog::setShowHidden(bool show)
{
    if (m_showHidden == show) {
        return;
    }
    m_showHidden = show;
    refresh();
    emit showHiddenChanged(m_showHidden);
}

void MdFileDialog::setMode(MdFileDialogMode mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    if (m_acceptButton) {
        m_acceptButton->setText(labelTextForMode());
    }
    update();
    emit modeChanged(m_mode);
}

void MdFileDialog::setNameFilters(const QStringList &filters)
{
    if (m_nameFilters == filters) {
        return;
    }
    m_nameFilters = filters;
    if (m_filterIndex >= m_nameFilters.size()) {
        m_filterIndex = m_nameFilters.isEmpty() ? -1 : 0;
        emit currentFilterIndexChanged(m_filterIndex);
    }
    refresh();
    emit nameFiltersChanged(m_nameFilters);
}

void MdFileDialog::setCurrentFilterIndex(int index)
{
    const int clamped = m_nameFilters.isEmpty() ? -1 : qBound(0, index, m_nameFilters.size() - 1);
    if (m_filterIndex == clamped) {
        return;
    }
    m_filterIndex = clamped;
    refresh();
    emit currentFilterIndexChanged(m_filterIndex);
}

void MdFileDialog::setCurrentIndex(int index)
{
    const int clamped = m_entries.isEmpty() ? -1 : qBound(-1, index, m_entries.size() - 1);
    if (m_currentIndex == clamped) {
        return;
    }
    m_currentIndex = clamped;
    if (m_currentIndex >= 0 && m_currentIndex < m_entries.size()) {
        const MdFileEntry &entry = m_entries.at(m_currentIndex);
        if (!entry.isDir) {
            m_selectedName = entry.name;
            if (m_nameField) {
                m_nameField->setText(m_selectedName);
            }
            emit selectedNameChanged(m_selectedName);
        }
    }
    update();
    emit currentIndexChanged(m_currentIndex);
}

void MdFileDialog::refresh()
{
    QDir dir(m_directory);
    if (!dir.exists()) {
        dir = QDir::home();
        m_directory = dir.absolutePath();
        if (m_pathField) {
            m_pathField->setText(m_directory);
        }
    }

    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (m_showHidden) {
        filters |= QDir::Hidden | QDir::System;
    }

    m_entries.clear();
    const QFileInfoList list = dir.entryInfoList(filters, QDir::DirsFirst | QDir::Name);
    for (const QFileInfo &info : list) {
        if (!rowMatchesFilter(info)) {
            continue;
        }
        MdFileEntry entry;
        entry.name = info.fileName();
        entry.absolutePath = info.absoluteFilePath();
        entry.isDir = info.isDir();
        entry.size = info.isDir() ? 0 : info.size();
        entry.modified = info.lastModified();
        m_entries.append(entry);
    }
    if (m_currentIndex >= m_entries.size()) {
        m_currentIndex = -1;
    }
    update();
    emit entriesChanged();
}

bool MdFileDialog::rowMatchesFilter(const QFileInfo &info) const
{
    if (info.isDir()) {
        return true;
    }
    if (m_filterIndex < 0 || m_filterIndex >= m_nameFilters.size()) {
        return true;
    }
    // "Images (*.png *.jpg)" — take the parenthesised glob list.
    const QString filter = m_nameFilters.at(m_filterIndex);
    const int open = filter.indexOf(QLatin1Char('('));
    const int close = filter.lastIndexOf(QLatin1Char(')'));
    if (open < 0 || close <= open) {
        return true;
    }
    const QStringList patterns =
        filter.mid(open + 1, close - open - 1).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &raw : patterns) {
        QString pattern = raw.trimmed();
        if (pattern.startsWith(QLatin1String("*."))) {
            if (info.fileName().endsWith(pattern.mid(1), Qt::CaseInsensitive)) {
                return true;
            }
        } else if (pattern == QLatin1String("*") || pattern == QLatin1String("*.*")) {
            return true;
        }
    }
    return false;
}

void MdFileDialog::activateCurrent()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_entries.size()) {
        return;
    }
    const MdFileEntry entry = m_entries.at(m_currentIndex);
    if (entry.isDir) {
        setDirectory(entry.absolutePath);
    } else {
        acceptPath(entry.absolutePath);
    }
}

QRectF MdFileDialog::containerRect() const
{
    return QRectF(rect());
}

int MdFileDialog::exec()
{
    QEventLoop loop;
    int result = 0;
    const QMetaObject::Connection ok = connect(this, &MdFileDialog::accepted, &loop, [&](const QString &) {
        result = 1;
        loop.quit();
    });
    const QMetaObject::Connection ko = connect(this, &MdFileDialog::rejected, &loop, [&]() {
        result = 0;
        loop.quit();
    });
    show();
    raise();
    setFocus();
    loop.exec();
    disconnect(ok);
    disconnect(ko);
    return result;
}

void MdFileDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    // md.comp.dialog.container.shape = corner-extra-large.
    const QList<qreal> radii =
        MdShape::resolvedRadii(ShapeCorner::ExtraLarge, box.size());
    const QPainterPath shape = MdShape::roundedRect(box, radii);
    painter.fillPath(shape, MdTheme::instance().color(m_tokens.containerColor));

    // Title.
    {
        QRectF titleRect(kDialogPadding, kDialogPadding, box.width() - 2 * kDialogPadding, kTitleRow);
        painter.setFont(MdTypeScale::font(TypeStyle::HeadlineSmall));
        painter.setPen(MdTheme::instance().color(m_tokens.headlineColor));
        painter.drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, m_title);
    }

    // Location column (navigation drawer rail).
    const QRectF locations = locationsRect();
    painter.setFont(MdTypeScale::font(TypeStyle::LabelLarge));
    for (int i = 0; i < m_locations.size(); ++i) {
        const QRectF row(locations.left(), locations.top() + i * kRowHeight, locations.width(),
                         kRowHeight);
        const bool selected = (i == m_locationIndex);
        const bool hovered = (i == m_hoveredLocation && !selected);
        if (selected) {
            const QList<qreal> rowRadii =
                MdShape::resolvedRadii(ShapeCorner::Full, row.size());
            painter.fillPath(MdShape::roundedRect(row, rowRadii),
                             MdTheme::instance().color(ColorRole::SecondaryContainer));
        } else if (hovered) {
            QColor hover = MdTheme::instance().color(ColorRole::OnSurface);
            hover.setAlpha(qRound(kHoverOpacity * 255));
            painter.fillRect(row.adjusted(4, 0, -4, 0), hover);
        }
        const QColor content = selected
            ? MdTheme::instance().color(ColorRole::OnSecondaryContainer)
            : MdTheme::instance().color(ColorRole::OnSurfaceVariant);
        const MdFileLocation &loc = m_locations.at(i);
        const QRectF iconRect(row.left() + 16, row.top() + (kRowHeight - kRowIconSize) / 2,
                              kRowIconSize, kRowIconSize);
        MdIcon::paint(&painter, iconRect, loc.iconName, content);
        const QRectF textRect(iconRect.right() + 12, row.top(), row.width() - 52, kRowHeight);
        painter.setPen(content);
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, loc.label);
    }

    // File list.
    const QRectF list = listRect();
    painter.setFont(MdTypeScale::font(TypeStyle::BodyLarge));
    for (int i = 0; i < m_entries.size(); ++i) {
        const QRectF row(list.left(), list.top() + i * kRowHeight, list.width(), kRowHeight);
        if (row.bottom() < list.top() || row.top() > list.bottom()) {
            continue;
        }
        const bool selected = (i == m_currentIndex);
        const bool hovered = (i == m_hoveredIndex && !selected);
        if (selected) {
            painter.fillRect(row, MdTheme::instance().color(ColorRole::SecondaryContainer));
        } else if (hovered) {
            QColor hover = MdTheme::instance().color(ColorRole::OnSurface);
            hover.setAlpha(qRound(kHoverOpacity * 255));
            painter.fillRect(row, hover);
        }
        // outline-variant divider under the row.
        painter.fillRect(QRectF(row.left(), row.bottom() - 1.0, row.width(), 1.0),
                         MdTheme::instance().color(ColorRole::OutlineVariant));

        const QColor label = selected
            ? MdTheme::instance().color(ColorRole::OnSecondaryContainer)
            : MdTheme::instance().color(ColorRole::OnSurface);
        const QColor meta = selected
            ? MdTheme::instance().color(ColorRole::OnSecondaryContainer)
            : MdTheme::instance().color(ColorRole::OnSurfaceVariant);

        const MdFileEntry &entry = m_entries.at(i);
        const QRectF iconRect(row.left() + 16, row.top() + (kRowHeight - kRowIconSize) / 2,
                              kRowIconSize, kRowIconSize);
        MdIcon::paint(&painter, iconRect, entry.iconName(), meta);

        const QRectF nameRect(iconRect.right() + 12, row.top(), row.width() * 0.5, kRowHeight);
        painter.setPen(label);
        painter.drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft, entry.name);

        if (!entry.isDir) {
            painter.setFont(MdTypeScale::font(TypeStyle::BodySmall));
            painter.setPen(meta);
            const QRectF sizeRect(row.right() - 160, row.top(), 70, kRowHeight);
            painter.drawText(sizeRect, Qt::AlignVCenter | Qt::AlignRight,
                             entry.size < 1024
                                 ? QStringLiteral("%1 B").arg(entry.size)
                                 : QStringLiteral("%1 KB").arg(entry.size / 1024));
            const QRectF dateRect(row.right() - 84, row.top(), 70, kRowHeight);
            painter.drawText(dateRect, Qt::AlignVCenter | Qt::AlignRight,
                             entry.modified.toString(QStringLiteral("yyyy-MM-dd")));
            painter.setFont(MdTypeScale::font(TypeStyle::BodyLarge));
        }
    }
}

void MdFileDialog::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        emit rejected();
        event->accept();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_currentIndex >= 0) {
            activateCurrent();
        } else {
            onAcceptClicked();
        }
        event->accept();
        return;
    case Qt::Key_Up:
        setCurrentIndex(m_currentIndex - 1);
        event->accept();
        return;
    case Qt::Key_Down:
        setCurrentIndex(m_currentIndex + 1);
        event->accept();
        return;
    case Qt::Key_Backspace:
        goUp();
        event->accept();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdFileDialog::mousePressEvent(QMouseEvent *event)
{
    const QPointF pos = mousePosition(event);
    const int loc = locationAt(pos);
    if (loc >= 0) {
        m_locationIndex = loc;
        setDirectory(m_locations.at(loc).path);
        update();
        return;
    }
    const int row = rowAt(pos);
    if (row >= 0) {
        setCurrentIndex(row);
        return;
    }
    QWidget::mousePressEvent(event);
}

void MdFileDialog::mouseDoubleClickEvent(QMouseEvent *event)
{
    const QPointF pos = mousePosition(event);
    const int row = rowAt(pos);
    if (row >= 0) {
        setCurrentIndex(row);
        activateCurrent();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void MdFileDialog::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF pos = mousePosition(event);
    const int row = rowAt(pos);
    const int loc = locationAt(pos);
    if (row != m_hoveredIndex || loc != m_hoveredLocation) {
        m_hoveredIndex = row;
        m_hoveredLocation = loc;
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void MdFileDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

void MdFileDialog::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    layoutChildren();
}

void MdFileDialog::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange) {
        layoutChildren();
        update();
    }
}

void MdFileDialog::onAcceptClicked()
{
    acceptPath(selectedFilePath());
}

void MdFileDialog::onRejectClicked()
{
    emit rejected();
}

void MdFileDialog::onUpClicked()
{
    goUp();
}

void MdFileDialog::onNewFolderClicked()
{
    QDir dir(m_directory);
    QString name = QStringLiteral("New folder");
    int n = 1;
    while (dir.exists(name)) {
        name = QStringLiteral("New folder %1").arg(++n);
    }
    dir.mkpath(name);
    refresh();
    // Highlight the folder we just created.
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).name == name) {
            setCurrentIndex(i);
            break;
        }
    }
}

void MdFileDialog::onPathEditingFinished()
{
    if (!m_pathField) {
        return;
    }
    setDirectory(m_pathField->text());
}

void MdFileDialog::onNameEditingFinished()
{
    if (!m_nameField) {
        return;
    }
    setSelectedName(m_nameField->text());
}

void MdFileDialog::onThemeChanged()
{
    update();
}

void MdFileDialog::layoutChildren()
{
    const QRectF box = QRectF(rect());
    const QRectF locations = locationsRect();
    const QRectF list = listRect();

    // Path field sits above the list, sharing its horizontal span.
    if (m_pathField) {
        const QRectF pathRect(list.left(), locations.top(), list.width() - 2 * 48 - 8, kFieldHeight);
        m_pathField->setGeometry(pathRect.toRect());
    }
    if (m_upButton) {
        const QRectF upRect(list.right() - 48 - 48 - 4, locations.top(), 48, 48);
        m_upButton->setGeometry(upRect.toRect());
    }
    if (m_newFolderButton) {
        const QRectF nfRect(list.right() - 48, locations.top() + 4, 48, 48);
        m_newFolderButton->setGeometry(nfRect.toRect());
    }

    // Name field + actions along the bottom.
    const qreal actionsY = box.bottom() - kDialogPadding - kFieldHeight;
    if (m_nameField) {
        const qreal actionButtonsWidth = 200;
        m_nameField->setGeometry(QRectF(list.left(), actionsY,
                                        list.width() - actionButtonsWidth - kActionSpacing,
                                        kFieldHeight).toRect());
    }
    if (m_acceptButton && m_rejectButton) {
        const QSize acceptHint = m_acceptButton->sizeHint();
        const QSize rejectHint = m_rejectButton->sizeHint();
        const qreal totalWidth = acceptHint.width() + kActionSpacing + rejectHint.width();
        qreal x = list.right() - totalWidth;
        m_acceptButton->setGeometry(QRectF(x, actionsY, acceptHint.width(), kFieldHeight).toRect());
        x += acceptHint.width() + kActionSpacing;
        m_rejectButton->setGeometry(QRectF(x, actionsY, rejectHint.width(), kFieldHeight).toRect());
    }

    update();
}

QRectF MdFileDialog::listRect() const
{
    const QRectF box = QRectF(rect());
    const QRectF locations = locationsRect();
    const qreal top = locations.top() + kFieldHeight + kLocationGap;
    const qreal bottom = box.bottom() - kDialogPadding - kFieldHeight - kActionSpacing;
    const qreal left = locations.right() + kLocationGap;
    return QRectF(left, top, box.right() - kDialogPadding - left, qMax(0.0, bottom - top));
}

QRectF MdFileDialog::locationsRect() const
{
    const QRectF box = QRectF(rect());
    const bool rtl = MdStyleBase::isRtl(this);
    const qreal top = kDialogPadding + kTitleRow + kLocationGap;
    const qreal height = box.bottom() - kDialogPadding - top;
    const qreal x = rtl ? box.right() - kDialogPadding - kLocationWidth : kDialogPadding;
    return QRectF(x, top, kLocationWidth, qMax(0.0, height));
}

int MdFileDialog::rowAt(const QPointF &pos) const
{
    const QRectF list = listRect();
    if (!list.contains(pos)) {
        return -1;
    }
    const int index = int((pos.y() - list.top()) / kRowHeight);
    return (index >= 0 && index < m_entries.size()) ? index : -1;
}

int MdFileDialog::locationAt(const QPointF &pos) const
{
    const QRectF locations = locationsRect();
    if (!locations.contains(pos)) {
        return -1;
    }
    const int index = int((pos.y() - locations.top()) / kRowHeight);
    return (index >= 0 && index < m_locations.size()) ? index : -1;
}

void MdFileDialog::goUp()
{
    QDir dir(m_directory);
    if (dir.cdUp()) {
        setDirectory(dir.absolutePath());
    }
}

void MdFileDialog::acceptPath(const QString &path)
{
    emit accepted(path);
}

QString MdFileDialog::labelTextForMode() const
{
    switch (m_mode) {
    case MdFileDialogMode::OpenDirectory:
        return QStringLiteral("Select folder");
    case MdFileDialogMode::SaveFile:
        return QStringLiteral("Save");
    case MdFileDialogMode::OpenFile:
    case MdFileDialogMode::Count:
        break;
    }
    return QStringLiteral("Open");
}

} // namespace md
