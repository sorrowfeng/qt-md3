#ifndef MD_NAV_H
#define MD_NAV_H

// MdNav / MdNavItem — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "复用 Navigation rail / drawer 的 item 语义".
//
// A generic destination list that reuses the navigation-drawer item
// language without being a drawer: full-width 56 px rows, leading 24 px
// Material Symbols glyph, label-large caption, secondary-container
// selection, and the drawer item's 16 / 24 / 12 / 24 content inset.
//
// MdNavItem is the row. MdNav is the column of rows (or the horizontal
// strip when `orientation` is horizontal). Selection is single-index, the
// same contract MdNavigationDrawer publishes.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

class MdNavItem;

/// Layout axis of an MdNav.
enum class MdNavOrientation {
    Vertical,
    Horizontal,
    Count,
};

/// One destination row: glyph + label, optional badge text.
class QT_MD3_EXPORT MdNavItem : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    Q_PROPERTY(QString badgeText READ badgeText WRITE setBadgeText NOTIFY badgeTextChanged)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)
    Q_PROPERTY(bool itemEnabled READ isItemEnabled WRITE setItemEnabled NOTIFY itemEnabledChanged)

public:
    explicit MdNavItem(QWidget *parent = nullptr);
    explicit MdNavItem(const QString &text, const QString &iconName = QString(),
                       QWidget *parent = nullptr);
    ~MdNavItem() override;

    QString text() const { return m_text; }
    void setText(const QString &text);

    QString iconName() const { return m_iconName; }
    void setIconName(const QString &name);

    QString badgeText() const { return m_badgeText; }
    void setBadgeText(const QString &text);

    bool isSelected() const { return m_selected; }
    void setSelected(bool selected);

    bool isItemEnabled() const { return m_itemEnabled; }
    void setItemEnabled(bool enabled);

    QColor resolvedContentColor() const;
    QColor resolvedSelectedBackground() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void textChanged(const QString &text);
    void iconNameChanged(const QString &name);
    void badgeTextChanged(const QString &text);
    void selectedChanged(bool selected);
    void itemEnabledChanged(bool enabled);
    void clicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void init();

    QString m_text;
    QString m_iconName;
    QString m_badgeText;
    bool m_selected = false;
    bool m_itemEnabled = true;
    bool m_hovered = false;
    bool m_pressed = false;
};

/// The destination list itself.
class QT_MD3_EXPORT MdNav : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(md::MdNavOrientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged)
    Q_PROPERTY(bool itemLabelsVisible READ itemLabelsVisible WRITE setItemLabelsVisible NOTIFY
                   itemLabelsVisibleChanged)

public:
    explicit MdNav(QWidget *parent = nullptr);
    ~MdNav() override;

    void addItem(MdNavItem *item);
    void insertItem(int index, MdNavItem *item);
    void removeItem(MdNavItem *item);
    void clearItems();
    QList<MdNavItem *> items() const { return m_items; }
    MdNavItem *itemAt(int index) const;
    int indexOf(const MdNavItem *item) const;
    int count() const { return int(m_items.size()); }

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    MdNavOrientation orientation() const { return m_orientation; }
    void setOrientation(MdNavOrientation orientation);

    /// When false the labels are hidden and only the glyphs show (rail mode).
    bool itemLabelsVisible() const { return m_labelsVisible; }
    void setItemLabelsVisible(bool visible);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void currentIndexChanged(int index);
    void orientationChanged(md::MdNavOrientation orientation);
    void itemLabelsVisibleChanged(bool visible);
    void itemClicked(int index);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onItemClick();

private:
    void init();
    void relayout();
    void syncSelection();

    QList<MdNavItem *> m_items;
    int m_currentIndex = -1;
    MdNavOrientation m_orientation = MdNavOrientation::Vertical;
    bool m_labelsVisible = true;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdNavOrientation)

#endif // MD_NAV_H
