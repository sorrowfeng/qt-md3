#ifndef MD_UPLOAD_H
#define MD_UPLOAD_H

// MdUpload — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "List + Progress indicator + Icon button".
// MD3 has no upload family. The control is the standard file-queue surface
// every Material import / attach flow eventually needs:
//
//   row         List row semantics — one-line height 56, hover state layer
//               0.08, outline-variant dividers, leading icon slot
//   name        body-medium / on-surface (list one-line title)
//   progress    a real `MdProgressIndicator` (linear, determinate) under the
//               name while the entry is transferring
//   remove      a real `MdIconButton` (standard / xsmall, `close`) at the
//               trailing edge — the icon-button anchor
//
// Nothing here invents a colour token: the row chrome is the List family's,
// the progress bar is the progress-indicator family's, and the remove control
// is a real Stage 1 IconButton so its ripple / state layer / focus ring are
// the published ones.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

class MdIconButton;
class MdProgressIndicator;

/// One queued file. A plain value type so the `files` property can round-trip
/// through QVariant without a metatype of its own beyond the list.
struct QT_MD3_EXPORT MdUploadFile
{
    QString name;
    /// 0..1 transfer progress. `>= 1` means done.
    qreal progress = 0.0;
    /// Transfer size in bytes, for the secondary line. 0 hides it.
    qint64 sizeBytes = 0;
    /// True while the entry is still transferring.
    bool transferring = false;

    bool operator==(const MdUploadFile &other) const
    {
        return name == other.name
               && (qFuzzyIsNull(progress - other.progress))
               && sizeBytes == other.sizeBytes
               && transferring == other.transferring;
    }
    bool operator!=(const MdUploadFile &other) const { return !(*this == other); }
};

class QT_MD3_EXPORT MdUpload : public QWidget
{
    Q_OBJECT

    /// The queued files, in display order.
    Q_PROPERTY(QList<md::MdUploadFile> files READ files WRITE setFiles NOTIFY filesChanged)
    /// Maximum visible rows before the list clips. 0 means unbounded.
    Q_PROPERTY(int maxVisibleRows READ maxVisibleRows WRITE setMaxVisibleRows
                   NOTIFY maxVisibleRowsChanged)

public:
    explicit MdUpload(QWidget *parent = nullptr);
    ~MdUpload() override;

    QList<MdUploadFile> files() const { return m_files; }
    void setFiles(const QList<MdUploadFile> &files);

    int maxVisibleRows() const { return m_maxVisibleRows; }
    void setMaxVisibleRows(int rows);

    /// Append a file to the queue.
    void addFile(const QString &name, qint64 sizeBytes = 0);
    /// Update one entry's progress (and transferring flag). Index is 0-based.
    void setFileProgress(int index, qreal progress, bool transferring = true);
    /// Remove one entry. Emits `fileRemoved`.
    void removeFile(int index);
    void clearFiles();

    int fileCount() const { return m_files.size(); }

    /// Row height — the List one-line height, 56.
    static qreal rowHeight();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void filesChanged(const QList<md::MdUploadFile> &files);
    void maxVisibleRowsChanged(int rows);
    /// Emitted when the trailing remove button deletes `index`.
    void fileRemoved(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onRemoveClicked();

private:
    void init();
    void rebuildRows();
    void placeRows();
    void clearRowWidgets();

    QList<MdUploadFile> m_files;
    int m_maxVisibleRows = 0;

    QList<MdIconButton *> m_removeButtons;
    QList<MdProgressIndicator *> m_progressBars;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdUploadFile)
Q_DECLARE_METATYPE(QList<md::MdUploadFile>)

#endif // MD_UPLOAD_H
