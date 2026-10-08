#ifndef GALLERY_I18N_H
#define GALLERY_I18N_H

// Gallery-side bilingual copy helper.
//
// The gallery ships exactly two languages: Simplified Chinese (the default)
// and English. There is no Qt translation machinery on purpose — page copy is
// written as `L("中文", "English")` and evaluated at build/paint time, so a
// language toggle re-renders through the normal theme-change path and the
// measurement/painting contract (measure with a null painter) keeps holding.

#include "core/MdTheme.h"

#include <QtCore/QString>

namespace gallery {

/// Returns the Simplified Chinese copy when the theme's language tag is
/// `zh-Hans`, otherwise the English copy.
inline QString L(const char *zh, const char *en)
{
    return md::MdTheme::instance().languageTag() == QStringLiteral("zh-Hans")
               ? QString::fromUtf8(zh)
               : QString::fromUtf8(en);
}

} // namespace gallery

#endif // GALLERY_I18N_H
