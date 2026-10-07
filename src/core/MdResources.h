#ifndef MD_RESOURCES_H
#define MD_RESOURCES_H

// Access to the files compiled into the library under `:/qt-md3/`.
//
// This exists because of a static-library linker rule, not because the library
// wants a resource API. CMake's AUTORCC turns `resources/qt-md3.qrc` into
// `qrc_qt-md3.cpp`, whose only export is an initialiser object with internal
// linkage plus `qInitResources_qt_md3()`. Inside a static archive nothing
// references either, so the linker drops the object file entirely and every
// `:/qt-md3/...` lookup fails — in the test binaries and, more importantly, in
// any application that links the static library.
//
// The fix Qt documents is a `Q_INIT_RESOURCE` call from *inside* the library:
// that leaves an undefined reference to `qInitResources_qt_md3()`, which pulls
// the resource object out of the archive.
//
// Library entry points that read a resource call ensure() themselves, so an
// application never has to know about this. It is public only so a consumer can
// force registration at a chosen moment.

#include "QtMd3Export.h"

namespace md {

class QT_MD3_EXPORT MdResources
{
public:
    /// Registers the embedded resources. Idempotent and cheap after the first
    /// call — the work happens once, guarded by a function-local static.
    static void ensure();

    /// Whether ensure() has completed at least once. Useful in diagnostics that
    /// want to distinguish "no such icon" from "resources never registered".
    static bool isInitialized();
};

} // namespace md

#endif // MD_RESOURCES_H
