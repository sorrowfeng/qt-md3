#ifndef MD_BACK_TOP_H
#define MD_BACK_TOP_H

// MdBackTop — Stage 2 Qt extension (Appendix D, P3).
//
// Semantic anchor (appendix D): "FAB 的 small variant" — a small FAB that
// appears once a scroll surface has moved past a threshold, and scrolls it
// back to the top when activated.
//
// MD3 has no back-top family. The control is the FAB family's small size
// (`md.comp.fab.small.*`: 40 px container, corner-medium) carrying the
// `arrow_upward` glyph, plus a visibility rule:
//
//   resting   hidden until the watched scroll area's value exceeds
//             `visibilityThreshold` (default 320 px — past one viewport
//             on the compact size class)
//   shown     a real MdFab (FabSize::Small, FabVariant::Surface) with the
//             Stage 1 ripple / state layer / `:focus-visible` ring
//   activate  scrolls the watched surface to 0 and emits `clicked`
//
// The widget parents a real MdFab so the interactive surface is the Stage 1
// one and cannot drift. `:focus-visible` is MdFab's own ring.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QAbstractScrollArea>
#include <QtWidgets/QWidget>

namespace md {

class MdFab;

class QT_MD3_EXPORT MdBackTop : public QWidget
{
    Q_OBJECT

    /// Scroll offset that reveals the button (logical px, default 320).
    Q_PROPERTY(int visibilityThreshold READ visibilityThreshold WRITE setVisibilityThreshold
                   NOTIFY visibilityThresholdChanged)
    /// Icon on the FAB (`arrow_upward` by default).
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    /// Visible when the watched scroll area is past the threshold.
    Q_PROPERTY(bool buttonVisible READ isButtonVisible NOTIFY buttonVisibleChanged)
    /// The scroll surface being watched (may be null).
    Q_PROPERTY(QAbstractScrollArea *scrollArea READ scrollArea WRITE setScrollArea
                   NOTIFY scrollAreaChanged)

public:
    explicit MdBackTop(QWidget *parent = nullptr);
    ~MdBackTop() override;

    int visibilityThreshold() const { return m_threshold; }
    void setVisibilityThreshold(int pixels);

    QString iconName() const;
    void setIconName(const QString &name);

    bool isButtonVisible() const;

    QAbstractScrollArea *scrollArea() const { return m_scrollArea; }
    void setScrollArea(QAbstractScrollArea *area);

    /// The underlying small FAB (owned; read-only for tests).
    MdFab *fab() const { return m_fab; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    /// Scroll the watched surface to the top and emit `clicked()`.
    void scrollToTop();
    /// Re-evaluate visibility against the scroll area's current offset.
    void refresh();

signals:
    void visibilityThresholdChanged(int pixels);
    void iconNameChanged(const QString &name);
    void buttonVisibleChanged(bool visible);
    void scrollAreaChanged(QAbstractScrollArea *area);
    /// Emitted after an activation (click / Space / Enter).
    void clicked();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onThemeChanged();
    void onFabClicked();
    void onScrollChanged();

private:
    void init();
    void layoutFab();
    void setButtonVisible(bool visible);
    void hookScrollArea();
    void unhookScrollArea();

    int m_threshold = 320;
    bool m_visible = false;
    QAbstractScrollArea *m_scrollArea = nullptr;
    MdFab *m_fab = nullptr;
    QMetaObject::Connection m_scrollConn;
};

} // namespace md

#endif // MD_BACK_TOP_H
