#ifndef MD_RESULT_H
#define MD_RESULT_H

// MdResult — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "Display typography + shape + Text button".
// The status-page companion to MdEmptyState: the same centred column of art
// disc + display title + body description + Text button, but the disc and
// accent carry a *status* (success / info / warning / error) instead of a
// neutral empty-list mark.
//
//   art      a shape-scale disc tinted by the status's semantic colour
//   title    Display / Headline type in on-surface
//   subtitle Body-large in on-surface-variant
//   extra    Body-small supporting line under the subtitle
//   action   an optional Text / Filled MdButton
//
// Semantic colours reuse the roles the library already publishes — `primary`
// for success and info (the positive outcomes are not a separate MD3 role),
// `tertiary` for warning, `error` for error. Nothing here invents a token.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

class MdButton;

/// Outcome the result page reports. Drives the art-disc tint and default glyph.
enum class MdResultStatus {
    Success,
    Info,
    Warning,
    Error,
    Custom,
    Count,
};

class QT_MD3_EXPORT MdResult : public QWidget
{
    Q_OBJECT

    /// Outcome the page reports.
    Q_PROPERTY(md::MdResultStatus status READ status WRITE setStatus NOTIFY statusChanged)
    /// Headline text (HeadlineMedium by default).
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// Supporting line under the title.
    Q_PROPERTY(QString subtitle READ subtitle WRITE setSubtitle NOTIFY subtitleChanged)
    /// Optional extra detail line under the subtitle.
    Q_PROPERTY(QString extraText READ extraText WRITE setExtraText NOTIFY extraTextChanged)
    /// Material Symbols glyph in the art disc. Empty uses the status default.
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    /// Text of the primary action. Empty hides the button.
    Q_PROPERTY(QString actionText READ actionText WRITE setActionText NOTIFY actionTextChanged)
    /// When true the action is a Filled button; when false a Text button.
    Q_PROPERTY(bool actionFilled READ isActionFilled WRITE setActionFilled NOTIFY
                   actionFilledChanged)
    /// Type style used for the title.
    Q_PROPERTY(md::TypeStyle titleStyle READ titleStyle WRITE setTitleStyle NOTIFY titleStyleChanged)
    /// Edge length of the art disc, in logical px. 0 means the default 96.
    Q_PROPERTY(int artSize READ artSize WRITE setArtSize NOTIFY artSizeChanged)

public:
    explicit MdResult(QWidget *parent = nullptr);
    MdResult(MdResultStatus status, const QString &title, QWidget *parent = nullptr);
    ~MdResult() override;

    MdResultStatus status() const { return m_status; }
    void setStatus(MdResultStatus status);

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString subtitle() const { return m_subtitle; }
    void setSubtitle(const QString &subtitle);

    QString extraText() const { return m_extraText; }
    void setExtraText(const QString &text);

    QString iconName() const { return m_iconName; }
    void setIconName(const QString &name);

    QString actionText() const { return m_actionText; }
    void setActionText(const QString &text);

    bool isActionFilled() const { return m_actionFilled; }
    void setActionFilled(bool filled);

    TypeStyle titleStyle() const { return m_titleStyle; }
    void setTitleStyle(TypeStyle style);

    int artSize() const { return m_artSize; }
    void setArtSize(int size);

    /// Semantic colour of the status: primary / tertiary / error.
    QColor resolvedStatusColor() const;
    /// Art-disc background: the status colour at the container tone.
    QColor resolvedArtBackground() const;
    /// Art-disc content colour.
    QColor resolvedArtContent() const;

    /// The action button (always created; hidden when `actionText` is empty).
    MdButton *actionButton() const { return m_actionButton; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void statusChanged(md::MdResultStatus status);
    void titleChanged(const QString &title);
    void subtitleChanged(const QString &subtitle);
    void extraTextChanged(const QString &text);
    void iconNameChanged(const QString &name);
    void actionTextChanged(const QString &text);
    void actionFilledChanged(bool filled);
    void titleStyleChanged(md::TypeStyle style);
    void artSizeChanged(int size);
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
    void syncActionVariant();
    QString defaultIconName() const;
    QRectF artRect(const QRectF &content) const;
    QRectF titleRect(const QRectF &content) const;
    QRectF subtitleRect(const QRectF &content) const;
    QRectF extraRect(const QRectF &content) const;
    QRectF actionRect(const QRectF &content) const;

    MdResultStatus m_status = MdResultStatus::Success;
    QString m_title;
    QString m_subtitle;
    QString m_extraText;
    QString m_iconName;
    QString m_actionText;
    bool m_actionFilled = false;
    TypeStyle m_titleStyle = TypeStyle::HeadlineMedium;
    int m_artSize = 96;
    MdButton *m_actionButton = nullptr;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdResultStatus)

#endif // MD_RESULT_H
