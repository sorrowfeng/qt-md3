#!/usr/bin/env bash
#
# Launches the gallery that was built into build/.
#
#   tools/run-gallery.sh                       # normal launch
#   tools/run-gallery.sh --theme dark          # open in dark mode
#   tools/run-gallery.sh --dynamic             # open with dynamic colour on
#   tools/run-gallery.sh --smoke-exit-ms 3000  # render, then quit (CI)
#   tools/run-gallery.sh --screenshot DIR      # write every page as a PNG
#
# The build deploys the Qt runtime next to the executable (see
# cmake/QtMd3Deploy.cmake), so the binary is runnable on its own and this script
# is only a convenience wrapper. The PATH fallback below exists for builds made
# with -DQT_MD3_DEPLOY_EXAMPLE=OFF; set QT_BIN_DIR to point it at a kit
# explicitly if the cache guess is wrong.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build}"

EXE="${BUILD_DIR}/examples/qt-md3-example"
[ -x "${EXE}" ] || EXE="${EXE}.exe"

if [ ! -x "${EXE}" ]; then
    echo "error: ${EXE} not found." >&2
    echo "       Configure and build first, for example:" >&2
    echo "         cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \\" >&2
    echo "           -DCMAKE_PREFIX_PATH=<qt> -DQT_MD3_BUILD_EXAMPLES=ON" >&2
    echo "         cmake --build build" >&2
    exit 1
fi

EXE_DIR="$(cd "$(dirname "${EXE}")" && pwd)"

# A deployed runtime means no PATH work is needed at all.
if [ ! -f "${EXE_DIR}/Qt6Core.dll" ] && [ ! -f "${EXE_DIR}/Qt5Core.dll" ]; then
    if [ -n "${QT_BIN_DIR:-}" ]; then
        export PATH="${QT_BIN_DIR}:${PATH}"
    elif [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
        # Recover the kit from the cache so the script never guesses.
        qt_dir="$(sed -n 's/^Qt6_DIR:PATH=//p' "${BUILD_DIR}/CMakeCache.txt" | head -n1)"
        if [ -z "${qt_dir}" ]; then
            qt_dir="$(sed -n 's/^Qt5_DIR:PATH=//p' "${BUILD_DIR}/CMakeCache.txt" | head -n1)"
        fi
        if [ -n "${qt_dir}" ]; then
            # .../<prefix>/lib/cmake/Qt6  ->  .../<prefix>/bin
            qt_bin="$(cd "${qt_dir}/../../../bin" 2>/dev/null && pwd || true)"
            [ -n "${qt_bin}" ] && export PATH="${qt_bin}:${PATH}"
        fi
    fi
fi

exec "${EXE}" "$@"
