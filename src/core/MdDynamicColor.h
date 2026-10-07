#ifndef MD_DYNAMIC_COLOR_H
#define MD_DYNAMIC_COLOR_H

// Dynamic colour: turn one seed colour into a full MD3 colour scheme.
//
// Ported from material-color-utilities (Apache-2.0): the scheme variants come
// from cpp/scheme/scheme_*.cc (their tone/chroma rotation tables are
// transcribed verbatim), and harmonize() from cpp/blend/blend.cc.

// SPDX-License-Identifier: Apache-2.0
// Ported from material-color-utilities (Copyright 2023 Google LLC).

#include "MdColorMath.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtGui/QColor>

namespace md {

/// A DynamicScheme: six tonal palettes plus the appearance choices that decide
/// which tone each colour role resolves to.
class QT_MD3_EXPORT MdDynamicScheme
{
public:
    MdDynamicScheme() = default;

    /// Builds the scheme for a seed colour. `variant` selects the palette
    /// derivation rules; `contrast` selects the contrast level.
    static MdDynamicScheme create(Argb seed,
                                 ThemeMode mode,
                                 SchemeVariant variant,
                                 ContrastLevel contrast = ContrastLevel::Standard);

    Argb sourceColor() const { return m_sourceColor; }
    MdHct sourceHct() const { return m_sourceHct; }
    ThemeMode mode() const { return m_mode; }
    bool isDark() const { return m_mode == ThemeMode::Dark; }
    SchemeVariant variant() const { return m_variant; }
    ContrastLevel contrastLevel() const { return m_contrast; }

    const MdTonalPalette &primaryPalette() const { return m_primary; }
    const MdTonalPalette &secondaryPalette() const { return m_secondary; }
    const MdTonalPalette &tertiaryPalette() const { return m_tertiary; }
    const MdTonalPalette &neutralPalette() const { return m_neutral; }
    const MdTonalPalette &neutralVariantPalette() const { return m_neutralVariant; }
    const MdTonalPalette &errorPalette() const { return m_error; }

    /// Resolved colour for an MD3 colour role.
    Argb color(ColorRole role) const;

    /// The tone this scheme maps `role` to (0..100). Exposed so the token
    /// audit test can check the mapping without reverse-engineering colours.
    static double toneFor(ColorRole role, bool isDark);

private:
    Argb m_sourceColor = 0xFF6750A4u;
    MdHct m_sourceHct;
    ThemeMode m_mode = ThemeMode::Light;
    SchemeVariant m_variant = SchemeVariant::TonalSpot;
    ContrastLevel m_contrast = ContrastLevel::Standard;

    MdTonalPalette m_primary;
    MdTonalPalette m_secondary;
    MdTonalPalette m_tertiary;
    MdTonalPalette m_neutral;
    MdTonalPalette m_neutralVariant;
    MdTonalPalette m_error;
};

/// Static helpers mirroring the material-color-utilities top-level API.
class QT_MD3_EXPORT MdDynamicColor
{
public:
    /// blend.harmonize — nudges a design colour's hue toward a source colour
    /// by at most 15 degrees, keeping it recognisable while feeling related.
    static Argb harmonize(Argb designColor, Argb sourceColor);

    /// blend.hctHue — replaces a colour's hue with the CAM16-UCS hue of the
    /// blend between `from` and `to`.
    static Argb hctHue(Argb from, Argb to, double amount);

    /// Convenience: resolved role colour for a seed/mode/variant combination.
    static QColor roleColor(const QColor &seed,
                            ThemeMode mode,
                            ColorRole role,
                            SchemeVariant variant = SchemeVariant::TonalSpot,
                            ContrastLevel contrast = ContrastLevel::Standard);
};

} // namespace md

#endif // MD_DYNAMIC_COLOR_H
