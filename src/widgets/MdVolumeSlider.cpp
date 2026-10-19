#include "MdVolumeSlider.h"

#include "MdIconButton.h"
#include "MdSlider.h"
#include "core/MdIcon.h"

#include <QtGui/QResizeEvent>

namespace md {

namespace {

/// Icon-button medium container height (md.comp.icon-button.medium).
constexpr int kButtonSize = 40;
constexpr int kGap = 8;

QString levelIcon(qreal value, bool muted)
{
    if (muted || value <= 0.0) {
        return QStringLiteral("volume_off");
    }
    if (value < 50.0) {
        return QStringLiteral("volume_down");
    }
    return QStringLiteral("volume_up");
}

} // namespace

MdVolumeSlider::MdVolumeSlider(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdVolumeSlider::~MdVolumeSlider() = default;

void MdVolumeSlider::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("volume"));

    m_button = new MdIconButton(QStringLiteral("volume_up"), this);
    m_button->setVariant(IconButtonVariant::Standard);
    m_button->setButtonSize(ButtonSize::Medium);
    m_button->setCheckable(true);
    m_button->setAccessibleName(QStringLiteral("Mute"));

    m_slider = new MdSlider(this);
    m_slider->setMin(0.0);
    m_slider->setMax(100.0);
    m_slider->setValue(m_value);
    m_slider->setAccessibleName(QStringLiteral("Volume level"));

    connect(m_button, &QAbstractButton::clicked, this, [this](bool) {
        setMuted(!m_muted);
        emit muteToggled(m_muted);
    });
    connect(m_slider, &MdSlider::valueChanged, this, [this](qreal v) {
        if (qFuzzyCompare(m_value, v)) {
            return;
        }
        m_value = v;
        if (m_muted && m_value > 0.0) {
            m_muted = false;
            m_button->setChecked(false);
            emit mutedChanged(false);
        }
        refreshIcon();
        emit valueChanged(m_value);
    });
    connect(m_slider, &MdSlider::sliderPressed, this, &MdVolumeSlider::sliderPressed);
    connect(m_slider, &MdSlider::sliderReleased, this, &MdVolumeSlider::sliderReleased);
    connect(m_slider, &MdSlider::sliderMoved, this, &MdVolumeSlider::sliderMoved);

    refreshIcon();
    layoutParts();
}

qreal MdVolumeSlider::value() const
{
    return m_value;
}

void MdVolumeSlider::setValue(qreal value)
{
    const qreal clamped = qBound(0.0, value, 100.0);
    if (qFuzzyCompare(m_value, clamped)) {
        return;
    }
    m_value = clamped;
    if (m_slider) {
        m_slider->setValue(m_value);
    }
    if (m_muted && m_value > 0.0) {
        m_muted = false;
        if (m_button) {
            m_button->setChecked(false);
        }
        emit mutedChanged(false);
    }
    refreshIcon();
    emit valueChanged(m_value);
}

void MdVolumeSlider::setMuted(bool muted)
{
    if (m_muted == muted) {
        return;
    }
    m_muted = muted;
    if (m_button) {
        m_button->setChecked(m_muted);
    }
    refreshIcon();
    emit mutedChanged(m_muted);
}

QString MdVolumeSlider::iconName() const
{
    return levelIcon(m_value, m_muted);
}

void MdVolumeSlider::setIconVisible(bool visible)
{
    if (m_iconVisible == visible) {
        return;
    }
    m_iconVisible = visible;
    if (m_button) {
        m_button->setVisible(m_iconVisible);
    }
    layoutParts();
    emit iconVisibleChanged(m_iconVisible);
}

QSize MdVolumeSlider::sizeHint() const
{
    const int w = (m_iconVisible ? kButtonSize + kGap : 0) + 160;
    return QSize(w, 48);
}

QSize MdVolumeSlider::minimumSizeHint() const
{
    const int w = (m_iconVisible ? kButtonSize + kGap : 0) + 80;
    return QSize(w, 48);
}

void MdVolumeSlider::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutParts();
}

void MdVolumeSlider::layoutParts()
{
    if (!m_button || !m_slider) {
        return;
    }
    const int y = (height() - kButtonSize) / 2;
    int x = 0;
    if (m_iconVisible) {
        m_button->setGeometry(0, y, kButtonSize, kButtonSize);
        x = kButtonSize + kGap;
    }
    m_slider->setGeometry(x, 0, qMax(0, width() - x), height());
}

void MdVolumeSlider::refreshIcon()
{
    if (!m_button) {
        return;
    }
    const QString name = levelIcon(m_value, m_muted);
    if (m_button->iconName() != name) {
        m_button->setIconName(name);
        emit iconNameChanged();
    }
}

} // namespace md
