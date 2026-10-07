#ifndef GALLERY_PAGE_H
#define GALLERY_PAGE_H

// Gallery page scaffolding.
//
// These classes are *example application* code, not part of the library: they
// are deliberately not named `Md*` so they can never be confused with a Stage
// 1 component. Their only job is to give every token and every base module a
// page to be inspected on.
//
// Two rules the gallery obeys so it stays a fair test of the library:
//
//   * No widget styling at all — no QSS, no QWidget::setPalette, no
//     QWidget::setFont. Every colour, font, radius and duration is read from
//     md::MdTheme / md::MdTokens / md::MdTypeScale at paint time.
//   * Measurement and painting run the *same* code path. `GalleryContext`
//     takes a null painter during measurement, so a page can never lay itself
//     out differently from how it draws.

#include "core/MdElevation.h"
#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace gallery {

/// Sequential top-to-bottom layout cursor shared by measurement and painting.
class GalleryContext
{
public:
    GalleryContext() = default;
    GalleryContext(QPainter *painter, qreal width, qreal top);

    /// Set while measuring: helpers advance `y` but draw nothing.
    bool isMeasuring() const { return m_painter == nullptr; }
    QPainter *painter() const { return m_painter; }
    qreal width() const { return m_width; }
    qreal y() const { return m_y; }
    void setY(qreal y) { m_y = y; }

    /// Bold section heading on its own line.
    void section(const QString &text);
    /// Body copy, word wrapped, in on-surface-variant.
    void paragraph(const QString &text);
    /// Small monospace-ish detail line (hex values, token names).
    void detail(const QString &text);
    /// Vertical space.
    void space(qreal amount);

    /// Reserve a band of `height` and return its rect, advancing y past it.
    QRectF band(qreal height);

    /// Draw an inline chip. Advances y.
    void chip(const QString &text, const QColor &background, const QColor &foreground);

private:
    qreal textHeight(const QFont &font, const QString &text, qreal width) const;

    QPainter *m_painter = nullptr;
    qreal m_width = 0.0;
    qreal m_y = 0.0;
};

/// Base class for a gallery page. Pages implement build(); the base handles
/// the scroll-area height contract and theme reactivity.
class GalleryPage : public QWidget
{
    Q_OBJECT

public:
    explicit GalleryPage(QWidget *parent = nullptr);

    virtual QString title() const = 0;
    virtual QString subtitle() const = 0;
    /// Describe the page into `context`, top to bottom.
    virtual void build(GalleryContext &context) = 0;

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /// Run build() with a null painter and return the height it consumed.
    qreal measure(qreal width) const;

    /// Content rect of the most recent paint, in widget coordinates. Pages
    /// that draw interactive overlays after the base paintEvent use this to
    /// reproduce the same transform.
    QRectF contentRect() const { return m_contentRect; }

    /// Prepare a painter already translated to the content origin, mirroring
    /// what the base paintEvent does. Used by overlay drawing.
    void beginOverlay(QPainter &painter) const;

    // --- geometry for pages that own child widgets ------------------------
    //
    // A page made of token swatches only ever draws. A page made of *components*
    // owns real child widgets, and those have to be positioned before Qt paints
    // them — which is before the parent's paintEvent, so contentRect() is still
    // empty at that point. These three give such a page the same numbers the
    // base uses, so it can place its children from resizeEvent() and never have
    // to guess.

    /// Content gutter on every side of a page.
    static constexpr qreal contentGutter() { return 32.0; }

    /// The content rect for the widget's current size. Identical to the one
    /// paintEvent installs.
    QRectF contentRectForCurrentSize() const;

    /// Y at which build()'s coordinate system starts, in widget coordinates:
    /// below the page title and subtitle. `build()` is handed this as its
    /// starting cursor.
    qreal buildOriginY() const;

    /// Run build() against a null painter to refresh whatever a page records
    /// during layout, without drawing. Safe to call at any time.
    void remeasure();

    // --- shared painting helpers (all theme driven) -----------------------
    // Public because GalleryContext draws with them too.
    static QFont fontFor(md::TypeStyle style,
                         md::TypeEmphasis emphasis = md::TypeEmphasis::Baseline);
    static QColor role(md::ColorRole colorRole);

    /// A rounded, filled surface using the theme's current corner tokens.
    static void fillRounded(QPainter &painter, const QRectF &rect, const QColor &color,
                            md::ShapeCorner corner = md::ShapeCorner::Medium);
    /// The standard "card" every block sits on: surface-container + outline.
    static void paintCard(QPainter &painter, const QRectF &rect);
    /// Fill `rect` with the surface at the given elevation level.
    static void paintElevated(QPainter &painter, const QRectF &rect,
                              md::ElevationLevel level);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QRectF m_contentRect;
};

} // namespace gallery

#endif // GALLERY_PAGE_H
