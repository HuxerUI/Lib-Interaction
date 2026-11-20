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

  // ScrollSpy (vertical + horizontal active tracking, bottom-of-scroll rule, Smooth vs. Instant jumps).
  {
    Application spy_app(
        []() -> View {
          auto v_scroll = UseScrollController();
          auto h_scroll = UseScrollController();
          auto v_spy = UseScrollSpyController();
          auto h_spy = UseScrollSpyController();
          auto v_changes = UseState(0);
          auto v_mode = UseState(ScrollJumpBehavior::Smooth);

          std::vector<View> v_sections;
          for(std::size_t i = 0; i < 4; ++i) {
            v_sections.push_back(Text("VSec " + std::to_string(i))
                                     .With(Frame{.width = 200.0F, .height = i == 3 ? 60.0F : 120.0F})
                                     .Key("vsec-" + std::to_string(i))
                                 | ScrollSpySection(i));
          }

          std::vector<View> h_sections;
          for(std::size_t i = 0; i < 3; ++i) {
            h_sections.push_back(Text("HSec " + std::to_string(i))
                                     .With(Frame{.width = 140.0F, .height = 60.0F})
                                     .Key("hsec-" + std::to_string(i))
                                 | ScrollSpySection(i));
          }

          return Column {
            Row {
              (ScrollView(Column(std::move(v_sections)))
                   .Controller(v_scroll)
                   .With(Frame{.width = 220.0F, .height = 140.0F})
                   .Key("vspy-scroll")
               | ScrollSpyArea({.scroll_controller = v_scroll,
                                .activation_offset = 16.0F,
                                .jump_behavior = v_mode.Get(),
                                .scroll_motion = TweenSpec{.duration = 0.24, .easing = Easing::Linear},
                                .controller = v_spy},
                               [v_changes](std::size_t) { v_changes += 1; })),
              (ScrollView(Row(std::move(h_sections)))
                   .ScrollAxis(Axis::Horizontal)
                   .Controller(h_scroll)
                   .With(Frame{.width = 180.0F, .height = 80.0F})
                   .Key("hspy-scroll")
               | ScrollSpyArea({.scroll_controller = h_scroll,
                                .axis = Axis::Horizontal,
                                .activation_offset = 16.0F,
                                .controller = h_spy})),
            }.With(Spacing(16.0F)),
            Row {
              Button("Jump V0").OnClick([v_spy] { v_spy.ScrollToSection(0); }).Key("vjump-0"),
              Button("Jump V1").OnClick([v_spy] { v_spy.ScrollToSection(1); }).Key("vjump-1"),
              Button("Jump V2").OnClick([v_spy] { v_spy.ScrollToSection(2); }).Key("vjump-2"),
              Button("Jump V0 Instant")
                  .OnClick([v_spy] { v_spy.ScrollToSection(0, ScrollJumpBehavior::Instant); })
                  .Key("vjump-0-instant"),
              Button("Mode Toggle")
                  .OnClick([v_mode] {
                    v_mode = v_mode.Get() == ScrollJumpBehavior::Smooth ? ScrollJumpBehavior::Instant
                                                                        : ScrollJumpBehavior::Smooth;
                  })
                  .Key("v-mode-toggle"),
              Button("Jump H0").OnClick([h_spy] { h_spy.ScrollToSection(0); }).Key("hjump-0"),
            }.With(Spacing(8.0F)),
            Text("v_active: " + std::to_string(v_spy.ActiveSection())),
            Text("v_count: " + std::to_string(v_spy.SectionCount())),
            Text("v_changes: " + std::to_string(v_changes.Get())),
            Text("h_active: " + std::to_string(h_spy.ActiveSection())),
          }.With(Padding(16.0F), Spacing(10.0F));
        },
        {.window = {.initial_size = {640, 320}}, .show_debug_overlay = false});
    UiTest sui(spy_app, {.viewport = {640, 320}});
    sui.PumpAndSettle();

    CHECK(sui.Find(UiSelector::Text("v_active: 0")).Exists());
    CHECK(sui.Find(UiSelector::Text("v_count: 4")).Exists());
    CHECK(sui.Find(UiSelector::Text("h_active: 0")).Exists());
    const float vsec0_y0 = sui.Find(UiSelector::Key("vsec-0")).One().bounds.y;

    const Point v_center = Center(sui.Find(UiSelector::Key("vspy-scroll")).One().bounds);
    // Scroll down past Section 0 (height 120) -> Section 1 active.
    sui.SendScroll({.position = v_center, .delta_y = 120.0F});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("v_active: 1")).Exists());

    // Scroll down further to bottom -> short Section 3 becomes active via bottom-of-scroll rule.
    sui.SendScroll({.position = v_center, .delta_y = 400.0F});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("v_active: 3")).Exists());
    CHECK(sui.Find(UiSelector::Text("v_changes: 2")).Exists());

    // Programmatic jump back to Section 1 and Section 0.
    sui.Find(UiSelector::Key("vjump-1")).Tap();
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("v_active: 1")).Exists());

    sui.Find(UiSelector::Key("vjump-0")).Tap();
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("v_active: 0")).Exists());
    CHECK(Near(sui.Find(UiSelector::Key("vsec-0")).One().bounds.y, vsec0_y0));

    // Smooth jump from 0 -> Section 2 (240px): intermediate pumps stay between 0 and 240, then settle at 240.
    sui.Find(UiSelector::Key("vjump-2")).Tap();
    for(int f = 0; f < 4; ++f) {
      sui.Pump(std::chrono::milliseconds(16));
    }
    const float mid_y = sui.Find(UiSelector::Key("vsec-0")).One().bounds.y;
    CHECK(mid_y < vsec0_y0 - 10.0F);
    CHECK(mid_y > vsec0_y0 - 230.0F);
    sui.PumpAndSettle();
    CHECK(Near(sui.Find(UiSelector::Key("vsec-0")).One().bounds.y, vsec0_y0 - 240.0F));
    CHECK(sui.Find(UiSelector::Text("v_active: 2")).Exists());

    // User wheel input mid-animation cancels Smooth jump cleanly.
    sui.Find(UiSelector::Key("vjump-0")).Tap();
    sui.Pump(std::chrono::milliseconds(16));
    sui.Pump(std::chrono::milliseconds(16));
    sui.SendScroll({.position = v_center, .delta_y = 60.0F});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Key("vsec-0")).One().bounds.y < vsec0_y0 - 50.0F);

    // Per-call ScrollJumpBehavior::Instant override lands in a single 0ms Pump().
    sui.Find(UiSelector::Key("vjump-0-instant")).Tap();
    sui.Pump();
    CHECK(Near(sui.Find(UiSelector::Key("vsec-0")).One().bounds.y, vsec0_y0));
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("v_active: 0")).Exists());

    // ScrollSpyStyle::jump_behavior == Instant lands in a single 0ms Pump().
    sui.Find(UiSelector::Key("v-mode-toggle")).Tap();
    sui.PumpAndSettle();
    sui.Find(UiSelector::Key("vjump-2")).Tap();
    sui.Pump();
    CHECK(Near(sui.Find(UiSelector::Key("vsec-0")).One().bounds.y, vsec0_y0 - 240.0F));
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("v_active: 2")).Exists());

    // Horizontal axis ScrollSpy.
    const Point h_center = Center(sui.Find(UiSelector::Key("hspy-scroll")).One().bounds);
    sui.SendScroll({.position = h_center, .delta_x = 140.0F});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("h_active: 1")).Exists());

    sui.Find(UiSelector::Key("hjump-0")).Tap();
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("h_active: 0")).Exists());
  }

  // StickyHeader (unpinned at rest -> pinned -> push-up collision -> switch -> Instant/Smooth jump ->
  // pin_at_rest + sticky_offset -> Horizontal axis).
  {
    Application sticky_app(
        []() -> View {
          auto v_scroll = UseScrollController();
          auto v_sticky = UseStickyHeaderController();
          auto v_changes = UseState(0);

          auto r_scroll = UseScrollController();
          auto r_sticky = UseStickyHeaderController();

          auto h_scroll = UseScrollController();
          auto h_sticky = UseStickyHeaderController();

          std::vector<StickyHeaderGroup> v_groups;
          for(std::size_t i = 0; i < 3; ++i) {
            v_groups.push_back(StickyHeaderGroup{
                .header =
                    [i](StickyHeaderSnapshot s) -> View {
                  return Stack {
                    Text((s.pinned ? "vpin:" : "vflow:") + std::to_string(s.index)),
                  }
                      .With(Frame{.height = 30.0F}, Background{Color::Rgb(50, 90, 150)})
                      .Key((s.pinned ? "v-pin-hdr-" : "v-flow-hdr-") + std::to_string(s.index));
                },
                .body = Stack {Text("vbody:" + std::to_string(i))}.With(Frame{.height = 120.0F}),
            });
          }

          std::vector<StickyHeaderGroup> h_groups;
          for(std::size_t i = 0; i < 3; ++i) {
            h_groups.push_back(StickyHeaderGroup{
                .header =
                    [i](StickyHeaderSnapshot s) -> View {
                  return Stack {
                    Text((s.pinned ? "hpin:" : "hflow:") + std::to_string(s.index)),
                  }
                      .With(Frame{.width = 40.0F}, Background{Color::Rgb(80, 120, 90)})
                      .Key((s.pinned ? "h-pin-hdr-" : "h-flow-hdr-") + std::to_string(s.index));
                },
                .body = Stack {Text("hbody:" + std::to_string(i))}.With(Frame{.width = 110.0F}),
            });
          }

          const auto vpinned = v_sticky.PinnedSection();
          const auto rpinned = r_sticky.PinnedSection();
          const auto hpinned = h_sticky.PinnedSection();
          const int push_pct = static_cast<int>(v_sticky.PushProgress() * 100.0F);

          return Column {
            Row {
              StickyHeaderList(
                  std::move(v_groups),
                  StickyHeaderStyle{
                      .scroll_controller = v_scroll,
                      .controller = v_sticky,
                  },
                  [v_changes](std::optional<std::size_t>) { v_changes += 1; })
                  .With(Frame{.width = 220.0F, .height = 140.0F})
                  .Key("v-sticky-box"),
              (ScrollView(Column {
                 (Column {
                   Stack {Text("rest-flow-0")}.With(Frame{.height = 28.0F}),
                   Stack {Text("rest-body-0")}.With(Frame{.height = 120.0F}),
                 } | StickyHeaderSection(0)),
                 (Column {
                   Stack {Text("rest-flow-1")}.With(Frame{.height = 28.0F}),
                   Stack {Text("rest-body-1")}.With(Frame{.height = 120.0F}),
                 } | StickyHeaderSection(1)),
               })
                   .Controller(r_scroll)
                   .With(Frame{.width = 200.0F, .height = 140.0F})
               | StickyHeaderArea(
                   [](StickyHeaderSnapshot s) -> View {
                     return Stack {Text("rest-pin:" + std::to_string(s.index))}
                         .With(Frame{.height = 28.0F})
                         .Key("rest-pin-" + std::to_string(s.index));
                   },
                   StickyHeaderStyle{
                       .scroll_controller = r_scroll,
                       .sticky_offset = 12.0F,
                       .pin_at_rest = true,
                       .controller = r_sticky,
                   }))
                  .Key("rest-sticky-box"),
              StickyHeaderList(
                  std::move(h_groups),
                  StickyHeaderStyle{
                      .scroll_controller = h_scroll,
                      .axis = Axis::Horizontal,
                      .controller = h_sticky,
                  })
                  .With(Frame{.width = 200.0F, .height = 100.0F})
                  .Key("h-sticky-box"),
            }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Start)),
            Row {
              Button("VJump2").OnClick([v_sticky] { v_sticky.ScrollToSection(2, ScrollJumpBehavior::Instant); }).Key("vsticky-jump2"),
              Button("VJump0").OnClick([v_sticky] { v_sticky.ScrollToSection(0, ScrollJumpBehavior::Smooth); }).Key("vsticky-jump0"),
              Button("VPush").OnClick([v_scroll] { (void)v_scroll.ScrollTo(135.0F); }).Key("vsticky-push"),
            }.With(Spacing(8.0F)),
            Text("vpinned: " + (vpinned.has_value() ? std::to_string(*vpinned) : std::string("none"))),
            Text("vcount: " + std::to_string(v_sticky.SectionCount())),
            Text("vchanges: " + std::to_string(v_changes.Get())),
            Text("vpush_pct: " + std::to_string(push_pct)),
            Text("rpinned: " + (rpinned.has_value() ? std::to_string(*rpinned) : std::string("none"))),
            Text("hpinned: " + (hpinned.has_value() ? std::to_string(*hpinned) : std::string("none"))),
          }.With(Padding(16.0F), Spacing(10.0F));
        },
        {.window = {.initial_size = {720, 380}}, .show_debug_overlay = false});
    UiTest ui(sticky_app, {.viewport = {720, 380}});
    ui.PumpAndSettle();

    const Rect v_box = ui.Find(UiSelector::Key("v-sticky-box")).One().bounds;
    const Rect r_box = ui.Find(UiSelector::Key("rest-sticky-box")).One().bounds;
    const Rect h_box = ui.Find(UiSelector::Key("h-sticky-box")).One().bounds;

    // Initial unpinned state at offset 0 (pin_at_rest = false), while pin_at_rest = true pins at sticky_offset.
    CHECK(ui.Find(UiSelector::Text("vpinned: none")).Exists());
    CHECK(ui.Find(UiSelector::Text("vcount: 3")).Exists());
    CHECK(!ui.Find(UiSelector::Key("v-pin-hdr-0")).Exists());
    CHECK(ui.Find(UiSelector::Text("rpinned: 0")).Exists());
    CHECK(ui.Find(UiSelector::Key("rest-pin-0")).Exists());
    CHECK(Near(ui.Find(UiSelector::Key("rest-pin-0")).One().bounds.y, r_box.y + 12.0F));

    // Scroll into Section 0 -> pins Section 0 flush at viewport top.
    const Point v_center = Center(v_box);
    ui.SendScroll({.position = v_center, .delta_y = 60.0F});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("vpinned: 0")).Exists());
    CHECK(ui.Find(UiSelector::Key("v-pin-hdr-0")).Exists());
    CHECK(Near(ui.Find(UiSelector::Key("v-pin-hdr-0")).One().bounds.y, v_box.y));
    CHECK(ui.Find(UiSelector::Text("vpush_pct: 0")).Exists());

    // Scroll into push-up collision (offset 135; Section 1 starts at 150, header height 30 -> ~15px push upward).
    ui.Find(UiSelector::Key("vsticky-push")).Tap();
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("vpinned: 0")).Exists());
    const float pushed_y = ui.Find(UiSelector::Key("v-pin-hdr-0")).One().bounds.y;
    CHECK(pushed_y < v_box.y - 10.0F);
    CHECK(pushed_y > v_box.y - 20.0F);
    CHECK(ui.Find(UiSelector::Text("vpush_pct: 50")).Exists());

    // Scroll past Section 1 boundary -> Section 1 takes over cleanly at v_box.y with 0 push.
    ui.SendScroll({.position = v_center, .delta_y = 30.0F});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("vpinned: 1")).Exists());
    CHECK(!ui.Find(UiSelector::Key("v-pin-hdr-0")).Exists());
    CHECK(ui.Find(UiSelector::Key("v-pin-hdr-1")).Exists());
    CHECK(Near(ui.Find(UiSelector::Key("v-pin-hdr-1")).One().bounds.y, v_box.y));
    CHECK(ui.Find(UiSelector::Text("vpush_pct: 0")).Exists());

    // Instant jump to Section 2 lands immediately in a single 0ms Pump().
    ui.Find(UiSelector::Key("vsticky-jump2")).Tap();
    ui.Pump();
    CHECK(ui.Find(UiSelector::Key("v-pin-hdr-2")).Exists());
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("vpinned: 2")).Exists());

    // Smooth jump back to Section 0 animates smoothly before settling unpinned at offset 0.
    ui.Find(UiSelector::Key("vsticky-jump0")).Tap();
    for(int f = 0; f < 4; ++f) {
      ui.Pump(std::chrono::milliseconds(16));
    }
    CHECK(ui.Find(UiSelector::Key("v-pin-hdr-0")).Exists() || ui.Find(UiSelector::Key("v-pin-hdr-1")).Exists());
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("vpinned: none")).Exists());

    // Horizontal axis StickyHeaderList pins along X and pushes left before Section 1.
    const Point h_center = Center(h_box);
    ui.SendScroll({.position = h_center, .delta_x = 50.0F});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("hpinned: 0")).Exists());
    CHECK(ui.Find(UiSelector::Key("h-pin-hdr-0")).Exists());
    CHECK(Near(ui.Find(UiSelector::Key("h-pin-hdr-0")).One().bounds.x, h_box.x));

    ui.SendScroll({.position = h_center, .delta_x = 80.0F});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("hpinned: 0")).Exists());
    CHECK(ui.Find(UiSelector::Key("h-pin-hdr-0")).One().bounds.x < h_box.x - 10.0F);

    ui.SendScroll({.position = h_center, .delta_x = 30.0F});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("hpinned: 1")).Exists());
    CHECK(Near(ui.Find(UiSelector::Key("h-pin-hdr-1")).One().bounds.x, h_box.x));
  }

  // ScrollParallax (ScrollParallaxArea vertical progress/scale/opacity + positive vs negative speed +
  // standalone ScrollController hero shift + Horizontal axis).
  {
    Application parallax_app(
        []() -> View {
          auto v_scroll = UseScrollController();
          auto hero_scroll = UseScrollController();
          auto h_scroll = UseScrollController();

          return Column {
            Row {
              (ScrollView(Column {
                 Stack {}.With(Frame{.width = 180.0F, .height = 100.0F}),
                 ScrollParallax(
                     [](ScrollParallaxSnapshot s) -> View {
                       return Text("vp:" + std::to_string(static_cast<int>(s.progress * 100.0F)) + ":op:" +
                                   std::to_string(static_cast<int>(s.opacity * 100.0F)))
                           .With(Frame{.width = 160.0F, .height = 40.0F}, Offset(s.offset), Scale(s.scale),
                                 Opacity(s.opacity))
                           .Key("vpar-inner");
                     },
                     {.speed = 1.0F, .maximum_offset = 20.0F, .center_scale = 1.2F, .edge_opacity = 0.4F})
                     .Key("vpar-outer"),
                 (Stack {Text("neg")}.With(Frame{.width = 160.0F, .height = 40.0F}).Key("vpar-neg-inner")
                  | ScrollParallax({.speed = -1.0F, .maximum_offset = 20.0F}))
                     .Key("vpar-neg-outer"),
                 Stack {}.With(Frame{.width = 180.0F, .height = 220.0F}),
               })
                   .Controller(v_scroll)
                   .With(Frame{.width = 200.0F, .height = 140.0F})
                   .Key("vpar-scroll")
               | ScrollParallaxArea({.scroll_controller = v_scroll})),
              ScrollView(Column {
                (Stack {Text("hero")}.With(Frame{.width = 160.0F, .height = 60.0F}).Key("hero-inner")
                 | ScrollParallax({.scroll_controller = hero_scroll,
                                   .speed = 0.5F,
                                   .maximum_offset = 60.0F,
                                   .clip_to_bounds = true}))
                    .Key("hero-outer"),
                Stack {}.With(Frame{.width = 160.0F, .height = 300.0F}),
              })
                  .Controller(hero_scroll)
                  .With(Frame{.width = 180.0F, .height = 140.0F})
                  .Key("hero-scroll"),
              (ScrollView(Row {
                 Stack {}.With(Frame{.width = 120.0F, .height = 80.0F}),
                 (Stack {Text("hitem")}.With(Frame{.width = 40.0F, .height = 80.0F}).Key("hpar-inner")
                  | ScrollParallax({.speed = 1.0F, .maximum_offset = 24.0F}))
                     .Key("hpar-outer"),
                 Stack {}.With(Frame{.width = 200.0F, .height = 80.0F}),
               })
                   .ScrollAxis(Axis::Horizontal)
                   .Controller(h_scroll)
                   .With(Frame{.width = 160.0F, .height = 100.0F})
                   .Key("hpar-scroll")
               | ScrollParallaxArea({.scroll_controller = h_scroll, .axis = Axis::Horizontal})),
            }.With(Spacing(16.0F)),
            Row {
              Button("Hero80").OnClick([hero_scroll] { (void)hero_scroll.ScrollTo(80.0F); }).Key("hero-80"),
              Button("Hero200").OnClick([hero_scroll] { (void)hero_scroll.ScrollTo(200.0F); }).Key("hero-200"),
            }.With(Spacing(8.0F)),
          }.With(Padding(16.0F), Spacing(10.0F));
        },
        {.window = {.initial_size = {660, 260}}, .show_debug_overlay = false});
    UiTest pui(parallax_app, {.viewport = {660, 260}});
    pui.PumpAndSettle();

    // Area mode (Vertical): item spans [100, 140] in [0, 140] viewport (center 120 > vp_center 70 -> progress > 0).
    const Rect v_outer0 = pui.Find(UiSelector::Key("vpar-outer")).One().bounds;
    const Rect v_inner0 = pui.Find(UiSelector::Key("vpar-inner")).One().bounds;
    CHECK(Center(v_inner0).y > Center(v_outer0).y + 2.0F);

    // Opposite sign speed shifts in the opposite direction from stationary outer bounds.
    const Rect neg_outer0 = pui.Find(UiSelector::Key("vpar-neg-outer")).One().bounds;
    const Rect neg_inner0 = pui.Find(UiSelector::Key("vpar-neg-inner")).One().bounds;
    CHECK(Center(neg_inner0).y < Center(neg_outer0).y - 2.0F);

    // Scroll down 50px so item [100, 140] centers at [50, 90] (center 70 == vp_center 70 -> progress == 0).
    const Point vp_center = Center(pui.Find(UiSelector::Key("vpar-scroll")).One().bounds);
    pui.SendScroll({.position = vp_center, .delta_y = 50.0F});
    pui.PumpAndSettle();
    const Rect v_outer1 = pui.Find(UiSelector::Key("vpar-outer")).One().bounds;
    const Rect v_inner1 = pui.Find(UiSelector::Key("vpar-inner")).One().bounds;
    CHECK(Near(Center(v_inner1).y, Center(v_outer1).y));
    CHECK(v_inner1.width > v_outer1.width * 1.15F);
    CHECK(pui.Find(UiSelector::Text("vp:0:op:100")).Exists());

    // Scroll further down (+60px -> total 110px) so item exits top (center 10 < vp_center 70 -> progress < 0).
    pui.SendScroll({.position = vp_center, .delta_y = 60.0F});
    pui.PumpAndSettle();
    const Rect v_outer2 = pui.Find(UiSelector::Key("vpar-outer")).One().bounds;
    const Rect v_inner2 = pui.Find(UiSelector::Key("vpar-inner")).One().bounds;
    CHECK(Center(v_inner2).y < Center(v_outer2).y - 2.0F);

    // Standalone ScrollController mode: hero banner shifts +0.5*offset (clamped at 60px).
    const float hero_y0 = pui.Find(UiSelector::Key("hero-inner")).One().bounds.y;
    pui.Find(UiSelector::Key("hero-80")).Tap();
    pui.PumpAndSettle();
    CHECK(Near(pui.Find(UiSelector::Key("hero-inner")).One().bounds.y, hero_y0 - 40.0F));
    pui.Find(UiSelector::Key("hero-200")).Tap();
    pui.PumpAndSettle();
    CHECK(Near(pui.Find(UiSelector::Key("hero-inner")).One().bounds.y, hero_y0 - 140.0F));

    // Horizontal ScrollParallaxArea shifts along X-axis across scroll.
    const Rect h_outer0 = pui.Find(UiSelector::Key("hpar-outer")).One().bounds;
    const Rect h_inner0 = pui.Find(UiSelector::Key("hpar-inner")).One().bounds;
    CHECK(Center(h_inner0).x > Center(h_outer0).x + 2.0F);
    const Point hp_center = Center(pui.Find(UiSelector::Key("hpar-scroll")).One().bounds);
    pui.SendScroll({.position = hp_center, .delta_x = 120.0F});
    pui.PumpAndSettle();
    const Rect h_outer1 = pui.Find(UiSelector::Key("hpar-outer")).One().bounds;
    const Rect h_inner1 = pui.Find(UiSelector::Key("hpar-inner")).One().bounds;
    CHECK(Center(h_inner1).x < Center(h_outer1).x - 2.0F);
  }

  // ScrollReveal (initial visible, once=true threshold, once=false hide/reveal, stagger delay,
  // viewport_margin expansion, Horizontal axis).
  {
    Application reveal_app(
        []() -> View {
          auto v_scroll = UseScrollController();
          auto m_scroll = UseScrollController();
          auto h_scroll = UseScrollController();

          auto item0_changes = UseState(0);
          auto item1_changes = UseState(0);
          auto item2_changes = UseState(0);

          return Column {
            Row {
              (ScrollView(Column {
                 ScrollReveal(
                     [](ScrollRevealSnapshot s) -> View {
                       return Text(std::string("s0:") + (s.visible ? "1" : "0") + ":" + (s.in_viewport ? "1" : "0"))
                           .With(Frame{.width = 180.0F, .height = 60.0F});
                     },
                     {.threshold = 0.2F, .once = true}, [item0_changes](bool) { item0_changes += 1; }),
                 Stack {}.With(Frame{.width = 180.0F, .height = 80.0F}),
                 ScrollReveal(
                     [](ScrollRevealSnapshot s) -> View {
                       return Text(std::string("s1:") + (s.visible ? "1" : "0") + ":" + (s.in_viewport ? "1" : "0"))
                           .With(Frame{.width = 180.0F, .height = 100.0F});
                     },
                     {.threshold = 0.5F, .once = true}, [item1_changes](bool) { item1_changes += 1; }),
                 ScrollReveal(
                     [](ScrollRevealSnapshot s) -> View {
                       return Text(std::string("s2:") + (s.visible ? "1" : "0") + ":" + (s.in_viewport ? "1" : "0"))
                           .With(Frame{.width = 180.0F, .height = 80.0F});
                     },
                     {.threshold = 0.25F, .once = false}, [item2_changes](bool) { item2_changes += 1; }),
                 ScrollReveal(
                     [](ScrollRevealSnapshot s) -> View {
                       return Text(std::string("s3:") + (s.visible ? "1" : "0") + ":" + (s.in_viewport ? "1" : "0"))
                           .With(Frame{.width = 180.0F, .height = 60.0F});
                     },
                     {.threshold = 0.15F, .once = true, .delay = 0.2}),
               })
                   .Controller(v_scroll)
                   .With(Frame{.width = 200.0F, .height = 140.0F})
                   .Key("vrev-scroll")
               | ScrollRevealArea({.scroll_controller = v_scroll})),
              (ScrollView(Column {
                 Stack {}.With(Frame{.width = 160.0F, .height = 140.0F}),
                 ScrollReveal(
                     [](ScrollRevealSnapshot s) -> View {
                       return Text(std::string("mvis:") + (s.visible ? "1" : "0"))
                           .With(Frame{.width = 160.0F, .height = 40.0F});
                     },
                     {.threshold = 0.2F, .once = true}),
               })
                   .Controller(m_scroll)
                   .With(Frame{.width = 180.0F, .height = 100.0F})
               | ScrollRevealArea({.scroll_controller = m_scroll, .viewport_margin = 60.0F})),
              (ScrollView(Row {
                 Stack {}.With(Frame{.width = 200.0F, .height = 60.0F}),
                 ScrollReveal(
                     [](ScrollRevealSnapshot s) -> View {
                       return Text(std::string("hvis:") + (s.visible ? "1" : "0"))
                           .With(Frame{.width = 60.0F, .height = 60.0F});
                     },
                     {.threshold = 0.25F, .once = true}),
               })
                   .ScrollAxis(Axis::Horizontal)
                   .Controller(h_scroll)
                   .With(Frame{.width = 160.0F, .height = 80.0F})
                   .Key("hrev-scroll")
               | ScrollRevealArea({.scroll_controller = h_scroll, .axis = Axis::Horizontal})),
            }.With(Spacing(16.0F)),
            Row {
              Button("V0").OnClick([v_scroll] { (void)v_scroll.ScrollTo(0.0F); }).Key("vrev-0"),
              Button("V140").OnClick([v_scroll] { (void)v_scroll.ScrollTo(140.0F); }).Key("vrev-140"),
              Button("V240").OnClick([v_scroll] { (void)v_scroll.ScrollTo(240.0F); }).Key("vrev-240"),
            }.With(Spacing(8.0F)),
            Text("c0: " + std::to_string(item0_changes.Get())),
            Text("c1: " + std::to_string(item1_changes.Get())),
            Text("c2: " + std::to_string(item2_changes.Get())),
          }.With(Padding(16.0F), Spacing(10.0F));
        },
        {.window = {.initial_size = {660, 280}}, .show_debug_overlay = false});
    UiTest rui(reveal_app, {.viewport = {660, 280}});
    rui.PumpAndSettle();

    // Item 0 & positive viewport_margin item reveal immediately at offset 0; off-screen items stay hidden.
    CHECK(rui.Find(UiSelector::Text("s0:1:1")).Exists());
    CHECK(rui.Find(UiSelector::Text("c0: 1")).Exists());
    CHECK(rui.Find(UiSelector::Text("s1:0:0")).Exists());
    CHECK(rui.Find(UiSelector::Text("s2:0:0")).Exists());
    CHECK(rui.Find(UiSelector::Text("s3:0:0")).Exists());
    CHECK(rui.Find(UiSelector::Text("mvis:1")).Exists());
    CHECK(rui.Find(UiSelector::Text("hvis:0")).Exists());

    // Item 1 (height 100 at [140, 240], threshold 0.5): 20% intersection stays unrevealed.
    const Point v_center = Center(rui.Find(UiSelector::Key("vrev-scroll")).One().bounds);
    rui.SendScroll({.position = v_center, .delta_y = 20.0F});
    rui.PumpAndSettle();
    CHECK(rui.Find(UiSelector::Text("s1:0:0")).Exists());
    CHECK(rui.Find(UiSelector::Text("c1: 0")).Exists());

    // 60% intersection crosses threshold 0.5 -> reveals s1; scrolling back out keeps s1 revealed (once=true).
    rui.SendScroll({.position = v_center, .delta_y = 40.0F});
    rui.PumpAndSettle();
    CHECK(rui.Find(UiSelector::Text("s1:1:1")).Exists());
    CHECK(rui.Find(UiSelector::Text("c1: 1")).Exists());

    rui.Find(UiSelector::Key("vrev-0")).Tap();
    rui.PumpAndSettle();
    CHECK(rui.Find(UiSelector::Text("s1:1:0")).Exists());
    CHECK(rui.Find(UiSelector::Text("c1: 1")).Exists());

    // Item 2 (once=false): reveals when scrolled in, hides again when scrolled out.
    rui.Find(UiSelector::Key("vrev-140")).Tap();
    rui.PumpAndSettle();
    CHECK(rui.Find(UiSelector::Text("s2:1:1")).Exists());
    CHECK(rui.Find(UiSelector::Text("c2: 1")).Exists());
    rui.Find(UiSelector::Key("vrev-0")).Tap();
    rui.PumpAndSettle();
    CHECK(rui.Find(UiSelector::Text("s2:0:0")).Exists());
    CHECK(rui.Find(UiSelector::Text("c2: 2")).Exists());

    // Item 3 (delay=0.2s): stays hidden right after entering viewport and after 100ms, reveals after 200ms.
    rui.Find(UiSelector::Key("vrev-240")).Tap();
    rui.Pump();
    rui.Pump();
    CHECK(rui.Find(UiSelector::Text("s3:0:1")).Exists());
    rui.Pump(std::chrono::milliseconds(100));
    rui.Pump();
    CHECK(rui.Find(UiSelector::Text("s3:0:1")).Exists());
    rui.Pump(std::chrono::milliseconds(150));
    rui.Pump();
    CHECK(rui.Find(UiSelector::Text("s3:1:1")).Exists());

    // Horizontal axis ScrollReveal.
    const Point h_center = Center(rui.Find(UiSelector::Key("hrev-scroll")).One().bounds);
    rui.SendScroll({.position = h_center, .delta_x = 120.0F});
    rui.PumpAndSettle();
    CHECK(rui.Find(UiSelector::Text("hvis:1")).Exists());
  }

  // ScrollToTop: threshold visibility, margin alignment, smooth animation, user interruption,
  // instant non-zero target, ScrollToTopTrigger (env & explicit controller), and show_on_scroll_up.
  {
    Application top_app(
        []() -> View {
          auto sc = UseScrollController();
          auto top_ctrl = UseScrollToTopController();
          auto wrapped_hits = UseState(0);
          auto inline_trigger_hits = UseState(0);
          auto ext_trigger_hits = UseState(0);
          const auto off = static_cast<int>(std::lround(top_ctrl.Offset()));
          const auto pct = static_cast<int>(std::lround(top_ctrl.Progress() * 100.0F));
          const int vis = top_ctrl.IsVisible() ? 1 : 0;
          const int anim = top_ctrl.IsScrollingToTop() ? 1 : 0;
          return Column {
            Row {
              Text("tvis:" + std::to_string(vis) + " tanim:" + std::to_string(anim) + " toff:" + std::to_string(off)
                   + " tpct:" + std::to_string(pct)),
              Text("thits:" + std::to_string(wrapped_hits.Get()) + ":" + std::to_string(ext_trigger_hits.Get())
                   + ":" + std::to_string(inline_trigger_hits.Get())),
              ScrollToTopTrigger(Text("ext-jump").Key("top-ext-trigger"), top_ctrl, ScrollJumpBehavior::Instant,
                                 [ext_trigger_hits] { ext_trigger_hits = ext_trigger_hits.Get() + 1; }),
            }.With(Spacing(8.0F)),
            ScrollToTop(
                ScrollView(
                    Column {
                      Stack{ScrollToTopTrigger(Text("in-jump").Key("top-inline-trigger"), {},
                                               ScrollJumpBehavior::Instant,
                                               [inline_trigger_hits] {
                                                 inline_trigger_hits = inline_trigger_hits.Get() + 1;
                                               })}
                          .With(Frame{.height = 40.0F}),
                      Stack{Text("row-1")}.With(Frame{.height = 120.0F}),
                      Stack{Text("row-2")}.With(Frame{.height = 120.0F}),
                      Stack{Text("row-3")}.With(Frame{.height = 120.0F}),
                      Stack{Text("row-4")}.With(Frame{.height = 120.0F}),
                    }.With(CrossAlign(CrossAxisAlignment::Stretch)))
                    .Controller(sc)
                    .Key("top-scroll")
                    .With(Frame{.width = 240.0F, .height = 120.0F}),
                Stack{Text("TOP").Key("top-fab-label")}.Key("top-fab").With(Frame{.width = 50.0F, .height = 24.0F}),
                {
                    .scroll_controller = sc,
                    .threshold = 100.0F,
                    .target_offset = 0.0F,
                    .jump_behavior = ScrollJumpBehavior::Smooth,
                    .scroll_motion = TweenSpec{.duration = 0.25, .easing = Easing::Linear},
                    .reveal_motion = {.motion = TweenSpec{.duration = 0.0}, .animate_on_mount = false},
                    .horizontal_alignment = HorizontalAlignment::End,
                    .vertical_alignment = VerticalAlignment::End,
                    .margin = {.top = 8.0F, .right = 12.0F, .bottom = 14.0F, .left = 10.0F},
                    .controller = top_ctrl,
                },
                [wrapped_hits] { wrapped_hits = wrapped_hits.Get() + 1; })
                .Key("top-stage"),
          }.With(Spacing(6.0F));
        },
        {.window = {.initial_size = {420, 300}}, .show_debug_overlay = false});
    UiTest tui(top_app, {.viewport = {420, 300}});

    tui.PumpAndSettle();
    const Rect vp = tui.Find(UiSelector::Key("top-scroll")).One().bounds;
    const Point sc_center = Center(vp);

    // Initially at offset 0 (< threshold 100): hidden and non-mounted.
    CHECK(tui.Find(UiSelector::Text("tvis:0 tanim:0 toff:0 tpct:0")).Exists());
    CHECK(!tui.Find(UiSelector::Key("top-fab")).Exists());

    // Scroll 180px down (>= threshold 100): visible, aligned with bottom-right margin (right=12, bottom=14).
    tui.SendScroll({.position = sc_center, .delta_y = 180.0F});
    tui.PumpAndSettle();
    CHECK(tui.Find(UiSelector::Key("top-fab")).Exists());
    const Rect fab_bounds = tui.Find(UiSelector::Key("top-fab")).One().bounds;
    CHECK(std::abs((fab_bounds.x + fab_bounds.width) - (vp.x + vp.width - 12.0F)) <= 2.0F);
    CHECK(std::abs((fab_bounds.y + fab_bounds.height) - (vp.y + vp.height - 14.0F)) <= 2.0F);
    CHECK(tui.Find(UiSelector::Text("tvis:1 tanim:0 toff:180 tpct:45")).Exists());

    // Tap wrapped FAB: animates smoothly mid-flight, settles at 0, hides FAB, fires on_triggered once.
    tui.TapAt(Center(fab_bounds));
    tui.Pump(std::chrono::milliseconds(16));
    tui.Pump(std::chrono::milliseconds(80));
    CHECK(tui.Find(UiSelector::Text("thits:1:0:0")).Exists());
    tui.Pump(std::chrono::milliseconds(16));
    const float row1_y_mid = tui.Find(UiSelector::Text("row-1")).One().bounds.y;
    CHECK(row1_y_mid > vp.y - 140.0F);
    CHECK(row1_y_mid < vp.y + 40.0F);
    tui.PumpAndSettle();
    CHECK(tui.Find(UiSelector::Text("tvis:0 tanim:0 toff:0 tpct:0")).Exists());
    CHECK(!tui.Find(UiSelector::Key("top-fab")).Exists());

    // Smooth animation interrupted by user wheel scroll stays scrolled rather than returning to 0.
    tui.SendScroll({.position = sc_center, .delta_y = 280.0F});
    tui.PumpAndSettle();
    CHECK(tui.Find(UiSelector::Key("top-fab")).Exists());
    tui.TapAt(Center(tui.Find(UiSelector::Key("top-fab")).One().bounds));
    tui.Pump(std::chrono::milliseconds(16));
    tui.Pump(std::chrono::milliseconds(16));
    tui.SendScroll({.position = sc_center, .delta_y = 60.0F});
    tui.PumpAndSettle();
    CHECK(tui.Find(UiSelector::Text("thits:2:0:0")).Exists());
    CHECK(tui.Find(UiSelector::Key("top-fab")).Exists());
    const float row1_y_interrupted = tui.Find(UiSelector::Text("row-1")).One().bounds.y;
    CHECK(row1_y_interrupted < vp.y - 100.0F);

    // Inline ScrollToTopTrigger (environment-bound) and external ScrollToTopTrigger (controller-bound).
    tui.Find(UiSelector::Key("top-ext-trigger")).Tap();
    tui.PumpAndSettle();
    CHECK(tui.Find(UiSelector::Text("thits:3:1:0")).Exists());
    CHECK(tui.Find(UiSelector::Text("tvis:0 tanim:0 toff:0 tpct:0")).Exists());

    tui.SendScroll({.position = sc_center, .delta_y = 140.0F});
    tui.PumpAndSettle();
    CHECK(tui.Find(UiSelector::Key("top-fab")).Exists());
  }

  // Non-zero target_offset, inline ScrollToTopTrigger, and show_on_scroll_up hysteresis.
  {
    Application up_app(
        []() -> View {
          auto sc = UseScrollController();
          auto top_ctrl = UseScrollToTopController();
          auto env_hits = UseState(0);
          auto area_hits = UseState(0);
          const auto off = static_cast<int>(std::lround(top_ctrl.Offset()));
          const int vis = top_ctrl.IsVisible() ? 1 : 0;
          return Column {
            Text("uvis:" + std::to_string(vis) + " uoff:" + std::to_string(off)),
            Text("uhits:" + std::to_string(env_hits.Get()) + ":" + std::to_string(area_hits.Get())),
            ScrollToTop(
                Column {
                  Stack{Text("jump40").Key("env-jump-40")}.With(Frame{.height = 24.0F})
                      | ScrollToTopTrigger({}, ScrollJumpBehavior::Instant,
                                           [env_hits] { env_hits = env_hits.Get() + 1; }),
                  ScrollView(
                      Column {
                        Stack{Text("u-0")}.With(Frame{.height = 140.0F}),
                        Stack{Text("u-1")}.With(Frame{.height = 140.0F}),
                        Stack{Text("u-2")}.With(Frame{.height = 140.0F}),
                        Stack{Text("u-3")}.With(Frame{.height = 140.0F}),
                      }.With(CrossAlign(CrossAxisAlignment::Stretch)))
                      .Controller(sc)
                      .Key("up-scroll")
                      .With(Frame{.width = 220.0F, .height = 120.0F}),
                }.With(CrossAlign(CrossAxisAlignment::Start)),
                Text("UPFAB").Key("up-fab"),
                {
                    .scroll_controller = sc,
                    .threshold = 100.0F,
                    .target_offset = 40.0F,
                    .show_on_scroll_up = true,
                    .scroll_up_delta = 12.0F,
                    .jump_behavior = ScrollJumpBehavior::Instant,
                    .reveal_motion = {.motion = TweenSpec{.duration = 0.0}, .animate_on_mount = false},
                    .controller = top_ctrl,
                },
                [area_hits] { area_hits = area_hits.Get() + 1; }),
          }.With(Spacing(6.0F));
        },
        {.window = {.initial_size = {360, 260}}, .show_debug_overlay = false});
    UiTest sui(up_app, {.viewport = {360, 260}});

    sui.PumpAndSettle();
    const Point up_center = Center(sui.Find(UiSelector::Key("up-scroll")).One().bounds);

    // Scrolling down past threshold (0 -> 240) keeps overlay hidden while scrolling downward.
    sui.SendScroll({.position = up_center, .delta_y = 240.0F});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("uvis:0 uoff:240")).Exists());
    CHECK(!sui.Find(UiSelector::Key("up-fab")).Exists());

    // Small upward scroll (-6px < scroll_up_delta 12px) stays hidden.
    sui.SendScroll({.position = up_center, .delta_y = -6.0F});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("uvis:0 uoff:234")).Exists());

    // Larger upward scroll (-20px -> peak 240 - 214 = 26 >= 12px) reveals overlay!
    sui.SendScroll({.position = up_center, .delta_y = -20.0F});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("uvis:1 uoff:214")).Exists());
    CHECK(sui.Find(UiSelector::Key("up-fab")).Exists());

    // Scrolling forward again (+20px >= 12px from trough 214 -> 234) hides overlay again.
    sui.SendScroll({.position = up_center, .delta_y = 20.0F});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("uvis:0 uoff:234")).Exists());
    CHECK(!sui.Find(UiSelector::Key("up-fab")).Exists());

    // Scroll back up past 12px above threshold (-30 -> 204), then activate environment-bound ScrollToTopTrigger to target 40.
    sui.SendScroll({.position = up_center, .delta_y = -30.0F});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("uvis:1 uoff:204")).Exists());
    sui.Find(UiSelector::Key("env-jump-40")).Tap();
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("uhits:1:1")).Exists());
    CHECK(sui.Find(UiSelector::Text("uvis:0 uoff:40")).Exists());
  }
}

#endif
