#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <string>

#ifdef INTERACTION_HAS_UI_TESTING
#include <huxerui/testing/ui_test.h>
using namespace huxerui::testing;
#endif

using namespace huxerui;
using namespace huxerui::interaction;

#ifdef INTERACTION_HAS_UI_TESTING

View InteractionGallery() {
  auto clicks = UseState(0);
  auto holds = UseState(0);
  auto doubles = UseState(0);
  auto shake = UseShake();
  auto pop = UsePop();
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
    Row {
      Shaken(Text("Shake").With(Frame{.width = 120.0F, .height = 48.0F},
                                Background{Color::Rgb(178, 88, 88)}), shake)
          .Key("shake"),
      Button("Trigger").OnClick([shake] { shake.Trigger(); }).Key("shake-trigger"),
    }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
    Row {
      Text("Spotlight").With(Frame{.width = 160.0F, .height = 48.0F},
                             Background{Color::Rgb(60, 70, 90)},
                             Spotlight{SpotlightStyle{.diameter = 120.0F}})
          .Key("spotlight"),
      Text("Highlight").With(Frame{.width = 160.0F, .height = 48.0F},
                             Background{Color::Rgb(90, 100, 70)},
                             HoverHighlight{HoverHighlightStyle{}})
          .Key("highlight"),
      DragSpring(Text("Drag").With(Frame{.width = 100.0F, .height = 40.0F},
                                   Background{Color::Rgb(70, 110, 130)}))
          .With(Frame{.width = 320.0F, .height = 56.0F})
          .Key("dragspring"),
    }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
    Row {
      LongPressable(Text("Hold").With(Frame{.width = 120.0F, .height = 48.0F},
                                      Background{Color::Rgb(96, 110, 160)}, CornerRadius(10.0F)),
                    {}, [holds] { holds += 1; })
          .Key("hold"),
      DoubleClickable(Text("Double-click").With(Frame{.width = 140.0F, .height = 48.0F},
                                                 Background{Color::Rgb(120, 140, 90)}, CornerRadius(10.0F)),
                      {}, [doubles](const MultiTapEvent&) { doubles += 1; })
          .Key("double-click"),
      Popped(Text("Pop").With(Frame{.width = 80.0F, .height = 48.0F},
                             Background{Color::Rgb(160, 96, 140)}, CornerRadius(10.0F)), pop)
          .Key("pop"),
      Button("Pop trigger").OnClick([pop] { pop.Trigger(); }).Key("pop-trigger"),
    }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
    Text("clicks: " + std::to_string(clicks.Get())).Key("count"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

void CheckInteractionContracts() {
  Application app(InteractionGallery, {.window = {.initial_size = {980, 480}}, .show_debug_overlay = false});
  UiTest ui(app);
  CHECK(ui.Find(UiSelector::Key("pressable")).One().size == (Size{200, 64}));
  CHECK(ui.Find(UiSelector::Key("lift")).One().size == (Size{160, 48}));
  CHECK(ui.Find(UiSelector::Key("pulse")).One().size == (Size{80, 48}));
  CHECK(ui.Find(UiSelector::Key("ripple")).Exists());
  CHECK(ui.Find(UiSelector::Key("spotlight")).Exists());
  CHECK(ui.Find(UiSelector::Key("highlight")).Exists());
  CHECK(ui.Find(UiSelector::Key("dragspring")).One().size == (Size{320, 56}));
  CHECK(ui.Find(UiSelector::Key("hold")).Exists());
  CHECK(ui.Find(UiSelector::Key("double-click")).Exists());
  CHECK(ui.Find(UiSelector::Key("pop")).Exists());
  CHECK(ui.Find(UiSelector::Text("clicks: 0")).Exists());
  ui.Find(UiSelector::Key("pressable")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());
  ui.Find(UiSelector::Key("ripple")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());
  ui.Find(UiSelector::Key("shake-trigger")).Tap();
  ui.PumpAndSettle();
  ui.Find(UiSelector::Key("highlight")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("shake")).Exists());
  ui.Find(UiSelector::Key("pop-trigger")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("pop")).Exists());
}

#endif
