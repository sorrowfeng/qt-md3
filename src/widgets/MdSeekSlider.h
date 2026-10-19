#ifndef MD_SEEK_SLIDER_H
#define MD_SEEK_SLIDER_H

// MdSeekSlider — Stage 2 Qt extension: a media seek slider.
//
// MD3 has no seek-slider family. The control is the media-progress
// composition every player surface needs: an `MdSlider` track plus a hover
// preview that shows the time under the pointer. Everything visual comes from
// the Stage 1 slider export (`md.comp.slider.*`); the preview is a plain
// tooltip-style chip painted with the inverse-surface pair the tooltip family
// publishes (`md.comp.plain-tooltip.*`).
//
// No QSS. Keyboard is MdSlider's.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QWidget>

class QPaintEvent;
class QResizeEvent;
class QMouseEvent;
class QEvent;

namespace md {

class MdSlider;

class QT_MD3_EXPORT MdSeekSlider : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(qreal min READ min WRITE setMin NOTIFY minChanged)
    Q_PROPERTY(qreal max READ max WRITE setMax NOTIFY maxChanged)
    Q_PROPERTY(qreal step READ step WRITE setStep NOTIFY stepChanged)
    Q_PROPERTY(bool previewEnabled READ isPreviewEnabled WRITE setPreviewEnabled NOTIFY
                   previewEnabledChanged)
    Q_PROPERTY(bool labeled READ isLabeled WRITE setLabeled NOTIFY labeledChanged)
    /// Seconds per value unit, used to format the preview. 0 means the value
    /// *is* the number of seconds.
    Q_PROPERTY(qreal secondsPerUnit READ secondsPerUnit WRITE setSecondsPerUnit NOTIFY
                   secondsPerUnitChanged)

public:
    explicit MdSeekSlider(QWidget *parent = nullptr);
    ~MdSeekSlider() override;

    qreal value() const;
    void setValue(qreal value);

    qreal min() const;
    void setMin(qreal min);

    qreal max() const;
    void setMax(qreal max);

    qreal step() const;
    void setStep(qreal step);

    bool isPreviewEnabled() const { return m_previewEnabled; }
    void setPreviewEnabled(bool enabled);

    bool isLabeled() const;
    void setLabeled(bool labeled);

    qreal secondsPerUnit() const { return m_secondsPerUnit; }
    void setSecondsPerUnit(qreal seconds);

    /// Format `seconds` as m:ss or h:mm:ss. Shared by the preview chip and
    /// callers that want the same clock.
    static QString formatTime(qreal seconds);

    /// The preview chip's rect (empty when the preview is not showing).
    QRect previewRect() const;

    /// The underlying slider, for callers that need the token / paint surface.
    MdSlider *slider() const { return m_slider; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void valueChanged(qreal value);
    void minChanged(qreal min);
    void maxChanged(qreal max);
    void stepChanged(qreal step);
    void previewEnabledChanged(bool enabled);
    void secondsPerUnitChanged(qreal seconds);
    void labeledChanged(bool labeled);
    void sliderPressed();
    void sliderReleased();
    void sliderMoved(qreal value);
    /// The pointer moved over the track; `value` is the value under it.
    void previewChanged(qreal value);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void init();
    void layoutSlider();
    void updatePreviewFromPos(const QPointF &pos);
    void hidePreview();

    MdSlider *m_slider = nullptr;
    bool m_previewEnabled = true;
    bool m_previewVisible = false;
    qreal m_previewValue = 0.0;
    qreal m_secondsPerUnit = 1.0;
    QRect m_previewRect;
};

} // namespace md

#endif // MD_SEEK_SLIDER_H
