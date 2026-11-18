#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <chrono>
#include <string>
#include <variant>
#include <vector>

#ifdef INTERACTION_HAS_UI_TESTING
#include <huxerui/testing/ui_test.h>
using namespace huxerui::testing;

using namespace huxerui;
using namespace huxerui::interaction;

namespace {

View ScrollA11yVisualGallery() {
  auto scroll_controller = UseScrollController();
  auto rolling_val = UseState(0);
  auto semantic_count = UseState(0);
  auto shortcut_count = UseState(0);
  auto reveals = UseState(0);
  auto reveal = UseRevealOnTap();
  auto shake = UseShake();
  auto pop = UsePop();

  std::vector<View> rows;
  for(int i = 0; i < 16; ++i) {
    rows.push_back(Text("row " + std::to_string(i)).With(Frame{.height = 28.0F}).Key("srow-" + std::to_string(i)));
  }

  View scroll = ScrollView(Column(std::move(rows)).With(Spacing(6.0F)))
                    .Controller(scroll_controller)
                    .With(Frame{.width = 220.0F, .height = 120.0F},
                          ScrollProgress{scroll_controller, ScrollProgressStyle{}},
                          ScrollEdgeFade{scroll_controller, ScrollEdgeFadeStyle{.color = Color::White()}})
                    .Key("scroll");

  View semantic = Text("semantic target")
                      .With(Frame{.width = 160.0F, .height = 40.0F},
                            Semantics{.role = SemanticRole::Button, .label = "Semantic target"},
                            SemanticActions{Semantics{}, SemanticActionMask(SemanticActionKind::Activate),
                                            [semantic_count](SemanticActionKind) { semantic_count += 1; }});

  View root = Column {
    Row {
      std::move(scroll),
      Column {
        RollingNumber(rolling_val.Get(), {.motion = TweenSpec{.duration = 0.2}}).Key("rolling"),
        Button("Inc").OnClick([rolling_val] { rolling_val += 1; }).Key("inc"),
        Pulsing(Text("Pulse").With(Frame{.width = 80.0F, .height = 32.0F}), {.duration = 0.3}).Key("pulse"),
        Text("Shimmer").With(Frame{.width = 80.0F, .height = 32.0F}, Shimmer{}).Key("shimmer"),
      }.With(Spacing(8.0F)),
    }.With(Spacing(16.0F)),
    Row {
      std::move(semantic),
      Button("Focus").Key("focus-btn"),
      Button("Reveal")
          .OnClick([reveal, reveals] { reveal.Run([reveals] { reveals += 1; }); })
          .Key("reveal-btn"),
      Shaken(Text("Shake").With(Frame{.width = 80.0F, .height = 36.0F}), shake).Key("shake"),
      Button("Trigger shake").OnClick([shake] { shake.Trigger(); }).Key("shake-btn"),
      Popped(Text("Pop").With(Frame{.width = 80.0F, .height = 36.0F}), pop).Key("pop"),
      Button("Trigger pop").OnClick([pop] { pop.Trigger(); }).Key("pop-btn"),
    }.With(Spacing(8.0F)),
    Text("semantic: " + std::to_string(semantic_count.Get())),
    Text("shortcut: " + std::to_string(shortcut_count.Get())),
    Text("reveals: " + std::to_string(reveals.Get())),
  }.With(Padding(16.0F), Spacing(12.0F));

  return std::move(root)
         | KeyboardShortcut({.key = Key::K, .modifiers = KeyModifiers{.control = true}},
                            [shortcut_count] { shortcut_count += 1; });
}

} // namespace

void CheckScrollA11yAndVisualContracts() {
  Application app(ScrollA11yVisualGallery, {.window = {.initial_size = {820, 460}}, .show_debug_overlay = false});
  UiTest ui(app, {.viewport = {820, 460}});

  // ScrollProgress & ScrollEdgeFade over ScrollView.
  const Point scroll_center = Center(ui.Find(UiSelector::Key("scroll")).One().bounds);
  const float row0_before = ui.Find(UiSelector::Key("srow-0")).One().bounds.y;
  ui.SendScroll({.position = scroll_center, .delta_y = 120.0F});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("srow-0")).One().bounds.y < row0_before);

  // RollingNumber digit travel.
  const float zero_before = ui.Find(UiSelector::Key("rolling")).Find(UiSelector::Text("0")).One().bounds.y;
  ui.Find(UiSelector::Key("inc")).Tap();
  ui.PumpAndSettle();
  const float zero_after = ui.Find(UiSelector::Key("rolling")).Find(UiSelector::Text("0")).One().bounds.y;
  CHECK(zero_after < zero_before - 1.0F);

  // SemanticActions.
  ui.FindSemantics(UiSemanticSelector::Label("Semantic target"))
      .PerformSemanticAction({SemanticActionKind::Activate, std::monostate{}});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("semantic: 1")).Exists());

  // KeyboardShortcut via operator| pipeline.
  ui.Find(UiSelector::Key("focus-btn")).Tap();
  ui.PumpAndSettle();
  ui.PressKey(Key::K, KeyModifiers{.control = true});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("shortcut: 1")).Exists());

  // Shake, Pop, RevealOnTap.
  ui.Find(UiSelector::Key("shake-btn")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("shake")).Exists());

  ui.Find(UiSelector::Key("pop-btn")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("pop")).Exists());

  ui.Find(UiSelector::Key("reveal-btn")).Tap();
  ui.Pump(std::chrono::milliseconds(400));
  CHECK(ui.Find(UiSelector::Text("reveals: 1")).Exists());
}

#endif
