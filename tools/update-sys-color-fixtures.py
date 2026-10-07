#!/usr/bin/env python3
"""Regenerate resources/tokens/md-sys-color-*.txt from material-web.

The six `md.sys.color` token sets published by material-web are Google's own
output for the six scheme variants qt-md3 supports:

    light  x standard / medium-contrast / high-contrast
    dark   x standard / medium-contrast / high-contrast

They are the authoritative reference for the whole contrast-level machinery in
`src/core/MdColorSpec.cpp`, so they are vendored verbatim (as resolved
role -> hex pairs) and compared against at test time.

Usage:

    python3 tools/update-sys-color-fixtures.py <path-to-material-web-tokens>

where <path-to-material-web-tokens> is a checkout of
`material-components/material-web`, specifically the `tokens/versions/latest/sass`
directory. Only the seven files listed below are read.

The output is deliberately a flat `role-name #rrggbb` list rather than the
SCSS: the fixture is then readable without a Sass toolchain and the test does
not have to re-implement `md-ref-palette.$primary30` indirection.

SPDX-License-Identifier: Apache-2.0
Input data copyright Google LLC, from material-web.
"""

import os
import re
import sys

# The six variant files, in the order the fixtures are written.
VARIANTS = [
    ("light-standard", "_md-sys-color.scss"),
    ("light-medium", "_md-sys-color__medium-contrast.scss"),
    ("light-high", "_md-sys-color__high-contrast.scss"),
    ("dark-standard", "_md-sys-color__dark.scss"),
    ("dark-medium", "_md-sys-color__dark__medium-contrast.scss"),
    ("dark-high", "_md-sys-color__dark__high-contrast.scss"),
]

PALETTE_FILE = "_md-ref-palette.scss"

# `$name: <value>;` at the start of a line.
DECLARATION = re.compile(r"^\$([a-z0-9-]+):\s*([^;]+);")
# `md-ref-palette.$primary30`
PALETTE_REFERENCE = re.compile(r"^md-ref-palette\.\$([a-z0-9-]+)$")
# `$primary` - a role aliasing another role in the same file, which is how
# `$surface-tint: $primary` is written.
ROLE_ALIAS = re.compile(r"^\$([a-z0-9-]+)$")

# Published tokens that are not MD3 system colour roles.
#
# `surface-tint-color` is deprecated in the source itself, with the comment
# "Please replace with md.sys.color.surface-tint. Token deprecated to align
# naming convention with other color roles by remove `color` from the role
# name". It carries exactly the same value as `surface-tint`, so keeping it
# would give the scheme two names for one role without adding a colour.
EXCLUDED_ROLES = {
    "surface-tint-color",
}


def read_declarations(path):
    """Returns {name: value} for every `$name: value;` line in a token file."""
    declarations = {}
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            match = DECLARATION.match(line)
            if match:
                declarations[match.group(1)] = match.group(2).strip()
    return declarations


def normalise_hex(value):
    """Expands `#fff` to `#ffffff` so the fixtures have one shape."""
    if not value.startswith("#"):
        return None
    digits = value[1:]
    if len(digits) == 3:
        digits = "".join(character * 2 for character in digits)
    if len(digits) != 6:
        return None
    return "#" + digits.lower()


ROLE_ENUM = {
    "background": "Background",
    "error": "Error",
    "error-container": "ErrorContainer",
    "inverse-on-surface": "InverseOnSurface",
    "inverse-primary": "InversePrimary",
    "inverse-surface": "InverseSurface",
    "on-background": "OnBackground",
    "on-error": "OnError",
    "on-error-container": "OnErrorContainer",
    "on-primary": "OnPrimary",
    "on-primary-container": "OnPrimaryContainer",
    "on-primary-fixed": "OnPrimaryFixed",
    "on-primary-fixed-variant": "OnPrimaryFixedVariant",
    "on-secondary": "OnSecondary",
    "on-secondary-container": "OnSecondaryContainer",
    "on-secondary-fixed": "OnSecondaryFixed",
    "on-secondary-fixed-variant": "OnSecondaryFixedVariant",
    "on-surface": "OnSurface",
    "on-surface-variant": "OnSurfaceVariant",
    "on-tertiary": "OnTertiary",
    "on-tertiary-container": "OnTertiaryContainer",
    "on-tertiary-fixed": "OnTertiaryFixed",
    "on-tertiary-fixed-variant": "OnTertiaryFixedVariant",
    "outline": "Outline",
    "outline-variant": "OutlineVariant",
    "primary": "Primary",
    "primary-container": "PrimaryContainer",
    "primary-fixed": "PrimaryFixed",
    "primary-fixed-dim": "PrimaryFixedDim",
    "scrim": "Scrim",
    "secondary": "Secondary",
    "secondary-container": "SecondaryContainer",
    "secondary-fixed": "SecondaryFixed",
    "secondary-fixed-dim": "SecondaryFixedDim",
    "shadow": "Shadow",
    "surface": "Surface",
    "surface-bright": "SurfaceBright",
    "surface-container": "SurfaceContainer",
    "surface-container-high": "SurfaceContainerHigh",
    "surface-container-highest": "SurfaceContainerHighest",
    "surface-container-low": "SurfaceContainerLow",
    "surface-container-lowest": "SurfaceContainerLowest",
    "surface-dim": "SurfaceDim",
    "surface-tint": "SurfaceTint",
    "surface-variant": "SurfaceVariant",
    "tertiary": "Tertiary",
    "tertiary-container": "TertiaryContainer",
    "tertiary-fixed": "TertiaryFixed",
    "tertiary-fixed-dim": "TertiaryFixedDim",
}

CPP_NAME = {
    "light-standard": ("kBaselineLight", "Baseline light — standard contrast"),
    "light-medium": ("kBaselineLightMedium", "Baseline light — medium contrast"),
    "light-high": ("kBaselineLightHigh", "Baseline light — high contrast"),
    "dark-standard": ("kBaselineDark", "Baseline dark — standard contrast"),
    "dark-medium": ("kBaselineDarkMedium", "Baseline dark — medium contrast"),
    "dark-high": ("kBaselineDarkHigh", "Baseline dark — high contrast"),
}


def emit_cpp(rows_by_variant):
    """Prints the MdColorScheme.cpp tables, so they never have to be typed."""
    for variant, _ in VARIANTS:
        name, comment = CPP_NAME[variant]
        rows = rows_by_variant[variant]
        print("// %s — material-web tokens/versions/latest/sass" % comment)
        print("// (%s)." % variant)
        print("const RoleMapping %s[] = {" % name)
        for role, _hex, reference in rows:
            enum_name = ROLE_ENUM.get(role)
            if enum_name is None:
                raise SystemExit("no ColorRole mapping for %r" % role)
            print('    {R::%-26s "%s"},' % (enum_name + ",", reference))
        print("};")
        print()


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2

    args = [argument for argument in sys.argv[1:] if not argument.startswith("--")]
    want_cpp = "--emit-cpp" in sys.argv

    tokens_dir = args[0]
    output_dir = args[1] if len(args) > 1 else os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "resources", "tokens")

    palette_path = os.path.join(tokens_dir, PALETTE_FILE)
    if not os.path.isfile(palette_path):
        print("missing %s" % palette_path, file=sys.stderr)
        return 1

    palette = read_declarations(palette_path)
    resolved_palette = {}
    for name, value in palette.items():
        hex_value = normalise_hex(value)
        if hex_value:
            resolved_palette[name] = hex_value

    os.makedirs(output_dir, exist_ok=True)
    used_references = []
    rows_by_variant = {}
    for variant, filename in VARIANTS:
        path = os.path.join(tokens_dir, filename)
        if not os.path.isfile(path):
            print("missing %s" % path, file=sys.stderr)
            return 1

        declarations = read_declarations(path)
        rows = []
        for role, value in sorted(declarations.items()):
            if role in EXCLUDED_ROLES:
                continue
            # Several roles are aliases for another role rather than a direct
            # palette tone: `$surface-tint: $primary`. Follow the alias chain
            # before looking for a palette reference, or the role silently
            # disappears from the fixture and MdColorScheme::baseline() ends up
            # one role short.
            for _ in range(8):
                alias = ROLE_ALIAS.match(value)
                if not alias:
                    break
                target = declarations.get(alias.group(1))
                if target is None:
                    print("unresolved role alias %s for %s" % (value, role), file=sys.stderr)
                    return 1
                value = target

            reference = PALETTE_REFERENCE.match(value)
            if not reference:
                # Anything that is not a reference into the tonal palette is
                # skipped rather than guessed at.
                print("skipping %s: %s is not a palette reference" % (role, value),
                      file=sys.stderr)
                continue
            hex_value = resolved_palette.get(reference.group(1))
            if hex_value is None:
                print("unresolved palette reference %s for %s" % (value, role),
                      file=sys.stderr)
                return 1
            rows.append((role, hex_value, reference.group(1)))
            used_references.append((role, reference.group(1)))
        rows_by_variant[variant] = rows

        output_path = os.path.join(output_dir, "md-sys-color-%s.txt" % variant)
        with open(output_path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write("# md.sys.color - %s\n" % variant)
            handle.write("#\n")
            handle.write("# GENERATED by tools/update-sys-color-fixtures.py from\n")
            handle.write("# material-web tokens/versions/latest/sass. Do not edit by hand.\n")
            handle.write("# Format: <role-token-name> <#rrggbb> <md.ref.palette token>\n")
            for role, hex_value, reference in rows:
                handle.write("%-28s %s %s\n" % (role, hex_value, reference))
        print("%-16s %3d roles -> %s" % (variant, len(rows), output_path))

    if want_cpp:
        print()
        emit_cpp(rows_by_variant)

    print()
    print("md.ref.palette tokens referenced by the six sets (%d distinct):"
          % len(set(reference for _, reference in used_references)))
    families = {}
    for _, reference in used_references:
        index = len(reference) - len(reference.rstrip("0123456789"))
        family = reference[:len(reference) - index] if index else reference
        families.setdefault(family, set()).add(reference[len(reference) - index:])
    for family in sorted(families):
        tones = families[family]
        print("  %-18s %s" % (family, " ".join(sorted(tones, key=int))))

    return 0


if __name__ == "__main__":
    sys.exit(main())
