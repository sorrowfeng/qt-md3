# qt-md3 library target.
#
# Layer map:
#   core/    — tokens, theme, color algorithms, type scale, shape, motion,
#              icons, utilities, export macro
#   styles/  — every Md*Style painting class
#   widgets/ — every public component (Md*.h)
#
# Every source file is listed explicitly. Do not switch to GLOB.

set(QT_MD3_PUBLIC_HEADERS
    core/QtMd3Export.h
    core/MdCore.h
)

set(QT_MD3_SOURCES
    core/MdCore.cpp
)

set(QT_MD3_RESOURCES
    ../resources/qt-md3.qrc
)

add_library(qt-md3
    ${QT_MD3_SOURCES}
    ${QT_MD3_PUBLIC_HEADERS}
    ${QT_MD3_RESOURCES}
)
add_library(qt-md3::qt-md3 ALIAS qt-md3)

target_include_directories(qt-md3
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${QT_MD3_GENERATED_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-md3>
    PRIVATE
        ${QT_MD3_GENERATED_INCLUDE_DIR}
)

target_link_libraries(qt-md3
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
)

# Export macro plumbing. A static build must tell consumers the symbols are not
# imported from a DLL; a shared build marks the building side. QT_MD3_STATIC is
# PUBLIC so it propagates through the installed export target.
if(BUILD_SHARED_LIBS)
    target_compile_definitions(qt-md3 PRIVATE QT_MD3_BUILDING_LIBRARY)
else()
    target_compile_definitions(qt-md3 PUBLIC QT_MD3_STATIC)
endif()

set_target_properties(qt-md3 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    set_target_properties(qt-md3 PROPERTIES
        DEBUG_POSTFIX "d"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    )
endif()

if(QT_MD3_ENABLE_ADDRESS_SANITIZER AND NOT MSVC)
    target_compile_options(qt-md3 PUBLIC -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(qt-md3 PUBLIC -fsanitize=address)
endif()
if(QT_MD3_ENABLE_UNDEFINED_SANITIZER AND NOT MSVC)
    target_compile_options(qt-md3 PUBLIC -fsanitize=undefined -fno-omit-frame-pointer)
    target_link_options(qt-md3 PUBLIC -fsanitize=undefined)
endif()

# Installed headers: public headers from core/ plus the generated version header.
install(FILES ${QT_MD3_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt-md3/core
)
install(FILES ${QT_MD3_VERSION_HEADER}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt-md3/core
)
