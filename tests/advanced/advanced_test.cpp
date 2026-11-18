#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>
#include <huxerui/testing/ui_test.h>

#include "../support/test.h"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

using namespace huxerui;
using namespace huxerui::interaction;
using namespace huxerui::testing;

namespace {

/// Cross-feature scenario 1: Multi-stage `operator|` pipeline combining Pressable, DoubleClickable, ContextMenuArea,
/// KeyboardShortcut, and retained Ripple/Spotlight on one card without mutual interference.
View PipelineIntegrationApp() {
  auto clicks = UseState(0);
  auto doubles = UseState(0);
  auto menus = UseState(0);
  auto shortcuts = UseState(0);

  View card = Text("pipeline card")
                  .With(Frame{.width = 220.0F, .height = 64.0F}, Background{Color::Rgb(64, 108, 178)},
                        CornerRadius(10.0F), Ripple{}, Spotlight{})
              | DoubleClickable([doubles](const MultiTapEvent&) { doubles += 1; })
              | ContextMenuArea([menus](Point) { menus += 1; })
              | Pressable([clicks] { clicks += 1; });

  return Column {
    std::move(card).Key("pipe-card"),
    Text("clicks: " + std::to_string(clicks.Get())),
    Text("doubles: " + std::to_string(doubles.Get())),
    Text("menus: " + std::to_string(menus.Get())),
    Text("shortcuts: " + std::to_string(shortcuts.Get())),
  }.With(Padding(16.0F), Spacing(12.0F))
         | KeyboardShortcut({.key = Key::J, .modifiers = KeyModifiers{.control = true}},
                            [shortcuts] { shortcuts += 1; });
}

/// Cross-feature scenario 2: Mutually exclusive SwipeActions list rows coordinated through SwipeController.
View ExclusiveSwipeListApp() {
  auto row_a = UseSwipeController();
  auto row_b = UseSwipeController();

  auto build_row = [](const char* key_prefix, SwipeController own, SwipeController sibling) {
    return SwipeActions(
        Text(std::string("row ") + key_prefix)
            .With(Frame{.width = 280.0F, .height = 52.0F}, Background{Color::Rgb(80, 110, 160)})
            .Key(std::string("content-") + key_prefix),
        [](SwipeActionContext ctx) {
          return Button("Close").OnClick(ctx.close);
        },
        {.action_extent = 90.0F, .activation_ratio = 0.35F, .controller = own},
        [sibling](bool opened) {
          if(opened) sibling.Close();
        });
  };

  return Column {
    build_row("a", row_a, row_b),
    build_row("b", row_b, row_a),
    Text(std::string("a open: ") + (row_a.IsOpen() ? "yes" : "no")),
    Text(std::string("b open: ") + (row_b.IsOpen() ? "yes" : "no")),
  }.With(Padding(16.0F), Spacing(12.0F));
}

/// Cross-feature scenario 3: Handle-only Reorderable containing interactive child buttons inside each slot.
/// Dragging the child area or clicking the button must not reorder; dragging ReorderHandle reorders cleanly.
View HandleOnlyReorderApp() {
  auto order = UseState(std::vector<int>{1, 2, 3});
  auto button_clicks = UseState(0);
  auto reorders = UseState(std::string("none"));

  std::vector<View> rows;
  for(int id : order.Get()) {
    View grip = Text("grip")
                    .With(Frame{.width = 48.0F, .height = 44.0F}, Background{Color::Rgb(50, 90, 160)})
                    .Key("grip-" + std::to_string(id))
                | ReorderHandle();
    View action = Button("Act " + std::to_string(id))
                      .OnClick([button_clicks] { button_clicks += 1; })
                      .Key("act-" + std::to_string(id));
    rows.push_back(Row {
      std::move(grip),
      Text("Body " + std::to_string(id)).With(Grow{}).Key("body-" + std::to_string(id)),
      std::move(action),
    }.With(Frame{.width = 300.0F, .height = 48.0F}, Spacing(8.0F), CrossAlign(CrossAxisAlignment::Center),
           Background{Color::Rgb(90, 110, 130)}));
  }

  return Column {
    Reorderable(
        std::move(rows),
        [order, reorders](std::size_t from, std::size_t to) {
          auto next = order.Get();
          const int val = next[from];
          next.erase(next.begin() + static_cast<std::ptrdiff_t>(from));
          next.insert(next.begin() + static_cast<std::ptrdiff_t>(to), val);
          order = std::move(next);
          reorders = std::to_string(from) + "->" + std::to_string(to);
        },
        {.handle_only = true}),
    Text("btn clicks: " + std::to_string(button_clicks.Get())),
    Text("reorders: " + reorders.Get()),
  }.With(Padding(16.0F), Spacing(12.0F));
}

/// Cross-feature scenario 4: External toolbar controlling and observing PanZoom through PanZoomController.
View ControlledPanZoomApp() {
  auto controller = UsePanZoomController();
  const int pct = static_cast<int>(std::lround(controller.Scale() * 100.0F));

  return Column {
    Row {
      Button("Zoom In").OnClick([controller] { controller.ZoomBy(2.0F); }).Key("zoom-in"),
      Button("Pan Right").OnClick([controller] { controller.PanTo({50.0F, 0.0F}); }).Key("pan-right"),
      Button("Reset").OnClick([controller] { controller.Reset(); }).Key("zoom-reset"),
      Text("zoom: " + std::to_string(pct) + "%"),
    }.With(Spacing(8.0F)),
    PanZoom(
        Text("canvas").With(Frame{.width = 120.0F, .height = 60.0F}, Background{Color::Rgb(70, 120, 150)}).Key("canvas"),
        {.minimum_scale = 0.5F, .maximum_scale = 4.0F, .controller = controller}),
  }.With(Padding(16.0F), Spacing(16.0F));
}

} // namespace

int main() {
  return Run([] {
    // 1. Pipeline integration.
    {
      Application app(PipelineIntegrationApp, {.window = {.initial_size = {420, 320}}, .show_debug_overlay = false});
      UiTest ui(app, {.viewport = {420, 320}});
      ui.Find(UiSelector::Key("pipe-card")).Tap();
      ui.Pump(std::chrono::milliseconds(80));
      ui.Find(UiSelector::Key("pipe-card")).Tap();
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("clicks: 2")).Exists());
      CHECK(ui.Find(UiSelector::Text("doubles: 1")).Exists());

      const Point center = Center(ui.Find(UiSelector::Key("pipe-card")).One().bounds);
      PointerEvent sec{.type = PointerEventType::Down,
                       .pointer_id = 1,
                       .position = center,
                       .device_kind = PointerDeviceKind::Mouse,
                       .changed_button = PointerButton::Secondary,
                       .pressed_buttons = PointerButton::Secondary};
      ui.SendPointer(sec);
      sec.type = PointerEventType::Up;
      sec.pressed_buttons = PointerButton::None;
      ui.SendPointer(sec);
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("menus: 1")).Exists());

      ui.PressKey(Key::J, KeyModifiers{.control = true});
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("shortcuts: 1")).Exists());
    }

    // 2. Mutual-exclusion SwipeController rows.
    {
      Application app(ExclusiveSwipeListApp, {.window = {.initial_size = {420, 320}}, .show_debug_overlay = false});
      UiTest ui(app, {.viewport = {420, 320}});
      const Point start_a = Center(ui.Find(UiSelector::Key("content-a")).One().bounds);
      ui.Drag(start_a, {start_a.x - 120.0F, start_a.y},
              {.duration = std::chrono::duration<double>(0.25), .segments = 10});
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("a open: yes")).Exists());
      CHECK(ui.Find(UiSelector::Text("b open: no")).Exists());

      const Point start_b = Center(ui.Find(UiSelector::Key("content-b")).One().bounds);
      ui.Drag(start_b, {start_b.x - 120.0F, start_b.y},
              {.duration = std::chrono::duration<double>(0.25), .segments = 10});
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("a open: no")).Exists());
      CHECK(ui.Find(UiSelector::Text("b open: yes")).Exists());
    }

    // 3. Handle-only Reorderable + child button isolation.
    {
      Application app(HandleOnlyReorderApp, {.window = {.initial_size = {420, 360}}, .show_debug_overlay = false});
      UiTest ui(app, {.viewport = {420, 360}});

      // Dragging the row body does NOT reorder when handle_only = true.
      ui.Drag(Center(ui.Find(UiSelector::Key("body-1")).One().bounds),
              Center(ui.Find(UiSelector::Key("body-3")).One().bounds),
              {.duration = std::chrono::duration<double>(0.25), .segments = 10});
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("reorders: none")).Exists());

      // Clicking the child button activates cleanly.
      ui.Find(UiSelector::Key("act-1")).Tap();
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("btn clicks: 1")).Exists());

      // Dragging the ReorderHandle grip reorders row 0 -> 2.
      ui.Drag(Center(ui.Find(UiSelector::Key("grip-1")).One().bounds),
              Center(ui.Find(UiSelector::Key("grip-3")).One().bounds),
              {.duration = std::chrono::duration<double>(0.3), .segments = 12});
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("reorders: 0->2")).Exists());
    }

    // 4. Controlled PanZoom via external toolbar.
    {
      Application app(ControlledPanZoomApp, {.window = {.initial_size = {480, 320}}, .show_debug_overlay = false});
      UiTest ui(app, {.viewport = {480, 320}});
      const Rect initial = ui.Find(UiSelector::Key("canvas")).One().bounds;
      CHECK(ui.Find(UiSelector::Text("zoom: 100%")).Exists());

      ui.Find(UiSelector::Key("zoom-in")).Tap();
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("zoom: 200%")).Exists());
      CHECK(Near(ui.Find(UiSelector::Key("canvas")).One().bounds.width, initial.width * 2.0F));

      ui.Find(UiSelector::Key("pan-right")).Tap();
      ui.PumpAndSettle();
      CHECK(Near(Center(ui.Find(UiSelector::Key("canvas")).One().bounds).x, Center(initial).x + 50.0F));

      ui.Find(UiSelector::Key("zoom-reset")).Tap();
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("zoom: 100%")).Exists());
      CHECK(Near(ui.Find(UiSelector::Key("canvas")).One().bounds.width, initial.width));
    }
  });
}
