if(HUXERUI_PROJECT_PLAN_OUTPUT)
    get_filename_component(_preview_plan_dir "${HUXERUI_PROJECT_PLAN_OUTPUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${_preview_plan_dir}")
    file(WRITE "${HUXERUI_PROJECT_PLAN_OUTPUT}" [=[
{
  "schema": 1,
  "kind": "app",
  "name": "Lib-Interaction Preview",
  "id": "org.huxerui.lib.interaction.preview",
  "target": "example_interaction"
}
]=])
endif()
if(HUXERUI_PROJECT_PLAN_ONLY)
    return()
endif()
if(NOT HUXERUI_LIBRARY_GRAPH_ONLY)
    enable_language(CXX)
endif()
