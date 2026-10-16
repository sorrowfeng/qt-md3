#ifndef MD_SCROLL_AREA_H
#define MD_SCROLL_AREA_H

// MdScrollArea — Stage 2 Qt extension (Appendix D, P0).
//
// MD3 has no scroll-area family; the appendix D row is "compose MdScrollBar".
// This control is therefore a QScrollArea whose scrollbars are MdScrollBar
// instances (the derived overlay chrome: on-surface pill thumb at the
// 0.38 / 0.46 / 0.54 ladder, no arrow buttons) and whose viewport surface
// is the theme's `md.sys.color.surface`.
//
// Behaviour the stock QScrollArea already owns is kept: wheel scrolling,
// keyboard scrolling (arrows / PageUp / PageDown / Home / End), widget
// resizing, focus handling, and the size policy of the contained widget.
// What changes is the chrome and the property surface:
//
//   * both bars are MdScrollBar, thickness / visibility mode configurable
//     from one place;
//   * the frame is off — MD3 surfaces have no inset border; the scrollable
//     content sits directly on the surface;
//   * theme changes recolour the bars and the viewport through the usual
//     `MdStyleBase::connectThemeUpdate` subscription.
//
// There is no QSS. The bars carry MdScrollBarStyle; this class only
// composes them.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"
#include "widgets/MdScrollBar.h"

#include <QtWidgets/QScrollArea>

namespace md {

class QT_MD3_EXPORT MdScrollArea : public QScrollArea
{
    Q_OBJECT

    /// Visible overlay handle width applied to both bars. Clamped to the
    /// MdScrollBar range [4, 24].
    Q_PROPERTY(int barThickness READ barThickness WRITE setBarThickness NOTIFY barThicknessChanged)
    /// When the overlay chrome is painted on both bars.
    Q_PROPERTY(md::MdScrollBarVisibility barVisibleMode READ barVisibleMode WRITE
                   setBarVisibleMode NOTIFY barVisibleModeChanged)
    /// True while either bar is hovered or dragged.
    Q_PROPERTY(bool anyBarLifted READ isAnyBarLifted NOTIFY anyBarLiftedChanged)

public:
    explicit MdScrollArea(QWidget *parent = nullptr);
    ~MdScrollArea() override;

    // --- scrollbar chrome ---------------------------------------------------
    int barThickness() const;
    void setBarThickness(int thickness);

    MdScrollBarVisibility barVisibleMode() const;
    void setBarVisibleMode(MdScrollBarVisibility mode);

    bool isAnyBarLifted() const;

    /// The composed bars. Non-null after construction.
    MdScrollBar *verticalMdScrollBar() const { return m_vBar; }
    MdScrollBar *horizontalMdScrollBar() const { return m_hBar; }

signals:
    void barThicknessChanged(int thickness);
    void barVisibleModeChanged(md::MdScrollBarVisibility mode);
    void anyBarLiftedChanged(bool lifted);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onBarLiftedChanged();
    void onThemeChanged();

private:
    void init();
    void applyChrome();
    void syncLifted();

    MdScrollBar *m_vBar = nullptr;
    MdScrollBar *m_hBar = nullptr;
    int m_barThickness = 8;
    MdScrollBarVisibility m_barVisibleMode = MdScrollBarVisibility::Auto;
    bool m_anyLifted = false;
};

} // namespace md

#endif // MD_SCROLL_AREA_H
