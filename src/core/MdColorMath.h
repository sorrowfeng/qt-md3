#ifndef MD_COLOR_MATH_H
#define MD_COLOR_MATH_H

// Colour science, ported from material-foundation/material-color-utilities
// (Apache-2.0), cpp/ tree: utils/utils.{h,cc}, cam/{cam,hct,hct_solver,
// viewing_conditions}.{h,cc}, palettes/{tones,core}.{h,cc}.
//
// The conversion maths are transcribed, not re-derived, so that qt-md3 agrees
// with the official implementation bit for bit. The SPDX header below records
// the provenance required by NOTICE.md.

// SPDX-License-Identifier: Apache-2.0
// Ported from material-color-utilities (Copyright 2022 Google LLC).

#include "QtMd3Export.h"

#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace md {

/// 0xAARRGGBB, matching material-color-utilities' Argb.
using Argb = quint32;

/// Three-component vector used by the CAM16 matrix maths.
struct MdVec3
{
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
};

/// CAM16 colour appearance model result.
struct MdCam16
{
    double hue = 0.0;
    double chroma = 0.0;
    double j = 0.0;
    double q = 0.0;
    double m = 0.0;
    double s = 0.0;
    double jstar = 0.0;
    double astar = 0.0;
    double bstar = 0.0;
};

/// Low-level colour maths: sRGB <-> linear <-> XYZ <-> L*, plus CAM16.
class QT_MD3_EXPORT MdColorMath
{
public:
    // --- sRGB / ARGB -----------------------------------------------------
    static Argb argbFromRgb(int red, int green, int blue);
    static Argb argbFromLinrgb(MdVec3 linrgb);
    static int redFromArgb(Argb argb);
    static int greenFromArgb(Argb argb);
    static int blueFromArgb(Argb argb);
    static int alphaFromArgb(Argb argb);
    static bool isOpaque(Argb argb);

    /// "#rrggbb" (or "#aarrggbb"); the leading '#' is optional.
    static Argb argbFromHex(const QString &hex);
    /// "ff6750a4" style, lowercase, no leading '#' (matches HexFromArgb).
    static QString hexFromArgb(Argb argb);
    /// "#6750a4" style, convenient for UI and tests.
    static QString prettyHex(Argb argb);

    // --- luminance / L* --------------------------------------------------
    static double linearized(int rgbComponent);
    static int delinearized(double rgbComponent);
    static double lstarFromArgb(Argb argb);
    static double yFromLstar(double lstar);
    static double lstarFromY(double y);
    static Argb intFromLstar(double lstar);

    // --- angles ----------------------------------------------------------
    static double sanitizeDegreesDouble(double degrees);
    static int sanitizeDegreesInt(int degrees);
    static double diffDegrees(double a, double b);

    // --- CAM16 -----------------------------------------------------------
    static MdCam16 camFromInt(Argb argb);
    static MdCam16 camFromXyz(double x, double y, double z);
    static double camDistance(const MdCam16 &a, const MdCam16 &b);

    /// CAM16-UCS (J*, a*, b*) -> CAM16 (cpp/cam/cam.cc
    /// CamFromUcsAndViewingConditions).
    static MdCam16 camFromUcs(double jstar, double astar, double bstar);
    /// CAM16 (J, C, h) -> CAM16 (CamFromJchAndViewingConditions).
    static MdCam16 camFromJch(double j, double c, double h);
    /// CAM16 -> sRGB (cpp/cam/cam.cc IntFromCamAndViewingConditions).
    static Argb intFromCam(const MdCam16 &cam);

    /// Solves for the sRGB colour with the given HCT coordinates. This is
    /// MCU's SolveToInt / IntFromHcl.
    static Argb intFromHcl(double hueDegrees, double chroma, double lstar);

    // --- accessibility ---------------------------------------------------
    /// WCAG 2.x relative-luminance contrast ratio, 1.0 .. 21.0.
    static double contrastRatio(Argb a, Argb b);

    /// WCAG-compliant on-colour for the given background.
    static Argb onColorFor(Argb background, Argb lightCandidate, Argb darkCandidate);

    /// MCU `DynamicColor.tonePrefersLightForeground`: tones below 49.5 take a
    /// light foreground. This is the rule MD3 itself uses to decide whether a
    /// tonal surface is dark enough for white text.
    static bool tonePrefersLightForeground(double tone);

    /// A legible black-or-white foreground for an arbitrary background.
    ///
    /// This is *tooling*, not a design token: real MD3 colours are always
    /// paired with an explicit `on-*` role. It exists so the gallery and the
    /// token report can label a swatch of any colour without inventing values.
    static Argb readableForegroundFor(Argb background);
};

/// HCT: CAM16 hue + chroma, and L* tone. The perceptual space MD3 tones live in.
class QT_MD3_EXPORT MdHct
{
public:
    MdHct();
    /// Corrects out-of-range inputs; chroma may be reduced when the requested
    /// hue/tone cannot carry it.
    MdHct(double hue, double chroma, double tone);
    explicit MdHct(Argb argb);

    double hue() const { return m_hue; }
    double chroma() const { return m_chroma; }
    double tone() const { return m_tone; }
    Argb toInt() const { return m_argb; }

    void setHue(double hue);
    void setChroma(double chroma);
    void setTone(double tone);

private:
    void setInternalState(Argb argb);

    double m_hue = 0.0;
    double m_chroma = 0.0;
    double m_tone = 0.0;
    Argb m_argb = 0;
};

/// A tonal palette: one hue/chroma, addressable at any tone 0..100.
class QT_MD3_EXPORT MdTonalPalette
{
public:
    MdTonalPalette();
    explicit MdTonalPalette(Argb argb);
    MdTonalPalette(double hue, double chroma);
    /// Takes hue and chroma from the HCT, ignoring its tone (MCU's
    /// TonalPalette(Hct)).
    explicit MdTonalPalette(const MdHct &hct);

    /// Converts a colour from lab.h in material-color-utilities.
    static MdVec3 labFromArgb(Argb argb);

    /// Colour at the given tone. Matches MCU's TonalPalette::get().
    Argb tone(double tone) const;

    double hue() const { return m_hue; }
    double chroma() const { return m_chroma; }
    MdHct keyColor() const { return m_keyColor; }

private:
    double m_hue = 0.0;
    double m_chroma = 0.0;
    MdHct m_keyColor;
};

/// The five foundational palettes plus error, generated from one seed colour.
class QT_MD3_EXPORT MdCorePalette
{
public:
    explicit MdCorePalette(Argb seed);

    const MdTonalPalette &primary() const { return m_primary; }
    const MdTonalPalette &secondary() const { return m_secondary; }
    const MdTonalPalette &tertiary() const { return m_tertiary; }
    const MdTonalPalette &neutral() const { return m_neutral; }
    const MdTonalPalette &neutralVariant() const { return m_neutralVariant; }
    /// MD3 always uses hue 25 / chroma 84 for the error palette.
    const MdTonalPalette &error() const { return m_error; }

private:
    MdTonalPalette m_primary;
    MdTonalPalette m_secondary;
    MdTonalPalette m_tertiary;
    MdTonalPalette m_neutral;
    MdTonalPalette m_neutralVariant;
    MdTonalPalette m_error;
};

} // namespace md

#endif // MD_COLOR_MATH_H
