#ifndef MD_CORE_H
#define MD_CORE_H

#include "QtMd3Export.h"

// Minimal library-identity API.
//
// This is scaffolding for the bootstrap phase. The numeric version values are
// generated from the repository VERSION file by CMake (see
// cmake/QtMd3Version.h.in). Component, token, and theming APIs are added under
// this same namespace as the Stage 1 base modules land.

namespace md {

QT_MD3_EXPORT const char *libraryVersion();
QT_MD3_EXPORT int libraryVersionMajor();
QT_MD3_EXPORT int libraryVersionMinor();
QT_MD3_EXPORT int libraryVersionPatch();

} // namespace md

#endif // MD_CORE_H
