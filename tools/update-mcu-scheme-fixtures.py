#!/usr/bin/env python3
"""Regenerate resources/tokens/mcu-scheme-expectations.txt.

material-color-utilities ships its own expected scheme values in
`swift/Tests/MaterialColorUtilitiesTests/SchemeTests/*.swift` and
`.../DynamicColorTests/*.swift`. Those assertions are the authoritative
reference for `src/core/MdColorSpec.cpp`: they are Google's numbers for every
variant, in both modes, at the minimum, standard and maximum contrast levels.

This script flattens them into one table so the C++ side can check the solver
against upstream without a Swift toolchain.

Usage:

    python3 tools/update-mcu-scheme-fixtures.py <path-to-material-color-utilities>

Output format, one assertion per line:

    <variant> <seed-argb> <light|dark> <contrast-level> <camelCaseRole> <#rrggbb>

Only plain `XCTAssertEqual(scheme.<role>, 0x<hex>)` assertions are extracted.
Assertions on floats, on `.getHct(...).tone`, on palettes, and on object
identity are skipped: they are not colour values and would need a different
comparison. That is deliberate rather than lossy - everything that is skipped is
either not a colour or is pinned by another test in qt-md3.

Extraction is scoped to one test function at a time, and a function contributes
nothing unless every part of its scheme construction is a literal. Tracking
`sourceColorHct` / `isDark` / `contrastLevel` as a running state across the file
looks simpler and is wrong in three separate ways, all of which produced
fixtures that looked plausible:

  * Swift splits a construction across lines
    (`let scheme = SchemeContent(\n  sourceColorHct: ...,\n  isDark: false,\n
    contrastLevel: -1\n)`), so a line-oriented parser misses the attributes that
    are not on the seed's line and attributes them the previous test's values;
  * the provider tests build a scheme from local variables
    (`let sourceColorHct = Hct(0xfffa_2bec)` ... `isDark: isDark`), which are not
    literals at that point;
  * state leaked across functions, so a stale `variant` could label one
    variant's numbers with another's name.

Anything ambiguous is dropped rather than guessed, and the same
(variant, seed, mode, contrast, role) key mapping to two different colours is a
hard error - that is a parser bug, and silently keeping both would ship
self-contradictory reference data.

SPDX-License-Identifier: Apache-2.0
Input data copyright Google LLC, from material-color-utilities.
"""

import glob
import os
import re
import sys

# `let scheme = SchemeTonalSpot(` -> variant "tonal-spot".
# The optional `MaterialDynamicColors.x ??` prefix covers DynamicColorTests.
CONSTRUCT_PATTERN = re.compile(
    r"(?:let|var)\s+([A-Za-z_]\w*)\s*=\s*"
    r"(?:MaterialDynamicColors\.[A-Za-z]+\s*\?\?\s*)?"
    r"Scheme([A-Za-z]+)\s*\(")

SEED_PATTERN = re.compile(r"sourceColorHct\s*:\s*Hct(?:\.fromInt)?\(\s*0x([0-9a-fA-F_]+)\s*\)")
DARK_PATTERN = re.compile(r"isDark\s*:\s*(true|false)")
CONTRAST_PATTERN = re.compile(r"contrastLevel\s*:\s*(-?\d+(?:\.\d+)?)")
FUNC_PATTERN = re.compile(r"\bfunc\s+(test\w+)\s*\(")

VARIANTS = {
    "TonalSpot": "tonal-spot",
    "Vibrant": "vibrant",
    "Expressive": "expressive",
    "Content": "content",
    "Fidelity": "fidelity",
    "Monochrome": "monochrome",
    "Neutral": "neutral",
    "Rainbow": "rainbow",
    "FruitSalad": "fruit-salad",
}

# The `md.sys.color` roles, in the Swift tests' camelCase spelling. The
# `*PaletteKeyColor` properties are deliberately absent: they are palette
# diagnostics rather than system colour roles, so qt-md3's ColorRole has no
# equivalent to compare against.
ROLES = {
    "primary", "onPrimary", "primaryContainer", "onPrimaryContainer",
    "secondary", "onSecondary", "secondaryContainer", "onSecondaryContainer",
    "tertiary", "onTertiary", "tertiaryContainer", "onTertiaryContainer",
    "error", "onError", "errorContainer", "onErrorContainer",
    "background", "onBackground",
    "surface", "onSurface", "surfaceVariant", "onSurfaceVariant",
    "surfaceDim", "surfaceBright", "surfaceContainerLowest",
    "surfaceContainerLow", "surfaceContainer", "surfaceContainerHigh",
    "surfaceContainerHighest", "inverseSurface", "inverseOnSurface",
    "inversePrimary", "outline", "outlineVariant", "scrim", "shadow",
    "surfaceTint",
    "primaryFixed", "primaryFixedDim", "onPrimaryFixed", "onPrimaryFixedVariant",
    "secondaryFixed", "secondaryFixedDim", "onSecondaryFixed",
    "onSecondaryFixedVariant",
    "tertiaryFixed", "tertiaryFixedDim", "onTertiaryFixed",
    "onTertiaryFixedVariant",
}

# `MaterialDynamicColors` in DynamicColorTests is built on the content spec;
# scheme constructors there are still `DynamicScheme` with an explicit variant,
# so that file contributes nothing that is not already covered. It is still
# parsed so a failure to find anything is visible rather than silent.
SCHEME_DIRS = ("SchemeTests", "DynamicColorTests")


def strip_underscores(value):
    return value.replace("_", "")


def balanced_args(text, open_index):
    """Return the argument text of the call whose '(' is at open_index."""
    depth = 0
    for index in range(open_index, len(text)):
        character = text[index]
        if character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
            if depth == 0:
                return text[open_index + 1:index]
    return None


def split_functions(text):
    """Yield (name, body) per `func test...`, body ending at the next func."""
    starts = [(m.start(), m.group(1)) for m in FUNC_PATTERN.finditer(text)]
    for position, (start, name) in enumerate(starts):
        end = starts[position + 1][0] if position + 1 < len(starts) else len(text)
        yield name, text[start:end]


def scope_rows(name, body, skipped):
    """Extract rows from one test function, or record why it yields nothing."""
    constructors = list(CONSTRUCT_PATTERN.finditer(body))
    if not constructors:
        return []

    rows = []
    for position, match in enumerate(constructors):
        variable = match.group(1)
        variant = VARIANTS.get(match.group(2))
        if variant is None:
            # e.g. SchemeContentProvider - a provider, not a scheme.
            continue
        args = balanced_args(body, match.end() - 1)
        if args is None:
            skipped.append("%s: unbalanced call to Scheme%s" % (name, match.group(2)))
            continue

        seed_match = SEED_PATTERN.search(args)
        dark_match = DARK_PATTERN.search(args)
        contrast_match = CONTRAST_PATTERN.search(args)
        if not (seed_match and dark_match and contrast_match):
            # Attributes are locals or otherwise not literal. Dropping is the
            # only safe option: guessing a seed here is how cross-contamination
            # started.
            skipped.append("%s: Scheme%s built from non-literal attributes"
                           % (name, match.group(2)))
            continue

        seed = "0x" + strip_underscores(seed_match.group(1)).lower()
        is_dark = dark_match.group(1) == "true"
        contrast = float(contrast_match.group(1))

        # This constructor owns the asserts up to the next one in the function.
        limit = (constructors[position + 1].start()
                 if position + 1 < len(constructors) else len(body))
        assert_pattern = re.compile(
            r"XCTAssertEqual\(\s*" + re.escape(variable)
            + r"\.([A-Za-z]+)\s*,\s*0x([0-9a-fA-F_]+)\s*\)")
        for role, raw in assert_pattern.findall(body[match.end():limit]):
            if role not in ROLES:
                continue
            value = strip_underscores(raw)
            if len(value) != 8:
                continue
            rows.append((variant, seed, "dark" if is_dark else "light",
                         contrast, role, "#" + value[2:].lower()))
    return rows


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    root = sys.argv[1]
    test_root = os.path.join(root, "swift", "Tests", "MaterialColorUtilitiesTests")
    if not os.path.isdir(test_root):
        print("missing %s" % test_root, file=sys.stderr)
        return 1

    files = []
    for directory in SCHEME_DIRS:
        files.extend(sorted(glob.glob(os.path.join(test_root, directory, "*.swift"))))
    if not files:
        print("no Swift test files under %s" % test_root, file=sys.stderr)
        return 1

    rows = []
    skipped = []
    for path in files:
        with open(path, encoding="utf-8") as handle:
            text = handle.read()
        for name, body in split_functions(text):
            rows.extend(scope_rows(name, body, skipped))

    if not rows:
        print("no assertions extracted; the upstream test layout probably changed",
              file=sys.stderr)
        return 1

    # A duplicate key with two different colours means the parser mis-attributed
    # a row. Fail loudly instead of emitting contradictory reference data.
    conflicts = {}
    for row in rows:
        key = row[:5]
        conflicts.setdefault(key, set()).add(row[5])
    contradictory = {key: values for key, values in conflicts.items() if len(values) > 1}
    if contradictory:
        print("conflicting expectations for the same scheme and role:", file=sys.stderr)
        for key, values in sorted(contradictory.items()):
            print("  %s -> %s" % (" ".join(str(part) for part in key),
                                  ", ".join(sorted(values))), file=sys.stderr)
        return 1

    unique = sorted(key + (next(iter(conflicts[key])),) for key in conflicts)

    output_dir = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "resources", "tokens")
    output_path = os.path.join(output_dir, "mcu-scheme-expectations.txt")
    with open(output_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("# Scheme colour expectations, taken verbatim from\n")
        handle.write("# material-color-utilities swift/Tests/.../"
                     "{SchemeTests,DynamicColorTests}.\n")
        handle.write("# GENERATED by tools/update-mcu-scheme-fixtures.py. "
                     "Do not edit by hand.\n")
        handle.write("#\n")
        handle.write("# <variant> <seed-argb> <light|dark> <contrast-level> <role> <#rrggbb>\n")
        for variant, seed, mode, contrast, role, hex_value in unique:
            handle.write("%-11s %-10s %-5s %-4s %-26s %s\n"
                         % (variant, seed, mode, contrast, role, hex_value))

    schemes = sorted({row[:4] for row in unique})
    print("%d assertions, %d scheme configurations -> %s"
          % (len(unique), len(schemes), output_path))
    for variant, seed, mode, contrast in schemes:
        count = len([1 for row in unique if row[:4] == (variant, seed, mode, contrast)])
        print("  %-11s %s %-5s contrast %-4s %2d roles"
              % (variant, seed, mode, contrast, count))

    if skipped:
        print("\n%d construction(s) skipped for lack of literal attributes:"
              % len(skipped))
        for reason in skipped:
            print("  %s" % reason)
    return 0


if __name__ == "__main__":
    sys.exit(main())
