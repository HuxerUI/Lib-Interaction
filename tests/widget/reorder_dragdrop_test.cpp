#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

#ifdef INTERACTION_HAS_UI_TESTING
#include <huxerui/testing/ui_test.h>
using namespace huxerui::testing;

using namespace huxerui;
using namespace huxerui::interaction;

namespace {

View ReorderAndDragDropGallery() {
  auto v_order = UseState(std::vector<int>{1, 2, 3});
  auto v_event = UseState(std::string("none"));
  auto h_order = UseState(std::vector<int>{1, 2, 3});
  auto h_event = UseState(std::string("none"));
  auto g_order = UseState(std::vector<int>{1, 2, 3, 4});
  auto g_event = UseState(std::string("none"));
  auto drops = UseState(0);
  auto payload = UseState(0);

  std::vector<View> v_slots;
  for(int v : v_order.Get()) {
    v_slots.push_back(Text("slot " + std::to_string(v))
                          .With(Frame{.width = 160.0F, .height = 40.0F}, Background{Color::Rgb(80, 110, 160)})
                          .Key("slot-" + std::to_string(v)));
  }

  std::vector<View> h_slots;
  for(int v : h_order.Get()) {
    h_slots.push_back(Text("h " + std::to_string(v))
                          .With(Frame{.width = 72.0F, .height = 48.0F}, Background{Color::Rgb(80, 110, 160)})
                          .Key("h-" + std::to_string(v)));
  }

  std::vector<View> g_cells;
  for(int v : g_order.Get()) {
    g_cells.push_back(Text("g " + std::to_string(v))
                          .With(Background{Color::Rgb(80, 110, 160)})
                          .Key("g-" + std::to_string(v)));
  }

  auto rotate_state = [](State<std::vector<int>> state, State<std::string> label) {
    return [state, label](std::size_t from, std::size_t to) {
      auto next = state.Get();
      if(from >= next.size() || to >= next.size()) return;
      const int value = next[from];
      next.erase(next.begin() + static_cast<std::ptrdiff_t>(from));
      next.insert(next.begin() + static_cast<std::ptrdiff_t>(to), value);
      state = std::move(next);
      label = std::to_string(from) + "->" + std::to_string(to);
    };
  };

  return Column {
    Row {
      Reorderable(std::move(v_slots), rotate_state(v_order, v_event)),
      ReorderableGrid(std::move(g_cells), rotate_state(g_order, g_event),
                      {.cell_width = 72.0F, .cell_height = 48.0F, .spacing = 8.0F})
          .With(Frame{.width = 240.0F, .height = 140.0F}),
    }.With(Spacing(16.0F)),
    Reorderable(std::move(h_slots), rotate_state(h_order, h_event), {.axis = Axis::Horizontal}),
    Row {
      Draggable(Text("drag source").With(Frame{.width = 140.0F, .height = 48.0F}, Background{Color::Rgb(70, 120, 160)}),
                42)
          .Key("drag-source"),
      DropZone<int>(
          [](bool hovered) {
            return Text(hovered ? "zone:hover" : "zone:idle")
                .With(Frame{.width = 160.0F, .height = 56.0F}, Background{Color::Rgb(90, 130, 100)});
          },
          [drops, payload](const int& v, const DropEvent&) {
            drops += 1;
            payload = v;
          })
          .Key("drop-zone"),
      FileDropZone(Text("files").With(Frame{.width = 120.0F, .height = 56.0F}), FileDropOptions{.extensions = {"png"}})
          .Key("file-zone"),
    }.With(Spacing(12.0F)),
    Text("v: " + v_event.Get()),
    Text("h: " + h_event.Get()),
    Text("g: " + g_event.Get()),
    Text("drops: " + std::to_string(drops.Get())),
    Text("payload: " + std::to_string(payload.Get())),
  }.With(Padding(16.0F), Spacing(12.0F));
}

} // namespace

void CheckReorderAndDragDropContracts() {
  Application app(ReorderAndDragDropGallery, {.window = {.initial_size = {640, 560}}, .show_debug_overlay = false});
  UiTest ui(app, {.viewport = {640, 560}});

  // Vertical Reorderable.
  ui.Drag(Center(ui.Find(UiSelector::Key("slot-1")).One().bounds),
          Center(ui.Find(UiSelector::Key("slot-3")).One().bounds),
          {.duration = std::chrono::duration<double>(0.3), .segments = 12});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("v: 0->2")).Exists());

  // Horizontal Reorderable.
  ui.Drag(Center(ui.Find(UiSelector::Key("h-1")).One().bounds),
          Center(ui.Find(UiSelector::Key("h-3")).One().bounds),
          {.duration = std::chrono::duration<double>(0.3), .segments = 12});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("h: 0->2")).Exists());

  // ReorderableGrid.
  ui.Drag(Center(ui.Find(UiSelector::Key("g-1")).One().bounds),
          Center(ui.Find(UiSelector::Key("g-3")).One().bounds),
          {.duration = std::chrono::duration<double>(0.3), .segments = 12});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("g: 0->2")).Exists());

  // Typed Draggable -> Headless DropZone<int> + FileDropZone mount.
  CHECK(ui.Find(UiSelector::Key("file-zone")).Exists());
  ui.Drag(Center(ui.Find(UiSelector::Key("drag-source")).One().bounds),
          Center(ui.Find(UiSelector::Key("drop-zone")).One().bounds),
          {.duration = std::chrono::duration<double>(0.3), .segments = 12});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("drops: 1")).Exists());
  CHECK(ui.Find(UiSelector::Text("payload: 42")).Exists());
}

#endif
