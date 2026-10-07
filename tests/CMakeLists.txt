# qt-md3 tests.

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
endforeach()

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
