add_executable(interaction_widget_tests
    widget/main.cpp
    widget/validation_test.cpp
    widget/press_test.cpp
    widget/gesture_test.cpp)
target_link_libraries(interaction_widget_tests PRIVATE HuxerUI::Interaction)
add_executable(interaction_api_tests api/api_surface_test.cpp)
target_link_libraries(interaction_api_tests PRIVATE HuxerUI::Interaction)
add_test(NAME interaction.api COMMAND interaction_api_tests)
if(TARGET HuxerUI::testing)
    target_compile_definitions(interaction_widget_tests PRIVATE INTERACTION_HAS_UI_TESTING=1)
    target_link_libraries(interaction_widget_tests PRIVATE HuxerUI::testing)
    add_executable(interaction_preview_tests
        preview/preview_test.cpp
        ../examples/preview/src/app.cpp
        ../examples/preview/src/demos/bloom_paw.cpp
        ../examples/preview/src/demos/drag.cpp
        ../examples/preview/src/demos/gesture.cpp
        ../examples/preview/src/demos/layout.cpp
        ../examples/preview/src/demos/paint.cpp
        ../examples/preview/src/demos/press.cpp
        ../examples/preview/src/demos/reorder.cpp
        ../examples/preview/src/demos/status.cpp)
    target_link_libraries(interaction_preview_tests PRIVATE HuxerUI::Interaction HuxerUI::testing)
    huxerui_enable_codegen(interaction_preview_tests)
    get_target_property(_interaction_test_resources HuxerUI::huxerui HUXERUI_RESOURCE_PACKAGE)
    target_compile_definitions(interaction_preview_tests PRIVATE INTERACTION_TEST_RESOURCES="${_interaction_test_resources}")
    add_test(NAME interaction.preview COMMAND interaction_preview_tests)
    add_executable(interaction_advanced_tests advanced/advanced_test.cpp)
    target_link_libraries(interaction_advanced_tests PRIVATE HuxerUI::Interaction HuxerUI::testing)
    add_test(NAME interaction.advanced COMMAND interaction_advanced_tests)
endif()
add_test(NAME interaction.widget COMMAND interaction_widget_tests)
foreach(_target IN ITEMS interaction_widget_tests interaction_api_tests interaction_preview_tests interaction_advanced_tests)
    if(TARGET ${_target})
        target_compile_options(${_target} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8;/W4;/permissive->")
    endif()
endforeach()
