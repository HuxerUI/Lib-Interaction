if(NOT HUXERUI_LIBRARY_GRAPH_ONLY AND NOT TARGET interaction_preview_views)
    add_library(interaction_preview_views STATIC
        "${CMAKE_CURRENT_LIST_DIR}/src/app.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/demos/bloom_paw.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/demos/drag.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/demos/gesture.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/demos/layout.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/demos/paint.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/demos/press.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/demos/reorder.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/src/demos/status.cpp")
    set_target_properties(interaction_preview_views PROPERTIES CXX_EXTENSIONS OFF POSITION_INDEPENDENT_CODE ON)
    target_compile_features(interaction_preview_views PUBLIC cxx_std_20)
    target_include_directories(interaction_preview_views PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src")
    target_link_libraries(interaction_preview_views PUBLIC HuxerUI::Interaction)
    huxerui_enable_codegen(interaction_preview_views)
    target_compile_options(interaction_preview_views PRIVATE
        "$<$<CXX_COMPILER_ID:MSVC>:/utf-8;/W4;/permissive->"
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic>")
endif()
