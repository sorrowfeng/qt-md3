#ifndef MD_VOLUME_SLIDER_H
#define MD_VOLUME_SLIDER_H

// MdVolumeSlider — Stage 2 Qt extension: a volume slider with a mute icon.
//
// MD3 has no volume-slider family. The control is the standard media volume
// composition: a leading `MdIconButton` that mutes on click and shows the
// level glyph (volume_off / volume_down / volume_up), plus an `MdSlider`
// track. Both halves use the Stage 1 token sets (`md.comp.icon-button.*` and
// `md.comp.slider.*`); nothing is invented.
//
// No QSS. Keyboard is the button's plus the slider's.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QResizeEvent;

namespace md {

class MdIconButton;
class MdSlider;

class QT_MD3_EXPORT MdVolumeSlider : public QWidget
{
    Q_OBJECT

    /// Volume in [0, 100].
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)
    Q_PROPERTY(bool iconVisible READ isIconVisible WRITE setIconVisible NOTIFY iconVisibleChanged)

public:
    explicit MdVolumeSlider(QWidget *parent = nullptr);
    ~MdVolumeSlider() override;

    qreal value() const;
    void setValue(qreal value);

    bool isMuted() const { return m_muted; }
    void setMuted(bool muted);

    /// The level glyph currently shown on the button.
    QString iconName() const;

    bool isIconVisible() const { return m_iconVisible; }
    void setIconVisible(bool visible);

    /// The underlying slider / button, for callers that need the token surface.
    MdSlider *slider() const { return m_slider; }
    MdIconButton *button() const { return m_button; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void valueChanged(qreal value);
    void mutedChanged(bool muted);
    void iconNameChanged();
    void iconVisibleChanged(bool visible);
    void sliderPressed();
    void sliderReleased();
    void sliderMoved(qreal value);
    /// The mute button was clicked. `muted` is already flipped.
    void muteToggled(bool muted);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void init();
    void layoutParts();
    void refreshIcon();

    MdIconButton *m_button = nullptr;
    MdSlider *m_slider = nullptr;
    qreal m_value = 80.0;
    bool m_muted = false;
    bool m_iconVisible = true;
};

} // namespace md

#endif // MD_VOLUME_SLIDER_H
