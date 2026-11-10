#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>
#include <huxerui/testing/ui_test.h>

#include "test.h"

#include <chrono>
#include <cmath>
#include <string>

using namespace huxerui;
using namespace huxerui::interaction;
using namespace huxerui::testing;

namespace {

bool Near(float actual, float expected) { return std::abs(actual - expected) <= 0.5F; }

View StateGallery() {
  auto clicks = UseState(0);
  View ripple = Text("ripple surface")
                    .With(Frame{.width = 200.0F, .height = 64.0F}, Background{Color::Rgb(80, 110, 160)},
                          CornerRadius(10.0F));
  ripple = std::move(ripple).OnClick([] {}).With(Focusable{true}, Ripple{RippleStyle{.duration = 0.4}});
  return Column {
    Pressable(Text("keyboard press")
                  .With(Frame{.width = 200.0F, .height = 64.0F}, Background{Color::Rgb(64, 108, 178)},
                        CornerRadius(10.0F)),
              {}, [clicks] { clicks += 1; })
        .Key("pressable"),
    std::move(ripple).Key("ripple"),
    Text("clicks: " + std::to_string(clicks.Get())).Key("clicks"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

} // namespace

int main() { return Run([] {
  Application app(StateGallery, {.window = {.initial_size = {420, 360}}, .show_debug_overlay = false});
  UiTest ui(app, {.viewport = {420, 360}});

  const Rect initial = ui.Find(UiSelector::Text("keyboard press")).One().bounds;
  ui.PressKey(Key::Tab);
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("pressable")).Find(UiSelector::Focused(true)).Exists());
  CHECK(ui.Find(UiSelector::Text("clicks: 0")).Exists());

  ui.SendKey({.type = KeyEventType::Down, .key = Key::Space});
  ui.PumpAndSettle();
  const Rect held = ui.Find(UiSelector::Text("keyboard press")).One().bounds;
  CHECK(held.width < initial.width);

  ui.SendKey({.type = KeyEventType::Up, .key = Key::Space});
  ui.PumpAndSettle();
  const Rect released = ui.Find(UiSelector::Text("keyboard press")).One().bounds;
  CHECK(Near(released.width, initial.width));
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());

  // Focus loss clears the held Space scale even if the key release is never delivered to the widget.
  ui.SendKey({.type = KeyEventType::Down, .key = Key::Space});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("keyboard press")).One().bounds.width < initial.width);
  ui.PressKey(Key::Tab);
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("ripple")).One().focused);
  CHECK(Near(ui.Find(UiSelector::Text("keyboard press")).One().bounds.width, initial.width));
  ui.SendKey({.type = KeyEventType::Up, .key = Key::Space});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());

  // Keyboard activation drives the same InteractionEvent path as pointer presses.
  ui.SendKey({.type = KeyEventType::Down, .key = Key::Space});
  ui.Pump(std::chrono::milliseconds(100));
  CHECK(ui.Find(UiSelector::Key("ripple")).One().focused);
  ui.SendKey({.type = KeyEventType::Up, .key = Key::Space});
  ui.PumpAndSettle();
}); }
