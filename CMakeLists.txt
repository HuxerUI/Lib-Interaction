cmake_minimum_required(VERSION 3.20)
if(CMAKE_HOST_APPLE AND NOT CMAKE_SYSTEM_NAME AND NOT CMAKE_TOOLCHAIN_FILE
        AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET AND "$ENV{MACOSX_DEPLOYMENT_TARGET}" STREQUAL "")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS deployment target")
endif()
project(lib_interaction VERSION 0.2.0 LANGUAGES NONE)

option(HUXERUI_INTERACTION_BUILD_TESTS "Build interaction widget tests" OFF)
option(HUXERUI_INTERACTION_BUILD_EXAMPLES "Build the interaction preview demo" OFF)
include("${CMAKE_CURRENT_SOURCE_DIR}/HuxerUIProject.cmake")
if(HUXERUI_PROJECT_PLAN_ONLY)
    return()
endif()

huxerui_add_library(huxerui_interaction SOURCES
    # internal shared infrastructure
    src/internal/reorder_core.cpp
    # press
    src/press/contextmenu.cpp
    src/press/copyable.cpp
    src/press/longpress.cpp
    src/press/multiclick.cpp
    src/press/pressable.cpp
    src/press/ripple.cpp
    src/press/surface.cpp
    # hover
    src/hover/highlight.cpp
    src/hover/hoverlift.cpp
    src/hover/hovershadow.cpp
    src/hover/magnetic.cpp
    src/hover/spotlight.cpp
    # gesture
    src/gesture/dismissible.cpp
    src/gesture/dragspring.cpp
    src/gesture/inertialdrag.cpp
    src/gesture/marqueeselect.cpp
    src/gesture/pressdraghandle.cpp
    src/gesture/resizehandle.cpp
    src/gesture/snapdrag.cpp
    src/gesture/swipe.cpp
    src/gesture/transform.cpp
    # reorder
    src/reorder/reorder.cpp
    src/reorder/reordergrid.cpp
    src/reorder/reorderhandle.cpp
    # drag and drop
    src/dragdrop/filedropzone.cpp
    # overlay
    src/overlay/hovercard.cpp
    src/overlay/popover.cpp
    # scroll
    src/scroll/nestedscroll.cpp
    src/scroll/scrolledgefade.cpp
    src/scroll/scrollprogress.cpp
    src/scroll/scrollspy.cpp
    # accessibility
    src/accessibility/backhandler.cpp
    src/accessibility/keyboardshortcut.cpp
    src/accessibility/rovingfocus.cpp
    src/accessibility/semanticactions.cpp
    src/accessibility/shortcutmap.cpp
    # visual
    src/visual/appear.cpp
    src/visual/expandable.cpp
    src/visual/fireworks.cpp
    src/visual/pop.cpp
    src/visual/pulse.cpp
    src/visual/revealontap.cpp
    src/visual/rollingnumber.cpp
    src/visual/shake.cpp
    src/visual/sharedmorph.cpp
    src/visual/shimmer.cpp)
add_library(HuxerUI::Interaction ALIAS huxerui_interaction)
if(HUXERUI_LIBRARY_GRAPH_ONLY)
    return()
endif()

set_target_properties(huxerui_interaction PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
target_compile_features(huxerui_interaction PUBLIC cxx_std_20)
target_include_directories(huxerui_interaction PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:include>")
target_link_libraries(huxerui_interaction PUBLIC HuxerUI::huxerui)
target_compile_options(huxerui_interaction PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/utf-8;/W4;/permissive->"
    "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic>")
if(HUXERUI_INTERACTION_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if(HUXERUI_INTERACTION_BUILD_EXAMPLES)
    add_subdirectory(examples/preview)
endif()
