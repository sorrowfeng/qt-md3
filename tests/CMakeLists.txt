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

# MdProgressIndicator, the second Communication family: the merged export's
# rows (base + linear + circular, the deprecated thick metrics and the
# deprecated-set four-color roles), the MDC-heritage animation keyframes as
# pure functions, the determinate transitions, the buffer dots contract and
# the non-interactivity contract.
add_executable(TestMd3ProgressIndicator
    TestMd3ProgressIndicator.cpp
)

target_link_libraries(TestMd3ProgressIndicator
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3ProgressIndicator COMMAND TestMd3ProgressIndicator)
set_tests_properties(TestMd3ProgressIndicator PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3ProgressIndicator)

# MdLoadingIndicator, the third Communication family: the Expressive
# token-only export (34.0.21) plus the Compose-port behaviour — the shape
# engine (graphics-shapes port), the radial morph, the spring/morph-grid
# pure functions, the determinate sweep and the non-interactivity contract.
add_executable(TestMd3LoadingIndicator
    TestMd3LoadingIndicator.cpp
)

target_link_libraries(TestMd3LoadingIndicator
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3LoadingIndicator COMMAND TestMd3LoadingIndicator)
set_tests_properties(TestMd3LoadingIndicator PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3LoadingIndicator)

# MdSnackbar, the fourth Communication family: the 34.0.21 export (one set,
# no variants, both elements' state rows) plus the Compose-port behaviour —
# the one-row/new-line measure policies, the keyboard activation contract and
# the host's queue/duration/transition rules.
add_executable(TestMd3Snackbar
    TestMd3Snackbar.cpp
)

target_link_libraries(TestMd3Snackbar
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Snackbar COMMAND TestMd3Snackbar)
set_tests_properties(TestMd3Snackbar PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Snackbar)

# MdTooltip, the Communication family's closing pair: both exports (plain +
# rich), the rich action's state rows, the Compose-port layout geometry and
# popup positioning, the caret, the pointer/keyboard contract and the host's
# trigger / mutex / auto-hide rules.
add_executable(TestMd3Tooltip
    TestMd3Tooltip.cpp
)

target_link_libraries(TestMd3Tooltip
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Tooltip COMMAND TestMd3Tooltip)
set_tests_properties(TestMd3Tooltip PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Tooltip)

# MdCard, the Containment family's opening surface: all three variants' state
# row tables, the Compose disabled compositing constants, the override
# parsing, the layout geometry, the state priority, the pointer/keyboard
# contract and the elevation animation/snap.
add_executable(TestMd3Card
    TestMd3Card.cpp
)

target_link_libraries(TestMd3Card
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Card COMMAND TestMd3Card)
set_tests_properties(TestMd3Card PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Card)

add_executable(TestMd3Dialog
    TestMd3Dialog.cpp
)

target_link_libraries(TestMd3Dialog
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Dialog COMMAND TestMd3Dialog)
set_tests_properties(TestMd3Dialog PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Dialog)

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
# MdBottomSheet + MdBottomSheetHost, the Containment family's closing pair:
# the sheet surface (standard + modal kinds, the anchor math and the drag
# settle) and the modal presentation (scrim, Escape / scrim click).
add_executable(TestMd3BottomSheet
    TestMd3BottomSheet.cpp
)

target_link_libraries(TestMd3BottomSheet
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3BottomSheet COMMAND TestMd3BottomSheet)
set_tests_properties(TestMd3BottomSheet PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3BottomSheet)

# MdSideSheet + MdSideSheetHost, the Containment family's edge-docked pair:
# the sheet surface (left/right delegates, the midpoint drag settle) and the
# modal presentation (scrim, Escape / scrim click).
add_executable(TestMd3SideSheet
    TestMd3SideSheet.cpp
)

target_link_libraries(TestMd3SideSheet
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3SideSheet COMMAND TestMd3SideSheet)
set_tests_properties(TestMd3SideSheet PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3SideSheet)

# MdCarousel, the Containment family's scrolling item row: the keyline math
# (arrangement fit, slot interpolation) and the drag/snap widget.
add_executable(TestMd3Carousel
    TestMd3Carousel.cpp
)

target_link_libraries(TestMd3Carousel
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Carousel COMMAND TestMd3Carousel)
set_tests_properties(TestMd3Carousel PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Carousel)

# MdDivider, the Containment family's grouping hairline: the two-row export
# (thickness 1px / outline-variant — the smallest token table in the library),
# the non-interactivity contract, the four inset modes (with the RTL inline
# mirroring), the vertical form and the device-pixel-aligned rendering.
add_executable(TestMd3Divider
    TestMd3Divider.cpp
)

target_link_libraries(TestMd3Divider
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Divider COMMAND TestMd3Divider)
set_tests_properties(TestMd3Divider PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Divider)

# MdList + MdListItem, the Containment family's vertical index: the list
# export (metrics, shapes, the two colour families, the disabled opacities),
# the Compose line-count rule, the shape-morph ladder, the spec's alignment
# breakpoint, the segmented outer corners, the list's selection modes and the
# roving-tabindex keyboard navigation.
add_executable(TestMd3List
    TestMd3List.cpp
)

target_link_libraries(TestMd3List
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3List COMMAND TestMd3List)
set_tests_properties(TestMd3List PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3List)

# TestMd3AppBar — the App bars family: MdTopAppBar + MdBottomAppBar. The token
# tables for the common set, the five size sets and the bottom app bar; the
# derived 16 px edge distance; the two-row row split and the title push; the
# colour transition table; the three scroll behaviours' consumption arithmetic;
# and the bottom app bar's FAB strip.
add_executable(TestMd3AppBar
    TestMd3AppBar.cpp
)

target_link_libraries(TestMd3AppBar
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3AppBar COMMAND TestMd3AppBar)
set_tests_properties(TestMd3AppBar PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3AppBar)

# TestMd3Toolbar — the Toolbars family: MdDockedToolbar + MdFloatingToolbar. The
# five token sets, the docked row's centred 32 px spacing and whole-height
# collapse, the floating pill's progress-driven length, its leading/trailing
# visibility rule, the action button's strip and 56/80 size sets, and the
# container-placement rule both share.
add_executable(TestMd3Toolbar
    TestMd3Toolbar.cpp
)

target_link_libraries(TestMd3Toolbar
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Toolbar COMMAND TestMd3Toolbar)
set_tests_properties(TestMd3Toolbar PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Toolbar)

# TestMd3Tabs — the Tabs family: MdTabs (both published families, fixed and
# scrollable) and its MdTab children. The families' indicator heights and
# shapes, the content-width rule, the centring divergence, the spatial spring,
# the scrollable edge padding / minimum width / auto-centre scroll, the
# inactive pressed layer's `primary` special case and the keyboard walk.
add_executable(TestMd3Tabs
    TestMd3Tabs.cpp
)

target_link_libraries(TestMd3Tabs
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Tabs COMMAND TestMd3Tabs)
set_tests_properties(TestMd3Tabs PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Tabs)

# TestMd3CheckBox — the Checkbox: MdCheckBox over QCheckBox. The two
# state-layer special cases (unselected pressed primary, selected pressed
# on-surface), the check path's Compose fractions, the snap-delay undo, the
# indeterminate gravitation, the disabled snap, the error variant and the
# drawBox/drawCheck port.
add_executable(TestMd3CheckBox
    TestMd3CheckBox.cpp
)

target_link_libraries(TestMd3CheckBox
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3CheckBox COMMAND TestMd3CheckBox)
set_tests_properties(TestMd3CheckBox PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3CheckBox)


# TestMd3RadioButton — the Radio button: MdRadioButton over QRadioButton. The
# pressed state-layer special cases (unselected pressed primary, selected
# pressed on-surface), the dot's fast-spatial scale, the colour fade, the
# disabled snap, the native group exclusivity and the circular focus ring.
add_executable(TestMd3RadioButton
    TestMd3RadioButton.cpp
)

target_link_libraries(TestMd3RadioButton
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3RadioButton COMMAND TestMd3RadioButton)
set_tests_properties(TestMd3RadioButton PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3RadioButton)


# TestMd3Switch — the Switch: MdSwitch over QCheckBox. The thumb's fast-spatial
# travel+resize with the pressed SnapSpec, the instant colour resolution, the
# disabled selected handle at full strength, the thumb-riding ripple and the
# whole-rect touch target.
add_executable(TestMd3Switch
    TestMd3Switch.cpp
)

target_link_libraries(TestMd3Switch
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Switch COMMAND TestMd3Switch)
set_tests_properties(TestMd3Switch PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Switch)

# TestMd3Chip — the Chips family: MdChip over the four published exports
# (assist / filter / input / suggestion), the flat and elevated kinds, the
# pressed state-layer swap, the two-spacing arrangement and the weighted
# label's trailing slot.
add_executable(TestMd3Chip
    TestMd3Chip.cpp
)

target_link_libraries(TestMd3Chip
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Chip COMMAND TestMd3Chip)
set_tests_properties(TestMd3Chip PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Chip)

# TestMd3Menu — the Menus family: MdMenu + MdMenuItem. The classic item's
# geometry (48 px rows, 112/280 width clamps), the insertion-ordered layout
# with the 8 px vertical padding and the divider's 12/2 insets, the popup's
# scale+alpha open with children painted through the transform, the inward
# keyboard-only focus ring, the state layer reading the unselected side for
# both selections and the arrow-key walk.

# TestMd3Slider — the Sliders family: MdSlider. The export's metric rows (the
# 4 px pill handle with its per-state widths 4/4/2/2/4, the five Expressive
# size rows), the colour tables' disabled opacities, the keyboard walk and the
# range form's two handles.

# TestMd3TimePicker — the Time pickers family: MdTimePicker. The export's
# metric rows (the 256 px dial, the 48 px selector handle on an 8 px centre
# and 2 px track, the 96x80 / 114x80 time selectors, the 216x38 period
# selector), the two selector colour tables, the dial's snapping and the
# face / period switching.

# TestMd3DatePicker — the Date pickers family: MdDatePicker. The export's
# metric rows (the 360x524 container, the 120 px header, the 40 px date
# cells, the 72x36 year cells), the date / year colour tables, the grid
# maths and the range form's two endpoints.

# TestMd3TextField — the Text fields family: MdTextField. The export's
# metric rows (the 56 px container, the 24 px icons, the indicator heights
# 1/1/2/1), the filled / outlined variants, the colour tables' state rows
# and the error overlay, the label's float and the disabled opacities.

# TestMd3SearchBar — the Search family: MdSearchBar. The export's metric
# rows (the 56 px container, the 24 px icons at the 16 px side spaces, the
# 30 px avatar / 48 px target), the three surfaces' shapes, the state-layer
# rows and the search-requested signal.

# TestMd3TextArea — the multiline text field: MdTextArea. The shared
# text-field token set serving both forms, the rows contract, the floating
# label and the error overlay.

# TestMd3Rtl — the library-wide RTL audit: geometry mirrors (leading /
# trailing swaps, the motion vectors) and text direction
# (MdStyleBase::leadingAlignment).
add_executable(TestMd3Rtl
    TestMd3Rtl.cpp
)

target_link_libraries(TestMd3Rtl
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Rtl COMMAND TestMd3Rtl)
set_tests_properties(TestMd3Rtl PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Rtl)
add_executable(TestMd3TextArea
    TestMd3TextArea.cpp
)

target_link_libraries(TestMd3TextArea
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3TextArea COMMAND TestMd3TextArea)
set_tests_properties(TestMd3TextArea PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3TextArea)
add_executable(TestMd3SearchBar
    TestMd3SearchBar.cpp
)

target_link_libraries(TestMd3SearchBar
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3SearchBar COMMAND TestMd3SearchBar)
set_tests_properties(TestMd3SearchBar PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3SearchBar)
add_executable(TestMd3TextField
    TestMd3TextField.cpp
)

target_link_libraries(TestMd3TextField
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3TextField COMMAND TestMd3TextField)
set_tests_properties(TestMd3TextField PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3TextField)
add_executable(TestMd3DatePicker
    TestMd3DatePicker.cpp
)

target_link_libraries(TestMd3DatePicker
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3DatePicker COMMAND TestMd3DatePicker)
set_tests_properties(TestMd3DatePicker PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3DatePicker)
add_executable(TestMd3TimePicker
    TestMd3TimePicker.cpp
)

target_link_libraries(TestMd3TimePicker
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3TimePicker COMMAND TestMd3TimePicker)
set_tests_properties(TestMd3TimePicker PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3TimePicker)
add_executable(TestMd3Slider
    TestMd3Slider.cpp
)

target_link_libraries(TestMd3Slider
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Slider COMMAND TestMd3Slider)
set_tests_properties(TestMd3Slider PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Slider)
add_executable(TestMd3Menu
    TestMd3Menu.cpp
)

target_link_libraries(TestMd3Menu
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Menu COMMAND TestMd3Menu)
set_tests_properties(TestMd3Menu PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Menu)

# TestMd3Navigation — the Navigation family: MdNavigationBar (both published
# families — the baseline `navigation-bar` one and the flexible `nav-bar` one,
# which coexist in a single export version) and its MdNavigationBarItem
# children. The two-family separation, the behaviour-won item gap, the Centered
# band fractions, the fill-the-slot placement rule, the width-only pill
# animation and the Start position's content-wrapping pill.
add_executable(TestMd3Navigation
    TestMd3Navigation.cpp
)

target_link_libraries(TestMd3Navigation
    PRIVATE
        qt-md3
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestMd3Navigation COMMAND TestMd3Navigation)
set_tests_properties(TestMd3Navigation PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

qt_md3_deploy_runtime(TestMd3Navigation)
