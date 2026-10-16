#include "MdColorPicker.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdSlider.h"
#include "widgets/MdTextField.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QResizeEvent>
#include <QtMath>

namespace md {

namespace {

/// `md.comp.slider` track height sits inside a 48 px control; the picker
/// stacks three of them with the spacing scale's 8 px gaps.
constexpr qreal kSliderGap = 8.0;
/// Swatch disc edge (`md.comp.icon-button` large container 96, optically 80).
constexpr qreal kSwatch = 80.0;
/// Gap between the sliders column and the swatch.
constexpr qreal kSwatchGap = 16.0;
/// Hex field height (filled text field on the default track).
constexpr qreal kHexHeight = 56.0;
/// Gap under the sliders before the hex field.
constexpr qreal kHexGap = 12.0;
/// Outer padding.
constexpr qreal kPadding = 8.0;

} // namespace

MdColorPicker::MdColorPicker(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdColorPicker::~MdColorPicker()
{
    // Children are destroyed in ~QWidget, which runs *before* ~QObject
    // auto-disconnects. Cut the wires first (the MdInputNumber pattern).
    if (m_hueSlider) {
        disconnect(m_hueSlider, nullptr, this, nullptr);
    }
    if (m_saturationSlider) {
        disconnect(m_saturationSlider, nullptr, this, nullptr);
    }
    if (m_valueSlider) {
        disconnect(m_valueSlider, nullptr, this, nullptr);
    }
    if (m_hexField) {
        disconnect(m_hexField, nullptr, this, nullptr);
    }
}

void MdColorPicker::init()
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setAccessibleName(QStringLiteral("color picker"));

    m_hueSlider = new MdSlider(this);
    m_hueSlider->setMin(0.0);
    m_hueSlider->setMax(360.0);
    m_hueSlider->setStep(1.0);
    m_hueSlider->setAccessibleName(QStringLiteral("Hue"));

    m_saturationSlider = new MdSlider(this);
    m_saturationSlider->setMin(0.0);
    m_saturationSlider->setMax(100.0);
    m_saturationSlider->setStep(1.0);
    m_saturationSlider->setAccessibleName(QStringLiteral("Saturation"));

    m_valueSlider = new MdSlider(this);
    m_valueSlider->setMin(0.0);
    m_valueSlider->setMax(100.0);
    m_valueSlider->setStep(1.0);
    m_valueSlider->setAccessibleName(QStringLiteral("Value"));

    m_hexField = new MdTextField(this);
    m_hexField->setVariant(MdTextFieldVariant::Filled);
    m_hexField->setLabelText(QStringLiteral("Hex"));
    m_hexField->setAccessibleName(QStringLiteral("Hex code"));

    connect(m_hueSlider, &MdSlider::valueChanged, this, &MdColorPicker::onHueSliderChanged);
    connect(m_saturationSlider, &MdSlider::valueChanged, this,
            &MdColorPicker::onSaturationSliderChanged);
    connect(m_valueSlider, &MdSlider::valueChanged, this, &MdColorPicker::onValueSliderChanged);
    connect(m_hexField, &QLineEdit::editingFinished, this, &MdColorPicker::onHexEditingFinished);

    m_color = QColor::fromHsvF(0.0, 1.0, 1.0);
    syncToSliders();
    syncToHex();

    MdStyleBase::connectThemeUpdate(this, &MdColorPicker::onThemeChanged);
}

void MdColorPicker::setColor(const QColor &color)
{
    if (!color.isValid() || color == m_color) {
        return;
    }
    m_color = color.toRgb();
    const qreal h = m_color.hsvHueF() < 0.0 ? m_hue : m_color.hsvHueF();
    m_hue = h;
    m_saturation = m_color.saturationF();
    m_value = m_color.valueF();
    syncToSliders();
    syncToHex();
    update();
    emitColorChange();
}

void MdColorPicker::setHue(qreal hue)
{
    const qreal clamped = qBound(0.0, hue, 359.0);
    if (qFuzzyCompare(m_hue, clamped)) {
        return;
    }
    m_hue = clamped;
    syncFromHsv();
}

void MdColorPicker::setSaturation(qreal saturation)
{
    const qreal clamped = qBound(0.0, saturation, 1.0);
    if (qFuzzyCompare(m_saturation, clamped)) {
        return;
    }
    m_saturation = clamped;
    syncFromHsv();
}

void MdColorPicker::setValue(qreal value)
{
    const qreal clamped = qBound(0.0, value, 1.0);
    if (qFuzzyCompare(m_value, clamped)) {
        return;
    }
    m_value = clamped;
    syncFromHsv();
}

void MdColorPicker::setHexVisible(bool visible)
{
    if (m_hexVisible == visible) {
        return;
    }
    m_hexVisible = visible;
    if (m_hexField) {
        m_hexField->setVisible(visible);
    }
    updateGeometry();
    layoutChildren();
    emit hexVisibleChanged(m_hexVisible);
}

void MdColorPicker::setSwatchShape(ShapeCorner shape)
{
    if (shape >= ShapeCorner::Count) {
        return;
    }
    if (m_swatchShape == shape) {
        return;
    }
    m_swatchShape = shape;
    update();
    emit swatchShapeChanged(m_swatchShape);
}

QString MdColorPicker::hexCode() const
{
    return m_color.name(QColor::HexRgb).toUpper();
}

void MdColorPicker::setHexCode(const QString &hex)
{
    QString trimmed = hex.trimmed();
    if (!trimmed.startsWith(QLatin1Char('#'))) {
        trimmed.prepend(QLatin1Char('#'));
    }
    const QColor parsed(trimmed);
    if (!parsed.isValid()) {
        syncToHex();
        return;
    }
    setColor(parsed);
}

QColor MdColorPicker::resolvedRingColor() const
{
    // md.sys.color.outline-variant — the identity-disc ring.
    return MdTheme::instance().color(ColorRole::OutlineVariant);
}

QSize MdColorPicker::sizeHint() const
{
    const qreal width = kPadding * 2.0 + 240.0 + kSwatchGap + kSwatch;
    qreal height = kPadding * 2.0 + 48.0 * 3.0 + kSliderGap * 2.0;
    if (m_hexVisible) {
        height += kHexGap + kHexHeight;
    }
    return QSize(qCeil(width), qCeil(height));
}

QSize MdColorPicker::minimumSizeHint() const
{
    return sizeHint();
}

void MdColorPicker::syncFromHsv()
{
    const QColor next = QColor::fromHsvF(m_hue / 360.0, m_saturation, m_value);
    m_color = next.toRgb();
    syncToSliders();
    syncToHex();
    update();
    emitColorChange();
}

void MdColorPicker::syncToSliders()
{
    if (!m_hueSlider) {
        return;
    }
    m_syncing = true;
    m_hueSlider->setValue(m_hue);
    m_saturationSlider->setValue(m_saturation * 100.0);
    m_valueSlider->setValue(m_value * 100.0);
    m_syncing = false;
}

void MdColorPicker::syncToHex()
{
    if (!m_hexField) {
        return;
    }
    m_syncing = true;
    m_hexField->setText(hexCode());
    m_syncing = false;
}

void MdColorPicker::emitColorChange()
{
    emit colorChanged(m_color);
    emit hueChanged(m_hue);
    emit saturationChanged(m_saturation);
    emit valueChanged(m_value);
}

void MdColorPicker::onHueSliderChanged(qreal v)
{
    if (m_syncing) {
        return;
    }
    m_hue = qBound(0.0, v, 359.0);
    syncFromHsv();
}

void MdColorPicker::onSaturationSliderChanged(qreal v)
{
    if (m_syncing) {
        return;
    }
    m_saturation = qBound(0.0, v / 100.0, 1.0);
    syncFromHsv();
}

void MdColorPicker::onValueSliderChanged(qreal v)
{
    if (m_syncing) {
        return;
    }
    m_value = qBound(0.0, v / 100.0, 1.0);
    syncFromHsv();
}

void MdColorPicker::onHexEditingFinished()
{
    if (m_syncing || !m_hexField) {
        return;
    }
    setHexCode(m_hexField->text());
}

void MdColorPicker::layoutChildren()
{
    const qreal contentLeft = kPadding;
    const qreal contentTop = kPadding;
    const qreal sliderWidth = 240.0;
    const qreal sliderHeight = 48.0;

    if (m_hueSlider) {
        m_hueSlider->setGeometry(QRect(qRound(contentLeft), qRound(contentTop),
                                       qRound(sliderWidth), qRound(sliderHeight)));
    }
    if (m_saturationSlider) {
        m_saturationSlider->setGeometry(QRect(qRound(contentLeft),
                                              qRound(contentTop + sliderHeight + kSliderGap),
                                              qRound(sliderWidth), qRound(sliderHeight)));
    }
    if (m_valueSlider) {
        m_valueSlider->setGeometry(QRect(qRound(contentLeft),
                                         qRound(contentTop + (sliderHeight + kSliderGap) * 2.0),
                                         qRound(sliderWidth), qRound(sliderHeight)));
    }
    if (m_hexField && m_hexVisible) {
        const qreal y = contentTop + (sliderHeight + kSliderGap) * 2.0 + sliderHeight + kHexGap;
        m_hexField->setGeometry(QRect(qRound(contentLeft), qRound(y),
                                      qRound(sliderWidth), qRound(kHexHeight)));
    }
}

void MdColorPicker::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

void MdColorPicker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Swatch disc on the trailing side.
    const qreal swatchX = width() - kPadding - kSwatch;
    const qreal swatchY = kPadding;
    const QRectF disc(swatchX, swatchY, kSwatch, kSwatch);
    const QList<qreal> radii = MdShape::resolvedRadii(m_swatchShape, disc.size());
    const QPainterPath path = MdShape::roundedRect(disc, radii);
    painter.fillPath(path, m_color);

    painter.setPen(QPen(resolvedRingColor(), 2.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void MdColorPicker::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange) {
        layoutChildren();
        update();
    }
}

void MdColorPicker::onThemeChanged()
{
    update();
}

} // namespace md
