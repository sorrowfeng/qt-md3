#include "MdTheme.h"

#include "MdTypeScale.h"

#include <QtWidgets/QApplication>

namespace {
/// QApplication::setLayoutDirection covers the whole widget tree, so a
/// direction change reaches QLineEdit & co. as well as the self-painted
/// styles (which read widget->layoutDirection()).
void applyLayoutDirection(Qt::LayoutDirection direction)
{
    if (qApp != nullptr) {
        qApp->setLayoutDirection(direction);
    }
}
} // namespace

namespace md {

MdTheme &MdTheme::instance()
{
    static MdTheme theme;
    return theme;
}

MdTheme::MdTheme(QObject *parent)
    : QObject(parent)
{
    m_scheme = MdColorScheme::baseline(m_mode, m_contrast);
    m_schemeDirty = false;
}

template <typename Fn>
void MdTheme::applyChange(Fn &&mutate)
{
    emit themeAboutToChange();
    mutate();
    m_schemeDirty = true;
    emit themeChanged();
}

void MdTheme::rebuildScheme()
{
    if (m_dynamicColor) {
        m_scheme = MdColorScheme::dynamic(m_seedColor, m_mode, m_variant, m_contrast);
    } else {
        // The static baseline honours the contrast level too: material-web
        // publishes six sets, one per (mode, level) pair.
        m_scheme = MdColorScheme::baseline(m_mode, m_contrast);
    }
    m_schemeDirty = false;
}

const MdColorScheme &MdTheme::scheme() const
{
    if (m_schemeDirty) {
        const_cast<MdTheme *>(this)->rebuildScheme();
    }
    return m_scheme;
}

QColor MdTheme::color(ColorRole role) const
{
    return scheme().color(role);
}

void MdTheme::setThemeMode(ThemeMode mode)
{
    if (mode == m_mode) {
        return;
    }
    applyChange([this, mode] { m_mode = mode; });
    emit themeModeChanged(m_mode);
}

void MdTheme::toggleThemeMode()
{
    setThemeMode(m_mode == ThemeMode::Light ? ThemeMode::Dark : ThemeMode::Light);
}

void MdTheme::setDynamicColor(bool enabled)
{
    if (enabled == m_dynamicColor) {
        return;
    }
    applyChange([this, enabled] { m_dynamicColor = enabled; });
}

void MdTheme::setSeedColor(const QColor &seed)
{
    if (seed == m_seedColor) {
        return;
    }
    applyChange([this, seed] { m_seedColor = seed; });
}

void MdTheme::setSchemeVariant(SchemeVariant variant)
{
    if (variant == m_variant) {
        return;
    }
    applyChange([this, variant] { m_variant = variant; });
}

void MdTheme::setContrastLevel(ContrastLevel level)
{
    if (level == m_contrast) {
        return;
    }
    applyChange([this, level] { m_contrast = level; });
}

void MdTheme::setDensity(Density density)
{
    if (density == m_density) {
        return;
    }
    applyChange([this, density] { m_density = density; });
    emit densityChanged(m_density);
}

void MdTheme::setDirection(Qt::LayoutDirection direction)
{
    if (direction == m_direction) {
        return;
    }
    applyChange([this, direction] { m_direction = direction; });
    applyLayoutDirection(direction);
    emit directionChanged(m_direction);
}

void MdTheme::setLanguageTag(const QString &languageTag)
{
    const QString tag = languageTag.trimmed().isEmpty() ? QStringLiteral("en")
                                                       : languageTag.trimmed();
    if (tag == m_languageTag) {
        return;
    }
    applyChange([this, tag] { m_languageTag = tag; });
    emit languageChanged(m_languageTag);
}

ScriptCategory MdTheme::scriptCategory() const
{
    return MdTypeScale::scriptCategoryForLanguage(m_languageTag);
}

} // namespace md
