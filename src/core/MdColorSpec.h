#ifndef MD_COLOR_SPEC_H
#define MD_COLOR_SPEC_H

// ColorSpec2021 — the solver that turns a MdDynamicScheme into 49 concrete
// colour roles, including the accessibility contrast levels.
//
// Ported from material-color-utilities (Apache-2.0), java/ tree:
//   dynamiccolor/ColorSpec2021.java   the role table + getTone()/getHct()
//   dynamiccolor/DynamicColor.java    foregroundTone(), the tone predicates
//   dynamiccolor/ContrastCurve.java   piecewise tone value over a contrast level
//   dynamiccolor/ToneDeltaPair.java   the tone-distance constraint between roles
//   dynamiccolor/TonePolarity.java    the direction of that constraint
//   contrast/Contrast.java            ratioOfYs(), lighter(), darker()
//
// The palette construction lives in MdDynamicColor; this file owns the *tone*
// each role resolves to. Keeping the two apart is what makes a contrast level
// possible at all: `md.sys.color` publishes one tone per role, but that table is
// only the contrast-level-Zero slice of a continuous function. Applying a level
// means re-solving every role against its background, and the roles constrain
// each other (a container and its `on-` colour must keep a tonal distance), so
// the solve is recursive rather than a table lookup.

// SPDX-License-Identifier: Apache-2.0
// Ported from material-color-utilities (Copyright 2023 Google LLC).

#include "MdColorMath.h"
#include "MdDynamicColor.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

// ---------------------------------------------------------------------------
// DislikeAnalyzer (java/utils/DislikeAnalyzer.java)
// ---------------------------------------------------------------------------

/// True for the muddy yellow-greens users reliably dislike.
QT_MD3_EXPORT bool isDislikedHct(const MdHct &hct);

/// Pushes a disliked colour up to tone 70, where the same hue reads better.
/// Anything else is returned unchanged.
QT_MD3_EXPORT MdHct fixIfDisliked(const MdHct &hct);

// ---------------------------------------------------------------------------
// Contrast curves and tone constraints
// ---------------------------------------------------------------------------

/// A value that changes with the contrast level. The four corners correspond to
/// contrast levels -1.0, 0.0, 0.5 and 1.0; anything between is interpolated
/// linearly. For contrast ratios the value is 1.0 .. 21.0.
struct QT_MD3_EXPORT MdContrastCurve
{
    double low = 0.0;    ///< value at contrast level -1.0
    double normal = 0.0; ///< value at contrast level  0.0
    double medium = 0.0; ///< value at contrast level  0.5
    double high = 0.0;   ///< value at contrast level  1.0

    constexpr MdContrastCurve() = default;
    constexpr MdContrastCurve(double low, double normal, double medium, double high)
        : low(low), normal(normal), medium(medium), high(high)
    {
    }

    double get(double contrastLevel) const;
};

/// How to read the difference between the two roles of a ToneDeltaPair.
enum class MdTonePolarity {
    Darker,         ///< roleA is at least `delta` darker than roleB
    Lighter,        ///< roleA is at least `delta` lighter than roleB
    RelativeDarker, ///< …darker in light mode, lighter in dark mode
    RelativeLighter, ///< …lighter in light mode, darker in dark mode
    /// Superseded by MdDeltaConstraint; kept because ColorSpec2021 still
    /// constructs pairs with it.
    Nearer,
    Farther,
    Count,
};

/// How to satisfy a tone-distance constraint.
enum class MdDeltaConstraint {
    Exact,
    Nearer,
    Farther,
    Count,
};

/// A required tone distance between two roles. Used where two colours have no
/// background/foreground relationship of their own but must stay visually
/// separated (a container and the accent it sits next to).
struct QT_MD3_EXPORT MdToneDeltaPair
{
    ColorRole roleA = ColorRole::Primary;
    ColorRole roleB = ColorRole::Primary;
    double delta = 0.0;
    MdTonePolarity polarity = MdTonePolarity::Nearer;
    bool stayTogether = true;
    MdDeltaConstraint constraint = MdDeltaConstraint::Exact;
};

/// The numeric contrast level a token maps to. MD3 names three (standard,
/// medium, high); material-color-utilities also defines a negative "reduced"
/// level, which qt-md3 does not expose because no M3 token names it.
QT_MD3_EXPORT double contrastLevelValue(ContrastLevel level);

// ---------------------------------------------------------------------------
// Contrast mathematics
// ---------------------------------------------------------------------------

/// Ports java/contrast/Contrast.java plus the tone predicates that live on
/// DynamicColor.java. All tones are HCT / L* values in 0..100, all ratios are
/// WCAG contrast ratios in 1.0..21.0.
class QT_MD3_EXPORT MdContrast
{
public:
    static constexpr double RatioMin = 1.0;
    static constexpr double RatioMax = 21.0;
    static constexpr double Ratio30 = 3.0;
    static constexpr double Ratio45 = 4.5;
    static constexpr double Ratio70 = 7.0;

    static double ratioOfYs(double y1, double y2);
    static double ratioOfTones(double t1, double t2);

    /// Smallest tone >= `tone` reaching `ratio`; -1 when unreachable.
    static double lighter(double tone, double ratio);
    /// Largest tone <= `tone` reaching `ratio`; -1 when unreachable.
    static double darker(double tone, double ratio);

    /// As above but clamped into 0..100 so callers always get a usable tone.
    /// The result may miss the requested ratio — hence "unsafe".
    static double lighterUnsafe(double tone, double ratio);
    static double darkerUnsafe(double tone, double ratio);

    /// Nearest foreground tone to the requested ratio, biased toward the
    /// direction a viewer expects for that background.
    static double foregroundTone(double bgTone, double ratio);

    /// DynamicColor.tonePrefersLightForeground — true below T60.
    static bool tonePrefersLightForeground(double tone);
    /// DynamicColor.toneAllowsLightForeground — true at or below T49.
    static bool toneAllowsLightForeground(double tone);
    /// Pushes a tone down to 49 when it wants light text but cannot carry it.
    static double enableLightForeground(double tone);
};

// ---------------------------------------------------------------------------
// ColorSpec2021
// ---------------------------------------------------------------------------

/// Resolves the 49 MD3 colour roles for a scheme, honouring its contrast level.
///
/// `tone()` re-runs the whole ColorSpec2021 solve for one role, which is what
/// makes medium and high contrast produce genuinely different colours from
/// standard. `standardTone()` exposes the raw pre-contrast tone so the token
/// audit can check the solver against the published `md.sys.color` table.
class QT_MD3_EXPORT MdColorSpec2021
{
public:
    /// The tone `role` resolves to in `scheme`.
    static double tone(const MdDynamicScheme &scheme, ColorRole role);

    /// The resolved colour of `role` in `scheme`.
    static Argb color(const MdDynamicScheme &scheme, ColorRole role);

    /// The role's tone *before* any contrast adjustment — i.e. what the role
    /// table in ColorSpec2021 declares for this variant and mode. At the
    /// standard contrast level the solver normally returns exactly this, which
    /// is the property the token audit relies on.
    static double rawTone(const MdDynamicScheme &scheme, ColorRole role);
};

} // namespace md

#endif // MD_COLOR_SPEC_H
