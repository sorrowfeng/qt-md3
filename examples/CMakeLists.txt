# qt-md3 example application: the component gallery.
#
# This is intentionally a thin shell for now. Each public component gets its own
# page plus a left-nav entry as it lands (see CONTRIBUTING.md, DoD item 7).

add_executable(qt-md3-example
    main.cpp
)

target_link_libraries(qt-md3-example
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
)

set_target_properties(qt-md3-example PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

if(MSVC)
    set_target_properties(qt-md3-example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    )
endif()
