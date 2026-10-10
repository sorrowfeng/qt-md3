# qt-md3 example application: the component gallery.
#
# The gallery is the inspection surface for the library and is built into the
# project's build/ directory. Each public component gets its own page plus a
# left-nav entry as it lands (see CONTRIBUTING.md, DoD item 7).
#
# The shell and the pages are example code, not library code: they are named
# `Gallery*` rather than `Md*` and live outside src/ so they can never be
# mistaken for a Stage 1 component or picked up by the coverage gate.

add_executable(qt-md3-example
    main.cpp
    gallery/GalleryPage.cpp
    gallery/GalleryPages.cpp
    gallery/GalleryPages2.cpp
    gallery/GalleryPages3.cpp
    gallery/GalleryPages4.cpp
    gallery/GalleryPages5.cpp
    gallery/GalleryPages6.cpp
    gallery/GalleryPages7.cpp
    gallery/GalleryPages8.cpp
    gallery/GalleryPages9.cpp
    gallery/GalleryPages10.cpp
    gallery/GalleryPages11.cpp
    gallery/GalleryPages12.cpp
    gallery/GalleryPages17.cpp
    gallery/GalleryPages18.cpp
    gallery/GalleryPages19.cpp
    gallery/GalleryPages20.cpp
    gallery/GalleryPages21.cpp
    gallery/GalleryPages22.cpp
    gallery/GalleryPages23.cpp
    gallery/GalleryPages24.cpp
    gallery/GalleryPages25.cpp
    gallery/GalleryPages26.cpp
    gallery/GalleryPages27.cpp
    gallery/GalleryPages28.cpp
    gallery/GalleryPages29.cpp
    gallery/GalleryPages30.cpp
    gallery/GalleryPages31.cpp
    gallery/GalleryPages32.cpp
    gallery/GalleryPages33.cpp
    gallery/GalleryPages34.cpp
    gallery/GalleryPages35.cpp
    gallery/GalleryPages36.cpp
    gallery/GalleryPages37.cpp
    gallery/GalleryPages38.cpp
    gallery/GalleryPages39.cpp
    gallery/GalleryPages40.cpp
    gallery/GalleryPages41.cpp
    gallery/GalleryPages42.cpp
    gallery/GalleryPages43.cpp
    gallery/GalleryWindow.cpp
)

target_link_libraries(qt-md3-example
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
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

# Windows links Qt dynamically, so the executable is unrunnable until the Qt
# modules, the C++ runtime and the platform plugin sit beside it. See
# cmake/QtMd3Deploy.cmake for why this does not just call windeployqt.
qt_md3_deploy_runtime(qt-md3-example)
