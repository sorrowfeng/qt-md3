# TestMd3CoveragePolicy
#
# Gate: docs/md3-coverage.md is the single source of truth for the Stage 1 ->
# Stage 2 switch. This script validates that matrix and it enforces the rule
# that Stage 2 (Qt extension) components must not exist until every Stage 1 MD3
# family is green across all nine columns.
#
# Run as:
#   cmake -DSRC_ROOT=<repo root> -P tests/TestMd3CoveragePolicy.cmake

if(NOT DEFINED SRC_ROOT)
    message(FATAL_ERROR "TestMd3CoveragePolicy: SRC_ROOT is not defined")
endif()

set(_matrix "${SRC_ROOT}/docs/md3-coverage.md")
if(NOT EXISTS "${_matrix}")
    message(FATAL_ERROR "TestMd3CoveragePolicy: coverage matrix not found at ${_matrix}")
endif()

file(READ "${_matrix}" _content)

# --- 0. The matrix must be parseable ----------------------------------------
string(FIND "${_content}" "✅" _has_done)
string(FIND "${_content}" "⬜" _has_todo)
if(_has_done EQUAL -1 AND _has_todo EQUAL -1)
    message(FATAL_ERROR
        "TestMd3CoveragePolicy: no status glyph (✅ / ⬜) found in docs/md3-coverage.md. "
        "The matrix must use ✅ for done and ⬜ for not started."
    )
endif()

# --- 1. The 36 Stage 1 families must each appear exactly once ---------------
set(_families
    "Buttons" "Button groups" "Icon buttons" "FABs" "Extended FABs"
    "FAB menu" "Split buttons" "Segmented buttons" "Badges"
    "Progress indicators" "Loading indicator" "Snackbar" "Tooltips"
    "Cards" "Dialogs" "Bottom sheets" "Side sheets" "Carousel" "Divider" "Lists"
    "App bars" "Toolbars" "Navigation bar" "Navigation rail" "Navigation drawer" "Tabs"
    "Checkbox" "Chips" "Date pickers" "Menus" "Radio button" "Sliders" "Switch"
    "Time pickers" "Text fields" "Search"
)
list(LENGTH _families _expected_family_count)
if(NOT _expected_family_count EQUAL 36)
    message(FATAL_ERROR
        "TestMd3CoveragePolicy (internal): expected 36 Stage 1 families, got ${_expected_family_count}")
endif()

# Each family row is | family | class | <nine status cells> | => 11 cells, 12 pipes.
set(_expected_pipes 12)

set(_problems "")
foreach(_family IN LISTS _families)
    # The family cell may carry the optional M3 Expressive "★" marker.
    string(REGEX MATCHALL "\\| (★ )?${_family} \\|" _matches "${_content}")
    list(LENGTH _matches _n)
    if(NOT _n EQUAL 1)
        list(APPEND _problems "family '${_family}' appears ${_n} time(s) in a matrix row (expected 1)")
        continue()
    endif()

    string(REGEX MATCH "\\| (★ )?${_family} \\|[^\n]*" _row "${_content}")
    string(REGEX MATCHALL "\\|" _row_pipes "${_row}")
    list(LENGTH _row_pipes _pipe_count)
    if(NOT _pipe_count EQUAL _expected_pipes)
        math(EXPR _cells "${_pipe_count} - 3")
        list(APPEND _problems
            "family '${_family}' row has ${_cells} status cell(s) (expected 9)")
    endif()
endforeach()
if(_problems)
    string(REPLACE ";" "\n  - " _report "${_problems}")
    message(FATAL_ERROR
        "TestMd3CoveragePolicy: coverage matrix row problems:\n  - ${_report}")
endif()

# --- 2. Stage 1 status ------------------------------------------------------
string(REGEX MATCHALL "⬜" _todo_matches "${_content}")
list(LENGTH _todo_matches _todo_count)

if(_todo_count EQUAL 0)
    set(_stage1_green TRUE)
else()
    set(_stage1_green FALSE)
endif()

# --- 3. Stage 2 components must not exist before Stage 1 is green ------------
set(_stage2_headers
    MdWindow.h MdFileDialog.h MdMessageBox.h MdInputDialog.h
    MdMenuBar.h MdStatusBar.h
    MdScrollBar.h MdScrollArea.h MdSplitter.h MdStackedWidget.h
    MdDockWidget.h MdDockManager.h
    MdTable.h MdTree.h MdTreeSelect.h MdAvatar.h MdEmptyState.h MdResult.h
    MdSkeleton.h MdPagination.h MdBreadcrumb.h MdSteps.h MdStepper.h MdTimeline.h
    MdDescriptions.h MdStatistic.h MdImage.h MdQRCode.h MdWatermark.h MdLog.h
    MdSelect.h MdInputNumber.h MdCascader.h MdTransfer.h MdUpload.h MdColorPicker.h
    MdRate.h MdMentions.h MdAutoComplete.h MdPlainTextEdit.h
    MdFlex.h MdGrid.h MdSpace.h MdScaffold.h MdMasonry.h
    MdDrawer.h MdPopover.h MdPopconfirm.h MdNotification.h MdCollapse.h
    MdTour.h MdCoachMark.h MdRibbon.h MdNav.h MdNavItem.h MdAffix.h MdAnchor.h MdBackTop.h
)

# Explicit whitelist for internal / example-level files that are allowed during
# Stage 1. Keep this empty unless there is a reviewed reason; never add a Stage 2
# public component here without updating the docs.
set(_stage2_whitelist
)

file(GLOB _widget_headers "${SRC_ROOT}/src/widgets/*.h")
set(_present_stage2 "")
set(_conflicts "")
foreach(_hdr IN LISTS _widget_headers)
    get_filename_component(_name "${_hdr}" NAME)
    list(FIND _stage2_headers "${_name}" _idx)
    if(NOT _idx EQUAL -1)
        list(APPEND _present_stage2 "${_name}")
        list(FIND _stage2_whitelist "${_name}" _wl)
        if(_wl EQUAL -1)
            list(APPEND _conflicts "${_name}")
        endif()
    endif()
endforeach()

if(NOT _stage1_green AND _conflicts)
    list(REMOVE_DUPLICATES _conflicts)
    string(REPLACE ";" "\n  - " _report "${_conflicts}")
    message(FATAL_ERROR
        "TestMd3CoveragePolicy: Stage 1 is not complete (${_todo_count} unchecked column(s) in "
        "docs/md3-coverage.md), but Stage 2 public component(s) already exist in src/widgets/:\n"
        "  - ${_report}\n"
        "Finish all 36 MD3 families before adding Stage 2 (Qt extension) components."
    )
endif()

# --- 4. Summary -------------------------------------------------------------
if(_stage1_green)
    list(LENGTH _present_stage2 _stage2_count)
    message(STATUS
        "TestMd3CoveragePolicy: Stage 1 is GREEN. 36/36 families complete. "
        "Stage 2 progress: ${_stage2_count} component header(s) present.")
else()
    message(STATUS
        "TestMd3CoveragePolicy: Stage 1 IN PROGRESS (${_todo_count} unchecked column(s)); "
        "Stage 2 gate is active. 36/36 families present in the matrix.")
endif()
