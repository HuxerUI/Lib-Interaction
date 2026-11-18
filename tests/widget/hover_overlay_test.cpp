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

View HoverAndOverlayGallery() {
  auto card_controller = UseHoverCardController();
  auto open_events = UseState(0);

  return Column {
    Row {
      Hoverable([](bool hovered) {
        return Text(hovered ? "hover:on" : "hover:off")
            .With(Frame{.width = 140.0F, .height = 48.0F}, Background{Color::Rgb(70, 120, 90)});
      }).Key("hoverable"),
      HoverLift(
          Text("Lift").With(Frame{.width = 140.0F, .height = 48.0F}, Background{Color::Rgb(88, 150, 110)}),
          {.lift = {0.0F, -8.0F}})
          .Key("lift"),
      Text("Spotlight").With(Frame{.width = 140.0F, .height = 48.0F}, Background{Color::Rgb(60, 70, 90)},
                             Spotlight{SpotlightStyle{.diameter = 120.0F}})
          .Key("spotlight"),
      Text("Highlight").With(Frame{.width = 140.0F, .height = 48.0F}, Background{Color::Rgb(90, 100, 70)},
                             HoverHighlight{HoverHighlightStyle{}})
          .Key("highlight"),
    }.With(Spacing(12.0F)),
    Row {
      HoverCard(
          Text("Hover anchor").With(Frame{.width = 160.0F, .height = 48.0F}, Background{Color::Rgb(80, 120, 160)}),
          [](PopupContext) {
            return Text("Popup body")
                .With(Frame{.width = 140.0F, .height = 48.0F}, Background{Color::Rgb(40, 44, 52)})
                .Key("popup-body");
          },
          {.show_delay = std::chrono::duration<double>(0.05),
           .hide_delay = std::chrono::duration<double>(0.05),
           .long_press = false,
           .controller = card_controller},
          [open_events](bool) { open_events += 1; })
          .Key("hover-card"),
      Button("Show card").OnClick([card_controller] { card_controller.Show(); }).Key("btn-show-card"),
      Button("Hide card").OnClick([card_controller] { card_controller.Hide(); }).Key("btn-hide-card"),
      Text(std::string("card open: ") + (card_controller.IsOpen() ? "yes" : "no")),
      Text("card events: " + std::to_string(open_events.Get())),
    }.With(Spacing(12.0F)),
  }.With(Padding(16.0F), Spacing(16.0F));
}

} // namespace

void CheckHoverAndOverlayContracts() {
  Application app(HoverAndOverlayGallery, {.window = {.initial_size = {820, 360}}, .show_debug_overlay = false});
  UiTest ui(app, {.viewport = {820, 360}});

  CHECK(ui.Find(UiSelector::Text("hover:off")).Exists());
  ui.SendPointer({.type = PointerEventType::Move, .position = Center(ui.Find(UiSelector::Key("hoverable")).One().bounds)});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("hover:on")).Exists());

  const Rect lift_rest = ui.Find(UiSelector::Key("lift")).One().bounds;
  ui.SendPointer({.type = PointerEventType::Move, .position = Center(lift_rest)});
  ui.PumpAndSettle();

  CHECK(ui.Find(UiSelector::Text("card open: no")).Exists());
  ui.Find(UiSelector::Key("btn-show-card")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("card open: yes")).Exists());
  CHECK(ui.Find(UiSelector::Key("popup-body")).Exists());

  ui.Find(UiSelector::Key("btn-hide-card")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("card open: no")).Exists());
  CHECK(ui.Find(UiSelector::Text("card events: 2")).Exists());
}

#endif
