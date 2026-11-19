#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <chrono>
#include <cstddef>
#include <string>

#ifdef INTERACTION_HAS_UI_TESTING
#include <huxerui/testing/ui_test.h>
using namespace huxerui::testing;

using namespace huxerui;
using namespace huxerui::interaction;

namespace {

View PressFamilyGallery() {
  auto clicks = UseState(0);
  auto surface_clicks = UseState(0);
  auto holds = UseState(0);
  auto holding_now = UseState(false);
  auto doubles = UseState(0);
  auto triples = UseState(0);
  auto context_requests = UseState(0);
  auto last_context_x = UseState(0);
  auto secondaries = UseState(0);

  View ripple_box = Text("ripple")
                        .With(Frame{.width = 160.0F, .height = 48.0F}, Background{Color::Rgb(120, 96, 170)},
                              Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.25F)}});
  ripple_box = std::move(ripple_box).OnClick([] {}).With(Focusable{true});

  return Column {
    Row {
      Pressable(
          Text("Pressable card")
              .With(Frame{.width = 200.0F, .height = 64.0F}, Background{Color::Rgb(64, 108, 178)}, CornerRadius(10.0F))
              .Key("pressable-inner"),
          [clicks] { clicks += 1; })
          .Key("pressable"),
      InteractiveSurface(
          [](const InteractionSnapshot& s) {
            const std::string label = std::string("surface:") + (s.hovered ? "H" : "-") + (s.pressed ? "P" : "-");
            return Text(label)
                .With(Frame{.width = 160.0F, .height = 48.0F}, Background{Color::Rgb(50, 120, 90)})
                .Key("surface-label");
          },
          [surface_clicks] { surface_clicks += 1; })
          .Key("surface"),
      std::move(ripple_box).Key("ripple"),
    }.With(Spacing(12.0F), CrossAlign(CrossAxisAlignment::Center)),
    Row {
      LongPressable(
          Text("Hold")
              .With(Frame{.width = 120.0F, .height = 48.0F}, Background{Color::Rgb(96, 110, 160)},
                    HoldProgressRing{HoldProgressStyle{.duration = 0.2}}),
          [holds] { holds += 1; },
          {.minimum_duration = std::chrono::duration<double>(0.2)},
          [holding_now](bool h) { holding_now = h; })
          .Key("hold"),
      DoubleClickable(
          Text("Double").With(Frame{.width = 120.0F, .height = 48.0F}, Background{Color::Rgb(120, 140, 90)}),
          [doubles](const MultiTapEvent&) { doubles += 1; },
          {.maximum_interval = std::chrono::duration<double>(0.25)})
          .Key("double"),
      MultiClickable(
          Text("Triple").With(Frame{.width = 120.0F, .height = 48.0F}, Background{Color::Rgb(120, 90, 150)}),
          [triples](const MultiTapEvent&) { triples += 1; },
          {.count = 3})
          .Key("triple"),
    }.With(Spacing(12.0F), CrossAlign(CrossAxisAlignment::Center)),
    Row {
      ContextMenuArea(
          Text("Context").With(Frame{.width = 160.0F, .height = 48.0F}, Background{Color::Rgb(110, 90, 150)}),
          [context_requests, last_context_x](Point p) {
            context_requests += 1;
            last_context_x = static_cast<int>(p.x);
          })
          .Key("context"),
      SecondaryClickable(
          Text("Secondary").With(Frame{.width = 160.0F, .height = 48.0F}, Background{Color::Rgb(90, 130, 100)}),
          [secondaries](Point) { secondaries += 1; })
          .Key("secondary"),
    }.With(Spacing(12.0F), CrossAlign(CrossAxisAlignment::Center)),
    Text("clicks: " + std::to_string(clicks.Get())),
    Text("surface clicks: " + std::to_string(surface_clicks.Get())),
    Text("holds: " + std::to_string(holds.Get())),
    Text(std::string("holding: ") + (holding_now.Get() ? "yes" : "no")),
    Text("doubles: " + std::to_string(doubles.Get())),
    Text("triples: " + std::to_string(triples.Get())),
    Text("context: " + std::to_string(context_requests.Get())),
    Text("last x: " + std::to_string(last_context_x.Get())),
    Text("secondaries: " + std::to_string(secondaries.Get())),
  }.With(Padding(16.0F), Spacing(12.0F));
}

} // namespace

void CheckPressContracts() {
  Application app(PressFamilyGallery, {.window = {.initial_size = {960, 520}}, .show_debug_overlay = false});
  UiTest ui(app, {.viewport = {960, 520}});

  // Keyboard focus + Space press/release + focus-loss recovery (tested before pointer clicks change focus).
  const Rect initial_pressable = ui.Find(UiSelector::Key("pressable-inner")).One().bounds;
  ui.PressKey(Key::Tab);
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("pressable")).Find(UiSelector::Focused(true)).Exists());
  ui.SendKey({.type = KeyEventType::Down, .key = Key::Space});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("pressable-inner")).One().bounds.width < initial_pressable.width);
  ui.PressKey(Key::Tab); // Focus leaves Pressable -> Space press scale clears without firing click.
  ui.PumpAndSettle();
  CHECK(Near(ui.Find(UiSelector::Key("pressable-inner")).One().bounds.width, initial_pressable.width));
  ui.SendKey({.type = KeyEventType::Up, .key = Key::Space});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 0")).Exists());

  // Primary click vs secondary pointer down on Pressable: secondary down must not shrink Pressable.
  const Point pressable_center = Center(initial_pressable);
  PointerEvent secondary_down{.type = PointerEventType::Down,
                              .pointer_id = 1,
                              .position = pressable_center,
                              .device_kind = PointerDeviceKind::Mouse,
                              .changed_button = PointerButton::Secondary,
                              .pressed_buttons = PointerButton::Secondary};
  ui.SendPointer(secondary_down);
  ui.PumpAndSettle();
  CHECK(Near(ui.Find(UiSelector::Key("pressable-inner")).One().bounds.width, initial_pressable.width));
  secondary_down.type = PointerEventType::Up;
  secondary_down.pressed_buttons = PointerButton::None;
  ui.SendPointer(secondary_down);
  ui.PumpAndSettle();

  ui.Find(UiSelector::Key("pressable")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());

  // Headless InteractiveSurface exposes hover and press states directly to builder.
  const Point surface_center = Center(ui.Find(UiSelector::Key("surface")).One().bounds);
  ui.SendPointer({.type = PointerEventType::Move, .position = surface_center});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("surface:H-")).Exists());
  ui.Find(UiSelector::Key("surface")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("surface clicks: 1")).Exists());

  // LongPressable + on_holding_changed.
  const Point hold_center = Center(ui.Find(UiSelector::Key("hold")).One().bounds);
  PointerEvent hold_ev{.type = PointerEventType::Down,
                       .pointer_id = 7,
                       .position = hold_center,
                       .device_kind = PointerDeviceKind::Touch,
                       .changed_button = PointerButton::Primary,
                       .pressed_buttons = PointerButton::Primary};
  ui.SendPointer(hold_ev);
  ui.Pump(std::chrono::milliseconds(50));
  CHECK(ui.Find(UiSelector::Text("holds: 0")).Exists());
  ui.Pump(std::chrono::milliseconds(200));
  ui.Pump();
  CHECK(ui.Find(UiSelector::Text("holds: 1")).Exists());
  CHECK(ui.Find(UiSelector::Text("holding: yes")).Exists());
  hold_ev.type = PointerEventType::Up;
  hold_ev.pressed_buttons = PointerButton::None;
  ui.SendPointer(hold_ev);
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("holding: no")).Exists());

  // DoubleClickable & MultiClickable.
  ui.Find(UiSelector::Key("double")).Tap();
  ui.Pump(std::chrono::milliseconds(80));
  ui.Find(UiSelector::Key("double")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("doubles: 1")).Exists());

  for(int i = 0; i < 3; ++i) {
    ui.Find(UiSelector::Key("triple")).Tap();
    ui.Pump(std::chrono::milliseconds(50));
  }
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("triples: 1")).Exists());

  // ContextMenuArea (mouse secondary + touch long press) & SecondaryClickable.
  const Point ctx_point = Center(ui.Find(UiSelector::Key("context")).One().bounds);
  PointerEvent sec{.type = PointerEventType::Down,
                   .pointer_id = 9,
                   .position = ctx_point,
                   .device_kind = PointerDeviceKind::Mouse,
                   .changed_button = PointerButton::Secondary,
                   .pressed_buttons = PointerButton::Secondary};
  ui.SendPointer(sec);
  sec.type = PointerEventType::Up;
  sec.pressed_buttons = PointerButton::None;
  ui.SendPointer(sec);
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("context: 1")).Exists());

  PointerEvent touch_hold{.type = PointerEventType::Down,
                          .pointer_id = 10,
                          .position = ctx_point,
                          .device_kind = PointerDeviceKind::Touch,
                          .changed_button = PointerButton::Primary,
                          .pressed_buttons = PointerButton::Primary};
  ui.SendPointer(touch_hold);
  ui.Pump(std::chrono::milliseconds(600));
  ui.Pump();
  touch_hold.type = PointerEventType::Up;
  touch_hold.pressed_buttons = PointerButton::None;
  ui.SendPointer(touch_hold);
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("context: 2")).Exists());

  const Point sec_point = Center(ui.Find(UiSelector::Key("secondary")).One().bounds);
  sec.type = PointerEventType::Down;
  sec.pointer_id = 11;
  sec.position = sec_point;
  sec.pressed_buttons = PointerButton::Secondary;
  ui.SendPointer(sec);
  sec.type = PointerEventType::Up;
  sec.pressed_buttons = PointerButton::None;
  ui.SendPointer(sec);
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("secondaries: 1")).Exists());

  // Copyable headless builder + clipboard write + timed feedback reset.
  {
    Application copy_app(
        []() -> View {
          const auto clipboard = UseApplication().Clipboard();
          auto clip_text = UseState(std::string{});
          auto callbacks = UseState(0);
          return Column {
            Copyable(
                [](bool copied) {
                  return Text(copied ? "copy:done" : "copy:idle").With(Frame{.width = 140.0F, .height = 44.0F});
                },
                "copied-secret",
                [clipboard, clip_text, callbacks](bool ok) {
                  if(ok && clipboard) {
                    callbacks += 1;
                    clip_text = clipboard->ReadText().value_or("");
                  }
                },
                CopyableStyle{.feedback_duration = std::chrono::milliseconds(200)})
                .Key("copyable-chip"),
            Text("callbacks: " + std::to_string(callbacks.Get())),
            Text("clip: " + clip_text.Get()),
          }.With(Padding(16.0F), Spacing(8.0F));
        },
        {.window = {.initial_size = {360, 200}}, .show_debug_overlay = false});
    UiTest cui(copy_app, {.viewport = {360, 200}});
    CHECK(cui.Find(UiSelector::Text("copy:idle")).Exists());
    cui.Find(UiSelector::Key("copyable-chip")).Tap();
    cui.Pump();
    CHECK(cui.Find(UiSelector::Text("callbacks: 1")).Exists());
    CHECK(cui.Find(UiSelector::Text("clip: copied-secret")).Exists());
    CHECK(cui.Find(UiSelector::Text("copy:done")).Exists());
    cui.PumpAndSettle();
    CHECK(cui.Find(UiSelector::Text("copy:idle")).Exists());
  }

  // HoldRepeat: tap, secondary-button filter, accelerating hold + controller, cancel_on_leave,
  // fire_on_press=false, dynamic Enabled(false) cutoff, keyboard hold/repeat swallowing, and Cancel().
  {
    Application hold_app(
        []() -> View {
          auto hr_ctrl = UseHoldRepeatController();
          auto kbd_ctrl = UseHoldRepeatController();

          auto hr_count = UseState(0);
          auto hr_last_step = UseState<std::size_t>(0);
          auto hr_cb_holding = UseState(false);

          auto stay_count = UseState(0);
          auto delayed_count = UseState(0);
          auto capped_count = UseState(0);
          auto capped_holding = UseState(false);
          auto kbd_count = UseState(0);

          return Column {
            Row {
              HoldRepeat(
                  [](HoldRepeatSnapshot s) -> View {
                    const std::string label = std::string("kbd:") + (s.holding ? "H" : "-") +
                                              (s.repeating ? "R" : "-") + ":" + std::to_string(s.trigger_count);
                    return Text(label).With(Frame{.width = 140.0F, .height = 44.0F});
                  },
                  [kbd_count] { kbd_count += 1; },
                  HoldRepeatStyle{
                      .initial_delay = std::chrono::milliseconds(150),
                      .repeat_interval = std::chrono::milliseconds(60),
                      .controller = kbd_ctrl,
                  })
                  .Key("hr-kbd"),
              Button("cancel-kbd").OnClick([kbd_ctrl] { kbd_ctrl.Cancel(); }).Key("cancel-kbd"),
            }.With(Spacing(12.0F), CrossAlign(CrossAxisAlignment::Center)),
            Row {
              HoldRepeat(
                  [](HoldRepeatSnapshot s) -> View {
                    const std::string label = std::string("hr:") + (s.holding ? "H" : "-") +
                                              (s.repeating ? "R" : "-") + ":" + std::to_string(s.trigger_count);
                    return Text(label)
                        .With(Frame{.width = 160.0F, .height = 48.0F}, Background{Color::Rgb(70, 110, 160)});
                  },
                  [hr_count, hr_last_step](std::size_t step) {
                    hr_count += 1;
                    hr_last_step = step;
                  },
                  HoldRepeatStyle{
                      .initial_delay = std::chrono::milliseconds(200),
                      .repeat_interval = std::chrono::milliseconds(100),
                      .minimum_interval = std::chrono::milliseconds(40),
                      .acceleration = 0.5F,
                      .cancel_on_leave = true,
                      .keyboard_focusable = false,
                      .controller = hr_ctrl,
                  },
                  [hr_cb_holding](bool h) { hr_cb_holding = h; })
                  .Key("hr-accel"),
              HoldRepeat(
                  Text("stay").With(Frame{.width = 120.0F, .height = 48.0F}),
                  [stay_count] { stay_count += 1; },
                  HoldRepeatStyle{
                      .initial_delay = std::chrono::milliseconds(150),
                      .repeat_interval = std::chrono::milliseconds(60),
                      .cancel_on_leave = false,
                      .keyboard_focusable = false,
                  })
                  .Key("hr-stay"),
              HoldRepeat(
                  [](HoldRepeatSnapshot s) -> View {
                    const std::string label = std::string("delayed:") + (s.holding ? "H" : "-") +
                                              (s.repeating ? "R" : "-") + ":" + std::to_string(s.trigger_count);
                    return Text(label).With(Frame{.width = 140.0F, .height = 48.0F});
                  },
                  [delayed_count] { delayed_count += 1; },
                  HoldRepeatStyle{
                      .initial_delay = std::chrono::milliseconds(150),
                      .repeat_interval = std::chrono::milliseconds(60),
                      .fire_on_press = false,
                      .keyboard_focusable = false,
                  })
                  .Key("hr-delayed"),
              HoldRepeat(
                  Text("capped").With(Frame{.width = 120.0F, .height = 48.0F}),
                  [capped_count] { capped_count += 1; },
                  HoldRepeatStyle{
                      .initial_delay = std::chrono::milliseconds(120),
                      .repeat_interval = std::chrono::milliseconds(50),
                      .keyboard_focusable = false,
                  },
                  [capped_holding](bool h) { capped_holding = h; })
                  .With(Enabled{capped_count.Get() < 3})
                  .Key("hr-capped"),
            }.With(Spacing(12.0F), CrossAlign(CrossAxisAlignment::Center)),
            Text("hr_count: " + std::to_string(hr_count.Get())),
            Text("hr_last_step: " + std::to_string(hr_last_step.Get())),
            Text(std::string("hr_cb: ") + (hr_cb_holding.Get() ? "yes" : "no")),
            Text(std::string("ctrl_hold: ") + (hr_ctrl.IsHolding() ? "yes" : "no")),
            Text(std::string("ctrl_rep: ") + (hr_ctrl.IsRepeating() ? "yes" : "no")),
            Text("ctrl_count: " + std::to_string(hr_ctrl.TriggerCount())),
            Text("stay_count: " + std::to_string(stay_count.Get())),
            Text("delayed_count: " + std::to_string(delayed_count.Get())),
            Text("capped_count: " + std::to_string(capped_count.Get())),
            Text(std::string("capped_hold: ") + (capped_holding.Get() ? "yes" : "no")),
            Text("kbd_count: " + std::to_string(kbd_count.Get())),
            Text(std::string("kbd_ctrl_hold: ") + (kbd_ctrl.IsHolding() ? "yes" : "no")),
          }.With(Padding(16.0F), Spacing(8.0F));
        },
        {.window = {.initial_size = {920, 480}}, .show_debug_overlay = false});
    UiTest hui(hold_app, {.viewport = {920, 480}});

    // Keyboard hold on hr-kbd: Space Down triggers once, OS auto-repeat events are swallowed,
    // timer drives repeat ticks, Tab focus-loss cancels immediately, and Controller::Cancel stops repeats.
    hui.PressKey(Key::Tab);
    hui.PumpAndSettle();
    CHECK(hui.Find(UiSelector::Key("hr-kbd")).Find(UiSelector::Focused(true)).Exists());
    hui.SendKey({.type = KeyEventType::Down, .key = Key::Space});
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("kbd_count: 1")).Exists());
    CHECK(hui.Find(UiSelector::Text("kbd_ctrl_hold: yes")).Exists());
    CHECK(hui.Find(UiSelector::Text("kbd:H-:1")).Exists());
    // OS key repeat must be ignored while HoldRepeat timer runs.
    hui.SendKey({.type = KeyEventType::Down, .key = Key::Space, .repeat = true});
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("kbd_count: 1")).Exists());
    hui.Pump(std::chrono::milliseconds(160));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("kbd_count: 2")).Exists());
    CHECK(hui.Find(UiSelector::Text("kbd:HR:2")).Exists());
    // Tab moves focus away -> stops keyboard hold cleanly.
    hui.PressKey(Key::Tab);
    hui.PumpAndSettle();
    CHECK(hui.Find(UiSelector::Text("kbd_ctrl_hold: no")).Exists());
    CHECK(hui.Find(UiSelector::Text("kbd:--:0")).Exists());
    hui.SendKey({.type = KeyEventType::Up, .key = Key::Space});
    hui.PumpAndSettle();
    CHECK(hui.Find(UiSelector::Text("kbd_count: 2")).Exists());

    // Refocus hr-kbd with Shift+Tab and verify programmatic Controller::Cancel() mid-hold.
    hui.PressKey(Key::Tab, {.shift = true});
    hui.PumpAndSettle();
    hui.SendKey({.type = KeyEventType::Down, .key = Key::Space});
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("kbd_count: 3")).Exists());
    CHECK(hui.Find(UiSelector::Text("kbd_ctrl_hold: yes")).Exists());
    hui.Find(UiSelector::Key("cancel-kbd")).Tap();
    hui.PumpAndSettle();
    CHECK(hui.Find(UiSelector::Text("kbd_ctrl_hold: no")).Exists());
    hui.Pump(std::chrono::milliseconds(220));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("kbd_count: 3")).Exists());
    hui.SendKey({.type = KeyEventType::Up, .key = Key::Space});
    hui.PumpAndSettle();

    // Quick primary tap fires exactly once and does not repeat.
    hui.Find(UiSelector::Key("hr-accel")).Tap();
    hui.PumpAndSettle();
    CHECK(hui.Find(UiSelector::Text("hr_count: 1")).Exists());
    CHECK(hui.Find(UiSelector::Text("ctrl_hold: no")).Exists());
    hui.Pump(std::chrono::milliseconds(300));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("hr_count: 1")).Exists());

    // Secondary pointer button down/hold must be ignored.
    const Point accel_center = Center(hui.Find(UiSelector::Key("hr-accel")).One().bounds);
    PointerEvent sec_hold{.type = PointerEventType::Down,
                          .pointer_id = 30,
                          .position = accel_center,
                          .device_kind = PointerDeviceKind::Mouse,
                          .changed_button = PointerButton::Secondary,
                          .pressed_buttons = PointerButton::Secondary};
    hui.SendPointer(sec_hold);
    hui.Pump(std::chrono::milliseconds(250));
    hui.Pump();
    sec_hold.type = PointerEventType::Up;
    sec_hold.pressed_buttons = PointerButton::None;
    hui.SendPointer(sec_hold);
    hui.PumpAndSettle();
    CHECK(hui.Find(UiSelector::Text("hr_count: 1")).Exists());
    CHECK(hui.Find(UiSelector::Text("hr_cb: no")).Exists());

    // Sustained hold with acceleration (initial 200ms, repeat 100ms * 0.5 -> 50ms -> clamped 40ms).
    PointerEvent accel_ev{.type = PointerEventType::Down,
                          .pointer_id = 31,
                          .position = accel_center,
                          .device_kind = PointerDeviceKind::Touch,
                          .changed_button = PointerButton::Primary,
                          .pressed_buttons = PointerButton::Primary};
    hui.SendPointer(accel_ev);
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("hr_count: 2")).Exists());
    CHECK(hui.Find(UiSelector::Text("hr_last_step: 1")).Exists());
    CHECK(hui.Find(UiSelector::Text("hr_cb: yes")).Exists());
    CHECK(hui.Find(UiSelector::Text("ctrl_hold: yes")).Exists());
    CHECK(hui.Find(UiSelector::Text("ctrl_rep: no")).Exists());
    CHECK(hui.Find(UiSelector::Text("ctrl_count: 1")).Exists());
    CHECK(hui.Find(UiSelector::Text("hr:H-:1")).Exists());

    // Before initial_delay expires, count remains unchanged.
    hui.Pump(std::chrono::milliseconds(150));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("hr_count: 2")).Exists());

    // Past initial_delay -> step #2 triggers, repeating starts.
    hui.Pump(std::chrono::milliseconds(60));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("hr_count: 3")).Exists());
    CHECK(hui.Find(UiSelector::Text("hr_last_step: 2")).Exists());
    CHECK(hui.Find(UiSelector::Text("ctrl_rep: yes")).Exists());
    CHECK(hui.Find(UiSelector::Text("ctrl_count: 2")).Exists());
    CHECK(hui.Find(UiSelector::Text("hr:HR:2")).Exists());

    // First repeat interval (100ms) -> step #3; next interval accelerates to 50ms.
    hui.Pump(std::chrono::milliseconds(100));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("hr_count: 4")).Exists());
    CHECK(hui.Find(UiSelector::Text("hr_last_step: 3")).Exists());

    // Accelerated interval (50ms < 100ms) -> step #4; next interval clamps at 40ms.
    hui.Pump(std::chrono::milliseconds(55));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("hr_count: 5")).Exists());
    CHECK(hui.Find(UiSelector::Text("hr_last_step: 4")).Exists());

    // Clamped minimum_interval (40ms) -> step #5.
    hui.Pump(std::chrono::milliseconds(45));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("hr_count: 6")).Exists());
    CHECK(hui.Find(UiSelector::Text("hr_last_step: 5")).Exists());

    // Pointer Up settles cleanly and stops further repeats.
    accel_ev.type = PointerEventType::Up;
    accel_ev.pressed_buttons = PointerButton::None;
    hui.SendPointer(accel_ev);
    hui.PumpAndSettle();
    CHECK(hui.Find(UiSelector::Text("hr_cb: no")).Exists());
    CHECK(hui.Find(UiSelector::Text("ctrl_hold: no")).Exists());
    CHECK(hui.Find(UiSelector::Text("ctrl_rep: no")).Exists());
    CHECK(hui.Find(UiSelector::Text("ctrl_count: 0")).Exists());
    CHECK(hui.Find(UiSelector::Text("hr:--:0")).Exists());
    hui.Pump(std::chrono::milliseconds(300));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("hr_count: 6")).Exists());

    // cancel_on_leave = true aborts active hold when pointer moves outside bounds.
    accel_ev.type = PointerEventType::Down;
    accel_ev.pointer_id = 32;
    accel_ev.pressed_buttons = PointerButton::Primary;
    hui.SendPointer(accel_ev);
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("hr_count: 7")).Exists());
    CHECK(hui.Find(UiSelector::Text("ctrl_hold: yes")).Exists());
    PointerEvent leave_move{.type = PointerEventType::Move,
                           .pointer_id = 32,
                           .position = {accel_center.x + 250.0F, accel_center.y + 150.0F},
                           .device_kind = PointerDeviceKind::Touch,
                           .pressed_buttons = PointerButton::Primary};
    hui.SendPointer(leave_move);
    hui.PumpAndSettle();
    CHECK(hui.Find(UiSelector::Text("ctrl_hold: no")).Exists());
    hui.Pump(std::chrono::milliseconds(250));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("hr_count: 7")).Exists());
    leave_move.type = PointerEventType::Up;
    leave_move.changed_button = PointerButton::Primary;
    leave_move.pressed_buttons = PointerButton::None;
    hui.SendPointer(leave_move);
    hui.PumpAndSettle();

    // cancel_on_leave = false keeps repeating while captured outside bounds until Up.
    const Point stay_center = Center(hui.Find(UiSelector::Key("hr-stay")).One().bounds);
    PointerEvent stay_ev{.type = PointerEventType::Down,
                         .pointer_id = 33,
                         .position = stay_center,
                         .device_kind = PointerDeviceKind::Touch,
                         .changed_button = PointerButton::Primary,
                         .pressed_buttons = PointerButton::Primary};
    hui.SendPointer(stay_ev);
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("stay_count: 1")).Exists());
    stay_ev.type = PointerEventType::Move;
    stay_ev.changed_button = PointerButton::None;
    stay_ev.position = {stay_center.x + 250.0F, stay_center.y + 150.0F};
    hui.SendPointer(stay_ev);
    hui.Pump(std::chrono::milliseconds(160));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("stay_count: 2")).Exists());
    stay_ev.type = PointerEventType::Up;
    stay_ev.changed_button = PointerButton::Primary;
    stay_ev.pressed_buttons = PointerButton::None;
    hui.SendPointer(stay_ev);
    hui.PumpAndSettle();

    // fire_on_press = false skips immediate press fire and emits first tick after initial_delay.
    const Point delayed_center = Center(hui.Find(UiSelector::Key("hr-delayed")).One().bounds);
    PointerEvent delayed_ev{.type = PointerEventType::Down,
                            .pointer_id = 34,
                            .position = delayed_center,
                            .device_kind = PointerDeviceKind::Touch,
                            .changed_button = PointerButton::Primary,
                            .pressed_buttons = PointerButton::Primary};
    hui.SendPointer(delayed_ev);
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("delayed_count: 0")).Exists());
    CHECK(hui.Find(UiSelector::Text("delayed:H-:0")).Exists());
    hui.Pump(std::chrono::milliseconds(160));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("delayed_count: 1")).Exists());
    CHECK(hui.Find(UiSelector::Text("delayed:HR:1")).Exists());
    delayed_ev.type = PointerEventType::Up;
    delayed_ev.pressed_buttons = PointerButton::None;
    hui.SendPointer(delayed_ev);
    hui.PumpAndSettle();

    // Dynamic Enabled(false) when cap reaches 3 stops further repeats and settles holding state.
    const Point capped_center = Center(hui.Find(UiSelector::Key("hr-capped")).One().bounds);
    PointerEvent capped_ev{.type = PointerEventType::Down,
                           .pointer_id = 35,
                           .position = capped_center,
                           .device_kind = PointerDeviceKind::Touch,
                           .changed_button = PointerButton::Primary,
                           .pressed_buttons = PointerButton::Primary};
    hui.SendPointer(capped_ev);
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("capped_count: 1")).Exists());
    CHECK(hui.Find(UiSelector::Text("capped_hold: yes")).Exists());
    hui.Pump(std::chrono::milliseconds(130));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("capped_count: 2")).Exists());
    hui.Pump(std::chrono::milliseconds(60));
    hui.Pump();
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("capped_count: 3")).Exists());
    CHECK(hui.Find(UiSelector::Text("capped_hold: no")).Exists());
    hui.Pump(std::chrono::milliseconds(300));
    hui.Pump();
    CHECK(hui.Find(UiSelector::Text("capped_count: 3")).Exists());
    capped_ev.type = PointerEventType::Up;
    capped_ev.pressed_buttons = PointerButton::None;
    hui.SendPointer(capped_ev);
    hui.PumpAndSettle();
  }
}

#endif
