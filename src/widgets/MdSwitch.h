#ifndef MD_SWITCH_H
#define MD_SWITCH_H

// MdSwitch — the MD3 switch, over Qt's QCheckBox.
//
// Compose's `Switch` is a 52×32 corner-full track with a travelling handle,
// and its behaviours are what this widget ports (`ThumbNode.measure` verbatim):
//
//   * **the thumb travels and resizes on one spring** — the fast spatial
//     spring animates *both* the diameter (16 unselected → 24 selected →
//     28 pressed) and the x offset (the unchecked thumb sits at the track's
//     left inner inset; the checked one at the far bound). **While pressed
//     both run `SnapSpec`** — the press lands instantly; releasing springs
//     back.
//   * **no colour animation** — the track/handle/icon colours resolve by
//     state and land this frame.
//   * **the ripple rides the thumb** — an unbounded 40 px circle at the
//     thumb's centre, coloured with the state-layer row.
//
// The group semantics are `QCheckBox`'s (independent toggles); `hitButton` is
// the whole rect — the native hit area knows nothing of the self-drawn track.
// The paint is `MdSwitchStyle`'s, reached through the shared paint filter.

#include "core/MdQtCompat.h"
#include "core/MdSwitchTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtWidgets/QCheckBox>

class QTimer;

namespace md {

class MdFocusRingController;
class MdRippleController;

class QT_MD3_EXPORT MdSwitch : public QCheckBox
{
    Q_OBJECT

    /// The checkmark glyph inside the thumb (Compose's `thumbContent`). The
    /// with-icon handle is 24 px — the same diameter as a checked thumb.
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    Q_PROPERTY(md::MdIconSet iconSet READ iconSet WRITE setIconSet NOTIFY iconSetChanged)
    Q_PROPERTY(md::MdIconFamily iconFamily READ iconFamily WRITE setIconFamily NOTIFY
                   iconFamilyChanged)

public:
    explicit MdSwitch(QWidget *parent = nullptr);
    explicit MdSwitch(const QString &text, QWidget *parent = nullptr);
    ~MdSwitch() override;

    // --- state --------------------------------------------------------------
    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    md::MdIconSet iconSet() const { return m_iconSet; }
    void setIconSet(md::MdIconSet set);
    md::MdIconFamily iconFamily() const { return m_iconFamily; }
    void setIconFamily(md::MdIconFamily family);

    // --- geometry ---------------------------------------------------------------
    /// The 52×32 track, centred in the widget.
    QRectF trackRect() const;
    /// The thumb's current box: the animated diameter at the animated offset.
    QRectF thumbRect() const;
    /// The 40 px ripple bounds at the thumb's centre.
    QRectF stateLayerRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- animation readouts ---------------------------------------------------------
    /// The thumb's animated diameter and x offset within the track.
    qreal animatedThumbDiameter() const { return m_thumbDiameter; }
    qreal animatedThumbOffset() const { return m_thumbOffset; }
    bool thumbAnimationRunning() const { return m_thumbTimer->isActive(); }

    // --- tokens -----------------------------------------------------------------------
    const MdSwitchTokens &switchTokens() const;
    void setSwitchTokens(const MdSwitchTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- interaction state ----------------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- controllers -------------------------------------------------------------------------
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

signals:
    void iconNameChanged(const QString &iconName);
    void iconSetChanged(md::MdIconSet iconSet);
    void iconFamilyChanged(md::MdIconFamily iconFamily);

protected:
    bool hitButton(const QPoint &pos) const override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onThumbTick();

private:
    void init();
    void invalidateTokens();
    void restartThumbAnimation(bool snap);
    void retargetThumb();

    /// Compose's `ThumbNode.measure` targets, one pair per state: the
    /// diameter and the x offset the thumb animates between.
    struct ThumbTarget
    {
        qreal diameter = 0.0;
        qreal offset = 0.0;
    };
    ThumbTarget thumbTarget() const;

    QString m_iconName;
    md::MdIconSet m_iconSet = MdIconSet::MaterialSymbols;
    md::MdIconFamily m_iconFamily = MdIconFamily::Outlined;
    bool m_hovered = false;
    bool m_focusIsKeyboard = false;
    bool m_lastChecked = false;
    bool m_wasPressed = false;
    bool m_thumbInitialised = false;

    qreal m_thumbDiameter = 0.0;
    qreal m_thumbOffset = 0.0;
    qreal m_thumbFromDiameter = 0.0;
    qreal m_thumbToDiameter = 0.0;
    qreal m_thumbFromOffset = 0.0;
    qreal m_thumbToOffset = 0.0;
    QElapsedTimer m_thumbClock;
    bool m_thumbSnap = false;

    mutable MdSwitchTokens m_tokens;
    mutable bool m_tokensDirty = true;
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;

    QTimer *m_thumbTimer = nullptr;
    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
};

} // namespace md

#endif // MD_SWITCH_H
