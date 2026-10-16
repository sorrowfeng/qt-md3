#ifndef MD_TRANSFER_H
#define MD_TRANSFER_H

// MdTransfer — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "List × 2 + Buttons" — a dual-list shuttle.
//
// MD3 has no transfer family. The control is the standard "move items
// between two sets" composition, assembled from published families:
//
//   panels    List one-line rows (md.comp.list.list-item.*): 56 px floor,
//             TitleMedium headline, hover 0.08, selected secondary-container
//             (md.sys.color.secondary-container)
//   chrome    surface-container (md.sys.color.surface-container) band
//   actions   Text IconButton pair (`md.comp.icon-button.*`) in the middle
//             gutter: chevron_right moves right, chevron_left moves left
//   titles    LabelLarge (md.sys.typescale.label-large) on
//             on-surface-variant (md.sys.color.on-surface-variant)
//
// Value model (`MdTransferItem` rows) rather than item widgets, matching the
// other Stage 2 value APIs. Space / Enter on a focused row toggles
// selection; the arrow buttons move every selected row across.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtWidgets/QWidget>

class QPaintEvent;
class QKeyEvent;
class QMouseEvent;

namespace md {

class MdButton;

/// One shuttle row.
struct MdTransferItem
{
    QString id;
    QString title;
    QString supporting;

    MdTransferItem() = default;
    MdTransferItem(const QString &id_, const QString &title_,
                   const QString &supporting_ = QString())
        : id(id_)
        , title(title_)
        , supporting(supporting_)
    {
    }

    bool operator==(const MdTransferItem &other) const
    {
        return id == other.id && title == other.title
                && supporting == other.supporting;
    }
};

class QT_MD3_EXPORT MdTransfer : public QWidget
{
    Q_OBJECT

    /// Title of the source panel (LabelLarge).
    Q_PROPERTY(QString sourceTitle READ sourceTitle WRITE setSourceTitle NOTIFY sourceTitleChanged)
    /// Title of the target panel (LabelLarge).
    Q_PROPERTY(QString targetTitle READ targetTitle WRITE setTargetTitle NOTIFY targetTitleChanged)
    /// Items still in the source panel.
    Q_PROPERTY(QList<md::MdTransferItem> sourceItems READ sourceItems WRITE setSourceItems
                   NOTIFY sourceItemsChanged)
    /// Items already moved to the target panel.
    Q_PROPERTY(QList<md::MdTransferItem> targetItems READ targetItems WRITE setTargetItems
                   NOTIFY targetItemsChanged)
    /// Allow multi-select in both panels (default true).
    Q_PROPERTY(bool multiSelect READ multiSelect WRITE setMultiSelect NOTIFY multiSelectChanged)

public:
    explicit MdTransfer(QWidget *parent = nullptr);
    ~MdTransfer() override;

    QString sourceTitle() const { return m_sourceTitle; }
    void setSourceTitle(const QString &title);

    QString targetTitle() const { return m_targetTitle; }
    void setTargetTitle(const QString &title);

    QList<MdTransferItem> sourceItems() const { return m_sourceItems; }
    void setSourceItems(const QList<MdTransferItem> &items);

    QList<MdTransferItem> targetItems() const { return m_targetItems; }
    void setTargetItems(const QList<MdTransferItem> &items);

    bool multiSelect() const { return m_multiSelect; }
    void setMultiSelect(bool multi);

    /// Selected source / target row indices (stable across moves).
    QList<int> selectedSourceRows() const { return m_sourceSelection; }
    QList<int> selectedTargetRows() const { return m_targetSelection; }

    void selectSourceRow(int row, bool selected = true);
    void selectTargetRow(int row, bool selected = true);
    void clearSelection();

    /// Move every selected source row to the target panel.
    void moveSelectedToTarget();
    /// Move every selected target row back to the source panel.
    void moveSelectedToSource();

    /// Resolved selected-row background: secondary-container.
    QColor resolvedSelectedBackground() const;
    /// Resolved selected-row content: on-secondary-container.
    QColor resolvedSelectedContent() const;
    /// Resolved panel chrome: surface-container.
    QColor resolvedPanelColor() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void sourceTitleChanged(const QString &title);
    void targetTitleChanged(const QString &title);
    void sourceItemsChanged(const QList<md::MdTransferItem> &items);
    void targetItemsChanged(const QList<md::MdTransferItem> &items);
    void multiSelectChanged(bool multi);
    /// Emitted after rows land in the target panel.
    void movedToTarget(const QList<md::MdTransferItem> &items);
    /// Emitted after rows return to the source panel.
    void movedToSource(const QList<md::MdTransferItem> &items);
    void selectionChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onMoveToTarget();
    void onMoveToSource();

private:
    void init();
    void layoutButtons();
    QRectF sourcePanelRect() const;
    QRectF targetPanelRect() const;
    QRectF rowRect(bool source, int row, const QRectF &panel) const;
    int rowAt(bool source, const QPointF &pos) const;
    void toggleRow(bool source, int row);

    QString m_sourceTitle;
    QString m_targetTitle;
    QList<MdTransferItem> m_sourceItems;
    QList<MdTransferItem> m_targetItems;
    QList<int> m_sourceSelection;
    QList<int> m_targetSelection;
    bool m_multiSelect = true;
    bool m_focusInSource = true;
    int m_focusRow = 0;
    MdButton *m_toTarget = nullptr;
    MdButton *m_toSource = nullptr;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdTransferItem)
Q_DECLARE_METATYPE(QList<md::MdTransferItem>)

#endif // MD_TRANSFER_H
