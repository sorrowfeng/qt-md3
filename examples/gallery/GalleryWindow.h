#ifndef GALLERY_WINDOW_H
#define GALLERY_WINDOW_H

// The gallery shell: a hand-painted navigation rail, a hand-painted control
// strip, and a stack of pages.
//
// Example code, not library code. It exists to make the base modules and the
// token tables inspectable, and it deliberately uses the primitives under test
// (ripple, focus ring, state layers, type scale, shape tokens) rather than any
// widget styling API.

#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdTypes.h"

#include <QtCore/QVector>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>

class QStackedWidget;

namespace gallery {

class GalleryPage;

/// The left navigation rail. Draws its own items, handles hover / press /
/// selection, and shows a ripple on the item being pressed.
class GalleryNav : public QWidget
{
    Q_OBJECT

public:
    explicit GalleryNav(QWidget *parent = nullptr);

    void setItems(const QStringList &titles);
    int currentIndex() const { return m_current; }
    void setCurrentIndex(int index);

signals:
    void currentChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QRectF itemRect(int index) const;
    int itemAt(const QPointF &position) const;

    QStringList m_titles;
    QVector<md::MdRippleController *> m_ripples;
    int m_current = 0;
    int m_hovered = -1;
    int m_pressed = -1;
};

/// The top strip: product name, page title, and the theme controls.
class GalleryHeader : public QWidget
{
    Q_OBJECT

public:
    explicit GalleryHeader(QWidget *parent = nullptr);

    void setTitle(const QString &title);

signals:
    void themeModeToggled();
    void dynamicColorToggled();
    void variantCycled();
    void contrastCycled();
    void densityCycled();
    void directionToggled();
    void languageCycled();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct Button
    {
        QRectF rect;
        QString label;
        int action = 0;
        QString icon;
    };

    void rebuildButtons();
    int buttonAt(const QPointF &position) const;

    QString m_title;
    QVector<Button> m_buttons;
    int m_hovered = -1;
    int m_pressed = -1;
};

class GalleryWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GalleryWindow(QWidget *parent = nullptr);

    /// Number of pages in the stack, in navigation order.
    int pageCount() const;
    /// Title of page `index`, or an empty string when out of range.
    QString pageTitle(int index) const;
    /// Select a page by index. Out-of-range values are ignored.
    void setCurrentPage(int index);
    /// Index of the visible page.
    int currentPage() const;

private slots:
    void onThemeChanged();

private:
    GalleryNav *m_nav = nullptr;
    GalleryHeader *m_header = nullptr;
    QStackedWidget *m_stack = nullptr;
    QVector<GalleryPage *> m_pages;
};

} // namespace gallery

#endif // GALLERY_WINDOW_H
