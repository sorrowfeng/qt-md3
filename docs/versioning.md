# Versioning

`qt-md3` follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Single source of truth

The version number lives in the repository-root [`VERSION`](../VERSION) file. CMake
reads it, validates it against `MAJOR.MINOR.PATCH`, and uses it to generate
`core/QtMd3Version.h`. Nothing else may hard-code a version.

## Branches

| Branch | Purpose |
| --- | --- |
| `dev` | Active development. All normal changes land here. |
| `main` | Release branch. Updated only when cutting a release. |

## Release steps

1. Update `VERSION` to the new `MAJOR.MINOR.PATCH`.
2. Move the `Unreleased` notes in [`CHANGELOG.md`](../CHANGELOG.md) into a new
   version section with the release date.
3. Commit on `dev`, then merge `dev` into `main`.
4. Tag the release on `main` as `vX.Y.Z`.
5. Push the branch and tag; publish the GitHub release from the changelog entry.

## Compatibility

The installed CMake package is versioned with `SameMajorVersion` compatibility, so
consumers on the same major version keep resolving. Breaking changes bump the
major version.
