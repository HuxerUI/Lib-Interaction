#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "test.h"

#include <cmath>
#include <limits>
#ifdef INTERACTION_HAS_UI_TESTING
#include <huxerui/testing/ui_test.h>
using namespace huxerui::testing;
#endif

using namespace huxerui;
using namespace huxerui::interaction;

void CheckValidation() {
  // Valid compositions construct without a composition context.
  (void)Pressable(Text("ok"));
  (void)Pressable(Text("ok"), {.hovered_scale = 1.0F, .pressed_scale = 0.9F, .animation = SpringSpec{}});
  (void)Pressable(Text("ok"), {}, [] {});
  (void)HoverLift(Text("ok"));
  (void)HoverLift(Text("ok"), {.lift = {6.0F, -6.0F}, .hovered_scale = 1.0F});
  (void)Pulsing(Text("ok"));
  (void)Pulsing(Text("ok"), {.scale_amplitude = 0.0F, .minimum_opacity = 0.0F, .animate_opacity = true});
  (void)Text("ok").With(Ripple{});
  (void)Text("ok").With(Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.1F), .duration = 0.2}});

  // Pressable scale validation.
  Throws<std::invalid_argument>([] { (void)Pressable(Text("x"), {.hovered_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)Pressable(Text("x"), {.pressed_scale = -0.5F}); });
  Throws<std::invalid_argument>([] {
    (void)Pressable(Text("x"), {.hovered_scale = std::numeric_limits<float>::infinity()});
  });
  // HoverLift validation.
  Throws<std::invalid_argument>([] { (void)HoverLift(Text("x"), {.lift = {0.0F, std::nanf("")}}); });
  Throws<std::invalid_argument>([] { (void)HoverLift(Text("x"), {.hovered_scale = 0.0F}); });
  // Pulsing validation.
  Throws<std::invalid_argument>([] { (void)Pulsing(Text("x"), {.scale_amplitude = -0.1F}); });
  Throws<std::invalid_argument>([] { (void)Pulsing(Text("x"), {.minimum_opacity = 1.5F}); });
  Throws<std::invalid_argument>([] { (void)Pulsing(Text("x"), {.duration = 0.0}); });
  Throws<std::invalid_argument>([] {
    (void)Pulsing(Text("x"), {.animate_scale = false, .animate_opacity = false});
  });
  // Ripple validation.
  Throws<std::invalid_argument>([] { (void)Text("x").With(Ripple{RippleStyle{.duration = 0.0}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Ripple{RippleStyle{.duration = -0.4}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Ripple{RippleStyle{.maximum_opacity = 1.5F}}); });
}

#ifdef INTERACTION_HAS_UI_TESTING

View InteractionGallery() {
  auto clicks = UseState(0);
  return Column {
    Row {
      Pressable(Text("Pressable card").With(Frame{.width = 200.0F, .height = 64.0F},
                                            Background{Color::Rgb(64, 108, 178)}, CornerRadius(10.0F)),
                {}, [clicks] { clicks += 1; })
          .Key("pressable"),
      HoverLift(Text("Hover lift").With(Frame{.width = 160.0F, .height = 48.0F},
                                        Background{Color::Rgb(88, 150, 110)}))
          .Key("lift"),
      Pulsing(Text("Pulse").With(Frame{.width = 80.0F, .height = 48.0F},
                                 Background{Color::Rgb(190, 120, 60)}),
              {.duration = 0.3})
          .Key("pulse"),
      Text("Ripple").With(Frame{.width = 160.0F, .height = 48.0F},
                          Background{Color::Rgb(120, 96, 170)},
                          Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.25F)}})
          .Key("ripple"),
    }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
    Text("clicks: " + std::to_string(clicks.Get())).Key("count"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

void CheckInteractionContracts() {
  Application app(InteractionGallery, {.window = {.initial_size = {720, 320}}, .show_debug_overlay = false});
  UiTest ui(app);
  CHECK(ui.Find(UiSelector::Key("pressable")).One().size == (Size{200, 64}));
  CHECK(ui.Find(UiSelector::Key("lift")).One().size == (Size{160, 48}));
  CHECK(ui.Find(UiSelector::Key("pulse")).One().size == (Size{80, 48}));
  CHECK(ui.Find(UiSelector::Key("ripple")).Exists());
  CHECK(ui.Find(UiSelector::Text("clicks: 0")).Exists());
  ui.Find(UiSelector::Key("pressable")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());
  ui.Find(UiSelector::Key("ripple")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());
}

#endif

int main() { return Run([] {
  CheckValidation();
#ifdef INTERACTION_HAS_UI_TESTING
  CheckInteractionContracts();
#endif
}); }
