#include "MdIcon.h"

#include "MdFont.h"
#include "MdResources.h"

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QFontInfo>
#include <QtGui/QFontMetricsF>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtSvg/QSvgRenderer>
#include <QtGui/QTransform>

#include <cmath>

namespace md {

namespace {

/// The published codepoint table lives in the resource bundle. It is
/// generated from google/material-design-icons'
/// variablefont/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].codepoints.
constexpr char kCodepointsPath[] = ":/qt-md3/icons/material-symbols-codepoints.txt";
constexpr char kClassicDir[] = ":/qt-md3/icons/classic/24px";

/// `md.comp.icon.size`.
constexpr qreal kDefaultSize = 24.0;

QHash<QString, uint> &codepointTable()
{
    static QHash<QString, uint> table;
    return table;
}

bool &codepointsLoaded()
{
    static bool loaded = false;
    return loaded;
}

/// SVG rasterisations, keyed by "name@size#aarrggbb".
QHash<QString, QPixmap> &pixmapCache()
{
    static QHash<QString, QPixmap> cache;
    return cache;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
constexpr bool kAxesSupported = true;
#else
constexpr bool kAxesSupported = false;
#endif

} // namespace

// ---------------------------------------------------------------------------
// Codepoint table
// ---------------------------------------------------------------------------

QString MdIcon::codepointsResourcePath()
{
    return QString::fromUtf8(kCodepointsPath);
}

bool MdIcon::loadCodepointsFromData(const QByteArray &data)
{
    if (data.isEmpty()) {
        return false;
    }
    QHash<QString, uint> parsed;
    parsed.reserve(4608);

    const QList<QByteArray> lines = data.split('\n');
    for (const QByteArray &rawLine : lines) {
        // Upstream format is "name codepoint" with a single space. Be tolerant
        // of trailing whitespace and CRLF, but not of anything else, so a
        // malformed file is reported rather than silently half-parsed.
        const QByteArray line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        const int space = line.indexOf(' ');
        if (space <= 0) {
            continue;
        }
        const QByteArray name = line.left(space).trimmed();
        const QByteArray hex = line.mid(space + 1).trimmed();
        bool ok = false;
        const uint value = hex.toUInt(&ok, 16);
        if (!ok || value == 0) {
            continue;
        }
        parsed.insert(QString::fromUtf8(name), value);
    }

    if (parsed.isEmpty()) {
        return false;
    }
    codepointTable() = parsed;
    codepointsLoaded() = true;
    return true;
}

bool MdIcon::loadCodepoints(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    return loadCodepointsFromData(file.readAll());
}

bool MdIcon::loadCodepoints()
{
    if (codepointsLoaded()) {
        return true;
    }
    // Mark as attempted even on failure so a miss does not re-stat the
    // resource on every lookup; contains() still reports false.
    codepointsLoaded() = true;
    MdResources::ensure();
    return loadCodepoints(codepointsResourcePath());
}

void MdIcon::registerIcon(const QString &name, uint codepointValue)
{
    if (name.isEmpty() || codepointValue == 0) {
        return;
    }
    codepointTable().insert(name, codepointValue);
    codepointsLoaded() = true;
}

bool MdIcon::contains(const QString &name)
{
    loadCodepoints();
    return codepointTable().contains(name);
}

uint MdIcon::codepoint(const QString &name)
{
    loadCodepoints();
    return codepointTable().value(name, 0u);
}

QString MdIcon::glyph(const QString &name)
{
    const uint value = codepoint(name);
    if (value == 0) {
        return QString();
    }
    // Material Symbols codepoints live in the Private Use Area, so they are
    // always a single UTF-16 code unit.
    return QString(QChar(ushort(value)));
}

QStringList MdIcon::names()
{
    loadCodepoints();
    QStringList result = codepointTable().keys();
    result.sort();
    return result;
}

int MdIcon::count()
{
    loadCodepoints();
    return codepointTable().size();
}

// ---------------------------------------------------------------------------
// Material Symbols font
// ---------------------------------------------------------------------------

QString MdIcon::fontFamily(MdIconFamily family)
{
    switch (family) {
    case MdIconFamily::Outlined: return QStringLiteral("Material Symbols Outlined");
    case MdIconFamily::Rounded: return QStringLiteral("Material Symbols Rounded");
    case MdIconFamily::Sharp: return QStringLiteral("Material Symbols Sharp");
    case MdIconFamily::Count: break;
    }
    return QStringLiteral("Material Symbols Outlined");
}

qreal MdIcon::defaultSize()
{
    return kDefaultSize;
}

QFont MdIcon::font(MdIconFamily family, const MdIconStyle &style, qreal pixelSize)
{
    QFont result(fontFamily(family));
    result.setPixelSize(qMax<qreal>(pixelSize, 1.0));
    // material-web's _icon.scss: font-weight 400, line-height 1,
    // letter-spacing normal.
    result.setWeight(QFont::Weight(qBound(100, int(std::lround(style.weight)), 700)));
    result.setLetterSpacing(QFont::AbsoluteSpacing, 0.0);
    result.setStyleStrategy(QFont::PreferAntialias);

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    // The four Material Symbols axes, in the font's own tag order.
    result.setVariableAxis(QFont::Tag("FILL"), float(style.fill));
    result.setVariableAxis(QFont::Tag("wght"), float(style.weight));
    result.setVariableAxis(QFont::Tag("GRAD"), float(style.grade));
    result.setVariableAxis(QFont::Tag("opsz"), float(style.opticalSize));
#else
    Q_UNUSED(style);
#endif
    return result;
}

bool MdIcon::isFontAvailable(MdIconFamily family)
{
    const QString expected = fontFamily(family);
    const QFont probe(expected);
    const QFontInfo info(probe);
    // QFontInfo::family() returns the resolved face; when the requested family
    // is missing Qt substitutes one and the names differ.
    return info.family().compare(expected, Qt::CaseInsensitive) == 0;
}

bool MdIcon::axesSupported()
{
    return kAxesSupported;
}

// ---------------------------------------------------------------------------
// Classic Material Icons SVG baseline
// ---------------------------------------------------------------------------

QString MdIcon::classicResourceDir()
{
    return QString::fromUtf8(kClassicDir);
}

QStringList MdIcon::classicNames()
{
    MdResources::ensure();
    QDir dir(classicResourceDir());
    QStringList result = dir.entryList({QStringLiteral("*.svg")}, QDir::Files);
    for (QString &entry : result) {
        entry.chop(4); // ".svg"
    }
    result.sort();
    return result;
}

bool MdIcon::hasClassicIcon(const QString &name)
{
    return !name.isEmpty() && QFileInfo::exists(classicPath(name));
}

QString MdIcon::classicPath(const QString &name)
{
    if (name.isEmpty()) {
        return QString();
    }
    // Every classic-SVG accessor funnels through here, so this is the one place
    // that has to make sure the resource archive was actually linked in.
    MdResources::ensure();
    return QStringLiteral("%1/%2.svg").arg(classicResourceDir(), name);
}

QPixmap MdIcon::classicPixmap(const QString &name, int size, const QColor &color)
{
    if (size <= 0) {
        return QPixmap();
    }
    const QString key = QStringLiteral("%1@%2#%3")
                            .arg(name)
                            .arg(size)
                            .arg(color.rgba(), 8, 16, QLatin1Char('0'));
    const auto cached = pixmapCache().constFind(key);
    if (cached != pixmapCache().constEnd()) {
        return cached.value();
    }

    const QString path = classicPath(name);
    if (!QFileInfo::exists(path)) {
        pixmapCache().insert(key, QPixmap());
        return QPixmap();
    }

    QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QSvgRenderer renderer(path);
        if (!renderer.isValid()) {
            pixmapCache().insert(key, QPixmap());
            return QPixmap();
        }
        renderer.render(&painter, QRectF(0, 0, size, size));
        // The upstream SVGs are pure black; recolour by keeping the alpha.
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(image.rect(), color);
    }
    const QPixmap pixmap = QPixmap::fromImage(image);
    pixmapCache().insert(key, pixmap);
    return pixmap;
}

// ---------------------------------------------------------------------------
// Unified entry points
// ---------------------------------------------------------------------------

MdIconSet MdIcon::resolveSet(MdIconSet set)
{
    if (set == MdIconSet::Auto) {
        // Symbols are the MD3 default; the SVG baseline is the fallback.
        return isFontAvailable(MdIconFamily::Outlined) ? MdIconSet::MaterialSymbols
                                                       : MdIconSet::Classic;
    }
    if (set == MdIconSet::MaterialSymbols && !isFontAvailable(MdIconFamily::Outlined)) {
        return MdIconSet::Classic;
    }
    return set;
}

QSizeF MdIcon::preferredSize(qreal opticalSize)
{
    return QSizeF(opticalSize, opticalSize);
}

bool MdIcon::paint(QPainter *painter,
                   const QRectF &rect,
                   const QString &name,
                   const QColor &color,
                   MdIconSet set,
                   MdIconFamily family,
                   const MdIconStyle &style)
{
    if (painter == nullptr || name.isEmpty() || rect.isEmpty()) {
        return false;
    }

    const MdIconSet resolved = resolveSet(set);

    if (resolved == MdIconSet::MaterialSymbols) {
        const QString character = glyph(name);
        const QFont iconFont = font(family, style, rect.height());
        if (!character.isEmpty()) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setRenderHint(QPainter::TextAntialiasing, true);
            painter->setFont(iconFont);
            painter->setPen(color);
            painter->setBrush(Qt::NoBrush);

            // material-web boxes the icon as a 1em square with `line-height: 1`
            // and centres it with flexbox. In CSS the baseline then lands at
            // `center.y + (ascent - descent) / 2`; reproducing that offset here
            // keeps a Symbols glyph optically centred rather than box-centred.
            const QFontMetricsF metrics(iconFont);
            const qreal baseline =
                rect.center().y() + (metrics.ascent() - metrics.descent()) / 2.0;
            const qreal advance = metrics.horizontalAdvance(character);
            painter->drawText(QPointF(rect.center().x() - advance / 2.0, baseline),
                              character);
            painter->restore();
            return true;
        }
        // Known glyph but the table has no entry: fall through to the SVG
        // baseline, which may yet have the icon under the same name.
    }

    // The classic SVGs are authored on a 24dp grid and rasterised to the
    // requested box, then centred.
    const int pixelSize = int(std::lround(rect.height()));
    if (pixelSize > 0) {
        const QPixmap pixmap = classicPixmap(name, pixelSize, color);
        if (!pixmap.isNull()) {
            QRectF target(QPointF(0, 0), pixmap.size());
            target.moveCenter(rect.center());
            painter->save();
            painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
            painter->drawPixmap(target.topLeft(), pixmap);
            painter->restore();
            return true;
        }
    }
    return false;
}

} // namespace md
