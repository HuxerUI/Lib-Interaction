#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <chrono>
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
}

#endif
