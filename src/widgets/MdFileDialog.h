#ifndef MD_FILE_DIALOG_H
#define MD_FILE_DIALOG_H

// MdFileDialog — Stage 2 Qt extension (Appendix D, P0), the file picker.
//
// Semantic anchor (appendix D): "Dialog + Navigation drawer（左侧位置）+
// List（文件列表）+ Text field". This is a composition widget, not a
// QFileDialog wrapper: every surface is a Stage 1 token set, and the
// control can be embedded (gallery, tests) or hosted in a top-level window
// exactly like MdMessageBox.
//
//   chrome      md.comp.dialog.* (export 34.0.21) — surface-container-high,
//               elevation level 3, corner extra-large; the same MdDialogTokens
//               MdMessageBox paints with
//   locations   the left column is a Navigation drawer rail: label-large rows,
//               56 px, selected = secondary-container + on-secondary-container
//               (the drawer item's selected state)
//   file list   List row semantics (the MdTable / MdList rules): one-line
//               height 56, hover 0.08, selected secondary-container,
//               outline-variant dividers, leading 24 px icon
//   path bar    filled MdTextField (`md.comp.filled-text-field`)
//   name bar    filled MdTextField for the accepted name
//   actions     MdButton Filled (accept) + Text (cancel), end-aligned FlowRow
//
// File-system access goes through QDir / QFileInfo so the dialog is a real
// picker; the *design* is the composition above. The dialog never opens a
// modal event loop by itself — `exec()` is provided for the QMessageBox
// style but the widget is equally usable modeless.
//
// There is no QSS.

#include "core/MdDialogTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtWidgets/QWidget>

class QEvent;
class QKeyEvent;
class QPaintEvent;

namespace md {

class MdButton;
class MdIconButton;
class MdTextField;

/// One entry in the file list.
struct QT_MD3_EXPORT MdFileEntry
{
    QString name;
    QString absolutePath;
    bool isDir = false;
    qint64 size = 0;
    QDateTime modified;

    /// Icon name for the leading slot: folder / description / insert_drive_file.
    QString iconName() const;
};

/// What the dialog is asking for.
enum class MdFileDialogMode {
    /// Pick an existing file (or enter a folder). Accept = Open.
    OpenFile,
    /// Pick an existing folder. Accept = Select Folder.
    OpenDirectory,
    /// Pick a name to write. Accept = Save.
    SaveFile,
    Count,
};

/// One entry in the left location column (navigation-drawer rail).
struct QT_MD3_EXPORT MdFileLocation
{
    QString label;
    QString iconName;
    QString path;
};

class QT_MD3_EXPORT MdFileDialog : public QWidget
{
    Q_OBJECT

    /// The dialog headline (headline-small, on-surface).
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// The working directory currently listed.
    Q_PROPERTY(QString directory READ directory WRITE setDirectory NOTIFY directoryChanged)
    /// The name currently typed in the name field (not a full path).
    Q_PROPERTY(QString selectedName READ selectedName WRITE setSelectedName NOTIFY
                   selectedNameChanged)
    /// Whether the file list shows hidden entries.
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)
    /// Which accept semantics the dialog runs (Open file / folder / Save).
    Q_PROPERTY(md::MdFileDialogMode mode READ mode WRITE setMode NOTIFY modeChanged)
    /// Name filters, `"Images (*.png *.jpg)"` style. Empty shows everything.
    Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters NOTIFY
                   nameFiltersChanged)
    /// Index into `nameFilters`, or -1 for no filter.
    Q_PROPERTY(int currentFilterIndex READ currentFilterIndex WRITE setCurrentFilterIndex NOTIFY
                   currentFilterIndexChanged)
    /// Index of the highlighted file-list row, or -1.
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    explicit MdFileDialog(QWidget *parent = nullptr);
    ~MdFileDialog() override;

    // --- content -------------------------------------------------------------
    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString directory() const { return m_directory; }
    void setDirectory(const QString &path);

    QString selectedName() const { return m_selectedName; }
    void setSelectedName(const QString &name);

    /// Full path of the current selection: directory + selectedName when a
    /// name is typed, else the highlighted row's absolute path.
    QString selectedFilePath() const;

    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool show);

    MdFileDialogMode mode() const { return m_mode; }
    void setMode(MdFileDialogMode mode);

    QStringList nameFilters() const { return m_nameFilters; }
    void setNameFilters(const QStringList &filters);

    int currentFilterIndex() const { return m_filterIndex; }
    void setCurrentFilterIndex(int index);

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    /// The rows currently listed, after filters.
    QList<MdFileEntry> entries() const { return m_entries; }

    /// Re-read the working directory. Called automatically by setDirectory().
    void refresh();

    /// Navigate into the highlighted directory, or accept the highlighted
    /// file. Equivalent to double-click.
    void activateCurrent();

    // --- presentation --------------------------------------------------------
    /// The painted dialog container, in widget coordinates.
    QRectF containerRect() const;

    /// Show and run a local event loop until accept/reject. Returns 1 on
    /// accept, 0 on reject.
    int exec();

signals:
    void titleChanged(const QString &title);
    void directoryChanged(const QString &directory);
    void selectedNameChanged(const QString &name);
    void showHiddenChanged(bool show);
    void modeChanged(md::MdFileDialogMode mode);
    void nameFiltersChanged(const QStringList &filters);
    void currentFilterIndexChanged(int index);
    void currentIndexChanged(int index);
    void entriesChanged();
    /// The user accepted. `path` is selectedFilePath().
    void accepted(const QString &path);
    /// The user cancelled or closed the dialog.
    void rejected();
    void fileActivated(const QString &path);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onAcceptClicked();
    void onRejectClicked();
    void onUpClicked();
    void onNewFolderClicked();
    void onPathEditingFinished();
    void onNameEditingFinished();
    void onThemeChanged();

private:
    void init();
    void layoutChildren();
    void rebuildEntries();
    bool rowMatchesFilter(const QFileInfo &info) const;
    int rowAt(const QPointF &pos) const;
    int locationAt(const QPointF &pos) const;
    QRectF listRect() const;
    QRectF locationsRect() const;
    void goUp();
    void acceptPath(const QString &path);
    QString labelTextForMode() const;

    MdDialogTokens m_tokens;
    QString m_title;
    QString m_directory;
    QString m_selectedName;
    QStringList m_nameFilters;
    int m_filterIndex = -1;
    int m_currentIndex = -1;
    int m_hoveredIndex = -1;
    int m_hoveredLocation = -1;
    int m_locationIndex = 0;
    bool m_showHidden = false;
    MdFileDialogMode m_mode = MdFileDialogMode::OpenFile;

    QList<MdFileEntry> m_entries;
    QList<MdFileLocation> m_locations;

    MdTextField *m_pathField = nullptr;
    MdTextField *m_nameField = nullptr;
    MdIconButton *m_upButton = nullptr;
    MdIconButton *m_newFolderButton = nullptr;
    MdButton *m_acceptButton = nullptr;
    MdButton *m_rejectButton = nullptr;
};

} // namespace md

#endif // MD_FILE_DIALOG_H
