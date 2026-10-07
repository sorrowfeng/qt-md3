cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" QT_MD3_VERSION LIMIT_COUNT 1)
string(STRIP "${QT_MD3_VERSION}" QT_MD3_VERSION)
if(NOT QT_MD3_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must use MAJOR.MINOR.PATCH semantic version format")
endif()

project(qt-md3
    VERSION ${QT_MD3_VERSION}
    LANGUAGES CXX
)

# PROJECT_IS_TOP_LEVEL was added after the minimum supported CMake release.
# Preserve identical subproject defaults for manual CMake 3.16-3.20 builds.
if(NOT DEFINED PROJECT_IS_TOP_LEVEL)
    if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
        set(PROJECT_IS_TOP_LEVEL ON)
    else()
        set(PROJECT_IS_TOP_LEVEL OFF)
    endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/FS /utf-8)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Svg)

set(QT_MD3_QT_MAJOR_VERSION ${QT_VERSION_MAJOR})
set(QT_MD3_QT_PACKAGE Qt${QT_VERSION_MAJOR})
if(QT_VERSION_MAJOR EQUAL 6)
    set(QT_MD3_MIN_QT_VERSION 6.5.0)
elseif(QT_VERSION_MAJOR EQUAL 5)
    set(QT_MD3_MIN_QT_VERSION 5.15.2)
else()
    message(FATAL_ERROR "qt-md3 supports only Qt 5 and Qt 6")
endif()
set(QT_MD3_DETECTED_QT_VERSION "${${QT_MD3_QT_PACKAGE}_VERSION}")
if(NOT QT_MD3_DETECTED_QT_VERSION)
    set(QT_MD3_DETECTED_QT_VERSION "${QT_VERSION}")
endif()
if(QT_MD3_DETECTED_QT_VERSION VERSION_LESS QT_MD3_MIN_QT_VERSION)
    message(FATAL_ERROR
        "qt-md3 requires ${QT_MD3_QT_PACKAGE} ${QT_MD3_MIN_QT_VERSION} or newer; "
        "found ${QT_MD3_DETECTED_QT_VERSION}"
    )
endif()
set(QT_MD3_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(QT_MD3_VERSION_HEADER "${QT_MD3_GENERATED_INCLUDE_DIR}/core/QtMd3Version.h")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/QtMd3Version.h.in
    ${QT_MD3_VERSION_HEADER}
    @ONLY
)

message(STATUS
    "qt-md3 using ${QT_MD3_QT_PACKAGE} ${QT_MD3_DETECTED_QT_VERSION} "
    "(minimum ${QT_MD3_MIN_QT_VERSION})"
)

# Options
set(_qt_md3_default_examples ${PROJECT_IS_TOP_LEVEL})
set(_qt_md3_default_tests ${PROJECT_IS_TOP_LEVEL})
if(PROJECT_IS_TOP_LEVEL AND DEFINED BUILD_EXAMPLES)
    set(_qt_md3_default_examples ${BUILD_EXAMPLES})
endif()
if(PROJECT_IS_TOP_LEVEL AND DEFINED BUILD_TESTS)
    set(_qt_md3_default_tests ${BUILD_TESTS})
endif()
option(QT_MD3_BUILD_EXAMPLES "Build the qt-md3 example application" ${_qt_md3_default_examples})
option(QT_MD3_BUILD_TESTS "Build qt-md3 tests (requires Qt Test)" ${_qt_md3_default_tests})
if(WIN32 AND PROJECT_IS_TOP_LEVEL)
    set(_qt_md3_default_deploy_example ON)
else()
    set(_qt_md3_default_deploy_example OFF)
endif()
option(QT_MD3_DEPLOY_EXAMPLE
    "Run windeployqt while installing the example application"
    ${_qt_md3_default_deploy_example}
)
option(QT_MD3_ENABLE_ADDRESS_SANITIZER
    "Enable AddressSanitizer for qt-md3 (link requirements propagate to build-tree consumers)"
    OFF
)
option(QT_MD3_ENABLE_UNDEFINED_SANITIZER
    "Enable UndefinedBehaviorSanitizer for qt-md3 (link requirements propagate to build-tree consumers)"
    OFF
)

unset(_qt_md3_default_examples)
unset(_qt_md3_default_tests)
unset(_qt_md3_default_deploy_example)

# Windows: put the Qt runtime next to every executable we produce, so the build
# tree is runnable by double-click instead of only from a Qt-aware shell.
include(cmake/QtMd3Deploy.cmake)

# Library
add_subdirectory(src)

# Examples
if(QT_MD3_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(QT_MD3_BUILD_TESTS)
    find_package(${QT_MD3_QT_PACKAGE} REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install targets
set(INSTALL_TARGETS qt-md3)
if(QT_MD3_BUILD_EXAMPLES)
    list(APPEND INSTALL_TARGETS qt-md3-example)
endif()

install(TARGETS ${INSTALL_TARGETS}
    EXPORT qt-md3-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT qt-md3-targets
    NAMESPACE qt-md3::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qt-md3
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt-md3Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/qt-md3Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qt-md3
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/qt-md3ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/qt-md3Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/qt-md3ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/qt-md3
)

# Windows deployment
if(QT_MD3_BUILD_EXAMPLES AND QT_MD3_DEPLOY_EXAMPLE AND WIN32)
    set(QT_MD3_QT_DIR_VARIABLE "${QT_MD3_QT_PACKAGE}_DIR")
    set(QT_MD3_QT_DIR "${${QT_MD3_QT_DIR_VARIABLE}}")
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_MD3_QT_DIR}/../../../bin")
    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR
            "QT_MD3_DEPLOY_EXAMPLE is ON, but windeployqt was not found. "
            "Set QT_MD3_DEPLOY_EXAMPLE=OFF for a library-only/non-deploying install."
        )
    endif()
    install(CODE "
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/qt-md3-example${CMAKE_EXECUTABLE_SUFFIX}\"
            RESULT_VARIABLE windeployqt_result
        )
        if(NOT windeployqt_result EQUAL 0)
            message(FATAL_ERROR \"windeployqt failed with exit code \${windeployqt_result}\")
        endif()
    ")
endif()
