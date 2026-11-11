cmake_minimum_required(VERSION 3.20)
if(CMAKE_HOST_APPLE AND NOT CMAKE_SYSTEM_NAME AND NOT CMAKE_TOOLCHAIN_FILE
        AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET AND "$ENV{MACOSX_DEPLOYMENT_TARGET}" STREQUAL "")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS deployment target")
endif()
project(lib_interaction VERSION 0.1.0 LANGUAGES NONE)

option(HUXERUI_INTERACTION_BUILD_TESTS "Build interaction widget tests" OFF)
option(HUXERUI_INTERACTION_BUILD_EXAMPLES "Build the interaction preview demo" OFF)
include("${CMAKE_CURRENT_SOURCE_DIR}/HuxerUIProject.cmake")
if(HUXERUI_PROJECT_PLAN_ONLY)
    return()
endif()

huxerui_add_library(huxerui_interaction SOURCES
    src/contextmenu.cpp
    src/dragspring.cpp
    src/filedropzone.cpp
    src/fireworks.cpp
    src/highlight.cpp
    src/inertialdrag.cpp
    src/hover.cpp
    src/longpress.cpp
    src/multitap.cpp
    src/panzoom.cpp
    src/pinch.cpp
    src/pop.cpp
    src/pressable.cpp
    src/pulse.cpp
    src/reorder.cpp
    src/reordergrid.cpp
    src/rollingnumber.cpp
    src/scrolledgefade.cpp
    src/scrollprogress.cpp
    src/ripple.cpp
    src/shake.cpp
    src/shimmer.cpp
    src/keyboardshortcut.cpp
    src/multiclick.cpp
    src/secondaryclick.cpp
    src/semanticactions.cpp
    src/spotlight.cpp
    src/dismissible.cpp
    src/pressdraghandle.cpp
    src/hovercard.cpp
    src/resizehandle.cpp
    src/revealontap.cpp
    src/swipe.cpp)
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
