#include "MdResources.h"

// Q_INIT_RESOURCE declares `extern int qInitResources_qt_md3();` and calls it.
// The declaration has to sit at global scope: inside a namespace it would name
// `md::qInitResources_qt_md3`, which nothing defines.
//
// `qt-md3-symbols.qrc` is the optional Material Symbols font bundle that
// CMake generates under the build tree (empty when resources/fonts/ has no
// MaterialSymbols*.ttf). Its initialiser is always linked so the call below
// always resolves; an empty bundle registers nothing.
static void qt_md3_init_resources()
{
    Q_INIT_RESOURCE(qt_md3);
    Q_INIT_RESOURCE(qt_md3_symbols);
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
