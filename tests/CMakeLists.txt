add_executable(interaction_widget_tests
    widget/main.cpp
    widget/validation_test.cpp
    widget/press_test.cpp
    widget/hover_overlay_test.cpp
    widget/gesture_test.cpp
    widget/reorder_dragdrop_test.cpp
    widget/scroll_a11y_visual_test.cpp)
target_link_libraries(interaction_widget_tests PRIVATE HuxerUI::Interaction)

add_executable(interaction_api_tests api/api_surface_test.cpp)
target_link_libraries(interaction_api_tests PRIVATE HuxerUI::Interaction)
add_test(NAME interaction.api COMMAND interaction_api_tests)

if(TARGET HuxerUI::testing)
    include("${CMAKE_CURRENT_SOURCE_DIR}/../examples/preview/PreviewViews.cmake")
    target_compile_definitions(interaction_widget_tests PRIVATE INTERACTION_HAS_UI_TESTING=1)
    target_link_libraries(interaction_widget_tests PRIVATE HuxerUI::testing)

    add_executable(interaction_preview_tests preview/preview_test.cpp)
    target_link_libraries(interaction_preview_tests PRIVATE interaction_preview_views HuxerUI::testing)
    get_target_property(_interaction_test_resources HuxerUI::huxerui HUXERUI_RESOURCE_PACKAGE)
    target_compile_definitions(interaction_preview_tests
        PRIVATE INTERACTION_TEST_RESOURCES="${_interaction_test_resources}")
    add_test(NAME interaction.preview COMMAND interaction_preview_tests)

    add_executable(interaction_advanced_tests advanced/advanced_test.cpp)
    target_link_libraries(interaction_advanced_tests PRIVATE HuxerUI::Interaction HuxerUI::testing)
    add_test(NAME interaction.advanced COMMAND interaction_advanced_tests)
endif()

add_test(NAME interaction.widget COMMAND interaction_widget_tests)

foreach(_target IN ITEMS interaction_widget_tests interaction_api_tests interaction_preview_tests interaction_advanced_tests)
    if(TARGET ${_target})
        set_target_properties(${_target} PROPERTIES CXX_EXTENSIONS OFF)
        target_compile_features(${_target} PRIVATE cxx_std_20)
        target_compile_options(${_target} PRIVATE
            "$<$<CXX_COMPILER_ID:MSVC>:/utf-8;/W4;/permissive->"
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall;-Wextra;-Wpedantic;-Wno-missing-field-initializers>")
    endif()
endforeach()
