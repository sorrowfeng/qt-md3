#ifndef MD_WATERMARK_H
#define MD_WATERMARK_H

// MdWatermark — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "on-surface 低透明度平铺" — a tiled
// low-alpha stamp over a surface.
//
// MD3 has no watermark family. The control reuses the on-surface content
// colour at the state-layer *hover* opacity (md.comp.state-layer.hover.opacity
// 0.08 — the lowest published non-zero on-surface overlay), which is the
// conventional "ghost mark" reading. It is drawn as a repeating tile of the
// `text` (or an optional `pixmap`) rotated by `angle`.
//
// Place it as the topmost child of a surface; it is non-interactive
// (transparent for mouse) so the content underneath stays clickable.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtGui/QColor>
#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

class QT_MD3_EXPORT MdWatermark : public QWidget
{
    Q_OBJECT

    /// Stamp text. Empty (with no pixmap) paints nothing.
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// Optional stamp image. Takes precedence over `text` when set.
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap NOTIFY pixmapChanged)
    /// Rotation of each tile, in degrees (default -22, the common stamp).
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
    /// Gap between tiles along both axes, in logical px (default 120).
    Q_PROPERTY(qreal gap READ gap WRITE setGap NOTIFY gapChanged)
    /// Type size of the stamp, in logical px (default 16, label-large size).
    Q_PROPERTY(qreal fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    /// Opacity of the stamp, 0..1 (default 0.08 = state-layer hover).
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)

public:
    explicit MdWatermark(QWidget *parent = nullptr);
    ~MdWatermark() override;

    QString text() const { return m_text; }
    void setText(const QString &text);

    QPixmap pixmap() const { return m_pixmap; }
    void setPixmap(const QPixmap &pixmap);

    qreal angle() const { return m_angle; }
    void setAngle(qreal angle);

    qreal gap() const { return m_gap; }
    void setGap(qreal gap);

    qreal fontSize() const { return m_fontSize; }
    void setFontSize(qreal size);

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity);

    /// Resolved stamp colour: on-surface (alpha applied at paint time).
    QColor resolvedStampColor() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void textChanged(const QString &text);
    void pixmapChanged(const QPixmap &pixmap);
    void angleChanged(qreal angle);
    void gapChanged(qreal gap);
    void fontSizeChanged(qreal size);
    void opacityChanged(qreal opacity);

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();

    QString m_text;
    QPixmap m_pixmap;
    qreal m_angle = -22.0;
    qreal m_gap = 120.0;
    qreal m_fontSize = 16.0;
    qreal m_opacity = 0.08;
};

} // namespace md

#endif // MD_WATERMARK_H
