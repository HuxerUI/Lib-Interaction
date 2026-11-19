#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
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

  // NestedScrollConnection + ScrollHideController (consuming and non-consuming modes).
  {
    Application nested_app(
        []() -> View {
          auto hide_consume = UseScrollHideController({.extent = 48.0F, .consume_scroll = true});
          auto hide_observe = UseScrollHideController({.extent = 48.0F, .consume_scroll = false});
          std::vector<View> rows_a;
          std::vector<View> rows_b;
          for(int i = 0; i < 16; ++i) {
            rows_a.push_back(Text("a " + std::to_string(i)).With(Frame{.height = 28.0F}).Key("na-" + std::to_string(i)));
            rows_b.push_back(Text("b " + std::to_string(i)).With(Frame{.height = 28.0F}).Key("nb-" + std::to_string(i)));
          }
          return Column {
            Row {
              ScrollView(Column(std::move(rows_a)).With(Spacing(4.0F)))
                  .With(Frame{.width = 200.0F, .height = 120.0F}, hide_consume.Connection())
                  .Key("nscroll-a"),
              ScrollView(Column(std::move(rows_b)).With(Spacing(4.0F)))
                  .With(Frame{.width = 200.0F, .height = 120.0F}, hide_observe.Connection())
                  .Key("nscroll-b"),
            }.With(Spacing(16.0F)),
            Row {
              Button("Hide A").OnClick([hide_consume] { hide_consume.Hide(); }).Key("nhide-a"),
              Button("Show A").OnClick([hide_consume] { hide_consume.Show(); }).Key("nshow-a"),
            }.With(Spacing(8.0F)),
            Text("ca_off: " + std::to_string(static_cast<int>(hide_consume.HiddenOffset()))),
            Text("ca_hid: " + std::string(hide_consume.IsHidden() ? "yes" : "no")),
            Text("cb_off: " + std::to_string(static_cast<int>(hide_observe.HiddenOffset()))),
          }.With(Padding(16.0F), Spacing(10.0F));
        },
        {.window = {.initial_size = {520, 320}}, .show_debug_overlay = false});
    UiTest nui(nested_app, {.viewport = {520, 320}});

    const Point sa_center = Center(nui.Find(UiSelector::Key("nscroll-a")).One().bounds);
    const float na0_before = nui.Find(UiSelector::Key("na-0")).One().bounds.y;
    nui.SendScroll({.position = sa_center, .delta_y = 20.0F});
    nui.PumpAndSettle();
    CHECK(nui.Find(UiSelector::Text("ca_off: 20")).Exists());
    CHECK(nui.Find(UiSelector::Text("ca_hid: no")).Exists());
    CHECK(nui.Find(UiSelector::Key("na-0")).One().bounds.y == na0_before);

    nui.SendScroll({.position = sa_center, .delta_y = 60.0F});
    nui.PumpAndSettle();
    CHECK(nui.Find(UiSelector::Text("ca_off: 48")).Exists());
    CHECK(nui.Find(UiSelector::Text("ca_hid: yes")).Exists());
    CHECK(nui.Find(UiSelector::Key("na-0")).One().bounds.y < na0_before - 1.0F);

    nui.Find(UiSelector::Key("nshow-a")).Tap();
    nui.PumpAndSettle();
    CHECK(nui.Find(UiSelector::Text("ca_off: 0")).Exists());
    nui.Find(UiSelector::Key("nhide-a")).Tap();
    nui.PumpAndSettle();
    CHECK(nui.Find(UiSelector::Text("ca_hid: yes")).Exists());

    const Point sb_center = Center(nui.Find(UiSelector::Key("nscroll-b")).One().bounds);
    const float nb0_before = nui.Find(UiSelector::Key("nb-0")).One().bounds.y;
    nui.SendScroll({.position = sb_center, .delta_y = 20.0F});
    nui.PumpAndSettle();
    CHECK(nui.Find(UiSelector::Text("cb_off: 20")).Exists());
    CHECK(nui.Find(UiSelector::Key("nb-0")).One().bounds.y < nb0_before - 1.0F);
  }

  // Expandable (vertical & horizontal layout sizing + collapsed input exclusion) and Appear.
  {
    Application visual_app(
        []() -> View {
          auto exp_v = UseState(false);
          auto exp_h = UseState(false);
          auto app_vis = UseState(true);
          return Column {
            Row {
              Button("Toggle V").OnClick([exp_v] { exp_v = !exp_v.Get(); }).Key("toggle-v"),
              Button("Toggle H").OnClick([exp_h] { exp_h = !exp_h.Get(); }).Key("toggle-h"),
              Button("Toggle Appear").OnClick([app_vis] { app_vis = !app_vis.Get(); }).Key("toggle-appear"),
            }.With(Spacing(8.0F)),
            Expandable(
                Text("Vertical body").With(Frame{.width = 180.0F, .height = 60.0F}).Key("exp-v-child"),
                exp_v.Get(),
                {.axis = Axis::Vertical, .motion = TweenSpec{.duration = 0.08, .easing = Easing::Linear}})
                .Key("exp-v"),
            Expandable(
                Text("Horizontal body").With(Frame{.width = 120.0F, .height = 40.0F}).Key("exp-h-child"),
                exp_h.Get(),
                {.axis = Axis::Horizontal, .motion = TweenSpec{.duration = 0.08, .easing = Easing::Linear}})
                .Key("exp-h"),
            Appear(
                Text("Appear body").With(Frame{.width = 160.0F, .height = 40.0F}).Key("appear-child"),
                app_vis.Get(),
                {.motion = TweenSpec{.duration = 0.08, .easing = Easing::Linear}})
                .Key("appear-box"),
          }.With(Padding(16.0F), Spacing(10.0F));
        },
        {.window = {.initial_size = {520, 320}}, .show_debug_overlay = false});
    UiTest vui(visual_app, {.viewport = {520, 320}});

    CHECK(vui.Find(UiSelector::Key("exp-v")).One().size.height == 0.0F);
    CHECK(!vui.Find(UiSelector::Key("exp-v-child")).One().participates_in_layout);
    CHECK(vui.Find(UiSelector::Key("exp-h")).One().size.width == 0.0F);
    CHECK(!vui.Find(UiSelector::Key("exp-h-child")).One().participates_in_layout);
    CHECK(vui.Find(UiSelector::Key("appear-child")).Exists());

    vui.Find(UiSelector::Key("toggle-v")).Tap();
    vui.Find(UiSelector::Key("toggle-h")).Tap();
    vui.PumpAndSettle();
    CHECK(vui.Find(UiSelector::Key("exp-v")).One().size.height >= 59.0F);
    CHECK(vui.Find(UiSelector::Key("exp-v-child")).One().participates_in_layout);
    CHECK(vui.Find(UiSelector::Key("exp-h")).One().size.width >= 119.0F);
    CHECK(vui.Find(UiSelector::Key("exp-h-child")).One().participates_in_layout);

    vui.Find(UiSelector::Key("toggle-v")).Tap();
    vui.PumpAndSettle();
    CHECK(vui.Find(UiSelector::Key("exp-v")).One().size.height == 0.0F);
    CHECK(!vui.Find(UiSelector::Key("exp-v-child")).One().participates_in_layout);

    vui.Find(UiSelector::Key("toggle-appear")).Tap();
    vui.PumpAndSettle();
    CHECK(vui.Find(UiSelector::Key("appear-child")).Exists());
  }

  // ShortcutMap multi-binding + RovingFocusGroup / RovingFocusItem / RovingFocusController + SharedMorph.
  {
    Application a11y_morph_app(
        []() -> View {
          auto s_hits = UseState(0);
          auto e_hits = UseState(0);
          auto map_enabled = UseState(true);
          auto rf_changes = UseState(0);
          auto rf = UseRovingFocusController(0);
          auto morph = UseSharedMorph();
          auto morph_swapped = UseState(false);

          return Column {
            (Column {
              Row {
                Button("S Focus").Key("smap-focus"),
                Button("Toggle Map").OnClick([map_enabled] { map_enabled = !map_enabled.Get(); }).Key("smap-toggle"),
              }.With(Spacing(8.0F)),
              (Row {
                Button("Item 0") | RovingFocusItem(0),
                Button("Item 1").With(Enabled{false}) | RovingFocusItem(1),
                Button("Item 2") | RovingFocusItem(2),
              }.With(Spacing(8.0F))
               | RovingFocusGroup({.orientation = RovingFocusOrientation::Horizontal, .controller = rf},
                                  [rf_changes](std::size_t) { rf_changes += 1; })),
              Button("Focus 2").OnClick([rf] { rf.FocusIndex(2); }).Key("rf-jump-2"),
              (Row {
                morph_swapped.Get()
                    ? View(Stack{}.With(Frame{.width = 80.0F, .height = 28.0F}))
                    : (Text("pill").With(Frame{.width = 60.0F, .height = 28.0F}).Key("morph-pill")
                       | MorphElement("m-pill")),
                morph_swapped.Get()
                    ? (Text("pill").With(Frame{.width = 60.0F, .height = 28.0F}).Key("morph-pill")
                       | MorphElement("m-pill"))
                    : View(Stack{}.With(Frame{.width = 80.0F, .height = 28.0F})),
                Button("Swap Morph")
                    .OnClick([morph, morph_swapped] {
                      morph.Run([morph_swapped] { morph_swapped = !morph_swapped.Get(); });
                    })
                    .Key("morph-swap"),
              }.With(Spacing(8.0F))
               | MorphScope(morph)),
              Text("s_hits: " + std::to_string(s_hits.Get())),
              Text("e_hits: " + std::to_string(e_hits.Get())),
              Text("rf_active: " + std::to_string(rf.ActiveIndex())),
              Text("rf_changes: " + std::to_string(rf_changes.Get())),
            }.With(Padding(16.0F), Spacing(10.0F))
             | ShortcutMap(
                 {
                     {.chord = {.key = Key::S, .modifiers = {.control = true}},
                      .on_trigger = [s_hits] { s_hits += 1; }},
                     {.chord = {.key = Key::E, .modifiers = {.control = true}},
                      .on_trigger = [e_hits] { e_hits += 1; }},
                 },
                 {.enabled = map_enabled.Get()})),
          };
        },
        {.window = {.initial_size = {600, 380}}, .show_debug_overlay = false});
    UiTest aui(a11y_morph_app, {.viewport = {600, 380}});

    // ShortcutMap multi-chord dispatch + enabled toggle.
    aui.Find(UiSelector::Key("smap-focus")).Tap();
    aui.PumpAndSettle();
    aui.PressKey(Key::S, KeyModifiers{.control = true});
    aui.PressKey(Key::E, KeyModifiers{.control = true});
    aui.PumpAndSettle();
    CHECK(aui.Find(UiSelector::Text("s_hits: 1")).Exists());
    CHECK(aui.Find(UiSelector::Text("e_hits: 1")).Exists());

    aui.Find(UiSelector::Key("smap-toggle")).Tap();
    aui.PumpAndSettle();
    aui.PressKey(Key::S, KeyModifiers{.control = true});
    aui.PumpAndSettle();
    CHECK(aui.Find(UiSelector::Text("s_hits: 1")).Exists());

    // RovingFocus skips disabled index 1, wraps, supports Home/End and controller.FocusIndex.
    aui.Find(UiSelector::Text("Item 0")).Tap();
    aui.PumpAndSettle();
    CHECK(aui.Find(UiSelector::Text("rf_active: 0")).Exists());
    aui.PressKey(Key::ArrowRight);
    aui.PumpAndSettle();
    CHECK(aui.Find(UiSelector::Text("rf_active: 2")).Exists());
    CHECK(aui.Find(UiSelector::Text("Item 2")).One().focused);
    aui.PressKey(Key::ArrowRight);
    aui.PumpAndSettle();
    CHECK(aui.Find(UiSelector::Text("rf_active: 0")).Exists());
    CHECK(aui.Find(UiSelector::Text("Item 0")).One().focused);
    aui.PressKey(Key::End);
    aui.PumpAndSettle();
    CHECK(aui.Find(UiSelector::Text("rf_active: 2")).Exists());
    aui.PressKey(Key::Home);
    aui.PumpAndSettle();
    CHECK(aui.Find(UiSelector::Text("rf_active: 0")).Exists());
    aui.Find(UiSelector::Key("rf-jump-2")).Tap();
    aui.PumpAndSettle();
    CHECK(aui.Find(UiSelector::Text("rf_active: 2")).Exists());
    CHECK(aui.Find(UiSelector::Text("Item 2")).One().focused);

    // SharedMorph local transition.
    const float pill_before = aui.Find(UiSelector::Key("morph-pill")).One().bounds.x;
    aui.Find(UiSelector::Key("morph-swap")).Tap();
    aui.PumpAndSettle();
    const float pill_after = aui.Find(UiSelector::Key("morph-pill")).One().bounds.x;
    CHECK(pill_after > pill_before + 10.0F);
  }

  // BackHandler (nested inner-wins + enabled toggle) and PredictiveBack (Escape commit).
  {
    Application back_app(
        []() -> View {
          auto outer_hits = UseState(0);
          auto inner_hits = UseState(0);
          auto pb_commits = UseState(0);
          auto inner_enabled = UseState(true);
          auto outer_enabled = UseState(true);
          auto pb_enabled = UseState(false);
          auto pb_ctrl = UsePredictiveBackController();

          return (Column {
            Row {
              Button("Toggle Inner")
                  .OnClick([inner_enabled] { inner_enabled = !inner_enabled.Get(); })
                  .Key("bh-toggle-inner"),
              Button("Toggle Outer")
                  .OnClick([outer_enabled] { outer_enabled = !outer_enabled.Get(); })
                  .Key("bh-toggle-outer"),
              Button("Toggle Predictive")
                  .OnClick([pb_enabled] { pb_enabled = !pb_enabled.Get(); })
                  .Key("pb-toggle"),
            }.With(Spacing(8.0F)),
            (Text("inner surface").With(Frame{.width = 200.0F, .height = 36.0F})
             | BackHandler([inner_hits] { inner_hits += 1; }, {.enabled = inner_enabled.Get()})
             | PredictiveBack([pb_commits] { pb_commits += 1; },
                              {.enabled = pb_enabled.Get(), .controller = pb_ctrl})),
            Text("outer_hits: " + std::to_string(outer_hits.Get())),
            Text("inner_hits: " + std::to_string(inner_hits.Get())),
            Text("pb_commits: " + std::to_string(pb_commits.Get())),
            Text(std::string("pb_active: ") + (pb_ctrl.IsActive() ? "yes" : "no")),
            Text("pb_phase: " + std::to_string(static_cast<int>(pb_ctrl.Phase()))),
          }.With(Padding(16.0F), Spacing(8.0F))
           | BackHandler([outer_hits] { outer_hits += 1; }, {.enabled = outer_enabled.Get()}));
        },
        {.window = {.initial_size = {520, 260}}, .show_debug_overlay = false});
    UiTest bui(back_app, {.viewport = {520, 260}});

    // Inner BackHandler is a child inside PredictiveBack, so it intercepts before PredictiveBack and outer BackHandler.
    CHECK(bui.Find(UiSelector::Text("pb_active: no")).Exists());
    bui.PressKey(Key::Escape);
    bui.PumpAndSettle();
    CHECK(bui.Find(UiSelector::Text("inner_hits: 1")).Exists());
    CHECK(bui.Find(UiSelector::Text("outer_hits: 0")).Exists());

    // Disabling inner BackHandler and enabling PredictiveBack lets PredictiveBack intercept before outer BackHandler.
    bui.Find(UiSelector::Key("bh-toggle-inner")).Tap();
    bui.Find(UiSelector::Key("pb-toggle")).Tap();
    bui.PumpAndSettle();
    bui.PressKey(Key::Escape);
    bui.PumpAndSettle();
    CHECK(bui.Find(UiSelector::Text("pb_commits: 1")).Exists());
    CHECK(bui.Find(UiSelector::Text("inner_hits: 1")).Exists());
    CHECK(bui.Find(UiSelector::Text("outer_hits: 0")).Exists());
    CHECK(bui.Find(UiSelector::Text("pb_active: no")).Exists());
    CHECK(bui.Find(UiSelector::Text("pb_phase: " + std::to_string(static_cast<int>(BackPhase::Commit)))).Exists());

    // Disabling PredictiveBack falls through to outer BackHandler.
    bui.Find(UiSelector::Key("pb-toggle")).Tap();
    bui.PumpAndSettle();
    bui.PressKey(Key::Escape);
    bui.PumpAndSettle();
    CHECK(bui.Find(UiSelector::Text("outer_hits: 1")).Exists());
    CHECK(bui.Find(UiSelector::Text("inner_hits: 1")).Exists());
    CHECK(bui.Find(UiSelector::Text("pb_commits: 1")).Exists());

    // Disabling outer BackHandler leaves Escape unhandled by BackHandlers.
    bui.Find(UiSelector::Key("bh-toggle-outer")).Tap();
    bui.PumpAndSettle();
    bui.PressKey(Key::Escape);
    bui.PumpAndSettle();
    CHECK(bui.Find(UiSelector::Text("outer_hits: 1")).Exists());
  }

  // PredictiveBack full Begin / Update / Cancel / Commit transaction via UiWindow::HandleBack(BackEvent).
  {
    struct BackTestRuntime final : huxerui::Runtime {
      BackTestRuntime() : Runtime([this](std::function<void()> task) { pending.push_back(std::move(task)); }) {}
      ~BackTestRuntime() override { Retire(); }
      void Start(const huxerui::Application& application) { InitializeApplication(application); }
      using Runtime::Retire;
      std::vector<std::function<void()>> pending;
      void OnRuntimeStopped() override {}
    };

    struct BackTestWindow final : huxerui::UiWindow {
      ~BackTestWindow() override { Retire(); }
      using UiWindow::InitializeWindow;
      using UiWindow::Retire;
      void RequestFrameAt(double) override {}
      huxerui::FontMetrics Metrics(const huxerui::Font&) override { return {.ascent = 15.0F, .descent = 5.0F}; }
      huxerui::TextRunMetrics MeasureRun(std::string_view text, const huxerui::TextStyle&,
                                        const huxerui::TextShapingOptions&) override {
        const float w = static_cast<float>(text.size()) * 8.0F;
        return {w, {0.0F, -15.0F, w, 20.0F}, {.ascent = 15.0F, .descent = 5.0F}};
      }
      huxerui::TextLayoutMetrics MeasureText(const huxerui::AttributedText& text, const huxerui::TextStyle&,
                                           float max_w, const huxerui::TextLayoutOptions&) override {
        const float nat = static_cast<float>(text.PlainText().size()) * 8.0F;
        const float w = std::isfinite(max_w) ? std::min(nat, max_w) : nat;
        return {{w, 20.0F}, 15.0F, 15.0F, 1};
      }
    };

    static PredictiveBackController pb_ctrl_slot;
    static int pb_commit_count = 0;
    static int pb_cancel_count = 0;
    pb_ctrl_slot = PredictiveBackController{};
    pb_commit_count = 0;
    pb_cancel_count = 0;

    Application pb_app(
        []() -> View {
          auto ctrl = UsePredictiveBackController();
          pb_ctrl_slot = ctrl;
          return PredictiveBack(
              [](PredictiveBackSnapshot snap) {
                return Text(snap.active ? "predicting" : "idle").With(Frame{.width = 200.0F, .height = 60.0F});
              },
              [] { ++pb_commit_count; },
              {.controller = ctrl},
              [] { ++pb_cancel_count; });
        },
        {.window = {.initial_size = {360, 200}}, .show_debug_overlay = false});

    BackTestRuntime runtime;
    runtime.Start(pb_app);
    BackTestWindow window;
    window.InitializeWindow(runtime, pb_app.root_factory, pb_app.options.window);
    window.SetWindowMetrics({.viewport = {360.0F, 200.0F}});
    window.BuildFrame();

    CHECK(static_cast<bool>(pb_ctrl_slot));
    CHECK(!pb_ctrl_slot.IsActive());
    CHECK(Near(pb_ctrl_slot.Progress(), 0.0F));

    CHECK(window.HandleBack({BackPhase::Begin, 0.1F}));
    window.BuildFrame();
    CHECK(pb_ctrl_slot.IsActive());
    CHECK(pb_ctrl_slot.Phase() == BackPhase::Begin);
    CHECK(Near(pb_ctrl_slot.Progress(), 0.1F));

    CHECK(window.HandleBack({BackPhase::Update, 0.55F}));
    window.BuildFrame();
    CHECK(pb_ctrl_slot.IsActive());
    CHECK(pb_ctrl_slot.Phase() == BackPhase::Update);
    CHECK(Near(pb_ctrl_slot.Progress(), 0.55F));

    CHECK(window.HandleBack({BackPhase::Cancel, 0.0F}));
    window.BuildFrame();
    CHECK(!pb_ctrl_slot.IsActive());
    CHECK(pb_ctrl_slot.Phase() == BackPhase::Cancel);
    CHECK(Near(pb_ctrl_slot.Progress(), 0.0F));
    CHECK(pb_cancel_count == 1);
    CHECK(pb_commit_count == 0);

    CHECK(window.HandleBack({BackPhase::Begin, 0.0F}));
    CHECK(window.HandleBack({BackPhase::Update, 0.8F}));
    CHECK(window.HandleBack({BackPhase::Commit, 1.0F}));
    window.BuildFrame();
    CHECK(!pb_ctrl_slot.IsActive());
    CHECK(pb_ctrl_slot.Phase() == BackPhase::Commit);
    CHECK(Near(pb_ctrl_slot.Progress(), 0.0F));
    CHECK(pb_commit_count == 1);
    CHECK(pb_cancel_count == 1);

    pb_ctrl_slot = PredictiveBackController{};
    window.Retire();
    runtime.Retire();
  }
}

#endif
