#include "MdStyleBase.h"

#include "core/MdShape.h"

#include <QtCore/QEvent>
#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <cmath>

namespace md {

namespace {

/// Central registry + application-wide Paint interceptor.
///
/// A widget of type T registered with installPaintFilter<T>() gets its Paint
/// event consumed and handed to the style's drawWidget(), including widgets of
/// derived types.
class MdPaintFilterHub : public QObject
{
public:
    static MdPaintFilterHub *instance()
    {
        // Intentionally never destroyed: it must outlive every widget.
        static MdPaintFilterHub *hub = new MdPaintFilterHub;
        return hub;
    }

    void add(const QMetaObject *metaObject, MdStyleBase *style)
    {
        QMutexLocker locker(&m_mutex);
        m_styles.insert(metaObject, style);
        ensureInstalled();
    }

    void remove(const QMetaObject *metaObject)
    {
        QMutexLocker locker(&m_mutex);
        m_styles.remove(metaObject);
    }

    void removeStyle(MdStyleBase *style)
    {
        QMutexLocker locker(&m_mutex);
        for (auto it = m_styles.begin(); it != m_styles.end();) {
            if (it.value() == style) {
                it = m_styles.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool contains(const QMetaObject *metaObject) const
    {
        QMutexLocker locker(&m_mutex);
        return m_styles.contains(metaObject);
    }

    MdStyleBase *styleFor(const QMetaObject *metaObject) const
    {
        for (const QMetaObject *current = metaObject; current; current = current->superClass()) {
            QMutexLocker locker(&m_mutex);
            const auto it = m_styles.constFind(current);
            if (it != m_styles.constEnd() && it.value()) {
                return it.value();
            }
        }
        return nullptr;
    }

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() != QEvent::Paint) {
            return false;
        }
        auto *widget = qobject_cast<QWidget *>(object);
        if (!widget) {
            return false;
        }
        MdStyleBase *style = styleFor(widget->metaObject());
        if (!style) {
            return false;
        }
        QPainter painter(widget);
        style->drawWidget(&painter, widget);
        return true;
    }

private:
    void ensureInstalled()
    {
        if (m_installed || !QApplication::instance()) {
            return;
        }
        QApplication::instance()->installEventFilter(this);
        m_installed = true;
    }

    mutable QMutex m_mutex;
    QHash<const QMetaObject *, MdStyleBase *> m_styles;
    bool m_installed = false;
};

/// Snap a rect to whole device pixels so 1 px strokes do not straddle two.
QRectF snapped(const QRectF &rect)
{
    const auto snap = [](qreal value) { return std::round(value); };
    const qreal left = snap(rect.left());
    const qreal top = snap(rect.top());
    const qreal right = snap(rect.right());
    const qreal bottom = snap(rect.bottom());
    return QRectF(left, top, right - left, bottom - top);
}

} // namespace

MdStyleBase::MdStyleBase(QObject *parent)
    : QProxyStyle()
{
    Q_UNUSED(parent)
    // The style invalidates its own caches whenever the theme changes.
    QObject::connect(&MdTheme::instance(), &MdTheme::themeChanged, this, [this] { onThemeUpdate(); });
}

MdStyleBase::~MdStyleBase()
{
    MdPaintFilterHub::instance()->removeStyle(this);
}

void MdStyleBase::registerPaintFilter(const QMetaObject *metaObject, MdStyleBase *style)
{
    if (!metaObject || !style) {
        return;
    }
    MdPaintFilterHub::instance()->add(metaObject, style);
}

void MdStyleBase::unregisterPaintFilter(const QMetaObject *metaObject)
{
    if (!metaObject) {
        return;
    }
    MdPaintFilterHub::instance()->remove(metaObject);
}

bool MdStyleBase::isRtl(const QWidget *widget)
{
    if (widget != nullptr) {
        return widget->layoutDirection() == Qt::RightToLeft;
    }
    return QApplication::layoutDirection() == Qt::RightToLeft;
}

qreal MdStyleBase::mirroredX(qreal x, qreal width)
{
    return width - x;
}

QRectF MdStyleBase::mirroredRect(const QRectF &rect, qreal width)
{
    return QRectF(width - rect.right(), rect.y(), rect.width(), rect.height());
}

Qt::Alignment MdStyleBase::leadingAlignment(const QWidget *widget)
{
    return isRtl(widget) ? Qt::AlignRight : Qt::AlignLeft;
}

Qt::Alignment MdStyleBase::trailingAlignment(const QWidget *widget)
{
    return isRtl(widget) ? Qt::AlignLeft : Qt::AlignRight;
}

bool MdStyleBase::hasPaintFilter(const QMetaObject *metaObject)
{
    return MdPaintFilterHub::instance()->contains(metaObject);
}

bool MdStyleBase::eventFilter(QObject *object, QEvent *event)
{
    // The application-wide interception lives in MdPaintFilterHub, because a
    // QStyle is not the parent of anything and would never see its widgets'
    // events on its own. This override exists so a subclass can still do its
    // own per-object filtering without losing the base behaviour.
    return QProxyStyle::eventFilter(object, event);
}

void MdStyleBase::drawWidget(QPainter *painter, QWidget *widget)
{
    // Pattern A subclasses paint here. The base implementation just paints the
    // widget's palette background so an un-overridden style is not invisible.
    Q_UNUSED(painter)
    Q_UNUSED(widget)
}

void MdStyleBase::onThemeUpdate()
{
    // Nothing to do by default; subclasses drop cached metrics here.
}

QPainterPath MdStyleBase::crispRoundedRectPath(const QRectF &rect, const QList<qreal> &radii)
{
    return MdShape::roundedRect(snapped(rect), radii);
}

QPainterPath MdStyleBase::crispRoundedRectPath(const QRectF &rect, qreal radius)
{
    return crispRoundedRectPath(rect, QList<qreal>{radius, radius, radius, radius});
}

void MdStyleBase::drawCrispRoundedRect(QPainter *painter,
                                       const QRectF &rect,
                                       const QList<qreal> &radii,
                                       const QBrush &brush,
                                       const QPen &pen)
{
    if (!painter || rect.isEmpty()) {
        return;
    }

    QRectF target = rect;
    // An odd-width outline sits on a half-pixel inset so it lands on one row
    // of device pixels.
    const bool hasPen = pen.style() != Qt::NoPen && pen.widthF() > 0.0;
    if (hasPen && std::fmod(pen.widthF(), 2.0) != 0.0) {
        target = target.adjusted(0.5, 0.5, -0.5, -0.5);
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(brush);
    painter->setPen(pen);
    painter->drawPath(MdShape::roundedRect(target, radii));
    painter->restore();
}

} // namespace md
