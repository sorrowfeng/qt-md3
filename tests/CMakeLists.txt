# qt-md3 tests.
#
# As in the example directory, the Qt runtime is deployed next to the binaries
# so `ctest` works from a plain shell rather than only from one that happens to
# have the Qt bin directory on PATH. Every test target shares this directory, so
# only the first deployment copies anything.

# --- Unit tests -------------------------------------------------------------
add_executable(TestMd3Version
    TestMd3Version.cpp
)

target_link_libraries(TestMd3Version
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Version COMMAND TestMd3Version)

qt_md3_deploy_runtime(TestMd3Version)

# Token audit: every foundation numeric value is compared against its
# authoritative source (material-web v0.192 / Compose Material3). This is the
# correctness bedrock of the whole library, so it is a normal (not "policy")
# test and must stay green at all times.
add_executable(TestMd3Tokens
    TestMd3Tokens.cpp
)

target_link_libraries(TestMd3Tokens
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Tokens COMMAND TestMd3Tokens)

qt_md3_deploy_runtime(TestMd3Tokens)

# Contrast levels, and the ColorSpec2021 tone solver underneath them.
#
# Two independent references are involved and they verify different things:
# the vendored md.sys.color sets pin the *static baseline*
# (MdColorScheme::baseline), and resources/tokens/mcu-scheme-expectations.txt —
# flattened from material-color-utilities' own Swift tests — pins the *dynamic
# solver* across nine variants, both modes and three contrast levels.
add_executable(TestMd3Contrast
    TestMd3Contrast.cpp
)

target_link_libraries(TestMd3Contrast
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Contrast COMMAND TestMd3Contrast)

# The vendored md.sys.color token sets are read from the source tree rather than
# from the Qt resource bundle: they are reference data for the audit, not
# assets the library installs.
target_compile_definitions(TestMd3Contrast
    PRIVATE
        QT_MD3_TOKEN_FIXTURES="${CMAKE_SOURCE_DIR}/resources/tokens"
)

qt_md3_deploy_runtime(TestMd3Contrast)

# MdTemperatureCache feeds the `content` and `fidelity` tertiary palettes and
# nothing else, so a defect in it is invisible from every other variant's
# numbers. Its expectations come straight from material-color-utilities'
# cpp/temperature/temperature_cache_test.cc.
add_executable(TestMd3TemperatureCache
    TestMd3TemperatureCache.cpp
)

target_link_libraries(TestMd3TemperatureCache
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3TemperatureCache COMMAND TestMd3TemperatureCache)

qt_md3_deploy_runtime(TestMd3TemperatureCache)

# The interaction primitives every component depends on: ripple geometry and
# timing, the focus indicator, the icon back ends, and the Pattern A paint hub.
#
# One binary per module, each with its own QTEST_MAIN. A single binary hosting
# several QObject test classes and driving them through QTest::qExec in a loop
# cannot honour `-o`/class-name arguments, so `ctest -R TestMd3Icon` would run
# the wrong suite. Separate binaries keep per-class selection working.
set(QT_MD3_PRIMITIVE_TESTS
    TestMd3Ripple
    TestMd3FocusRing
    TestMd3Icon
    TestMd3StyleBase
)

foreach(primitive IN LISTS QT_MD3_PRIMITIVE_TESTS)
    add_executable(${primitive} ${primitive}.cpp)
    target_link_libraries(${primitive}
        PRIVATE
            qt-md3
            Qt${QT_VERSION_MAJOR}::Core
            Qt${QT_VERSION_MAJOR}::Gui
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Svg
            Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${primitive} COMMAND ${primitive})
    set_tests_properties(${primitive} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
    qt_md3_deploy_runtime(${primitive})
endforeach()

# --- Component tests --------------------------------------------------------

# MdButton, the first Stage 1 component. Covers the md.comp.button token table
# (field by field, against the published size and variant sets), the widget
# contract, and a render smoke check over all 5 x 5 x 2 combinations.
add_executable(TestMd3Button
    TestMd3Button.cpp
)

target_link_libraries(TestMd3Button
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Button COMMAND TestMd3Button)
set_tests_properties(TestMd3Button PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Button)

# MdButtonGroup, the second Stage 1 component. A group publishes no colour at
# all — everything it contributes is spacing, shape and a selection model — so
# the checks here are the token table, the geometry measured on real widgets,
# and the four selection modes that differ from each other by one behaviour
# each.
add_executable(TestMd3ButtonGroup
    TestMd3ButtonGroup.cpp
)

target_link_libraries(TestMd3ButtonGroup
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3ButtonGroup COMMAND TestMd3ButtonGroup)
set_tests_properties(TestMd3ButtonGroup PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(TestMd3IconButton
    TestMd3IconButton.cpp
)

target_link_libraries(TestMd3IconButton
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3IconButton COMMAND TestMd3IconButton)
set_tests_properties(TestMd3IconButton PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3IconButton)

# --- FAB ---------------------------------------------------------------------
# MdFab: the token table (metrics per size, the four colour sets, the lowered
# elevation rows and the spec-filled disabled row), the widget contract, and a
# render smoke check over all 4 x 3 x 2 combinations.
add_executable(TestMd3Fab
    TestMd3Fab.cpp
)

target_link_libraries(TestMd3Fab
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Fab COMMAND TestMd3Fab)
set_tests_properties(TestMd3Fab PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Fab)
qt_md3_deploy_runtime(TestMd3ButtonGroup)

add_executable(TestMd3ExtendedFab
    TestMd3ExtendedFab.cpp
)

target_link_libraries(TestMd3ExtendedFab
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3ExtendedFab COMMAND TestMd3ExtendedFab)
set_tests_properties(TestMd3ExtendedFab PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3ExtendedFab)

add_executable(TestMd3FabMenu
    TestMd3FabMenu.cpp
)

target_link_libraries(TestMd3FabMenu
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3FabMenu COMMAND TestMd3FabMenu)
set_tests_properties(TestMd3FabMenu PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3FabMenu)

add_executable(TestMd3SplitButton
    TestMd3SplitButton.cpp
)

target_link_libraries(TestMd3SplitButton
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3SplitButton COMMAND TestMd3SplitButton)
set_tests_properties(TestMd3SplitButton PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3SplitButton)

add_executable(TestMd3SegmentedButton
    TestMd3SegmentedButton.cpp
)

target_link_libraries(TestMd3SegmentedButton
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3SegmentedButton COMMAND TestMd3SegmentedButton)
set_tests_properties(TestMd3SegmentedButton PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3SegmentedButton)

# --- Communication -----------------------------------------------------------
# MdBadge, the first Communication family: the token table (dot 6 px, content
# pill minimum 16 px, error / on-error, label-small), the non-interactivity
# contract (no state rows are published at all), the anchoring offsets of
# MdBadgedBox, and the reserved-overhang Qt adaptation.
add_executable(TestMd3Badge
    TestMd3Badge.cpp
)

target_link_libraries(TestMd3Badge
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Badge COMMAND TestMd3Badge)
set_tests_properties(TestMd3Badge PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Badge)

# --- Policy gates -----------------------------------------------------------
# These are CMake scripts, not compiled tests. They enforce project-wide rules
# that must hold from day one.

add_test(
    NAME TestMd3NoQss
    COMMAND ${CMAKE_COMMAND}
        -DSRC_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/TestMd3NoQss.cmake
)

add_test(
    NAME TestMd3CoveragePolicy
    COMMAND ${CMAKE_COMMAND}
        -DSRC_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/TestMd3CoveragePolicy.cmake
)

# Source encoding: every file valid UTF-8, and no UTF-8 literal decoded through
# a Latin-1 API. The failure this prevents is invisible in an editor and only
# shows up as mojibake in a rendered screenshot.
add_test(
    NAME TestMd3SourceEncoding
    COMMAND ${CMAKE_COMMAND}
        -DSRC_ROOT=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/TestMd3SourceEncoding.cmake
)

set_tests_properties(TestMd3NoQss TestMd3CoveragePolicy TestMd3SourceEncoding
    PROPERTIES
    LABELS "policy"
)

# --- Windows runtime deployment ---------------------------------------------
# A dynamically linked executable starts only when every DLL it imports sits
# beside it. Nothing in the source, in the compiler or in the rest of this file
# notices a missing one — the first version of cmake/QtMd3Deploy.cmake deployed
# the modules the gallery links and left out Qt6Test.dll, at which point five of
# the six suites above could not launch, and the only reason it was caught at
# all was that CTest ran them with an empty PATH by accident.
#
# These entries reproduce the double-click environment deliberately: PATH
# reduced to the Windows directories, and QT_PLUGIN_PATH / QT_QPA_PLATFORM
# cleared so that a Qt-aware shell cannot mask an incomplete deployment. They
# cover both a unit-test binary and the example application because the two
# resolve different Qt module sets — the example does not link Qt6::Test, so a
# gate on either one alone would miss half the failure.
if(WIN32 AND QT_MD3_DEPLOY_EXAMPLE)
    # A quoted argument keeps the `;` inside PATH from being read as a list
    # separator.
    file(TO_CMAKE_PATH "$ENV{SystemRoot}" _qt_md3_system_root)
    set(_qt_md3_sanitized_path "${_qt_md3_system_root}/System32;${_qt_md3_system_root}")

    add_test(NAME TestMd3DeploymentTestBinary
        COMMAND ${CMAKE_COMMAND} -E env
            --unset=QT_PLUGIN_PATH
            --unset=QT_QPA_PLATFORM
            "PATH=${_qt_md3_sanitized_path}"
            $<TARGET_FILE:TestMd3Version>
    )
    set_tests_properties(TestMd3DeploymentTestBinary PROPERTIES
        LABELS "deployment"
    )

    if(TARGET qt-md3-example)
        add_test(NAME TestMd3DeploymentExample
            COMMAND ${CMAKE_COMMAND} -E env
                --unset=QT_PLUGIN_PATH
                --unset=QT_QPA_PLATFORM
                "PATH=${_qt_md3_sanitized_path}"
                $<TARGET_FILE:qt-md3-example> --smoke-exit-ms 800
        )
        set_tests_properties(TestMd3DeploymentExample PROPERTIES
            LABELS "deployment"
        )
    endif()
endif()
