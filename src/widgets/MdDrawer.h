#ifndef MD_DRAWER_H
#define MD_DRAWER_H

// MdDrawer — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "复用阶段一的 Navigation drawer / Side sheet，
// 不另造" — the generic app drawer, built on the Stage 1 drawer language.
//
// MdDrawer is the *host* reading of the navigation drawer / side sheet pair:
// an edge-anchored panel that opens and closes over a scrim, carrying an
// arbitrary content widget. The chrome reuses the Stage 1 tokens —
// `md.comp.navigation-drawer.*` for the surface (`surface-container-low` at
// level 1 when modal, `surface` at level 0 when docked) and the side-sheet
// edge/mirroring rules. Nothing here invents a token.
//
//   edge     Start / End / Top / Bottom (RTL-mirrored for Start/End)
//   kind     Modal (over a scrim) or Standard (docked, no scrim)
//   open     slides the panel in on the spatial-fast spring
//   content  an arbitrary caller widget in the panel body
//
// The scrim is painted inside the drawer's parent window when the drawer is
// a top-level overlay; when embedded, `scrimColor()` / `scrimOpacity()`
// expose the carried rows for the host (the same contract
// MdNavigationDrawer publishes).
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QWidget>

class QPaintEvent;
class QTimer;

namespace md {

/// Which edge the drawer is anchored to. Start/End mirror in RTL.
enum class MdDrawerEdge {
    Start,
    End,
    Top,
    Bottom,
    Count,
};

/// Modal overlays a scrim; Standard is docked into the parent's layout.
enum class MdDrawerKind {
    Modal,
    Standard,
    Count,
};

class QT_MD3_EXPORT MdDrawer : public QWidget
{
    Q_OBJECT

    /// Anchored edge. Start/End mirror under RTL.
    Q_PROPERTY(md::MdDrawerEdge edge READ edge WRITE setEdge NOTIFY edgeChanged)
    /// Modal (scrim) or Standard (docked).
    Q_PROPERTY(md::MdDrawerKind kind READ kind WRITE setKind NOTIFY kindChanged)
    /// Whether the drawer is open.
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
    /// Panel width (Start/End) or height (Top/Bottom), in logical px.
    Q_PROPERTY(int panelSize READ panelSize WRITE setPanelSize NOTIFY panelSizeChanged)
    /// Optional headline drawn at the top of the panel.
    Q_PROPERTY(QString headline READ headline WRITE setHeadline NOTIFY headlineChanged)

public:
    explicit MdDrawer(QWidget *parent = nullptr);
    ~MdDrawer() override;

    MdDrawerEdge edge() const { return m_edge; }
    void setEdge(MdDrawerEdge edge);

    MdDrawerKind kind() const { return m_kind; }
    void setKind(MdDrawerKind kind);

    bool isOpen() const { return m_open; }
    void setOpen(bool open);

    int panelSize() const { return m_panelSize; }
    void setPanelSize(int size);

    QString headline() const { return m_headline; }
    void setHeadline(const QString &headline);

    /// Body widget filling the panel under the headline. Pass nullptr to clear.
    void setContentWidget(QWidget *content);
    QWidget *contentWidget() const { return m_content; }

    /// Scrim colour / opacity the modal kind uses. Carried, like the Stage 1
    /// navigation drawer: the scrim covers the parent and is painted by the
    /// host overlay when the drawer is embedded.
    ColorRole scrimColor() const;
    qreal scrimOpacity() const;

    /// Resolved panel surface colour.
    QColor resolvedSurfaceColor() const;

    /// Slide progress in 0..1 (0 = closed, 1 = fully open).
    qreal openProgress() const { return m_progress; }

    /// Panel rect in widget coordinates (empty when closed and not animating).
    QRectF panelRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void open();
    void close();
    void toggle();

signals:
    void edgeChanged(md::MdDrawerEdge edge);
    void kindChanged(md::MdDrawerKind kind);
    void openChanged(bool open);
    void panelSizeChanged(int size);
    void headlineChanged(const QString &headline);
    void contentWidgetChanged(QWidget *content);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onAnimationFrame();

private:
    void init();
    void relayout();
    void startAnimation(bool opening);
    bool isMirrored() const;

    MdDrawerEdge m_edge = MdDrawerEdge::Start;
    MdDrawerKind m_kind = MdDrawerKind::Modal;
    bool m_open = false;
    int m_panelSize = 256;
    QString m_headline;
    QWidget *m_content = nullptr;

    QTimer *m_timer = nullptr;
    QElapsedTimer m_clock;
    bool m_running = false;
    bool m_targetOpen = false;
    qreal m_progress = 0.0;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdDrawerEdge)
Q_DECLARE_METATYPE(md::MdDrawerKind)

#endif // MD_DRAWER_H
