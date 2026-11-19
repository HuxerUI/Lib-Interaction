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

/// Cross-feature scenario 5: RovingFocusGroup containing a Popover trigger inside an outer BackHandler scope.
/// Opening the popover confines Back/Escape to dismissing the popup first before reaching BackHandler.
View RovingPopoverBackApp() {
  auto rf = UseRovingFocusController(0);
  auto pop = UsePopoverController();
  auto back_hits = UseState(0);

  return Column {
    (Row {
      Button("Tab 0").Key("rf-0") | RovingFocusItem(0),
      Popover(
          [](PopoverTriggerContext ctx) {
            return Button("Menu").OnClick(ctx.toggle).Key("rf-1") | RovingFocusItem(1);
          },
          [](PopupContext ctx) {
            return Column {
              Text("Popover Body"),
              Button("Close").OnClick([ctx] { ctx.Dismiss(); }).Key("pop-close"),
            }.With(Padding(8.0F), Spacing(6.0F), Background{Color::Rgb(245, 245, 250)});
          },
          {.controller = pop}),
      Button("Tab 2").Key("rf-2") | RovingFocusItem(2),
    }.With(Spacing(10.0F))
     | RovingFocusGroup({.orientation = RovingFocusOrientation::Horizontal, .controller = rf})),
    Text("rf_active: " + std::to_string(rf.ActiveIndex())),
    Text(std::string("pop_open: ") + (pop.IsOpen() ? "yes" : "no")),
    Text("back_hits: " + std::to_string(back_hits.Get())),
  }.With(Padding(16.0F), Spacing(12.0F))
         | BackHandler([back_hits] { back_hits += 1; });
}

/// Cross-feature scenario 6: MarqueeSelectArea + Copyable clipboard export + ShortcutMap bulk actions.
View MarqueeCopyShortcutApp() {
  auto marquee = UseMarqueeSelectController();
  const auto clipboard = UseApplication().Clipboard();
  auto clip_text = UseState(std::string{});

  const std::vector<std::size_t> selected = marquee.Selection();
  std::string summary = selected.empty() ? std::string("none") : std::string();
  for(std::size_t i = 0; i < selected.size(); ++i) {
    if(i > 0) summary += ",";
    summary += std::to_string(selected[i]);
  }

  std::vector<View> tiles;
  for(std::size_t i = 0; i < 3; ++i) {
    const Color bg = marquee.IsSelected(i) ? Color::Rgb(31, 111, 235) : Color::Rgb(64, 78, 98);
    tiles.push_back(Text("M" + std::to_string(i))
                        .With(Frame{.width = 64.0F, .height = 44.0F}, Background{bg})
                        .Key("mq-" + std::to_string(i))
                    | MarqueeSelectItem(i));
  }

  return Column {
    Row {
      Button("Focus").Key("mq-focus"),
      Copyable(
          [](bool copied) {
            return Text(copied ? "copied: yes" : "copied: no").With(Frame{.width = 120.0F, .height = 36.0F});
          },
          "sel:" + summary,
          [clipboard, clip_text](bool ok) {
            if(ok && clipboard) {
              clip_text = clipboard->ReadText().value_or("");
            }
          },
          CopyableStyle{.feedback_duration = std::chrono::milliseconds(150)})
          .Key("mq-copy"),
    }.With(Spacing(10.0F)),
    (Row(std::move(tiles))
         .With(Spacing(12.0F), Padding(12.0F), Frame{.width = 260.0F, .height = 68.0F},
               Background{Color::Rgb(40, 48, 60)})
     | MarqueeSelectArea({.controller = marquee})),
    Text("sel: " + summary),
    Text("clip: " + clip_text.Get()),
  }.With(Padding(16.0F), Spacing(10.0F))
         | ShortcutMap({
             {.chord = {.key = Key::A, .modifiers = {.control = true}},
              .on_trigger = [marquee] { marquee.SetSelection({0, 1, 2}); }},
             {.chord = {.key = Key::D, .modifiers = {.control = true}},
              .on_trigger = [marquee] { marquee.Clear(); }},
         });
}

/// Cross-feature scenario 7: ScrollHideController + ScrollSpy + Expandable dynamic geometry synchronization.
View ScrollHideSpyExpandableApp() {
  auto scroll = UseScrollController();
  auto spy = UseScrollSpyController();
  auto hide = UseScrollHideController({.extent = 40.0F, .consume_scroll = false});
  auto expanded = UseState(false);

  return Column {
    Row {
      Button("Jump 0").OnClick([spy] { spy.ScrollToSection(0); }).Key("adv-jump-0"),
      Button("Jump 1").OnClick([spy] { spy.ScrollToSection(1); }).Key("adv-jump-1"),
      Button("Toggle Exp").OnClick([expanded] { expanded = !expanded.Get(); }).Key("adv-toggle-exp"),
    }.With(Spacing(8.0F)),
    (ScrollView(Column {
       (Column {
         Text("Sec 0").With(Frame{.width = 240.0F, .height = 80.0F}),
         Expandable(Text("Extra").With(Frame{.width = 240.0F, .height = 96.0F}), expanded.Get(),
                    {.motion = TweenSpec{.duration = 0.06, .easing = Easing::Linear}})
             .Key("adv-exp"),
       }
        | ScrollSpySection(0)),
       Text("Sec 1").With(Frame{.width = 240.0F, .height = 120.0F}) | ScrollSpySection(1),
       Text("Sec 2").With(Frame{.width = 240.0F, .height = 140.0F}) | ScrollSpySection(2),
     })
         .Controller(scroll)
         .With(Frame{.width = 280.0F, .height = 160.0F}, hide.Connection())
         .Key("adv-scroll")
     | ScrollSpyArea({.scroll_controller = scroll, .activation_offset = 16.0F, .controller = spy})),
    Text("spy_active: " + std::to_string(spy.ActiveSection())),
    Text(std::string("hide_hid: ") + (hide.IsHidden() ? "yes" : "no")),
    Text("offset: " + std::to_string(static_cast<int>(std::lround(scroll.Offset())))),
  }.With(Padding(16.0F), Spacing(10.0F));
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

    // 5. RovingFocusGroup + Popover + BackHandler priority.
    {
      Application app(RovingPopoverBackApp, {.window = {.initial_size = {460, 260}}, .show_debug_overlay = false});
      UiTest ui(app, {.viewport = {460, 260}});
      ui.Find(UiSelector::Key("rf-0")).Tap();
      ui.PumpAndSettle();
      ui.PressKey(Key::ArrowRight);
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("rf_active: 1")).Exists());
      CHECK(ui.Find(UiSelector::Key("rf-1")).One().focused);

      ui.Find(UiSelector::Key("rf-1")).Tap();
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("pop_open: yes")).Exists());
      CHECK(ui.Find(UiSelector::Text("Popover Body")).Exists());

      // First Escape dismisses the open Popover layer rather than invoking BackHandler.
      ui.PressKey(Key::Escape);
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("pop_open: no")).Exists());
      CHECK(!ui.Find(UiSelector::Text("Popover Body")).Exists());
      CHECK(ui.Find(UiSelector::Text("back_hits: 0")).Exists());

      // Second Escape reaches BackHandler once no popup layer is active.
      ui.PressKey(Key::Escape);
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("back_hits: 1")).Exists());

      // Roving continues across the group.
      ui.Find(UiSelector::Key("rf-1")).Tap();
      ui.PumpAndSettle();
      ui.PressKey(Key::Escape);
      ui.PumpAndSettle();
      ui.Find(UiSelector::Key("rf-0")).Tap();
      ui.PumpAndSettle();
      ui.PressKey(Key::ArrowRight);
      ui.PressKey(Key::ArrowRight);
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("rf_active: 2")).Exists());
      CHECK(ui.Find(UiSelector::Key("rf-2")).One().focused);
    }

    // 6. MarqueeSelectArea + Copyable + ShortcutMap.
    {
      Application app(MarqueeCopyShortcutApp, {.window = {.initial_size = {480, 280}}, .show_debug_overlay = false});
      UiTest ui(app, {.viewport = {480, 280}});
      const Rect b0 = ui.Find(UiSelector::Key("mq-0")).One().bounds;
      const Rect b1 = ui.Find(UiSelector::Key("mq-1")).One().bounds;
      ui.Drag({b0.x + 4.0F, b0.y + 4.0F}, {b1.x + b1.width * 0.5F, b1.y + b1.height - 4.0F},
              {.duration = std::chrono::duration<double>(0.2), .segments = 8});
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("sel: 0,1")).Exists());

      ui.Find(UiSelector::Key("mq-copy")).Tap();
      ui.Pump();
      CHECK(ui.Find(UiSelector::Text("copied: yes")).Exists());
      CHECK(ui.Find(UiSelector::Text("clip: sel:0,1")).Exists());
      ui.Pump(std::chrono::milliseconds(250));
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("copied: no")).Exists());

      ui.Find(UiSelector::Key("mq-focus")).Tap();
      ui.PumpAndSettle();
      ui.PressKey(Key::A, KeyModifiers{.control = true});
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("sel: 0,1,2")).Exists());

      ui.PressKey(Key::D, KeyModifiers{.control = true});
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("sel: none")).Exists());
    }

    // 7. ScrollHideController + ScrollSpy + Expandable dynamic geometry sync.
    {
      Application app(ScrollHideSpyExpandableApp,
                      {.window = {.initial_size = {420, 340}}, .show_debug_overlay = false});
      UiTest ui(app, {.viewport = {420, 340}});
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("spy_active: 0")).Exists());
      CHECK(ui.Find(UiSelector::Text("hide_hid: no")).Exists());
      CHECK(ui.Find(UiSelector::Key("adv-exp")).One().size.height == 0.0F);

      const Point scroll_center = Center(ui.Find(UiSelector::Key("adv-scroll")).One().bounds);
      ui.SendScroll({.position = scroll_center, .delta_y = 110.0F});
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("hide_hid: yes")).Exists());
      CHECK(ui.Find(UiSelector::Text("spy_active: 1")).Exists());

      ui.Find(UiSelector::Key("adv-jump-0")).Tap();
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("spy_active: 0")).Exists());

      ui.Find(UiSelector::Key("adv-toggle-exp")).Tap();
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Key("adv-exp")).One().size.height >= 95.0F);

      ui.Find(UiSelector::Key("adv-jump-1")).Tap();
      ui.PumpAndSettle();
      CHECK(ui.Find(UiSelector::Text("spy_active: 1")).Exists());
      // Expanded Section 0 is 80 + 96 = 176 px tall; jumping to Section 1 lands well past 150 px.
      CHECK(ui.Find(UiSelector::Text("offset: 176")).Exists());
    }
  });
}
