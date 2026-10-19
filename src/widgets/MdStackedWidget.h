#ifndef MD_STACKED_WIDGET_H
#define MD_STACKED_WIDGET_H

// MdStackedWidget — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "Surface container 层级" — the stacked page
// container, restyled as an MD3 surface.
//
// QStackedWidget with no chrome of its own: the pages are the content. What
// this port adds is the surface contract every Stage 2 container obeys:
//
//   surface   `md.sys.color.surface` fill, ShapeCorner::Medium clip
//   pages     caller widgets, one visible at a time
//   motion    page changes fade on the effects-fast spring (`MdSpring`,
//             never a QEasingCurve)
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class QT_MD3_EXPORT MdStackedWidget : public QWidget
{
    Q_OBJECT

    /// Index of the visible page. -1 when the stack is empty.
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    /// Number of pages.
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    /// When true the page change animates a short fade; when false it snaps.
    Q_PROPERTY(bool animated READ isAnimated WRITE setAnimated NOTIFY animatedChanged)

public:
    explicit MdStackedWidget(QWidget *parent = nullptr);
    ~MdStackedWidget() override;

    int addWidget(QWidget *page);
    int insertWidget(int index, QWidget *page);
    void removeWidget(QWidget *page);
    QWidget *widget(int index) const;
    int indexOf(QWidget *page) const;
    int count() const { return int(m_pages.size()); }
    void clear();

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    QWidget *currentWidget() const;

    bool isAnimated() const { return m_animated; }
    void setAnimated(bool on);

    /// Resolved surface fill.
    QColor resolvedSurfaceColor() const;

    /// Current page-change fade progress in 0..1 (1 = fully shown).
    qreal transitionProgress() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void setCurrentWidget(QWidget *page);

signals:
    void currentIndexChanged(int index);
    void countChanged(int count);
    void animatedChanged(bool on);
    void widgetAdded(int index);
    void widgetRemoved(int index);

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onAnimationFrame();

private:
    void init();
    void relayout();
    void startTransition();
    void updateTransitionState();
    void applyFadeOpacities();
    void finishOutgoing();
    static qreal pageOpacity(const QWidget *page);
    static void clearPageEffect(QWidget *page);

    QList<QWidget *> m_pages;
    int m_currentIndex = -1;
    bool m_animated = true;

    // Transition state: the outgoing page stays a moment while the incoming
    // fades in on the effects-fast spring.
    QTimer *m_timer = nullptr;
    QElapsedTimer m_clock;
    bool m_running = false;
    QWidget *m_outgoing = nullptr;
    qreal m_progress = 1.0;
    /// Opacity the outgoing page had on screen when the fade started, so a
    /// mid-flight retarget never snaps (gallery startPageFade parity).
    qreal m_outgoingFrom = 1.0;
};

} // namespace md

#endif // MD_STACKED_WIDGET_H
