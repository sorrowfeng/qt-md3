#ifndef MD_THEME_H
#define MD_THEME_H

// MdTheme — the single source of colour, density, direction and language for
// the whole application.
//
// Components never hold a colour of their own: they ask the theme. Theme
// changes are broadcast through three lifecycle signals so that painting can
// be prepared before values change and repainted after:
//
//   themeAboutToChange()  -> invalidate caches, snapshot what you need
//   (values mutate)
//   themeChanged()        -> recompute and repaint
//   themeModeChanged()    -> only when light/dark actually flipped

#include "MdColorScheme.h"
#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtGui/QColor>

namespace md {

class QT_MD3_EXPORT MdTheme : public QObject
{
    Q_OBJECT
    Q_PROPERTY(md::ThemeMode themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)
    Q_PROPERTY(bool dynamicColor READ isDynamicColor WRITE setDynamicColor NOTIFY themeChanged)
    Q_PROPERTY(QColor seedColor READ seedColor WRITE setSeedColor NOTIFY themeChanged)
    Q_PROPERTY(md::SchemeVariant schemeVariant READ schemeVariant WRITE setSchemeVariant NOTIFY themeChanged)
    Q_PROPERTY(md::ContrastLevel contrastLevel READ contrastLevel WRITE setContrastLevel NOTIFY themeChanged)
    Q_PROPERTY(md::Density density READ density WRITE setDensity NOTIFY densityChanged)
    Q_PROPERTY(Qt::LayoutDirection direction READ direction WRITE setDirection NOTIFY directionChanged)
    Q_PROPERTY(QString languageTag READ languageTag WRITE setLanguageTag NOTIFY languageChanged)

public:
    /// The application-wide theme. There is exactly one.
    static MdTheme &instance();

    // --- colour ----------------------------------------------------------
    ThemeMode themeMode() const { return m_mode; }
    void setThemeMode(ThemeMode mode);
    void toggleThemeMode();

    bool isDynamicColor() const { return m_dynamicColor; }
    void setDynamicColor(bool enabled);

    QColor seedColor() const { return m_seedColor; }
    void setSeedColor(const QColor &seed);

    SchemeVariant schemeVariant() const { return m_variant; }
    void setSchemeVariant(SchemeVariant variant);

    ContrastLevel contrastLevel() const { return m_contrast; }
    void setContrastLevel(ContrastLevel level);

    /// The currently resolved scheme (rebuilt whenever anything above changes).
    const MdColorScheme &scheme() const;
    QColor color(ColorRole role) const;

    // --- density / direction / language ----------------------------------
    Density density() const { return m_density; }
    void setDensity(Density density);

    Qt::LayoutDirection direction() const { return m_direction; }
    void setDirection(Qt::LayoutDirection direction);
    bool isRightToLeft() const;

    QString languageTag() const { return m_languageTag; }
    void setLanguageTag(const QString &languageTag);
    ScriptCategory scriptCategory() const;

    // --- component token overrides ---------------------------------------
    /// Application-wide `md.comp.*` overrides. An individual component may
    /// hold its own MdComponentTokens instance to override just itself.
    MdComponentTokens &componentTokens() { return m_componentTokens; }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

signals:
    void themeAboutToChange();
    void themeChanged();
    void themeModeChanged(md::ThemeMode mode);
    void densityChanged(md::Density density);
    void directionChanged(Qt::LayoutDirection direction);
    void languageChanged(const QString &languageTag);

private:
    explicit MdTheme(QObject *parent = nullptr);

    void rebuildScheme();
    /// Emits aboutToChange, mutates, emits changed.
    template <typename Fn>
    void applyChange(Fn &&mutate);

    ThemeMode m_mode = ThemeMode::Light;
    bool m_dynamicColor = false;
    QColor m_seedColor = QColor(QStringLiteral("#6750a4"));
    SchemeVariant m_variant = SchemeVariant::TonalSpot;
    ContrastLevel m_contrast = ContrastLevel::Standard;
    Density m_density = Density::Default;
    Qt::LayoutDirection m_direction = Qt::LeftToRight;
    QString m_languageTag = QStringLiteral("en");

    MdColorScheme m_scheme;
    MdComponentTokens m_componentTokens;
    bool m_schemeDirty = true;
};

inline bool MdTheme::isRightToLeft() const
{
    return m_direction == Qt::RightToLeft;
}

} // namespace md

#endif // MD_THEME_H
