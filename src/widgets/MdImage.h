#ifndef MD_IMAGE_H
#define MD_IMAGE_H

// MdImage — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "含全屏预览 — Full-screen dialog + 手势".
// MD3 has no image family; the control is the standard photo tile every
// Material gallery / detail surface needs:
//
//   tile      surface-container-lowest (md.sys.color.surface-container-lowest)
//             under the pixmap, ShapeCorner from the shape scale
//             (default Medium = md.sys.shape.corner.medium)
//   preview   Full-screen dialog: scrim (md.sys.color.scrim) over the
//             previous surface, the pixmap centred, a close IconButton
//             (md.comp.icon-button.* standard) top-end. Wheel / pinch
//             zooms (the gesture half of the brief).
//
// The tile paints the pixmap clipped to its shape; clicking it opens the
// preview. The preview is a Qt::Dialog so it is modal to the parent window
// and closes on Esc.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QShowEvent;
class QHideEvent;

namespace md {

class MdImagePreview;

/// Aspect behaviour of the pixmap inside the tile.
enum class MdImageFit {
    /// Scale to fit entirely (letterbox on surface-container-lowest).
    Contain,
    /// Scale to cover, cropping overflow (default — matches photo tiles).
    Cover,
    /// Draw at device pixel size, no scaling.
    None,
    Count,
};

class QT_MD3_EXPORT MdImage : public QWidget
{
    Q_OBJECT

    /// The pixmap shown in the tile and in the preview.
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap NOTIFY pixmapChanged)
    /// Path of the file the pixmap was loaded from (informational).
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    /// Aspect behaviour inside the tile.
    Q_PROPERTY(md::MdImageFit fit READ fit WRITE setFit NOTIFY fitChanged)
    /// Shape-scale corner of the tile (default Medium).
    Q_PROPERTY(md::ShapeCorner shape READ shape WRITE setShape NOTIFY shapeChanged)
    /// Open the full-screen preview on click. Off makes the tile passive.
    Q_PROPERTY(bool previewEnabled READ isPreviewEnabled WRITE setPreviewEnabled
                   NOTIFY previewEnabledChanged)
    /// Accessible label; falls back to `source` when empty.
    Q_PROPERTY(QString altText READ altText WRITE setAltText NOTIFY altTextChanged)

public:
    explicit MdImage(QWidget *parent = nullptr);
    explicit MdImage(const QPixmap &pixmap, QWidget *parent = nullptr);
    ~MdImage() override;

    QPixmap pixmap() const { return m_pixmap; }
    void setPixmap(const QPixmap &pixmap);

    QString source() const { return m_source; }
    void setSource(const QString &source);

    /// Load from a file path; sets `source` and `pixmap`. Returns success.
    bool load(const QString &path);

    MdImageFit fit() const { return m_fit; }
    void setFit(MdImageFit fit);

    ShapeCorner shape() const { return m_shape; }
    void setShape(ShapeCorner corner);

    bool isPreviewEnabled() const { return m_previewEnabled; }
    void setPreviewEnabled(bool enabled);

    QString altText() const { return m_altText; }
    void setAltText(const QString &text);

    /// Resolved tile background: surface-container-lowest.
    QColor resolvedBackgroundColor() const;
    /// Resolved scrim for the preview: scrim.
    QColor resolvedScrimColor() const;

    /// True while the full-screen preview is open.
    bool isPreviewOpen() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    /// Open the full-screen preview. No-op when `previewEnabled` is off.
    void openPreview();
    /// Close the preview if it is open.
    void closePreview();

signals:
    void pixmapChanged(const QPixmap &pixmap);
    void sourceChanged(const QString &source);
    void fitChanged(md::MdImageFit fit);
    void shapeChanged(md::ShapeCorner corner);
    void previewEnabledChanged(bool enabled);
    void altTextChanged(const QString &text);
    void previewOpened();
    void previewClosed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onPreviewDestroyed();

private:
    void init();
    void ensurePreview();

    QPixmap m_pixmap;
    QString m_source;
    MdImageFit m_fit = MdImageFit::Cover;
    ShapeCorner m_shape = ShapeCorner::Medium;
    bool m_previewEnabled = true;
    QString m_altText;
    MdImagePreview *m_preview = nullptr;
};

/// The full-screen preview dialog used by MdImage. Internal, exported so the
/// image can own it across the DLL boundary; callers use MdImage's API.
class QT_MD3_EXPORT MdImagePreview : public QWidget
{
    Q_OBJECT

public:
    explicit MdImagePreview(QWidget *parent = nullptr);
    ~MdImagePreview() override;

    void setImage(const QPixmap &pixmap, const QString &altText);

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();

signals:
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    QRectF imageRect() const;

    QPixmap m_pixmap;
    QString m_altText;
    qreal m_zoom = 1.0;
    QPointF m_offset;
    bool m_panning = false;
    QPointF m_panStart;
    QPointF m_offsetStart;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdImageFit)

#endif // MD_IMAGE_H
