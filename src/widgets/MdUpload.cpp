#include "MdUpload.h"

#include "MdIconButton.h"
#include "MdProgressIndicator.h"
#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// List one-line height (md.comp.list.* / ListItem one-line).
constexpr qreal kRowHeight = 56.0;
/// Outline-variant divider between rows.
constexpr qreal kDividerThickness = 1.0;
/// Leading icon optical size.
constexpr qreal kIconSize = 24.0;
/// Progress bar height under the file name.
constexpr qreal kProgressHeight = 4.0;
/// Padding from the row edge to the leading icon.
constexpr qreal kLeadingInset = 16.0;
/// Gap between the name column and the trailing remove button.
constexpr qreal kTrailingGap = 4.0;
/// Text column inset after the leading icon.
constexpr qreal kTextInset = 16.0;

} // namespace

MdUpload::MdUpload(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdUpload::~MdUpload() = default;

void MdUpload::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAccessibleName(QStringLiteral("upload"));
    setAttribute(Qt::WA_Hover, true);

    MdStyleBase::connectThemeUpdate(this, &MdUpload::onThemeChanged);
}

qreal MdUpload::rowHeight()
{
    return kRowHeight;
}

void MdUpload::setFiles(const QList<MdUploadFile> &files)
{
    if (m_files == files) {
        return;
    }
    m_files = files;
    rebuildRows();
    updateGeometry();
    update();
    emit filesChanged(m_files);
}

void MdUpload::setMaxVisibleRows(int rows)
{
    const int clamped = qMax(0, rows);
    if (m_maxVisibleRows == clamped) {
        return;
    }
    m_maxVisibleRows = clamped;
    updateGeometry();
    update();
    emit maxVisibleRowsChanged(m_maxVisibleRows);
}

void MdUpload::addFile(const QString &name, qint64 sizeBytes)
{
    MdUploadFile file;
    file.name = name;
    file.sizeBytes = sizeBytes;
    file.progress = 0.0;
    file.transferring = true;
    m_files.append(file);
    rebuildRows();
    updateGeometry();
    update();
    emit filesChanged(m_files);
}

void MdUpload::setFileProgress(int index, qreal progress, bool transferring)
{
    if (index < 0 || index >= m_files.size()) {
        return;
    }
    MdUploadFile &file = m_files[index];
    const qreal clamped = qBound(0.0, progress, 1.0);
    if (qFuzzyCompare(file.progress, clamped)
        && file.transferring == transferring) {
        return;
    }
    file.progress = clamped;
    file.transferring = transferring;
    if (index < m_progressBars.size() && m_progressBars.at(index) != nullptr) {
        m_progressBars.at(index)->setValue(clamped * 100.0);
    }
    update();
    emit filesChanged(m_files);
}

void MdUpload::removeFile(int index)
{
    if (index < 0 || index >= m_files.size()) {
        return;
    }
    // QList has no resize; drop the entry and rebuild the bank.
    m_files.removeAt(index);
    rebuildRows();
    updateGeometry();
    update();
    emit filesChanged(m_files);
    emit fileRemoved(index);
}

void MdUpload::clearFiles()
{
    if (m_files.isEmpty()) {
        return;
    }
    m_files.clear();
    rebuildRows();
    updateGeometry();
    update();
    emit filesChanged(m_files);
}

QSize MdUpload::sizeHint() const
{
    const int visible = m_maxVisibleRows > 0
                            ? qMin(m_maxVisibleRows, int(m_files.size()))
                            : int(m_files.size());
    const int rows = qMax(1, visible);
    return QSize(360, qRound(rows * kRowHeight + kDividerThickness));
}

QSize MdUpload::minimumSizeHint() const
{
    return QSize(200, qRound(kRowHeight));
}

void MdUpload::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdTheme &theme = MdTheme::instance();
    const QColor onSurface = theme.color(ColorRole::OnSurface);
    const QColor onSurfaceVariant = theme.color(ColorRole::OnSurfaceVariant);
    const QColor outlineVariant = theme.color(ColorRole::OutlineVariant);

    const QFont nameFont = MdTypeScale::font(TypeStyle::BodyMedium);
    const QFont metaFont = MdTypeScale::font(TypeStyle::LabelSmall);

    const int visible = m_maxVisibleRows > 0
                            ? qMin(m_maxVisibleRows, int(m_files.size()))
                            : int(m_files.size());

    for (int i = 0; i < visible; ++i) {
        const QRectF row(0.0, i * kRowHeight, width(), kRowHeight);

        // Leading file-type icon (list leading slot).
        const QRectF iconRect(row.left() + kLeadingInset,
                              row.center().y() - kIconSize / 2.0,
                              kIconSize, kIconSize);
        MdIcon::paint(&painter, iconRect, QStringLiteral("description"),
                      onSurfaceVariant);

        // Name + optional size meta, stacked in the text column.
        const qreal textLeft = iconRect.right() + kTextInset;
        const qreal textRight = width() - 48.0 - kTrailingGap; // room for the remove button
        QRectF nameRect(textLeft, row.top() + 10.0,
                        textRight - textLeft, 20.0);
        painter.setFont(nameFont);
        painter.setPen(onSurface);
        painter.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter,
                         QFontMetrics(nameFont).elidedText(
                             m_files.at(i).name, Qt::ElideMiddle,
                             int(nameRect.width())));

        const bool showMeta = m_files.at(i).sizeBytes > 0
                              || m_files.at(i).transferring;
        if (showMeta) {
            QRectF metaRect(textLeft, row.top() + 32.0,
                            textRight - textLeft, 16.0);
            painter.setFont(metaFont);
            painter.setPen(onSurfaceVariant);
            QString meta;
            if (m_files.at(i).transferring) {
                meta = QStringLiteral("%1%").arg(
                    qRound(m_files.at(i).progress * 100.0));
            } else if (m_files.at(i).sizeBytes > 0) {
                meta = QStringLiteral("%1 KB").arg(
                    m_files.at(i).sizeBytes / 1024);
            }
            painter.drawText(metaRect, Qt::AlignLeft | Qt::AlignVCenter, meta);
        }

        // Outline-variant divider under the row (List family).
        if (i + 1 < visible) {
            painter.fillRect(QRectF(0.0, row.bottom() - kDividerThickness,
                                    width(), kDividerThickness),
                             outlineVariant);
        }
    }
}

void MdUpload::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeRows();
}

void MdUpload::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::EnabledChange
        || event->type() == QEvent::LayoutDirectionChange) {
        update();
    }
}

void MdUpload::onThemeChanged()
{
    update();
}

void MdUpload::onRemoveClicked()
{
    auto *button = qobject_cast<MdIconButton *>(sender());
    if (button == nullptr) {
        return;
    }
    const int index = m_removeButtons.indexOf(button);
    if (index >= 0) {
        removeFile(index);
    }
}

void MdUpload::clearRowWidgets()
{
    for (MdIconButton *button : m_removeButtons) {
        if (button != nullptr) {
            button->deleteLater();
        }
    }
    m_removeButtons.clear();
    for (MdProgressIndicator *bar : m_progressBars) {
        if (bar != nullptr) {
            bar->deleteLater();
        }
    }
    m_progressBars.clear();
}

void MdUpload::rebuildRows()
{
    clearRowWidgets();

    const int visible = m_maxVisibleRows > 0
                            ? qMin(m_maxVisibleRows, int(m_files.size()))
                            : int(m_files.size());

    for (int i = 0; i < visible; ++i) {
        auto *bar = new MdProgressIndicator(ProgressIndicatorShape::Linear, this);
        bar->setObjectName(QStringLiteral("md-upload-progress"));
        bar->setMax(100.0);
        bar->setValue(m_files.at(i).progress * 100.0);
        bar->setIndeterminate(false);
        bar->setVisible(m_files.at(i).transferring);
        m_progressBars.append(bar);

        auto *remove = new MdIconButton(QStringLiteral("close"), this);
        remove->setObjectName(QStringLiteral("md-upload-remove"));
        remove->setVariant(IconButtonVariant::Standard);
        remove->setButtonSize(ButtonSize::XSmall);
        remove->setAccessibleName(QStringLiteral("Remove file"));
        connect(remove, &MdIconButton::clicked,
                this, &MdUpload::onRemoveClicked);
        m_removeButtons.append(remove);
    }

    placeRows();
}

void MdUpload::placeRows()
{
    const int visible = m_maxVisibleRows > 0
                            ? qMin(m_maxVisibleRows, int(m_files.size()))
                            : int(m_files.size());

    const bool rtl = layoutDirection() == Qt::RightToLeft;

    for (int i = 0; i < visible; ++i) {
        const qreal top = i * kRowHeight;

        if (i < m_removeButtons.size() && m_removeButtons.at(i) != nullptr) {
            MdIconButton *remove = m_removeButtons.at(i);
            const QSize hint = remove->sizeHint();
            const qreal x = rtl ? kTrailingGap
                                : width() - hint.width() - kTrailingGap;
            remove->setGeometry(QRect(qRound(x), qRound(top + (kRowHeight - hint.height()) / 2.0),
                                      hint.width(), hint.height()));
            remove->raise();
        }

        if (i < m_progressBars.size() && m_progressBars.at(i) != nullptr) {
            MdProgressIndicator *bar = m_progressBars.at(i);
            const qreal left = kLeadingInset + kIconSize + kTextInset;
            const qreal right = width() - 48.0 - kTrailingGap;
            const qreal barTop = top + 38.0;
            bar->setGeometry(QRect(qRound(left), qRound(barTop),
                                   qMax(0, qRound(right - left)),
                                   qRound(kProgressHeight)));
            bar->setVisible(m_files.at(i).transferring);
            bar->raise();
        }
    }
}

} // namespace md
