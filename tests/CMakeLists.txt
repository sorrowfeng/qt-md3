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

set_tests_properties(TestMd3NoQss TestMd3CoveragePolicy PROPERTIES
    LABELS "policy"
)
