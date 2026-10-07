#include "MdCore.h"

#include "core/QtMd3Version.h"

namespace md {

const char *libraryVersion()
{
    return QT_MD3_VERSION_STRING;
}

int libraryVersionMajor()
{
    return QT_MD3_VERSION_MAJOR;
}

int libraryVersionMinor()
{
    return QT_MD3_VERSION_MINOR;
}

int libraryVersionPatch()
{
    return QT_MD3_VERSION_PATCH;
}

} // namespace md
