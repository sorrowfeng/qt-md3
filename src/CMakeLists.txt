# qt-md3 library target.
#
# Layer map:
#   core/    — tokens, theme, colour algorithms, type scale, shape, motion,
#              icons, utilities, export macro
#   styles/  — every Md*Style painting class
#   widgets/ — every public component (Md*.h)
#
# Every source file is listed explicitly. Do not switch to GLOB.

set(QT_MD3_CORE_HEADERS
    core/QtMd3Export.h
    core/MdCore.h
    core/MdTypes.h
    core/MdTokens.h
    core/MdColorMath.h
    core/MdColorScheme.h
    core/MdColorSpec.h
    core/MdDynamicColor.h
    core/MdTemperatureCache.h
    core/MdShape.h
    core/MdMotion.h
    core/MdTypeScale.h
    core/MdElevation.h
    core/MdStateLayer.h
    core/MdRipple.h
    core/MdFocusRing.h
    core/MdBadgeTokens.h
    core/MdButtonTokens.h
    core/MdBottomSheetTokens.h
    core/MdCarouselTokens.h
    core/MdCardTokens.h
    core/MdDialogTokens.h
    core/MdAppBarScrollBehavior.h
    core/MdAppBarTokens.h
    core/MdDockedToolbarTokens.h
    core/MdFloatingToolbarTokens.h
    core/MdNavigationBarTokens.h
    core/MdNavigationRailTokens.h
    core/MdNavigationDrawerTokens.h
    core/MdDividerTokens.h
    core/MdListTokens.h
    core/MdButtonGroupTokens.h
    core/MdIconButtonTokens.h
    core/MdFabTokens.h
    core/MdExtendedFabTokens.h
    core/MdFabMenuTokens.h
    core/MdProgressIndicatorTokens.h
    core/MdLoadingIndicatorTokens.h
    core/MdSideSheetTokens.h
    core/MdSnackbarTokens.h
    core/MdTooltipTokens.h
    core/MdMaterialShapes.h
    core/MdSegmentedButtonTokens.h
    core/MdSplitButtonTokens.h
    core/MdQtCompat.h
    core/MdIcon.h
    core/MdResources.h
    core/MdTheme.h
    core/MdFont.h
    core/MdDesign.h
)

set(QT_MD3_STYLE_HEADERS
    styles/MdStyleBase.h
    styles/MdBadgeStyle.h
    styles/MdButtonStyle.h
    styles/MdBottomSheetStyle.h
    styles/MdCarouselStyle.h
    styles/MdSideSheetStyle.h
    styles/MdCardStyle.h
    styles/MdDialogStyle.h
    styles/MdBottomAppBarStyle.h
    styles/MdDockedToolbarStyle.h
    styles/MdFloatingToolbarStyle.h
    styles/MdNavigationBarStyle.h
    styles/MdNavigationBarItemStyle.h
    styles/MdNavigationRailStyle.h
    styles/MdNavigationDrawerItemStyle.h
    styles/MdNavigationDrawerStyle.h
    styles/MdChildBox.h
    styles/MdDividerStyle.h
    styles/MdListStyle.h
    styles/MdListItemStyle.h
    styles/MdTopAppBarStyle.h
    styles/MdButtonGroupStyle.h
    styles/MdIconButtonStyle.h
    styles/MdFabStyle.h
    styles/MdExtendedFabStyle.h
    styles/MdFabMenuStyle.h
    styles/MdProgressIndicatorStyle.h
    styles/MdLoadingIndicatorStyle.h
    styles/MdSnackbarStyle.h
    styles/MdTooltipStyle.h
    styles/MdSegmentedButtonStyle.h
    styles/MdSplitButtonStyle.h
)

set(QT_MD3_WIDGET_HEADERS
    widgets/MdBadge.h
    widgets/MdBadgedBox.h
    widgets/MdButton.h
    widgets/MdBottomSheet.h
    widgets/MdCarousel.h
    widgets/MdBottomSheetHost.h
    widgets/MdSideSheet.h
    widgets/MdSideSheetHost.h
    widgets/MdBottomSheetHost.h
    widgets/MdCard.h
    widgets/MdDialog.h
    widgets/MdDialogHost.h
    widgets/MdBottomAppBar.h
    widgets/MdDockedToolbar.h
    widgets/MdFloatingToolbar.h
    widgets/MdNavigationBar.h
    widgets/MdNavigationBarItem.h
    widgets/MdNavigationRail.h
    widgets/MdNavigationDrawer.h
    widgets/MdNavigationDrawerItem.h
    widgets/MdDivider.h
    widgets/MdList.h
    widgets/MdListItem.h
    widgets/MdTopAppBar.h
    widgets/MdButtonGroup.h
    widgets/MdIconButton.h
    widgets/MdFab.h
    widgets/MdExtendedFab.h
    widgets/MdFabMenu.h
    widgets/MdFabMenuItem.h
    widgets/MdProgressIndicator.h
    widgets/MdLoadingIndicator.h
    widgets/MdSnackbar.h
    widgets/MdSnackbarHost.h
    widgets/MdTooltip.h
    widgets/MdTooltipHost.h
    widgets/MdSegmentedButton.h
    widgets/MdSplitButton.h
)

set(QT_MD3_SOURCES
    core/MdCore.cpp
    core/MdTypes.cpp
    core/MdTokens.cpp
    core/MdColorMath.cpp
    core/MdColorScheme.cpp
    core/MdColorSpec.cpp
    core/MdDynamicColor.cpp
    core/MdTemperatureCache.cpp
    core/MdShape.cpp
    core/MdMotion.cpp
    core/MdTypeScale.cpp
    core/MdElevation.cpp
    core/MdStateLayer.cpp
    core/MdRipple.cpp
    core/MdFocusRing.cpp
    core/MdBadgeTokens.cpp
    core/MdButtonTokens.cpp
    core/MdBottomSheetTokens.cpp
    core/MdCarouselTokens.cpp
    core/MdSideSheetTokens.cpp
    core/MdCardTokens.cpp
    core/MdDialogTokens.cpp
    core/MdAppBarScrollBehavior.cpp
    core/MdAppBarTokens.cpp
    core/MdDockedToolbarTokens.cpp
    core/MdFloatingToolbarTokens.cpp
    core/MdNavigationBarTokens.cpp
    core/MdNavigationRailTokens.cpp
    core/MdNavigationDrawerTokens.cpp
    core/MdDividerTokens.cpp
    core/MdListTokens.cpp
    core/MdButtonGroupTokens.cpp
    core/MdIconButtonTokens.cpp
    core/MdFabTokens.cpp
    core/MdExtendedFabTokens.cpp
    core/MdFabMenuTokens.cpp
    core/MdProgressIndicatorTokens.cpp
    core/MdLoadingIndicatorTokens.cpp
    core/MdSnackbarTokens.cpp
    core/MdTooltipTokens.cpp
    core/MdMaterialShapes.cpp
    core/MdSegmentedButtonTokens.cpp
    core/MdSplitButtonTokens.cpp
    core/MdIcon.cpp
    core/MdResources.cpp
    core/MdTheme.cpp
    core/MdFont.cpp
    core/MdDesign.cpp
    styles/MdStyleBase.cpp
    styles/MdBadgeStyle.cpp
    styles/MdButtonStyle.cpp
    styles/MdBottomSheetStyle.cpp
    styles/MdCarouselStyle.cpp
    styles/MdSideSheetStyle.cpp
    styles/MdCardStyle.cpp
    styles/MdDialogStyle.cpp
    styles/MdBottomAppBarStyle.cpp
    styles/MdDockedToolbarStyle.cpp
    styles/MdFloatingToolbarStyle.cpp
    styles/MdNavigationBarStyle.cpp
    styles/MdNavigationBarItemStyle.cpp
    styles/MdNavigationRailStyle.cpp
    styles/MdNavigationDrawerItemStyle.cpp
    styles/MdNavigationDrawerStyle.cpp
    styles/MdChildBox.cpp
    styles/MdDividerStyle.cpp
    styles/MdListStyle.cpp
    styles/MdListItemStyle.cpp
    styles/MdTopAppBarStyle.cpp
    styles/MdButtonGroupStyle.cpp
    styles/MdIconButtonStyle.cpp
    styles/MdFabStyle.cpp
    styles/MdExtendedFabStyle.cpp
    styles/MdFabMenuStyle.cpp
    styles/MdProgressIndicatorStyle.cpp
    styles/MdLoadingIndicatorStyle.cpp
    styles/MdSnackbarStyle.cpp
    styles/MdTooltipStyle.cpp
    styles/MdSegmentedButtonStyle.cpp
    styles/MdSplitButtonStyle.cpp
    widgets/MdBadge.cpp
    widgets/MdBadgedBox.cpp
    widgets/MdButton.cpp
    widgets/MdBottomSheet.cpp
    widgets/MdCarousel.cpp
    widgets/MdBottomSheetHost.cpp
    widgets/MdSideSheet.cpp
    widgets/MdSideSheetHost.cpp
    widgets/MdBottomSheetHost.cpp
    widgets/MdCard.cpp
    widgets/MdDialog.cpp
    widgets/MdDialogHost.cpp
    widgets/MdBottomAppBar.cpp
    widgets/MdDockedToolbar.cpp
    widgets/MdFloatingToolbar.cpp
    widgets/MdNavigationBar.cpp
    widgets/MdNavigationBarItem.cpp
    widgets/MdNavigationRail.cpp
    widgets/MdNavigationDrawer.cpp
    widgets/MdNavigationDrawerItem.cpp
    widgets/MdDivider.cpp
    widgets/MdList.cpp
    widgets/MdListItem.cpp
    widgets/MdTopAppBar.cpp
    widgets/MdButtonGroup.cpp
    widgets/MdIconButton.cpp
    widgets/MdFab.cpp
    widgets/MdExtendedFab.cpp
    widgets/MdFabMenu.cpp
    widgets/MdFabMenuItem.cpp
    widgets/MdProgressIndicator.cpp
    widgets/MdLoadingIndicator.cpp
    widgets/MdSnackbar.cpp
    widgets/MdSnackbarHost.cpp
    widgets/MdTooltip.cpp
    widgets/MdTooltipHost.cpp
    widgets/MdSegmentedButton.cpp
    widgets/MdSplitButton.cpp
)

set(QT_MD3_RESOURCES
    ../resources/qt-md3.qrc
)

add_library(qt-md3
    ${QT_MD3_SOURCES}
    ${QT_MD3_CORE_HEADERS}
    ${QT_MD3_STYLE_HEADERS}
    ${QT_MD3_WIDGET_HEADERS}
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
        Qt${QT_VERSION_MAJOR}::Gui
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

# Installed headers: public headers keep their layer subdirectory, plus the
# generated version header.
install(FILES ${QT_MD3_CORE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt-md3/core
)
install(FILES ${QT_MD3_STYLE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt-md3/styles
)
if(QT_MD3_WIDGET_HEADERS)
    install(FILES ${QT_MD3_WIDGET_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt-md3/widgets
    )
endif()
install(FILES ${QT_MD3_VERSION_HEADER}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qt-md3/core
)
