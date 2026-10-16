#ifndef MD_EMPTY_STATE_H
#define MD_EMPTY_STATE_H

// MdEmptyState — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "Display typography 插画 + shape + Text button".
// MD3 has no empty-state family; the control is the standard "nothing here
// yet" composition every Material list / detail surface eventually needs:
//
//   art         a `surface-variant` shape-scale disc (or a caller widget)
//               carrying a Material Symbols glyph in `on-surface-variant`
//   title       Display / Headline type in `on-surface`
//   description Body-large / body-medium in `on-surface-variant`
//   action      an optional Text button (`md.comp.button.text`) in `primary`
//
// Nothing here invents a token: the art disc is `surface-variant` +
// `ShapeCorner::ExtraLarge` (the same identity mark MdAvatar promotes), the
// type styles come from the type scale, and the action is a real `MdButton`
// with `ButtonVariant::Text` so its ripple / state layer / focus ring are
// the Stage 1 ones.
//
// Layout is a centred column. The widget itself paints only the art disc and
// the text; the action is a child widget so it stays interactive and
// accessible.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

class MdButton;

class QT_MD3_EXPORT MdEmptyState : public QWidget
{
    Q_OBJECT

    /// Headline text. Drawn with HeadlineSmall (Display for very large layouts
    /// is available via `titleStyle`).
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// Supporting text under the title. Empty collapses the row.
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY
                   descriptionChanged)
    /// Material Symbols name painted in the art disc. Empty keeps the bare disc.
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    /// Text of the optional action button. Empty hides the button.
    Q_PROPERTY(QString actionText READ actionText WRITE setActionText NOTIFY actionTextChanged)
    /// Type style used for the title.
    Q_PROPERTY(md::TypeStyle titleStyle READ titleStyle WRITE setTitleStyle NOTIFY titleStyleChanged)
    /// Edge length of the art disc, in logical px. 0 means the default 96.
    Q_PROPERTY(int artSize READ artSize WRITE setArtSize NOTIFY artSizeChanged)
    /// Shape-scale corner of the art disc.
    Q_PROPERTY(md::ShapeCorner artCorner READ artCorner WRITE setArtCorner NOTIFY artCornerChanged)
    /// Replace the painted disc with a caller-owned widget (illustration).
    Q_PROPERTY(QWidget *artWidget READ artWidget WRITE setArtWidget NOTIFY artWidgetChanged)

public:
    explicit MdEmptyState(QWidget *parent = nullptr);
    explicit MdEmptyState(const QString &title, QWidget *parent = nullptr);
    ~MdEmptyState() override;

    // --- content ------------------------------------------------------------
    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString description() const { return m_description; }
    void setDescription(const QString &description);

    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    QString actionText() const { return m_actionText; }
    void setActionText(const QString &text);

    // --- appearance ---------------------------------------------------------
    TypeStyle titleStyle() const { return m_titleStyle; }
    void setTitleStyle(TypeStyle style);

    int artSize() const { return m_artSize; }
    void setArtSize(int size);

    ShapeCorner artCorner() const { return m_artCorner; }
    void setArtCorner(ShapeCorner corner);

    QWidget *artWidget() const { return m_artWidget; }
    void setArtWidget(QWidget *widget);

    /// Resolved art-disc background: surface-variant.
    QColor resolvedArtBackground() const;
    /// Resolved art content colour: on-surface-variant.
    QColor resolvedArtContent() const;

    /// The action button (nullptr only before init — always present after).
    MdButton *actionButton() const { return m_actionButton; }

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void titleChanged(const QString &title);
    void descriptionChanged(const QString &description);
    void iconNameChanged(const QString &iconName);
    void actionTextChanged(const QString &text);
    void titleStyleChanged(md::TypeStyle style);
    void artSizeChanged(int size);
    void artCornerChanged(md::ShapeCorner corner);
    void artWidgetChanged(QWidget *artWidget);
    /// Emitted when the action button is clicked.
    void actionTriggered();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void layoutAction();
    QRectF artRect(const QRectF &content) const;
    QRectF titleRect(const QRectF &content) const;
    QRectF descriptionRect(const QRectF &content) const;
    QRectF actionRect(const QRectF &content) const;

    QString m_title;
    QString m_description;
    QString m_iconName;
    QString m_actionText;
    TypeStyle m_titleStyle = TypeStyle::HeadlineSmall;
    int m_artSize = 96;
    ShapeCorner m_artCorner = ShapeCorner::ExtraLarge;
    QWidget *m_artWidget = nullptr;
    MdButton *m_actionButton = nullptr;
};

} // namespace md

#endif // MD_EMPTY_STATE_H
