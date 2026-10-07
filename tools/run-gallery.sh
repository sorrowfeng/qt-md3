#!/usr/bin/env bash
#
# Launches the gallery that was built into build/.
#
# The example links Qt dynamically, so on Windows the Qt DLLs have to be
# findable at run time. Rather than asking you to fix PATH by hand every time,
# this script finds the Qt kit that was used to configure the build and puts its
# bin directory first.
#
#   tools/run-gallery.sh                       # normal launch
#   tools/run-gallery.sh --theme dark          # open in dark mode
#   tools/run-gallery.sh --dynamic             # open with dynamic colour on
#   tools/run-gallery.sh --smoke-exit-ms 3000  # render, then quit (CI)
#
# Override the kit with QT_BIN_DIR=/path/to/qt/bin if the guess is wrong.

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

exec "${EXE}" "$@"
