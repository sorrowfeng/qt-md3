#ifndef MD_DESCRIPTIONS_H
#define MD_DESCRIPTIONS_H

// MdDescriptions — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "键值列表，label / body typography 对比" —
// a key/value list whose contrast comes from the type scale alone.
//
// MD3 has no descriptions family. The control is the standard detail grid
// every Material detail surface eventually needs, composed from published
// families only:
//
//   label   LabelMedium (md.sys.typescale.label-medium) on
//           on-surface-variant (md.sys.color.on-surface-variant)
//   value   BodyMedium (md.sys.typescale.body-medium) on
//           on-surface (md.sys.color.on-surface)
//   border  outline-variant (md.sys.color.outline-variant), optional
//   row     List one-line metrics (md.comp.list.list-item.*): 16 px
//           horizontal inset, 16 px vertical padding floor
//
// Layout: Vertical (label above value) or Horizontal (label left of value),
// 1..N columns. Rows are value objects; the widget paints itself.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtGui/QColor>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

/// One label / value pair in the list.
struct MdDescriptionItem
{
    QString label;
    QString value;

    MdDescriptionItem() = default;
    MdDescriptionItem(const QString &label_, const QString &value_)
        : label(label_)
        , value(value_)
    {
    }

    bool operator==(const MdDescriptionItem &other) const
    {
        return label == other.label && value == other.value;
    }
};

/// How each item stacks its label and value.
enum class MdDescriptionsLayout {
    /// Label above value (default; matches Material detail lists).
    Vertical,
    /// Label left of value.
    Horizontal,
    Count,
};

class QT_MD3_EXPORT MdDescriptions : public QWidget
{
    Q_OBJECT

    /// Label / value pairs, top-to-bottom (RTL mirrors columns).
    Q_PROPERTY(QList<md::MdDescriptionItem> items READ items WRITE setItems NOTIFY itemsChanged)
    /// Vertical (label above value) or Horizontal (label left of value).
    Q_PROPERTY(md::MdDescriptionsLayout layout READ layout WRITE setLayout NOTIFY layoutChanged)
    /// Number of item columns. 1 stacks every item; >1 flows them.
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    /// Draw an outline-variant hairline under every row.
    Q_PROPERTY(bool bordered READ isBordered WRITE setBordered NOTIFY borderedChanged)
    /// Extra gap between rows, in logical px (added to the row padding floor).
    Q_PROPERTY(qreal rowSpacing READ rowSpacing WRITE setRowSpacing NOTIFY rowSpacingChanged)

public:
    explicit MdDescriptions(QWidget *parent = nullptr);
    ~MdDescriptions() override;

    QList<MdDescriptionItem> items() const { return m_items; }
    void setItems(const QList<MdDescriptionItem> &items);

    /// Convenience mutators that notify like the property.
    void addItem(const QString &label, const QString &value);
    void clearItems();

    MdDescriptionsLayout layout() const { return m_layout; }
    void setLayout(MdDescriptionsLayout layout);

    int columns() const { return m_columns; }
    void setColumns(int columns);

    bool isBordered() const { return m_bordered; }
    void setBordered(bool bordered);

    qreal rowSpacing() const { return m_rowSpacing; }
    void setRowSpacing(qreal spacing);

    /// Resolved label colour: on-surface-variant.
    QColor resolvedLabelColor() const;
    /// Resolved value colour: on-surface.
    QColor resolvedValueColor() const;
    /// Resolved border colour: outline-variant.
    QColor resolvedBorderColor() const;

    int rowCount() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

signals:
    void itemsChanged(const QList<md::MdDescriptionItem> &items);
    void layoutChanged(md::MdDescriptionsLayout layout);
    void columnsChanged(int columns);
    void borderedChanged(bool bordered);
    void rowSpacingChanged(qreal spacing);

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    struct CellGeom
    {
        QRectF label;
        QRectF value;
        qreal height = 0.0;
    };
    CellGeom cellGeom(const QRectF &cell, const MdDescriptionItem &item) const;
    QSizeF measure(int width) const;

    QList<MdDescriptionItem> m_items;
    MdDescriptionsLayout m_layout = MdDescriptionsLayout::Vertical;
    int m_columns = 1;
    bool m_bordered = false;
    qreal m_rowSpacing = 0.0;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdDescriptionItem)
Q_DECLARE_METATYPE(md::MdDescriptionsLayout)
Q_DECLARE_METATYPE(QList<md::MdDescriptionItem>)

#endif // MD_DESCRIPTIONS_H
