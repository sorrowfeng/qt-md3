# TestMd3NoQss
#
# Gate: qt-md3 paints every component with QPainter / QProxyStyle. QSS must never
# be used to style a component. This script scans the source tree and fails if it
# finds any QSS usage.
#
# Run as:
#   cmake -DSRC_ROOT=<repo root> -P tests/TestMd3NoQss.cmake

if(NOT DEFINED SRC_ROOT)
    message(FATAL_ERROR "TestMd3NoQss: SRC_ROOT is not defined")
endif()

set(_scan_dirs src examples tests resources)
set(_extensions h hpp hh hxx c cc cpp cxx ui qss)

# Forbidden substrings. The gate script itself is a .cmake file and is never
# scanned, so listing the tokens here is safe.
set(_forbidden
    "setStyleSheet"
    "QStyleSheet"
    ".qss"
)

set(_scanned 0)
set(_violations "")

foreach(_dir IN LISTS _scan_dirs)
    set(_root "${SRC_ROOT}/${_dir}")
    if(NOT EXISTS "${_root}")
        continue()
    endif()
    foreach(_ext IN LISTS _extensions)
        file(GLOB_RECURSE _files "${_root}/*.${_ext}")
        foreach(_file IN LISTS _files)
            math(EXPR _scanned "${_scanned} + 1")
            file(READ "${_file}" _content)
            foreach(_token IN LISTS _forbidden)
                string(FIND "${_content}" "${_token}" _pos)
                if(NOT _pos EQUAL -1)
                    file(RELATIVE_PATH _rel "${SRC_ROOT}" "${_file}")
                    list(APPEND _violations "${_rel}  ->  '${_token}'")
                endif()
            endforeach()
        endforeach()
    endforeach()
endforeach()

if(_violations)
    list(REMOVE_DUPLICATES _violations)
    string(REPLACE ";" "\n  - " _report "${_violations}")
    message(FATAL_ERROR
        "TestMd3NoQss: QSS usage detected. qt-md3 must paint with QPainter only.\n"
        "Offending files:\n  - ${_report}"
    )
endif()

message(STATUS "TestMd3NoQss: scanned ${_scanned} file(s), no QSS usage found.")
