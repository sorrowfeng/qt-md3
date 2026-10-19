#ifndef MD_MINI_PLAYER_H
#define MD_MINI_PLAYER_H

// MdMiniPlayer — Stage 2 Qt extension: the persistent media mini bar.
//
// MD3 has no mini-player family. The control is the standard "now playing"
// strip every media app keeps above the navigation: a thumbnail, title /
// artist, a thin seek slider, and transport buttons. The chrome is the card
// family's filled surface (`md.comp.filled-card.*`) at the medium corner;
// the buttons are Stage 1 `MdIconButton`s and the progress is `MdSeekSlider`.
//
// No QSS. Every colour / radius / type style comes from the theme.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtGui/QColor>
#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

class QPaintEvent;
class QResizeEvent;

namespace md {

class MdIconButton;
class MdSeekSlider;

class QT_MD3_EXPORT MdMiniPlayer : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY artistChanged)
    Q_PROPERTY(QPixmap thumbnail READ thumbnail WRITE setThumbnail NOTIFY thumbnailChanged)
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(qreal max READ max WRITE setMax NOTIFY maxChanged)
    Q_PROPERTY(bool playing READ isPlaying WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool progressVisible READ isProgressVisible WRITE setProgressVisible NOTIFY
                   progressVisibleChanged)

public:
    explicit MdMiniPlayer(QWidget *parent = nullptr);
    ~MdMiniPlayer() override;

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString artist() const { return m_artist; }
    void setArtist(const QString &artist);

    QPixmap thumbnail() const { return m_thumbnail; }
    void setThumbnail(const QPixmap &pixmap);

    qreal value() const;
    void setValue(qreal value);

    qreal max() const;
    void setMax(qreal max);

    bool isPlaying() const { return m_playing; }
    void setPlaying(bool playing);

    bool isProgressVisible() const { return m_progressVisible; }
    void setProgressVisible(bool visible);

    /// The transport / progress children, for hosts that wire tooltips or
    /// accessible names.
    MdIconButton *playButton() const { return m_playButton; }
    MdIconButton *nextButton() const { return m_nextButton; }
    MdSeekSlider *progress() const { return m_progress; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void titleChanged(const QString &title);
    void artistChanged(const QString &artist);
    void thumbnailChanged(const QPixmap &pixmap);
    void valueChanged(qreal value);
    void maxChanged(qreal max);
    void playingChanged(bool playing);
    void progressVisibleChanged(bool visible);
    void playClicked();
    void nextClicked();
    void seekRequested(qreal value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void layoutParts();
    void refreshPlayIcon();

    QString m_title;
    QString m_artist;
    QPixmap m_thumbnail;
    bool m_playing = false;
    bool m_progressVisible = true;

    MdIconButton *m_playButton = nullptr;
    MdIconButton *m_nextButton = nullptr;
    MdSeekSlider *m_progress = nullptr;
};

} // namespace md

#endif // MD_MINI_PLAYER_H
