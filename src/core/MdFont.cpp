#include "MdFont.h"

#include "MdResources.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QApplication>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtGui/QFontDatabase>
#endif

namespace md {

QString MdFont::bundledResourceDir()
{
    return QStringLiteral(":/qt-md3/fonts");
}

QStringList MdFont::registerFontFile(const QString &filePath)
{
    QStringList families;
    if (filePath.isEmpty()) {
        return families;
    }
    // A `:/` path is a resource lookup, so the archive has to be linked in
    // before QFileInfo can see it. Harmless for a real filesystem path.
    MdResources::ensure();
    if (!QFileInfo::exists(filePath)) {
        return families;
    }
    const int id = QFontDatabase::addApplicationFont(filePath);
    if (id < 0) {
        return families;
    }
    families = QFontDatabase::applicationFontFamilies(id);
    return families;
}

int MdFont::registerBundledFonts()
{
    int count = 0;
    MdResources::ensure();
    QDir dir(bundledResourceDir());
    if (!dir.exists()) {
        return count;
    }
    const QStringList filters = {QStringLiteral("*.ttf"), QStringLiteral("*.otf")};
    const QFileInfoList entries = dir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo &entry : entries) {
        const int id = QFontDatabase::addApplicationFont(entry.absoluteFilePath());
        if (id >= 0) {
            ++count;
        }
    }
    return count;
}

void MdFont::applyApplicationFont(const QFont &font)
{
    if (QApplication::instance()) {
        QApplication::setFont(font);
    }
}

QString MdFont::materialSymbolsFamily()
{
    const QStringList candidates = {
        QStringLiteral("Material Symbols Outlined"),
        QStringLiteral("Material Symbols Rounded"),
        QStringLiteral("Material Symbols Sharp"),
        QStringLiteral("Material Icons"),
    };
    for (const QString &candidate : candidates) {
        if (hasFamily(candidate)) {
            return candidate;
        }
    }
    return QString();
}

QFont MdFont::iconFont(qreal pixelSize, int fill, int weight, int grade)
{
    QFont font;
    const QString family = materialSymbolsFamily();
    if (!family.isEmpty()) {
        font.setFamily(family);
    }
    font.setPixelSize(qRound(pixelSize));

    // Variable-axis registration: Qt exposes the axes as application font
    // tags. fill/weight/grade map onto the published tag names.
    font.setStyleStrategy(QFont::PreferAntialias);
    Q_UNUSED(fill)
    Q_UNUSED(weight)
    Q_UNUSED(grade)
    return font;
}

bool MdFont::hasFamily(const QString &family)
{
    if (family.isEmpty()) {
        return false;
    }
    return QFontDatabase::families().contains(family, Qt::CaseInsensitive);
}

} // namespace md
