# Notices

`qt-md3` re-implements Google's Material Design 3 design system for Qt Widgets.
The repository's own source code is MIT-licensed (see [LICENSE](LICENSE)). It also
ports, bundles, or derives assets from upstream projects that are licensed
**Apache-2.0**. The notices below exist for attribution and license clarity, and
they do **not** change the license of this repository's original source code.

> **Maintainer note:** because several of the items below are Apache-2.0, any file
> that is a direct port or derivative of them must retain its upstream Apache-2.0
> notice and header. When such a file is added, list it here. Do not remove the
> upstream attribution headers.

## Material Design 3 design tokens (`material-web`)

- Files (planned): `src/core/MdTokens.*`, `resources/tokens/*.json`
- Source: <https://github.com/material-components/material-web> (`tokens/`)
- License: Apache License 2.0
- Copyright: Google LLC and the Material Components for the Web authors

The `md.ref.*` / `md.sys.*` / `md.comp.*` token values are taken from the
`material-web` token sources, which are the authoritative numeric reference.
Those numeric values are ported verbatim.

## Material Color Utilities (`material-color-utilities`)

- Files (planned): `src/core/MdDynamicColor.*`, `src/core/MdHct.*`,
  `src/core/MdCam16.*`, `src/core/MdTonalPalette.*`, `src/core/MdCorePalette.*`,
  `src/core/MdScheme.*`
- Source: <https://github.com/material-foundation/material-color-utilities>
  (`cpp/`)
- License: Apache License 2.0
- Copyright: Google LLC and the Material Color Utilities authors

The dynamic-color / HCT / CAM16 algorithms are ported from the official C++
implementation. Those ported files are derivatives of Apache-2.0 code and keep
the upstream license.

## Material Symbols (variable icon font)

- Files (planned): `resources/fonts/MaterialSymbolsOutlined*.ttf`,
  `resources/fonts/MaterialSymbolsRounded*.ttf`, `resources/fonts/MaterialSymbolsSharp*.ttf`
- Source: <https://github.com/google/material-symbols>
- License: Apache License 2.0
- Copyright: Google LLC

## Material Icons (classic SVG baseline)

- Files (planned): `resources/icons/material/*.svg`
- Source: <https://github.com/google/material-design-icons>
- License: Apache License 2.0
- Copyright: Google LLC

## Roboto / Roboto Flex

- Files: `resources/fonts/Roboto-Regular.ttf`, `resources/fonts/Roboto-Medium.ttf`,
  `resources/fonts/Roboto-Bold.ttf` (committed, 2026-10), plus the licence copy
  at `resources/fonts/LICENSE-ROBOTO.txt`
- Source: <https://github.com/google/fonts> (`apache/roboto/static`, commit
  `ff11ed9`); upstream project <https://github.com/googlefonts/roboto>
- License: Apache License 2.0
- Copyright: The Roboto Project Authors

## Trademarks

"Material Design", "Material Symbols", "Roboto", and any related names, logos,
and marks are trademarks of Google LLC. "Qt" and the Qt logo are trademarks of
The Qt Company Ltd. They are referenced here only to describe design-system
compatibility. This project is an independent, community implementation and is
**not** affiliated with, endorsed by, or sponsored by Google LLC or The Qt
Company Ltd.
