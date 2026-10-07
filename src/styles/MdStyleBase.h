#ifndef MD_STYLE_BASE_H
#define MD_STYLE_BASE_H

// MdStyleBase — the QProxyStyle base every Md*Style derives from.
//
// Three rendering patterns are supported (see AGENTS.md):
//
//   Pattern A (preferred) — a custom QWidget registers an Md*Style with
//       installPaintFilter<T>(); the style paints it in drawWidget().
//   Pattern B — subclasses of standard Qt controls override drawControl() /
//       drawComplexControl() as usual.
//   Pattern C — self-contained containers paint themselves in paintEvent().
//
// Theme refresh deliberately does NOT scan the widget tree: widgets subscribe
// individually through connectThemeUpdate<T>().

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>
#include <QtGui/QBrush>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtWidgets/QProxyStyle>

class QEvent;
class QObject;
class QWidget;

namespace md {

class QT_MD3_EXPORT MdStyleBase : public QProxyStyle
{
    Q_OBJECT

public:
    explicit MdStyleBase(QObject *parent = nullptr);
    ~MdStyleBase() override;

    /// Hook the Paint event of every widget of type T (including subclasses)
    /// and route it to `style->drawWidget()`.
    template <typename T>
    static void installPaintFilter(MdStyleBase *style)
    {
        registerPaintFilter(&T::staticMetaObject, style);
    }

    /// Stop painting widgets of type T with a filter.
    template <typename T>
    static void removePaintFilter()
    {
        unregisterPaintFilter(&T::staticMetaObject);
    }

    /// Subscribe `object`'s slot to the theme lifecycle. This is the only
    /// sanctioned way to react to a theme change.
    template <typename T>
    static void connectThemeUpdate(T *object, void (T::*slot)())
    {
        QObject::connect(&MdTheme::instance(), &MdTheme::themeChanged, object, slot);
    }

    /// Same, for slots that want to run before values change.
    template <typename T>
    static void connectThemeAboutToChange(T *object, void (T::*slot)())
    {
        QObject::connect(&MdTheme::instance(), &MdTheme::themeAboutToChange, object, slot);
    }

    /// Paint a widget this style has been installed on.
    /// Pattern A subclasses implement this.
    virtual void drawWidget(QPainter *painter, QWidget *widget);

    /// Called when the theme changed. Override to drop cached metrics.
    /// The base implementation does nothing.
    virtual void onThemeUpdate();

    /// Rounded rectangle snapped to the pixel grid so a 1 px outline stays
    /// crisp instead of blurring across two device pixels.
    static QPainterPath crispRoundedRectPath(const QRectF &rect, const QList<qreal> &radii);
    static QPainterPath crispRoundedRectPath(const QRectF &rect, qreal radius);

    /// Convenience: anti-aliased fill + optional outline of a rounded rect.
    static void drawCrispRoundedRect(QPainter *painter,
                                     const QRectF &rect,
                                     const QList<qreal> &radii,
                                     const QBrush &brush,
                                     const QPen &pen = QPen(Qt::NoPen));

    /// True when a filter is registered for exactly this type.
    static bool hasPaintFilter(const QMetaObject *metaObject);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    static void registerPaintFilter(const QMetaObject *metaObject, MdStyleBase *style);
    static void unregisterPaintFilter(const QMetaObject *metaObject);
};

} // namespace md

#endif // MD_STYLE_BASE_H
