#ifndef MD_BREADCRUMB_H
#define MD_BREADCRUMB_H

// MdBreadcrumb — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "Label typography + separators".
//
// A horizontal trail of destinations. MD3 has no breadcrumb family, so the
// widget is composed from the type scale and the list/menu content colours:
//
//   crumb     label-large in on-surface-variant; the current (last) crumb
//             is label-large in on-surface and is not clickable
//   separator a Material Symbols `chevron_right` in on-surface-variant
//             (mirrored to `chevron_left` in RTL), 16 px optical size
//
// Clicking a crumb emits `crumbClicked(index)` and selects it. The widget
// paints itself; crumbs are hit-tested against painted label boxes.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QStringList>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

class QT_MD3_EXPORT MdBreadcrumb : public QWidget
{
    Q_OBJECT

    /// Crumb labels, left to right (or right to left in RTL).
    Q_PROPERTY(QStringList items READ items WRITE setItems NOTIFY itemsChanged)
    /// Index of the current (last, non-interactive) crumb. -1 means every
    /// crumb is interactive.
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    /// Material Symbols name used as the separator. Default `chevron_right`.
    Q_PROPERTY(QString separatorIconName READ separatorIconName WRITE setSeparatorIconName NOTIFY
                   separatorIconNameChanged)

public:
    explicit MdBreadcrumb(QWidget *parent = nullptr);
    ~MdBreadcrumb() override;

    QStringList items() const { return m_items; }
    void setItems(const QStringList &items);
    void addItem(const QString &label);
    void clearItems();

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    QString separatorIconName() const { return m_separatorIcon; }
    void setSeparatorIconName(const QString &name);

    int count() const { return int(m_items.size()); }

    /// Hit-test the painted crumb labels. -1 when `pos` is not on a crumb.
    int crumbAt(const QPointF &pos) const;

    /// Resolved colour of an ordinary (non-current) crumb label.
    QColor resolvedItemColor() const;
    /// Resolved colour of the current crumb label.
    QColor resolvedCurrentColor() const;
    /// Resolved separator colour.
    QColor resolvedSeparatorColor() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

signals:
    void itemsChanged(const QStringList &items);
    void currentIndexChanged(int index);
    void separatorIconNameChanged(const QString &name);
    /// A crumb (including the current one) was clicked.
    void crumbClicked(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    struct LayoutRow
    {
        QList<QRectF> crumbs;
        QList<QRectF> separators;
        qreal height = 0.0;
    };
    LayoutRow measure(int width) const;

    QStringList m_items;
    int m_currentIndex = -1;
    QString m_separatorIcon = QStringLiteral("chevron_right");
};

} // namespace md

#endif // MD_BREADCRUMB_H
