#ifndef MD_AVATAR_H
#define MD_AVATAR_H

// MdAvatar — Stage 2 Qt extension (Appendix D, P1).
//
// MD3 has no standalone Avatar family. The only official avatar-ish element
// is the list item's `leading.avatar` slot (a 40 px corner-full disc in
// primary-container) and the chip's leading element. This control promotes
// that slot into a first-class widget so lists, chips, headers and profile
// surfaces can share one component.
//
// Semantic anchor (appendix D): "shape (circle / full) + surface variant
// background". The published list-item leading avatar uses
// `primary-container` / `on-primary-container`; a standalone avatar is not
// inside a selected list row, so it takes the quieter pair the rest of the
// library uses for non-primary identity marks:
//
//   background  md.sys.color.surface-variant
//   content     md.sys.color.on-surface-variant
//
// Shape is the shape scale's `Full` corner, resolved against the widget box
// (a square box becomes a circle; a wider box becomes a capsule). There is
// no hard-coded radius.
//
// Content is either initials (label-large) or a 24 px Material Symbols
// glyph. Both are optional; an empty avatar still paints the disc so the
// layout does not jump when an image/icon is loaded later.
//
// Not interactive: no state layer, no ripple, no focus. Like MdBadge it is
// a display slot.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtGui/QColor>
#include <QtWidgets/QWidget>

namespace md {

class QT_MD3_EXPORT MdAvatar : public QWidget
{
    Q_OBJECT

    /// Initials drawn in the centre. Empty when the icon slot is used.
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// Material Symbols name drawn in the centre. Empty when initials are used.
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    /// The shape-scale corner the disc is rounded with.
    Q_PROPERTY(md::ShapeCorner corner READ corner WRITE setCorner NOTIFY cornerChanged)
    /// Preferred edge length in logical px. The widget still honours its
    /// layout's geometry; this drives sizeHint().
    Q_PROPERTY(int avatarSize READ avatarSize WRITE setAvatarSize NOTIFY avatarSizeChanged)
    /// Background override. An invalid colour means the surface-variant token.
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
                   backgroundColorChanged)
    /// Content override. An invalid colour means the on-surface-variant token.
    Q_PROPERTY(QColor contentColor READ contentColor WRITE setContentColor NOTIFY
                   contentColorChanged)

public:
    explicit MdAvatar(QWidget *parent = nullptr);
    explicit MdAvatar(const QString &text, QWidget *parent = nullptr);
    ~MdAvatar() override;

    // --- content ------------------------------------------------------------
    QString text() const { return m_text; }
    void setText(const QString &text);

    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    /// One or two initials derived from `name` ("Alice Lee" -> "AL",
    /// "alice" -> "A"). Stored as text(); kept separate so callers can
    /// refresh initials after a name change without re-deriving by hand.
    static QString initialsFrom(const QString &name);

    // --- appearance ---------------------------------------------------------
    ShapeCorner corner() const { return m_corner; }
    void setCorner(ShapeCorner corner);

    int avatarSize() const { return m_avatarSize; }
    void setAvatarSize(int size);

    QColor backgroundColor() const { return m_backgroundOverride; }
    void setBackgroundColor(const QColor &color);

    QColor contentColor() const { return m_contentOverride; }
    void setContentColor(const QColor &color);

    /// Resolved background: the override when set, else surface-variant.
    QColor resolvedBackgroundColor() const;
    /// Resolved content: the override when set, else on-surface-variant.
    QColor resolvedContentColor() const;

    // --- geometry -----------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void textChanged(const QString &text);
    void iconNameChanged(const QString &iconName);
    void cornerChanged(md::ShapeCorner corner);
    void avatarSizeChanged(int size);
    void backgroundColorChanged(const QColor &color);
    void contentColorChanged(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();

    QString m_text;
    QString m_iconName;
    ShapeCorner m_corner = ShapeCorner::Full;
    int m_avatarSize = 40;
    QColor m_backgroundOverride;
    QColor m_contentOverride;
};

} // namespace md

#endif // MD_AVATAR_H
