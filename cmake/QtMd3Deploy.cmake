# Runtime deployment for Windows builds.
#
# `build/examples/qt-md3-example.exe` links Qt dynamically, so before it can
# start Windows has to locate Qt6Core/Gui/Widgets/Svg, the MinGW C++ runtime and
# the `qwindows` platform plugin. None of those live in the build tree and none
# live in a system directory, so double-clicking the executable from Explorer
# fails with "The code execution cannot proceed because Qt6Core.dll was not
# found" — while the very same binary starts fine from a shell that happens to
# have the Qt bin directory on PATH. Removing that asymmetry is this module's
# entire job.
#
# Two obvious implementations are unavailable:
#
#   * `windeployqt` (Qt's own tool, still used by `cmake --install`) discovers
#     the installation layout by running `qtpaths` in a child process and reading
#     its output through a named pipe. Where named pipe instances are exhausted
#     it fails outright —
#         QProcess: CreateFile failed. (all pipe instances are busy)
#         Unable to query qtpaths: Error running binary qtpaths: pipe:
#     — and no amount of retrying, PATH editing or shell swapping fixes it.
#
#   * `file(GET_RUNTIME_DEPENDENCIES)` walks the import tables of the binary,
#     which is the authoritative answer, but it too spawns a helper process and
#     hangs indefinitely in the same environment (verified with a plain
#     `notepad.exe` as input, so it is not about our binaries).
#
# What is left, and what this module does, is to compute the dependency set from
# information CMake already has: the link closure of the target. That is exact
# rather than approximated — every Qt DLL the loader will need is a Qt module in
# that closure — needs no subprocess, and cannot silently rot the way a
# hand-written table does. (It did: the first version of this file listed the
# four modules the gallery links, and every test binary then failed to start
# because `TestMd3Version` also links `Qt6::Test`.)
#
# What gets copied:
#   * every `Qt<major>::<Module>` in the target's link closure that resolves to a
#     shared library — resolved through `$<TARGET_FILE:...>`, so the active
#     configuration is chosen automatically (MSVC packages ship separate
#     `Qt6Cored.dll` debug builds, MinGW packages only release names)
#   * the GCC runtime next to the compiler, when the toolchain is GCC
#   * every `platforms/` and `styles/` plugin carrying no debug symbols
#   * a `qt.conf` pinning Qt's plugin search to the executable's own directory
#
# `platforms/` is copied whole on purpose: Qt aborts during start-up with
# "could not find or load the Qt platform plugin" if the one it needs is
# missing, so four plugins (~3 MB) are cheaper than that failure mode. They also
# keep `QT_QPA_PLATFORM=offscreen` working for the gallery's `--screenshot` mode.
#
# `imageformats/` is deliberately absent. No code path in this project loads an
# image through Qt's plugin system: the embedded SVG icons go through
# QSvgRenderer and screenshots are written as PNG, both of which are built into
# QtGui. Add that kind below if that ever stops being true.

include_guard(GLOBAL)

# Prints the Qt installation's plugins/ directory, or an empty string when it
# cannot be determined.
function(_qt_md3_plugin_root out_var)
    if(NOT DEFINED QT_MD3_QT_PACKAGE)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    # `${Qt6_DIR}` is `<prefix>/lib/cmake/Qt6`, so three levels up is `<prefix>`.
    set(_dir_var "${QT_MD3_QT_PACKAGE}_DIR")
    if(NOT DEFINED ${_dir_var})
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()
    get_filename_component(_prefix "${${_dir_var}}/../../.." ABSOLUTE)

    if(EXISTS "${_prefix}/plugins")
        set(${out_var} "${_prefix}/plugins" PARENT_SCOPE)
    else()
        set(${out_var} "" PARENT_SCOPE)
    endif()
endfunction()

# Walks the link closure of <target> and prints every `Qt<major>::<Module>` in it
# that resolves to a shared library.
#
# Entries are read from both LINK_LIBRARIES and INTERFACE_LINK_LIBRARIES, and a
# regular expression pulls the Qt module name out of generator-expression
# wrappers. That matters for real cases: a target linking only `qt-md3` gets its
# Qt modules through the static library's interface as `$<LINK_ONLY:Qt6::Gui>`,
# which is not a plain target name and would otherwise be skipped. Private Qt
# modules (`Qt6::GuiPrivate` and friends) are interface-only and are dropped by
# the type filter.
function(_qt_md3_collect_qt_modules target out_var)
    set(_pending ${target})
    set(_visited)
    set(_modules)

    while(_pending)
        list(POP_FRONT _pending _current)

        if(_current IN_LIST _visited)
            continue()
        endif()
        list(APPEND _visited ${_current})

        if(_current MATCHES "^Qt[0-9]+::")
            get_target_property(_type ${_current} TYPE)
            if(_type STREQUAL "SHARED_LIBRARY")
                list(APPEND _modules ${_current})
            endif()
        endif()

        if(NOT TARGET ${_current})
            continue()
        endif()
        get_target_property(_interface ${_current} INTERFACE_LINK_LIBRARIES)
        get_target_property(_own ${_current} LINK_LIBRARIES)
        foreach(_entry IN LISTS _interface _own)
            # `Qt6::Gui` and `$<LINK_ONLY:Qt6::Gui>` both yield the module name.
            string(REGEX MATCHALL "Qt[0-9]+::[A-Za-z0-9_]+" _found "${_entry}")
            list(APPEND _pending ${_found})
            # Plain target names, such as our own static library, are followed
            # through the property walk above instead.
            if(TARGET ${_entry})
                list(APPEND _pending ${_entry})
            endif()
        endforeach()
    endwhile()

    list(REMOVE_DUPLICATES _modules)
    set(${out_var} ${_modules} PARENT_SCOPE)
endfunction()

# Copies the runtime dependencies of <target> next to its executable.
#
# Safe to call for several targets that share an output directory: every copy is
# `copy_if_different`, so the first one populates the directory and the rest
# collapse to a stat. The commands hang off POST_BUILD, so they run when the
# target is linked rather than on every build.
function(qt_md3_deploy_runtime target)
    if(NOT WIN32)
        return()
    endif()
    if(DEFINED QT_MD3_DEPLOY_EXAMPLE AND NOT QT_MD3_DEPLOY_EXAMPLE)
        message(STATUS
            "qt-md3: QT_MD3_DEPLOY_EXAMPLE=OFF, leaving ${target} without a "
            "local Qt runtime (run it from a Qt-aware shell)"
        )
        return()
    endif()
    if(NOT TARGET ${target})
        message(FATAL_ERROR "qt_md3_deploy_runtime: no such target '${target}'")
        return()
    endif()

    _qt_md3_collect_qt_modules(${target} _modules)
    if(NOT _modules)
        if(TARGET ${QT_MD3_QT_PACKAGE}::Core)
            get_target_property(_core_type ${QT_MD3_QT_PACKAGE}::Core TYPE)
            if(_core_type STREQUAL "STATIC_LIBRARY")
                message(STATUS
                    "qt-md3: ${QT_MD3_QT_PACKAGE} is static, nothing to deploy "
                    "for ${target}"
                )
                return()
            endif()
        endif()
        message(WARNING
            "qt-md3: ${target} links no shared Qt module, so nothing was "
            "deployed. If it fails to start, check its link libraries."
        )
        return()
    endif()

    set(_dest "$<TARGET_FILE_DIR:${target}>")
    set(_qt_dlls)
    foreach(_module IN LISTS _modules)
        list(APPEND _qt_dlls "$<TARGET_FILE:${_module}>")
    endforeach()

    # --- Toolchain runtime --------------------------------------------------
    # GCC links libstdc++/libgcc/libwinpthread dynamically and MinGW does not put
    # them anywhere Windows searches. `libgcc_s_*.dll` covers the seh / dw2 /
    # sjlj variants; only one is ever present.
    set(_toolchain_dlls)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        get_filename_component(_compiler_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        if(IS_ABSOLUTE "${_compiler_dir}" AND EXISTS "${_compiler_dir}")
            file(GLOB _toolchain_dlls
                "${_compiler_dir}/libgcc_s_*.dll"
                "${_compiler_dir}/libstdc++-6.dll"
                "${_compiler_dir}/libwinpthread-1.dll"
            )
            if(NOT _toolchain_dlls)
                message(WARNING
                    "qt-md3: no GCC runtime DLLs found in '${_compiler_dir}'. "
                    "${target} will fail to start unless that directory is on PATH."
                )
            endif()
        else()
            message(WARNING
                "qt-md3: CMAKE_CXX_COMPILER is not an absolute path "
                "('${CMAKE_CXX_COMPILER}'), so the GCC runtime cannot be located. "
                "${target} will need a Qt-aware PATH to start."
            )
        endif()
    elseif(MSVC)
        # The MSVC runtime is a redistributable, not a build artifact. Debug
        # builds additionally need the debug CRT, which is not redistributable;
        # `cmake --install` handles both through windeployqt.
        message(STATUS
            "qt-md3: MSVC toolchain — ${target} also needs the Visual C++ "
            "redistributable installed (windeployqt adds it on install)"
        )
    endif()

    # --- Plugins ------------------------------------------------------------
    _qt_md3_plugin_root(_plugin_root)
    set(_plugin_kinds platforms styles)
    set(_plugin_files)
    set(_plugin_count 0)
    if(_plugin_root)
        foreach(_kind IN LISTS _plugin_kinds)
            file(GLOB _kind_files "${_plugin_root}/${_kind}/*.dll")
            if(_kind_files)
                list(APPEND _plugin_files ${_kind_files})
            endif()
        endforeach()
        list(LENGTH _plugin_files _plugin_count)
    else()
        message(WARNING
            "qt-md3: Qt's plugins/ directory was not found next to "
            "'${${QT_MD3_QT_PACKAGE}_DIR}'. ${target} will fail with "
            "\"could not find or load the Qt platform plugin\" unless "
            "QT_PLUGIN_PATH points at the Qt installation."
        )
    endif()

    # --- qt.conf ------------------------------------------------------------
    # Relative paths inside qt.conf resolve against the directory holding the
    # file, so `Prefix = .` makes the executable's own directory the Qt prefix
    # and `plugins/` the plugin location. Without it Qt falls back to the
    # compiled-in path of the Qt installation that built the library, which is
    # exactly what breaks on a machine where Qt is not installed.
    set(_qt_conf "${CMAKE_BINARY_DIR}/qt-md3-runtime/qt.conf")
    get_filename_component(_qt_conf_dir "${_qt_conf}" DIRECTORY)
    if(NOT EXISTS "${_qt_conf_dir}")
        file(MAKE_DIRECTORY "${_qt_conf_dir}")
    endif()
    file(WRITE "${_qt_conf}" "[Paths]\nPrefix = .\n")

    # --- Commands -----------------------------------------------------------
    set(_commands
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_dest}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${_qt_dlls} ${_toolchain_dlls} "${_dest}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${_qt_conf}" "${_dest}/qt.conf"
    )
    foreach(_kind IN LISTS _plugin_kinds)
        file(GLOB _kind_files "${_plugin_root}/${_kind}/*.dll")
        if(_kind_files)
            list(APPEND _commands
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${_dest}/${_kind}"
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    ${_kind_files} "${_dest}/${_kind}"
            )
        endif()
    endforeach()

    add_custom_command(TARGET ${target} POST_BUILD ${_commands}
        COMMENT "Deploying the Qt runtime next to ${target}"
        VERBATIM
    )

    # One summary per output directory rather than one per executable: every
    # target sharing a directory resolves to the same file set.
    string(MD5 _report_key "${CMAKE_CURRENT_BINARY_DIR}")
    get_property(_reported GLOBAL PROPERTY QT_MD3_DEPLOY_REPORT_${_report_key})
    if(NOT _reported)
        set_property(GLOBAL PROPERTY QT_MD3_DEPLOY_REPORT_${_report_key} TRUE)
        set(_short_names)
        foreach(_module IN LISTS _modules)
            string(REPLACE "${QT_MD3_QT_PACKAGE}::" "" _short "${_module}")
            list(APPEND _short_names ${_short})
        endforeach()
        list(JOIN _short_names ", " _short_names)
        message(STATUS
            "qt-md3: deploying into ${CMAKE_CURRENT_BINARY_DIR} — "
            "Qt modules [${_short_names}], "
            "${CMAKE_CXX_COMPILER_ID} runtime, ${_plugin_count} plugin(s); "
            "disable with -DQT_MD3_DEPLOY_EXAMPLE=OFF"
        )
    endif()
endfunction()
