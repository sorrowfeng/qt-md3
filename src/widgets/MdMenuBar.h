#ifndef MD_MENU_BAR_H
#define MD_MENU_BAR_H

// MdMenuBar — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "Menu 语义横向化".
// MD3 has no menu-bar family; this is the classic application menu strip
// rebuilt from Stage 1 Menu semantics laid out horizontally:
//
//   surface     md.sys.color.surface (the bar itself is not a card)
//   item label  label-large / on-surface
//   hover       state layer 0.08 on on-surface
//   open item   state layer 0.12 (the "active" reading while a popup owns it)
//   popup       a real `MdMenu` (md.comp.menu.*) anchored under the item
//
// Interaction matches QMenuBar's mental model and the Menu family's keys:
//   * click / Space / Enter / Down opens the item's MdMenu under it
//   * Left / Right move the highlighted item (wrapping)
//   * Esc closes the open menu and returns focus to the bar
//   * opening one menu and moving to another swaps the popup without an
//     extra click (menu-bar mode, not menu mode)
//
// The bar paints only its own surface and the item state layers; every
// popup pixel is `MdMenu`'s Stage 1 painting. No QSS.
//
// Qt 5 portable: mouse tracking goes through `MdQtCompat::mousePosition`,
// no `QFontDatabase` statics, no `M_PI`.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtWidgets/QWidget>

namespace md {

class MdMenu;

class QT_MD3_EXPORT MdMenuBar : public QWidget
{
    Q_OBJECT

    /// Item titles, start to end. Menus are attached by index.
    Q_PROPERTY(QStringList items READ items WRITE setItems NOTIFY itemsChanged)
    /// Index of the highlighted / open item. -1 when nothing is active.
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    /// Index of the item whose MdMenu is currently open. -1 when closed.
    Q_PROPERTY(int openIndex READ openIndex NOTIFY openIndexChanged)
    /// Preferred bar height. 0 means the label-large row plus padding.
    Q_PROPERTY(int barHeight READ barHeight WRITE setBarHeight NOTIFY barHeightChanged)
    /// Background override. An invalid colour means the surface token.
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
                   backgroundColorChanged)

public:
    explicit MdMenuBar(QWidget *parent = nullptr);
    ~MdMenuBar() override;

    // --- items --------------------------------------------------------------
    QStringList items() const { return m_titles; }
    void setItems(const QStringList &titles);
    int itemCount() const { return m_titles.size(); }

    /// Append an item and return its index. The item has no menu until
    /// setMenu() is called; clicking it still highlights and emits
    /// itemTriggered().
    int addItem(const QString &title);
    void insertItem(int index, const QString &title);
    void removeItem(int index);
    void clearItems();

    QString itemText(int index) const;
    void setItemText(int index, const QString &title);

    bool isItemEnabled(int index) const;
    void setItemEnabled(int index, bool enabled);

    /// Attach a popup menu to an item. The bar does not take ownership of
    /// the MdMenu; the caller parents it (or leaves it parentless — the bar
    /// will parent it to itself on install).
    void setMenu(int index, MdMenu *menu);
    MdMenu *menu(int index) const;

    // --- state --------------------------------------------------------------
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    int openIndex() const { return m_openIndex; }

    int barHeight() const { return m_barHeight; }
    void setBarHeight(int height);

    QColor backgroundColor() const { return m_backgroundOverride; }
    void setBackgroundColor(const QColor &color);

    /// Resolved background: the override when set, else surface.
    QColor resolvedBackgroundColor() const;
    /// Resolved label colour: on-surface.
    QColor resolvedContentColor() const;

    /// Geometry of one item's label box, in widget coordinates. Empty for
    /// out-of-range indices. Used by tests and by MdMenu anchoring.
    QRect itemRect(int index) const;

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    /// Open the menu of `index` (or emit itemTriggered when the item has none).
    void openItem(int index);
    /// Close any open popup and clear `openIndex`.
    void closeMenu();

signals:
    void itemsChanged(const QStringList &items);
    void currentIndexChanged(int index);
    void openIndexChanged(int index);
    void barHeightChanged(int height);
    void backgroundColorChanged(const QColor &color);
    /// An item without a menu was activated (click / Space / Enter).
    void itemTriggered(int index, const QString &title);
    /// The item's MdMenu was opened.
    void menuOpened(int index, md::MdMenu *menu);
    /// The open MdMenu was closed.
    void menuClosed(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onMenuClosed();

private:
    void init();
    void setHoverIndex(int index);
    void setOpenIndex(int index);
    int indexAt(const QPoint &pos) const;
    void paintItem(QPainter *painter, int index) const;

    QStringList m_titles;
    QList<MdMenu *> m_menus;
    QList<bool> m_enabled;
    int m_currentIndex = -1;
    int m_hoverIndex = -1;
    int m_openIndex = -1;
    int m_barHeight = 0;
    QColor m_backgroundOverride;
    bool m_focusIsKeyboard = false;
};

} // namespace md

// Incomplete QObject subclass in menuOpened.
// Opaque + registered: either macro alone leaves the pointer unusable on Qt 5.
Q_DECLARE_OPAQUE_POINTER(md::MdMenu *)
Q_DECLARE_METATYPE(md::MdMenu *)

#endif // MD_MENU_BAR_H
