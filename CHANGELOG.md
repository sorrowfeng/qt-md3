# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Repository scaffolding: CMake build system (Qt 6 / Qt 5 auto-detection,
  installable package, Windows deployment), `src/core`, `src/styles`,
  `src/widgets`, `examples`, `tests`, `resources` layout.
- `QT_MD3_EXPORT` export macro and generated `core/QtMd3Version.h`.
- Policy gates: `TestMd3NoQss` (no QSS anywhere) and `TestMd3CoveragePolicy`
  (blocks premature Stage 2 components).
- `docs/md3-coverage.md` coverage matrix with all 36 MD3 component families
  tracked as not-started.

Stage 1 (MD3 official 36 families) and Stage 2 (Qt extensions) are not yet
implemented. See `docs/project-status.md` for the current state.

[Unreleased]: https://github.com/sorrowfeng/qt-md3/commits/dev
