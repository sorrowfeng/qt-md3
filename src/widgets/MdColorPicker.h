#ifndef MD_COLOR_PICKER_H
#define MD_COLOR_PICKER_H

// MdColorPicker — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "Dialog + Slider（HSV）+ Text field（Hex）；
// MD3 无色板规范，需自行设计" — MD3 has no colour-swatch spec, so the picker
// is assembled from the components that do have one:
//
//   Dialog       the container language (surface, extra-large corner) when
//                used as a popup; the widget also embeds in a page
//   Slider       three `MdSlider`s driving H / S / V
//   Text field   `MdTextField` for the hex code
//   Swatch       a shape-disc preview (ShapeCorner::Full) with an outline
//                variant ring — the same identity disc MdAvatar uses
//
// Hue / saturation / value are the three tracks. The swatch and the hex
// field stay in sync with the sliders in both directions. There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtGui/QColor>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

class MdSlider;
class MdTextField;

class QT_MD3_EXPORT MdColorPicker : public QWidget
{
    Q_OBJECT

    /// The current colour (the public value).
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    /// Hue in [0, 360).
    Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)
    /// Saturation in [0, 1].
    Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    /// Value (brightness) in [0, 1].
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    /// Show the hex text field under the sliders.
    Q_PROPERTY(bool hexVisible READ isHexVisible WRITE setHexVisible NOTIFY hexVisibleChanged)
    /// Swatch preview radius. Full (default) is the identity disc.
    Q_PROPERTY(md::ShapeCorner swatchShape READ swatchShape WRITE setSwatchShape
                   NOTIFY swatchShapeChanged)

public:
    explicit MdColorPicker(QWidget *parent = nullptr);
    ~MdColorPicker() override;

    QColor color() const { return m_color; }
    void setColor(const QColor &color);

    qreal hue() const { return m_hue; }
    void setHue(qreal hue);

    qreal saturation() const { return m_saturation; }
    void setSaturation(qreal saturation);

    qreal value() const { return m_value; }
    void setValue(qreal value);

    bool isHexVisible() const { return m_hexVisible; }
    void setHexVisible(bool visible);

    ShapeCorner swatchShape() const { return m_swatchShape; }
    void setSwatchShape(ShapeCorner shape);

    /// The hex code, e.g. "#FF8800" (no alpha).
    QString hexCode() const;
    void setHexCode(const QString &hex);

    /// Inner controls (for hosts that need to tune the MdSlider / MdTextField).
    MdSlider *hueSlider() const { return m_hueSlider; }
    MdSlider *saturationSlider() const { return m_saturationSlider; }
    MdSlider *valueSlider() const { return m_valueSlider; }
    MdTextField *hexField() const { return m_hexField; }

    /// Resolved swatch ring: `outline-variant`.
    QColor resolvedRingColor() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void colorChanged(const QColor &color);
    void hueChanged(qreal hue);
    void saturationChanged(qreal saturation);
    void valueChanged(qreal value);
    void hexVisibleChanged(bool visible);
    void swatchShapeChanged(md::ShapeCorner shape);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onHueSliderChanged(qreal v);
    void onSaturationSliderChanged(qreal v);
    void onValueSliderChanged(qreal v);
    void onHexEditingFinished();
    void onThemeChanged();

private:
    void init();
    void layoutChildren();
    void syncFromHsv();
    void syncToSliders();
    void syncToHex();
    void emitColorChange();

    QColor m_color;
    qreal m_hue = 0.0;
    qreal m_saturation = 1.0;
    qreal m_value = 1.0;
    bool m_hexVisible = true;
    ShapeCorner m_swatchShape = ShapeCorner::Full;

    MdSlider *m_hueSlider = nullptr;
    MdSlider *m_saturationSlider = nullptr;
    MdSlider *m_valueSlider = nullptr;
    MdTextField *m_hexField = nullptr;
    /// Swatch disc rect from the last `layoutChildren()`, mirrored under RTL.
    QRectF m_swatchRect;
    bool m_syncing = false;
};

} // namespace md

#endif // MD_COLOR_PICKER_H
