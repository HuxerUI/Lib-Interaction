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
      Magnetic(Text("Magnetic").With(Frame{.width = 160.0F, .height = 48.0F}, Background{Color::Rgb(70, 90, 140)})
                   .Key("mag-inner"),
               {.maximum_offset = {12.0F, 6.0F},
                .hovered_scale = 1.0F,
                .follow_motion = TweenSpec{.duration = 0.05, .easing = Easing::Linear},
                .return_motion = TweenSpec{.duration = 0.05, .easing = Easing::Linear}})
          .Key("mag-outer"),
      HoverShadow(Text("Shadow").With(Frame{.width = 160.0F, .height = 48.0F}, Background{Color::Rgb(50, 60, 80)},
                                      CornerRadius(10.0F)),
                  {.fade = 0.05, .corner_radii = CornerRadii{10.0F}})
          .Key("hover-shadow"),
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

  // Magnetic pulls inner child toward pointer while outer bounds stay stationary.
  const Rect outer_rest = ui.Find(UiSelector::Key("mag-outer")).One().bounds;
  const Rect inner_rest = ui.Find(UiSelector::Key("mag-inner")).One().bounds;
  ui.SendPointer({
      .type = PointerEventType::Move,
      .position = {outer_rest.x + outer_rest.width - 10.0F, outer_rest.y + outer_rest.height - 6.0F},
  });
  ui.PumpAndSettle();
  const Rect outer_hovered = ui.Find(UiSelector::Key("mag-outer")).One().bounds;
  const Rect inner_hovered = ui.Find(UiSelector::Key("mag-inner")).One().bounds;
  CHECK(outer_hovered == outer_rest);
  CHECK(inner_hovered.x > inner_rest.x + 1.0F);
  CHECK(inner_hovered.y > inner_rest.y + 1.0F);
  ui.SendPointer({.type = PointerEventType::Move, .position = {2.0F, 2.0F}});
  ui.PumpAndSettle();
  const Rect inner_settled = ui.Find(UiSelector::Key("mag-inner")).One().bounds;
  CHECK(inner_settled == inner_rest);

  // HoverShadow changes shadow/glow geometry with cursor position and settles back to rest.
  const Rect shadow_bounds = ui.Find(UiSelector::Key("hover-shadow")).One().bounds;
  const UiSnapshot shadow_rest = ui.CaptureSnapshot();
  ui.SendPointer({.type = PointerEventType::Move, .position = {shadow_bounds.x + 12.0F, shadow_bounds.y + 24.0F}});
  ui.PumpAndSettle();
  const UiSnapshot shadow_left = ui.CaptureSnapshot();
  CHECK(shadow_left != shadow_rest);
  ui.SendPointer({.type = PointerEventType::Move,
                  .position = {shadow_bounds.x + shadow_bounds.width - 12.0F, shadow_bounds.y + 24.0F}});
  ui.PumpAndSettle();
  const UiSnapshot shadow_right = ui.CaptureSnapshot();
  CHECK(shadow_right != shadow_left);
  ui.SendPointer({.type = PointerEventType::Move, .position = {2.0F, 2.0F}});
  ui.PumpAndSettle();
  const UiSnapshot shadow_settled = ui.CaptureSnapshot();
  CHECK(shadow_settled == shadow_rest);

  // Popover click-trigger + PopoverController + ctx.Dismiss() + Escape dismissal sync.
  {
    Application pop_app(
        []() -> View {
          auto pop_ctrl = UsePopoverController();
          auto pop_events = UseState(0);
          return Column {
            Row {
              Popover(
                  Button("Trigger Pop").Key("pop-trigger"),
                  [](PopupContext ctx) {
                    return Column {
                      Text("Popover Content").Key("pop-body"),
                      Button("Dismiss Inside").OnClick([ctx] { ctx.Dismiss(); }).Key("pop-dismiss"),
                    }.With(Padding(12.0F), Background{Color::Rgb(40, 44, 52)});
                  },
                  {.controller = pop_ctrl},
                  [pop_events](bool) { pop_events += 1; }),
              Button("Ctrl Open").OnClick([pop_ctrl] { pop_ctrl.Open(); }).Key("pop-ctrl-open"),
            }.With(Spacing(12.0F)),
            Text(std::string("pop open: ") + (pop_ctrl.IsOpen() ? "yes" : "no")),
            Text("pop events: " + std::to_string(pop_events.Get())),
          }.With(Padding(16.0F), Spacing(12.0F));
        },
        {.window = {.initial_size = {520, 320}}, .show_debug_overlay = false});
    UiTest pui(pop_app, {.viewport = {520, 320}});

    CHECK(pui.Find(UiSelector::Text("pop open: no")).Exists());
    CHECK(!pui.Find(UiSelector::Key("pop-body")).Exists());

    pui.Find(UiSelector::Key("pop-trigger")).Tap();
    pui.PumpAndSettle();
    CHECK(pui.Find(UiSelector::Text("pop open: yes")).Exists());
    CHECK(pui.Find(UiSelector::Key("pop-body")).Exists());
    CHECK(pui.Find(UiSelector::Text("pop events: 1")).Exists());

    pui.Find(UiSelector::Key("pop-dismiss")).Tap();
    pui.PumpAndSettle();
    CHECK(pui.Find(UiSelector::Text("pop open: no")).Exists());
    CHECK(!pui.Find(UiSelector::Key("pop-body")).Exists());
    CHECK(pui.Find(UiSelector::Text("pop events: 2")).Exists());

    pui.Find(UiSelector::Key("pop-ctrl-open")).Tap();
    pui.PumpAndSettle();
    CHECK(pui.Find(UiSelector::Text("pop open: yes")).Exists());
    CHECK(pui.Find(UiSelector::Key("pop-body")).Exists());

    pui.PressKey(Key::Escape);
    pui.PumpAndSettle();
    CHECK(pui.Find(UiSelector::Text("pop open: no")).Exists());
    CHECK(!pui.Find(UiSelector::Key("pop-body")).Exists());
    CHECK(pui.Find(UiSelector::Text("pop events: 4")).Exists());
  }
}

#endif
