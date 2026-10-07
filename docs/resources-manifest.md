# Resources Manifest

Inventory of everything under [`resources/`](../resources). Kept in step with
`resources/qt-md3.qrc` and with [`NOTICE.md`](../NOTICE.md).

## Layout

| Path | Contents | Status |
| --- | --- | --- |
| `resources/qt-md3.qrc` | Qt resource bundle | present |
| `resources/icons/material-symbols-codepoints.txt` | Complete Material Symbols name → codepoint table, 4299 entries | present |
| `resources/icons/classic/24px/*.svg` | Classic Material Icons SVG baseline, 49 icons | present |
| `resources/fonts/` | Material Symbols variable fonts (not committed, see below) | opt-in |
| `resources/images/` | README gallery + showcase screenshots | empty |

## Bundled, committed

### `icons/material-symbols-codepoints.txt`

Taken verbatim from
`google/material-design-icons/variablefont/MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].codepoints`
(Apache-2.0). The format is one `name codepoint` pair per line, lowercase hex, no
prefix. All three families — Outlined, Rounded and Sharp — publish byte-identical
tables, so one copy serves every family.

This file is what makes `MdIcon::glyph("home")` exact rather than a guess. It is
79 KB of text; regenerating it is a single download:

```sh
gh api "repos/google/material-design-icons/contents/variablefont/MaterialSymbolsOutlined%5BFILL,GRAD,opsz,wght%5D.codepoints" \
  --jq '.content' | base64 -d > resources/icons/material-symbols-codepoints.txt
```

### `icons/classic/24px/*.svg`

A curated subset of the pre-Symbols Material Icons set, taken from
`google/material-design-icons/src/<category>/<name>/materialicons/24px.svg`
(Apache-2.0). Each file is a plain 24×24 `viewBox` SVG with a single `path`, so
`MdIcon` can rasterise and tint it with `CompositionMode_SourceIn` without any
colour substitution in the file itself.

The subset is deliberately small — it exists so the library is usable with no
font installation at all, and so the gallery can show a working icon grid. It is
**not** a complete icon set and must not be treated as one: it has a single
weight, no optical size, no grade, and only the icons listed in
`tools/update-icons.sh`.

## Not bundled: the Material Symbols variable fonts

The fonts are not committed. Sizes as published by `google/material-design-icons`:

| File | Size |
| --- | --- |
| `MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].woff2` | 4.0 MB |
| `MaterialSymbolsRounded[FILL,GRAD,opsz,wght].woff2` | 5.4 MB |
| `MaterialSymbolsSharp[FILL,GRAD,opsz,wght].woff2` | 3.5 MB |
| `MaterialSymbolsOutlined[FILL,GRAD,opsz,wght].ttf` | 10.7 MB |
| `MaterialSymbolsRounded[FILL,GRAD,opsz,wght].ttf` | 15.2 MB |
| `MaterialSymbolsSharp[FILL,GRAD,opsz,wght].ttf` | 8.9 MB |

Three multi-megabyte binaries in a source repository is a cost every consumer
would pay and most would not need, so the library degrades instead:

* **Font installed** — `MdIcon::resolveSet(Auto)` picks Material Symbols and all
  four axes work through `QFont::setVariableAxis` (Qt 6.7+).
* **Font absent** — `MdIcon` falls back to the bundled classic SVGs, so icons
  still draw. `MdIcon::isFontAvailable()` and `MdIcon::axesSupported()` let an
  application detect and report the degraded state; the gallery's Overview page
  does exactly that.

To enable the font, either:

1. **Install it system-wide.** Any of the six files above works; drop it into the
   OS font directory (on Windows: right-click → *Install for all users*). Only
   the family name matters to the library: `Material Symbols Outlined`,
   `Material Symbols Rounded`, `Material Symbols Sharp`.
2. **Load it from the resource bundle.** Put the file in `resources/fonts/`, add
   it to `qt-md3.qrc` under the `fonts/` prefix, and call
   `MdFont::registerFontFile(":/qt-md3/fonts/<file>")` before the first paint.
   `MdDesign::initialize()` calls `MdFont::registerBundledFonts()` for you, which
   registers everything the bundle contains.

`tools/update-icons.sh` downloads and commits option 1's font files into
`resources/fonts/` if you would rather vendor them; the script prints the size
first and asks for confirmation.

## Fonts for text (Roboto / Roboto Flex)

Not committed either, for the same reason. `MdTypeScale` resolves family names
through `MdTypeScale::setFamily()` and falls back to the platform's default
sans-serif, and `MdTypeScale::setCjkFallbackFamilies()` handles the CJK pairing.
A project that wants the exact MD3 look bundles the fonts itself and registers
them with `MdFont`.

## Attribution

Attribution for every asset above lives in [`NOTICE.md`](../NOTICE.md). Record a
newly added asset here and in `NOTICE.md` in the same change.
