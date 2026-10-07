#include "MdResources.h"

// Q_INIT_RESOURCE declares `extern int qInitResources_qt_md3();` and calls it.
// The declaration has to sit at global scope: inside a namespace it would name
// `md::qInitResources_qt_md3`, which nothing defines.
static void qt_md3_init_resources()
{
    Q_INIT_RESOURCE(qt_md3);
}

namespace md {

namespace {

bool g_initialized = false;

} // namespace

void MdResources::ensure()
{
    if (g_initialized) {
        return;
    }
    g_initialized = true;
    qt_md3_init_resources();
}

bool MdResources::isInitialized()
{
    return g_initialized;
}

} // namespace md
