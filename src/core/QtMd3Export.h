#ifndef QT_MD3_EXPORT_H
#define QT_MD3_EXPORT_H

// Export/import macro for the qt-md3 library.
//
// The library is static by default (BUILD_SHARED_LIBS=OFF), so QT_MD3_EXPORT
// expands to nothing unless a shared build is requested. When building the
// library itself, QT_MD3_BUILDING_LIBRARY is defined (see src/CMakeLists.txt).

#include <QtCore/QtGlobal>

#if defined(QT_MD3_STATIC)
#  define QT_MD3_EXPORT
#elif defined(QT_MD3_BUILDING_LIBRARY)
#  define QT_MD3_EXPORT Q_DECL_EXPORT
#else
#  define QT_MD3_EXPORT Q_DECL_IMPORT
#endif

#endif // QT_MD3_EXPORT_H
