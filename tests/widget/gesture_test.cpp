#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <chrono>
#include <cmath>
#include <string>

#ifdef INTERACTION_HAS_UI_TESTING
#include <huxerui/testing/ui_test.h>
using namespace huxerui::testing;

using namespace huxerui;
using namespace huxerui::interaction;

namespace {

View PinchAndSwipeGallery() {
  auto deleted = UseState(0);
  auto dismissed = UseState(0);
  auto width = UseState(100.0F);
  return Column {
    PinchZoom(
        Text("zoom target")
            .With(Frame{.width = 100.0F, .height = 50.0F}, Background{Color::Rgb(90, 130, 90)})
            .Key("zoom-content"),
        {.minimum_scale = 1.0F, .maximum_scale = 3.0F, .spring_back = false})
        .Key("pinch"),
    SwipeActions(
        Text("swipe row")
            .With(Frame{.width = 300.0F, .height = 56.0F}, Background{Color::Rgb(80, 110, 160)})
            .Key("swipe-content"),
        [deleted](SwipeActionContext ctx) {
          return Button("Delete")
              .OnClick([deleted, close = ctx.close] {
                deleted += 1;
                close();
              })
              .Key("delete");
        },
        {.action_extent = 96.0F, .activation_ratio = 0.4F})
        .Key("swipe"),
    Dismissible(
        Text("dismiss target")
            .With(Frame{.width = 200.0F, .height = 48.0F}, Background{Color::Rgb(80, 120, 160)}),
        [](float progress, float) {
          return Text(progress > 0.5F ? "release" : "drag").With(Background{Color::Rgb(180, 60, 60)});
        },
        [dismissed] { dismissed += 1; },
        {.dismiss_threshold = 80.0F, .minimum_velocity = 10000.0F, .exit_distance = 240.0F, .exit_duration = 0.15})
        .Key("dismiss"),
    Row {
      Text("panel").With(Frame{.width = width.Get(), .height = 48.0F}, Background{Color::Rgb(80, 110, 160)}),
      ResizeHandle(Axis::Horizontal, [width](float delta) {
        width = std::clamp(width.Get() + delta, 60.0F, 240.0F);
      }).Key("resize-handle"),
    }.With(CrossAlign(CrossAxisAlignment::Stretch)),
    Text("deleted: " + std::to_string(deleted.Get())),
    Text("dismissed: " + std::to_string(dismissed.Get())),
    Text("width: " + std::to_string(static_cast<int>(std::lround(width.Get())))),
  }.With(Padding(16.0F), Spacing(12.0F));
}

View TransformAndInertiaGallery() {
  auto started = UseState(0);
  auto ended = UseState(0);
  auto last = UseState(0);
  return Column {
    PanZoom(
        Text("transform target")
            .With(Frame{.width = 160.0F, .height = 80.0F}, Background{Color::Rgb(80, 120, 160)})
            .Key("transform-content"),
        {.minimum_scale = 0.5F, .maximum_scale = 3.0F})
        .Key("panzoom"),
    InertialDrag(
        Text("inertial target")
            .With(Frame{.width = 160.0F, .height = 50.0F}, Background{Color::Rgb(150, 100, 70)})
            .Key("inertial-content"),
        {.projection_time = 0.2F, .maximum_projection = 300.0F})
        .Key("inertial"),
    PressDragHandle(
        Text("handle").With(Frame{.width = 160.0F, .height = 48.0F}, Background{Color::Rgb(120, 90, 150)}),
        {.on_started = [started](const DragEvent&) { started += 1; },
         .on_changed = [last](const DragEvent& e) { last = static_cast<int>(e.translation.x); },
         .on_ended = [ended](const DragEvent&) { ended += 1; }},
        {.press_duration = std::chrono::duration<double>(0.2)})
        .Key("press-drag"),
    Text("started: " + std::to_string(started.Get())),
    Text("ended: " + std::to_string(ended.Get())),
    Text("last: " + std::to_string(last.Get())),
  }.With(Padding(16.0F), Spacing(12.0F));
}

} // namespace

void CheckGestureContracts() {
  {
    Application app(PinchAndSwipeGallery, {.window = {.initial_size = {520, 520}}, .show_debug_overlay = false});
    UiTest ui(app, {.viewport = {520, 520}});

    // PinchZoom wheel zoom clamping.
    const Rect initial_zoom = ui.Find(UiSelector::Key("zoom-content")).One().bounds;
    const Point zoom_center = Center(initial_zoom);
    const Point plain = ui.SendScroll({.position = zoom_center, .delta_y = 120.0F});
    ui.PumpAndSettle();
    CHECK(plain.y == 0.0F);
    CHECK(Near(ui.Find(UiSelector::Key("zoom-content")).One().bounds.width, initial_zoom.width));

    const Point consumed = ui.SendScroll(
        {.position = zoom_center, .delta_y = -1000.0F, .modifiers = KeyModifiers{.control = true}});
    ui.PumpAndSettle();
    CHECK(consumed.y != 0.0F);
    CHECK(Near(ui.Find(UiSelector::Key("zoom-content")).One().bounds.width, initial_zoom.width * 3.0F));

    ui.SendScroll({.position = zoom_center, .delta_y = 1000.0F, .modifiers = KeyModifiers{.control = true}});
    ui.PumpAndSettle();
    CHECK(Near(ui.Find(UiSelector::Key("zoom-content")).One().bounds.width, initial_zoom.width));

    // SwipeActions reveal and close via SwipeActionContext.
    const Rect swipe_initial = ui.Find(UiSelector::Key("swipe-content")).One().bounds;
    const Point swipe_start = Center(swipe_initial);
    ui.Drag(swipe_start, {swipe_start.x - 120.0F, swipe_start.y},
            {.duration = std::chrono::duration<double>(0.3), .segments = 12});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Key("swipe-content")).One().bounds.x < swipe_initial.x - 40.0F);
    ui.Find(UiSelector::Key("delete")).Tap();
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("deleted: 1")).Exists());
    CHECK(Near(ui.Find(UiSelector::Key("swipe-content")).One().bounds.x, swipe_initial.x));

    // Dismissible with custom background builder.
    const Rect dismiss_rect = ui.Find(UiSelector::Key("dismiss")).One().bounds;
    const Point dismiss_start{dismiss_rect.x + 40.0F, dismiss_rect.y + dismiss_rect.height * 0.5F};
    ui.Drag(dismiss_start, {dismiss_start.x + 120.0F, dismiss_start.y},
            {.duration = std::chrono::duration<double>(0.2), .segments = 8});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("dismissed: 1")).Exists());

    // ResizeHandle delta forwarding.
    const Point rh = Center(ui.Find(UiSelector::Key("resize-handle")).One().bounds);
    ui.Drag(rh, {rh.x + 40.0F, rh.y}, {.duration = std::chrono::duration<double>(0.2), .segments = 8});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("width: 140")).Exists());
  }

  {
    Application app(TransformAndInertiaGallery, {.window = {.initial_size = {520, 520}}, .show_debug_overlay = false});
    UiTest ui(app, {.viewport = {520, 520}});

    // Two-pointer PanZoom scale + pan + rotation.
    const Rect initial = ui.Find(UiSelector::Key("transform-content")).One().bounds;
    const Point center = Center(initial);
    const Point left{center.x - 30.0F, center.y};
    const Point right{center.x + 30.0F, center.y};
    PointerEvent first{.type = PointerEventType::Down,
                       .pointer_id = 21,
                       .position = left,
                       .device_kind = PointerDeviceKind::Touch,
                       .changed_button = PointerButton::Primary,
                       .pressed_buttons = PointerButton::Primary};
    PointerEvent second = first;
    second.pointer_id = 22;
    second.position = right;
    ui.SendPointer(first);
    ui.SendPointer(second);
    ui.Pump();
    first.type = PointerEventType::Move;
    first.position = {center.x - 60.0F, center.y};
    second.type = PointerEventType::Move;
    second.position = {center.x + 60.0F, center.y};
    ui.SendPointer(first);
    ui.SendPointer(second);
    ui.PumpAndSettle();
    const Rect scaled = ui.Find(UiSelector::Key("transform-content")).One().bounds;
    CHECK(scaled.width > initial.width * 1.2F);
    first.type = PointerEventType::Up;
    first.pressed_buttons = PointerButton::None;
    second.type = PointerEventType::Up;
    second.pressed_buttons = PointerButton::None;
    ui.SendPointer(first);
    ui.SendPointer(second);
    ui.PumpAndSettle();

    // InertialDrag velocity projection.
    const Rect inertia_initial = ui.Find(UiSelector::Key("inertial-content")).One().bounds;
    const Point inertia_start = Center(inertia_initial);
    ui.Drag(inertia_start, {inertia_start.x + 60.0F, inertia_start.y},
            {.duration = std::chrono::duration<double>(0.1), .segments = 5});
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Key("inertial-content")).One().bounds.x > inertia_initial.x + 60.0F);

    // PressDragHandle hold-then-drag.
    const Point pd_start = Center(ui.Find(UiSelector::Key("press-drag")).One().bounds);
    PointerEvent hold{.type = PointerEventType::Down,
                      .pointer_id = 61,
                      .position = pd_start,
                      .device_kind = PointerDeviceKind::Touch,
                      .changed_button = PointerButton::Primary,
                      .pressed_buttons = PointerButton::Primary};
    ui.SendPointer(hold);
    ui.Pump(std::chrono::milliseconds(250));
    hold.type = PointerEventType::Move;
    hold.position = {pd_start.x + 40.0F, pd_start.y};
    ui.SendPointer(hold);
    ui.Pump();
    hold.type = PointerEventType::Up;
    hold.pressed_buttons = PointerButton::None;
    ui.SendPointer(hold);
    ui.PumpAndSettle();
    CHECK(ui.Find(UiSelector::Text("started: 1")).Exists());
    CHECK(ui.Find(UiSelector::Text("ended: 1")).Exists());
    CHECK(ui.Find(UiSelector::Text("last: 40")).Exists());
  }

  // MarqueeSelectArea + MarqueeSelectItem + MarqueeSelectController (replace, additive, Toggle, Clear).
  {
    Application marquee_app(
        []() -> View {
          auto marquee = UseMarqueeSelectController();
          auto additive = UseState(false);
          auto changed_count = UseState(0);
          auto committed_count = UseState(0);
          const std::vector<std::size_t> selected = marquee.Selection();
          std::string label = "sel:";
          for(std::size_t index : selected) {
            label += " " + std::to_string(index);
          }
          if(selected.empty()) label = "sel: none";

          return Column {
            (Row {
              Text("M0").With(Frame{.width = 96.0F, .height = 56.0F}).Key("m-0") | MarqueeSelectItem(0),
              Text("M1").With(Frame{.width = 96.0F, .height = 56.0F}).Key("m-1") | MarqueeSelectItem(1),
              Text("M2").With(Frame{.width = 96.0F, .height = 56.0F}).Key("m-2") | MarqueeSelectItem(2),
            }.With(Spacing(12.0F), Padding(20.0F))
             | MarqueeSelectArea(
                 {.additive = additive.Get(), .controller = marquee},
                 [changed_count](const std::vector<std::size_t>&) { changed_count += 1; },
                 [committed_count](const std::vector<std::size_t>&) { committed_count += 1; }))
                .Key("marquee-area"),
            Row {
              Button("Toggle Additive").OnClick([additive] { additive = !additive.Get(); }).Key("m-additive"),
              Button("Toggle 1").OnClick([marquee] { marquee.Toggle(1); }).Key("m-toggle-1"),
              Button("Clear Sel").OnClick([marquee] { marquee.Clear(); }).Key("m-clear"),
            }.With(Spacing(8.0F)),
            Text(label),
            Text("changed: " + std::to_string(changed_count.Get())),
            Text("committed: " + std::to_string(committed_count.Get())),
          }.With(Padding(16.0F), Spacing(12.0F));
        },
        {.window = {.initial_size = {480, 260}}, .show_debug_overlay = false});
    UiTest mui(marquee_app, {.viewport = {480, 260}});

    CHECK(mui.Find(UiSelector::Text("sel: none")).Exists());
    const Rect r0 = mui.Find(UiSelector::Key("m-0")).One().bounds;
    const Rect r1 = mui.Find(UiSelector::Key("m-1")).One().bounds;
    const Rect r2 = mui.Find(UiSelector::Key("m-2")).One().bounds;

    mui.Drag({r0.x - 8.0F, r0.y - 8.0F}, {r1.x + r1.width * 0.5F, r1.y + r1.height - 6.0F},
             {.duration = std::chrono::duration<double>(0.15), .segments = 6});
    mui.PumpAndSettle();
    CHECK(mui.Find(UiSelector::Text("sel: 0 1")).Exists());
    CHECK(mui.Find(UiSelector::Text("committed: 1")).Exists());

    mui.Find(UiSelector::Key("m-additive")).Tap();
    mui.PumpAndSettle();
    mui.Drag({r2.x + 10.0F, r2.y + 10.0F}, {r2.x + r2.width - 10.0F, r2.y + r2.height - 10.0F},
             {.duration = std::chrono::duration<double>(0.15), .segments = 6});
    mui.PumpAndSettle();
    CHECK(mui.Find(UiSelector::Text("sel: 0 1 2")).Exists());
    CHECK(mui.Find(UiSelector::Text("committed: 2")).Exists());

    mui.Find(UiSelector::Key("m-toggle-1")).Tap();
    mui.PumpAndSettle();
    CHECK(mui.Find(UiSelector::Text("sel: 0 2")).Exists());

    mui.Find(UiSelector::Key("m-clear")).Tap();
    mui.PumpAndSettle();
    CHECK(mui.Find(UiSelector::Text("sel: none")).Exists());
  }

  // SnapDrag (Corners drag & flick, programmatic animated vs instant SnapToAnchor, HorizontalEdges, Custom).
  {
    Application snap_app(
        []() -> View {
          auto snap_corners = UseSnapDragController();
          auto snap_edges = UseSnapDragController();
          auto snap_custom = UseSnapDragController();
          auto snapped_count = UseState(0);
          auto last_anchor = UseState<std::size_t>(0);

          return Column {
            Row {
              SnapDrag(
                  [](SnapDragSnapshot s) {
                    return Text(s.dragging ? "pip-drag" : "pip-rest")
                        .With(Frame{.width = 60.0F, .height = 40.0F})
                        .Key("pip-c");
                  },
                  {.mode = SnapMode::Corners,
                   .initial_anchor = 0,
                   .margin = 10.0F,
                   .snap_motion = TweenSpec{.duration = 0.2, .easing = Easing::Linear},
                   .controller = snap_corners},
                  [snapped_count, last_anchor](std::size_t idx, Point) {
                    snapped_count += 1;
                    last_anchor = idx;
                  })
                  .With(Frame{.width = 240.0F, .height = 160.0F})
                  .Key("stage-c"),
              SnapDrag(
                  Text("edge").With(Frame{.width = 60.0F, .height = 40.0F}).Key("pip-e"),
                  {.mode = SnapMode::HorizontalEdges, .initial_anchor = 0, .margin = 10.0F, .controller = snap_edges})
                  .With(Frame{.width = 200.0F, .height = 160.0F})
                  .Key("stage-e"),
              SnapDrag(
                  Text("cust").With(Frame{.width = 40.0F, .height = 40.0F}).Key("pip-u"),
                  {.mode = SnapMode::Custom,
                   .anchors = {{0.0F, 0.5F}, {0.5F, 0.5F}, {1.0F, 0.5F}},
                   .initial_anchor = 0,
                   .margin = 10.0F,
                   .controller = snap_custom})
                  .With(Frame{.width = 200.0F, .height = 120.0F})
                  .Key("stage-u"),
            }.With(Spacing(16.0F)),
            Row {
              Button("Snap TR").OnClick([snap_corners] { snap_corners.SnapToAnchor(1, true); }).Key("snap-tr"),
              Button("Snap BL Instant")
                  .OnClick([snap_corners] { snap_corners.SnapToAnchor(2, false); })
                  .Key("snap-bl-now"),
            }.With(Spacing(8.0F)),
            Text("c_anchor: " + std::to_string(snap_corners.ActiveAnchor())),
            Text("c_snaps: " + std::to_string(snapped_count.Get())),
            Text("e_anchor: " + std::to_string(snap_edges.ActiveAnchor())),
            Text("u_anchor: " + std::to_string(snap_custom.ActiveAnchor())),
          }.With(Padding(16.0F), Spacing(10.0F));
        },
        {.window = {.initial_size = {740, 320}}, .show_debug_overlay = false});
    UiTest sui(snap_app, {.viewport = {740, 320}});
    sui.PumpAndSettle();

    // 1. Starts at TopLeft {margin, margin} inside stage-c.
    const Rect stage_c = sui.Find(UiSelector::Key("stage-c")).One().bounds;
    const Rect pip_c0 = sui.Find(UiSelector::Key("pip-c")).One().bounds;
    CHECK(Near(pip_c0.x, stage_c.x + 10.0F));
    CHECK(Near(pip_c0.y, stage_c.y + 10.0F));
    CHECK(sui.Find(UiSelector::Text("c_anchor: 0")).Exists());

    // Drag toward bottom-right corner -> snaps to anchor 3 (BottomRight).
    sui.Drag(Center(pip_c0), {stage_c.x + stage_c.width - 30.0F, stage_c.y + stage_c.height - 25.0F},
             {.duration = std::chrono::duration<double>(0.2), .segments = 8});
    sui.PumpAndSettle();
    const Rect pip_c3 = sui.Find(UiSelector::Key("pip-c")).One().bounds;
    CHECK(sui.Find(UiSelector::Text("c_anchor: 3")).Exists());
    CHECK(sui.Find(UiSelector::Text("c_snaps: 1")).Exists());
    CHECK(Near(pip_c3.x, stage_c.x + stage_c.width - 10.0F - 60.0F));
    CHECK(Near(pip_c3.y, stage_c.y + stage_c.height - 10.0F - 40.0F));

    // 2. Animated SnapToAnchor(1, true) moves through intermediate positions before resting at TopRight.
    sui.Find(UiSelector::Key("snap-tr")).Tap();
    for(int f = 0; f < 4; ++f) {
      sui.Pump(std::chrono::milliseconds(16));
    }
    const Rect pip_mid = sui.Find(UiSelector::Key("pip-c")).One().bounds;
    CHECK(pip_mid.y < pip_c3.y - 5.0F);
    CHECK(pip_mid.y > stage_c.y + 15.0F);
    sui.PumpAndSettle();
    const Rect pip_c1 = sui.Find(UiSelector::Key("pip-c")).One().bounds;
    CHECK(sui.Find(UiSelector::Text("c_anchor: 1")).Exists());
    CHECK(Near(pip_c1.x, stage_c.x + stage_c.width - 10.0F - 60.0F));
    CHECK(Near(pip_c1.y, stage_c.y + 10.0F));

    // 3. Instant SnapToAnchor(2, false) reaches BottomLeft in a single zero-duration Pump().
    sui.Find(UiSelector::Key("snap-bl-now")).Tap();
    sui.Pump();
    const Rect pip_c2 = sui.Find(UiSelector::Key("pip-c")).One().bounds;
    CHECK(Near(pip_c2.x, stage_c.x + 10.0F));
    CHECK(Near(pip_c2.y, stage_c.y + stage_c.height - 10.0F - 40.0F));
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("c_anchor: 2")).Exists());

    // 4. HorizontalEdges preserves intermediate Y while docking horizontally to Right edge (anchor 1).
    const Rect stage_e = sui.Find(UiSelector::Key("stage-e")).One().bounds;
    const Rect pip_e0 = sui.Find(UiSelector::Key("pip-e")).One().bounds;
    sui.Drag(Center(pip_e0), {stage_e.x + stage_e.width - 40.0F, stage_e.y + 80.0F},
             {.duration = std::chrono::duration<double>(0.2), .segments = 8});
    sui.PumpAndSettle();
    const Rect pip_e1 = sui.Find(UiSelector::Key("pip-e")).One().bounds;
    CHECK(sui.Find(UiSelector::Text("e_anchor: 1")).Exists());
    CHECK(Near(pip_e1.x, stage_e.x + stage_e.width - 10.0F - 60.0F));
    CHECK(pip_e1.y > stage_e.y + 40.0F);

    // 5. Custom anchors: gentle drag lands on center anchor 1, fast flick carries to right anchor 2.
    const Rect stage_u = sui.Find(UiSelector::Key("stage-u")).One().bounds;
    const Rect pip_u0 = sui.Find(UiSelector::Key("pip-u")).One().bounds;
    sui.Drag(Center(pip_u0), Center(stage_u), {.duration = std::chrono::duration<double>(0.8), .segments = 8});
    sui.PumpAndSettle();
    const Rect pip_u1 = sui.Find(UiSelector::Key("pip-u")).One().bounds;
    CHECK(sui.Find(UiSelector::Text("u_anchor: 1")).Exists());
    CHECK(Near(Center(pip_u1).x, Center(stage_u).x));
    CHECK(Near(Center(pip_u1).y, Center(stage_u).y));

    sui.Drag(Center(pip_u1), {Center(pip_u1).x + 30.0F, Center(pip_u1).y},
             {.duration = std::chrono::duration<double>(0.08), .segments = 6});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("u_anchor: 2")).Exists());
  }

  // SwipeDeck (sub-threshold return, swipe commit, programmatic Swipe/Rewind/Reset, empty state).
  {
    Application deck_app(
        []() -> View {
          auto deck = UseSwipeDeckController();
          auto swipes = UseState(0);
          auto last = UseState(std::string{"none"});

          return Column {
            SwipeDeck(
                3,
                [](SwipeDeckCardContext ctx) {
                  return Text("Card " + std::to_string(ctx.index))
                      .With(Frame{.width = 180.0F, .height = 100.0F})
                      .Key("deck-card-" + std::to_string(ctx.index));
                },
                [swipes, last](std::size_t idx, SwipeDirection dir) {
                  swipes += 1;
                  const char* tag = dir == SwipeDirection::Right ? "R" : (dir == SwipeDirection::Left ? "L" : "O");
                  last = std::to_string(idx) + ":" + tag;
                },
                {.swipe_threshold = 90.0F,
                 .minimum_velocity = 600.0F,
                 .exit_distance = 320.0F,
                 .return_motion = TweenSpec{.duration = 0.08, .easing = Easing::Linear},
                 .exit_motion = TweenSpec{.duration = 0.1, .easing = Easing::Linear},
                 .rewind_motion = TweenSpec{.duration = 0.08, .easing = Easing::Linear},
                 .controller = deck},
                []() { return Text("Deck empty").With(Frame{.width = 180.0F, .height = 100.0F}); })
                .Key("deck"),
            Row {
              Button("Swipe Left").OnClick([deck] { deck.Swipe(SwipeDirection::Left); }).Key("deck-left"),
              Button("Swipe Right").OnClick([deck] { deck.Swipe(SwipeDirection::Right); }).Key("deck-right"),
              Button("Rewind").OnClick([deck] { deck.Rewind(); }).Key("deck-rewind"),
              Button("Reset").OnClick([deck] { deck.Reset(); }).Key("deck-reset"),
            }.With(Spacing(8.0F)),
            Text("top: " + std::to_string(deck.TopIndex())),
            Text("rem: " + std::to_string(deck.RemainingCount())),
            Text(std::string("rewind: ") + (deck.CanRewind() ? "yes" : "no")),
            Text("swipes: " + std::to_string(swipes.Get())),
            Text("last: " + last.Get()),
          }.With(Padding(16.0F), Spacing(10.0F));
        },
        {.window = {.initial_size = {480, 320}}, .show_debug_overlay = false});
    UiTest dui(deck_app, {.viewport = {480, 320}});
    dui.PumpAndSettle();

    CHECK(dui.Find(UiSelector::Text("top: 0")).Exists());
    CHECK(dui.Find(UiSelector::Text("rem: 3")).Exists());
    CHECK(dui.Find(UiSelector::Text("rewind: no")).Exists());

    const Rect card0_rest = dui.Find(UiSelector::Key("deck-card-0")).One().bounds;
    // 1. Slow sub-threshold drag returns cleanly to rest without advancing TopIndex.
    dui.Drag(Center(card0_rest), {Center(card0_rest).x + 30.0F, Center(card0_rest).y},
             {.duration = std::chrono::duration<double>(0.6), .segments = 8});
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("top: 0")).Exists());
    CHECK(dui.Find(UiSelector::Text("swipes: 0")).Exists());
    CHECK(Near(dui.Find(UiSelector::Key("deck-card-0")).One().bounds.x, card0_rest.x));

    // 2. Drag past threshold right commits card 0 -> card 1 promoted to top.
    dui.Drag(Center(card0_rest), {Center(card0_rest).x + 130.0F, Center(card0_rest).y},
             {.duration = std::chrono::duration<double>(0.2), .segments = 8});
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("top: 1")).Exists());
    CHECK(dui.Find(UiSelector::Text("rem: 2")).Exists());
    CHECK(dui.Find(UiSelector::Text("rewind: yes")).Exists());
    CHECK(dui.Find(UiSelector::Text("swipes: 1")).Exists());
    CHECK(dui.Find(UiSelector::Text("last: 0:R")).Exists());

    // 3. Programmatic Swipe Left commits card 1.
    dui.Find(UiSelector::Key("deck-left")).Tap();
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("top: 2")).Exists());
    CHECK(dui.Find(UiSelector::Text("rem: 1")).Exists());
    CHECK(dui.Find(UiSelector::Text("last: 1:L")).Exists());

    // 4. Rewind brings card 1 back to top.
    dui.Find(UiSelector::Key("deck-rewind")).Tap();
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("top: 1")).Exists());
    CHECK(dui.Find(UiSelector::Text("rem: 2")).Exists());
    CHECK(dui.Find(UiSelector::Key("deck-card-1")).Exists());

    // 5. Swiping remaining 2 cards reveals empty state; Reset restores full deck.
    dui.Find(UiSelector::Key("deck-right")).Tap();
    dui.PumpAndSettle();
    dui.Find(UiSelector::Key("deck-right")).Tap();
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("top: 3")).Exists());
    CHECK(dui.Find(UiSelector::Text("rem: 0")).Exists());
    CHECK(dui.Find(UiSelector::Text("Deck empty")).Exists());

    dui.Find(UiSelector::Key("deck-reset")).Tap();
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("top: 0")).Exists());
    CHECK(dui.Find(UiSelector::Text("rem: 3")).Exists());
    CHECK(dui.Find(UiSelector::Text("rewind: no")).Exists());
  }

  // CompareSlider: horizontal curtain geometry, clamping, custom handle, double-click reset,
  // controller SetProgress/Reset, keyboard steps, and vertical drag_anywhere=false.
  {
    Application cmp_app(
        []() -> View {
          auto ctrl = UseCompareSliderController(0.5F);
          auto changes = UseState(0);
          auto last_pct = UseState(50);
          const int pct = static_cast<int>(std::lround(ctrl.Progress() * 100.0F));
          return Column {
            CompareSlider(
                Stack{Text("BEFORE").Key("cmp-b-txt")}.Key("cmp-before").With(Frame{.width = 300.0F, .height = 140.0F}),
                Stack{Text("AFTER").Key("cmp-a-txt")}.Key("cmp-after").With(Frame{.width = 300.0F, .height = 140.0F}),
                [](CompareSliderSnapshot snap) -> View {
                  return Stack {
                    Text(std::string("focus:") + (snap.focused ? "1" : "0") + " drag:" + (snap.dragging ? "1" : "0")),
                  }.Key("cmp-handle").With(Frame{.width = 24.0F, .height = 24.0F});
                },
                {
                    .axis = Axis::Horizontal,
                    .initial_progress = 0.5F,
                    .min_progress = 0.2F,
                    .max_progress = 0.85F,
                    .keyboard_step = 0.05F,
                    .controller = ctrl,
                },
                [changes, last_pct](float p) {
                  changes = changes.Get() + 1;
                  last_pct = static_cast<int>(std::lround(p * 100.0F));
                })
                .Key("cmp-stage")
                .With(Frame{.width = 300.0F, .height = 140.0F}),
            Row {
              Text("cpct:" + std::to_string(pct)),
              Text("clast:" + std::to_string(last_pct.Get())),
              Text(changes.Get() > 0 ? "cchg>0" : "cchg:0"),
            }.With(Spacing(8.0F)),
            Row {
              Button("Jump25").Key("cmp-j25").OnClick([ctrl] { ctrl.SetProgress(0.25F, false); }),
              Button("Reset").Key("cmp-rst").OnClick([ctrl] { ctrl.Reset(true); }),
            }.With(Spacing(8.0F)),
          }.With(Spacing(8.0F), Padding(16.0F));
        },
        {.window = {.initial_size = {380, 260}}, .show_debug_overlay = false});
    UiTest cui(cmp_app, {.viewport = {380, 260}});
    cui.PumpAndSettle();

    const Rect stage = cui.Find(UiSelector::Key("cmp-stage")).One().bounds;
    const Rect before0 = cui.Find(UiSelector::Key("cmp-before")).One().bounds;
    const Rect after0 = cui.Find(UiSelector::Key("cmp-after")).One().bounds;
    const Rect handle0 = cui.Find(UiSelector::Key("cmp-handle")).One().bounds;
    CHECK(Near(before0.width, 300.0F));
    CHECK(Near(after0.width, 300.0F));
    CHECK(std::abs(Center(handle0).x - (stage.x + 150.0F)) <= 1.5F);
    CHECK(std::abs(Center(handle0).y - (stage.y + 70.0F)) <= 1.5F);
    CHECK(cui.Find(UiSelector::Text("cpct:50")).Exists());
    CHECK(cui.Find(UiSelector::Text("cchg:0")).Exists());
    CHECK(cui.Find(UiSelector::Text("clast:50")).Exists());

    // Drag right past max_progress -> clamps at 85% while keeping `cmp-before` unsquished at 300px wide.
    cui.Drag({stage.x + 150.0F, stage.y + 70.0F}, {stage.x + 295.0F, stage.y + 70.0F},
             {.duration = std::chrono::duration<double>(0.15), .segments = 6});
    cui.PumpAndSettle();
    const Rect before1 = cui.Find(UiSelector::Key("cmp-before")).One().bounds;
    const Rect handle1 = cui.Find(UiSelector::Key("cmp-handle")).One().bounds;
    CHECK(Near(before1.width, 300.0F));
    CHECK(std::abs(Center(handle1).x - (stage.x + 255.0F)) <= 2.0F);
    CHECK(cui.Find(UiSelector::Text("cpct:85")).Exists());
    CHECK(cui.Find(UiSelector::Text("cchg>0")).Exists());
    CHECK(cui.Find(UiSelector::Text("clast:85")).Exists());

    // Double-click near handle -> animates smoothly back to initial_progress (50%).
    const Point dbl_pt = Center(handle1);
    cui.TapAt(dbl_pt);
    cui.Pump(std::chrono::milliseconds(80));
    cui.TapAt(dbl_pt);
    cui.PumpAndSettle();
    CHECK(cui.Find(UiSelector::Text("cpct:50")).Exists());

    // Programmatic SetProgress(0.25, false) & Reset(true).
    cui.Find(UiSelector::Key("cmp-j25")).Tap();
    cui.PumpAndSettle();
    CHECK(cui.Find(UiSelector::Text("cpct:25")).Exists());
    cui.Find(UiSelector::Key("cmp-rst")).Tap();
    cui.PumpAndSettle();
    CHECK(cui.Find(UiSelector::Text("cpct:50")).Exists());

    // Keyboard control: Tab focuses slider (first focusable), ArrowRight (+5%), Shift+ArrowRight (+1%),
    // PageUp (+25%), End (85%), Home (20%).
    cui.PressKey(Key::Tab);
    cui.PumpAndSettle();
    CHECK(cui.Find(UiSelector::Text("focus:1 drag:0")).Exists());
    cui.PressKey(Key::ArrowRight);
    cui.PumpAndSettle();
    CHECK(cui.Find(UiSelector::Text("cpct:55")).Exists());
    cui.PressKey(Key::ArrowRight, {.shift = true});
    cui.PumpAndSettle();
    CHECK(cui.Find(UiSelector::Text("cpct:56")).Exists());
    cui.PressKey(Key::PageUp);
    cui.PumpAndSettle();
    CHECK(cui.Find(UiSelector::Text("cpct:81")).Exists());
    cui.PressKey(Key::End);
    cui.PumpAndSettle();
    CHECK(cui.Find(UiSelector::Text("cpct:85")).Exists());
    cui.PressKey(Key::Home);
    cui.PumpAndSettle();
    CHECK(cui.Find(UiSelector::Text("cpct:20")).Exists());
  }

  // Vertical axis + drag_anywhere = false: ignores drags outside handle_hit_extent, scrubs from divider.
  {
    Application vcmp_app(
        []() -> View {
          auto ctrl = UseCompareSliderController(0.4F);
          const int pct = static_cast<int>(std::lround(ctrl.Progress() * 100.0F));
          return Column {
            CompareSlider(
                Stack{Text("VB")}.Key("vcmp-b").With(Frame{.width = 240.0F, .height = 200.0F}),
                Stack{Text("VA")}.Key("vcmp-a").With(Frame{.width = 240.0F, .height = 200.0F}),
                {
                    .axis = Axis::Vertical,
                    .initial_progress = 0.4F,
                    .min_progress = 0.1F,
                    .max_progress = 0.9F,
                    .drag_anywhere = false,
                    .handle_hit_extent = 24.0F,
                    .controller = ctrl,
                })
                .Key("vcmp-stage")
                .With(Frame{.width = 240.0F, .height = 200.0F}),
            Text("vpct:" + std::to_string(pct)),
          }.With(Spacing(8.0F), Padding(16.0F));
        },
        {.window = {.initial_size = {320, 280}}, .show_debug_overlay = false});
    UiTest vui(vcmp_app, {.viewport = {320, 280}});
    vui.PumpAndSettle();

    const Rect vstage = vui.Find(UiSelector::Key("vcmp-stage")).One().bounds;
    CHECK(vui.Find(UiSelector::Text("vpct:40")).Exists());

    // Drag far below split (0.85 * 200 = 170 vs split 80, > 12px half hit extent) -> ignored.
    vui.Drag({vstage.x + 120.0F, vstage.y + 170.0F}, {vstage.x + 120.0F, vstage.y + 40.0F},
             {.duration = std::chrono::duration<double>(0.1), .segments = 5});
    vui.PumpAndSettle();
    CHECK(vui.Find(UiSelector::Text("vpct:40")).Exists());

    // Drag directly from vertical split (y0 + 80) to y0 + 150 (75%) -> updates cleanly.
    vui.Drag({vstage.x + 120.0F, vstage.y + 80.0F}, {vstage.x + 120.0F, vstage.y + 150.0F},
             {.duration = std::chrono::duration<double>(0.15), .segments = 6});
    vui.PumpAndSettle();
    CHECK(vui.Find(UiSelector::Text("vpct:75")).Exists());
  }

  // ScrubValue: drag_slop, horizontal scrub with mid-drag Shift (Fine) & Alt (Coarse) multipliers,
  // Escape cancel without commit, double-click reset to default_value, vertical axis, keyboard, and controller.
  {
    Application scrub_app(
        []() -> View {
          auto h_ctrl = UseScrubValueController(0.0);
          auto v_ctrl = UseScrubValueController(10.0);
          auto changes = UseState(0);
          auto commits = UseState(0);
          const auto h_x10 = static_cast<int>(std::lround(h_ctrl.Value() * 10.0));
          const auto v_val = static_cast<int>(std::lround(v_ctrl.Value()));
          return Column {
            ScrubValue(
                [](ScrubValueSnapshot s) -> View {
                  const int x10 = static_cast<int>(std::lround(s.value * 10.0));
                  const int dx10 = static_cast<int>(std::lround(s.drag_delta * 10.0));
                  const char* tier =
                      s.precision == ScrubPrecision::Fine ? "F"
                                                          : (s.precision == ScrubPrecision::Coarse ? "C" : "N");
                  return Column {
                    Text("sx:" + std::to_string(x10) + " sd:" + std::to_string(dx10)
                         + " ss:" + (s.scrubbing ? "1" : "0") + " sp:" + tier),
                    Text("sf:" + std::string(s.focused ? "1" : "0")),
                  }.Key("scrub-h-box").With(Frame{.width = 220.0F, .height = 52.0F}, Padding(6.0F));
                },
                {
                    .initial_value = 0.0,
                    .default_value = 12.0,
                    .min_value = -50.0,
                    .max_value = 100.0,
                    .sensitivity = 1.0,
                    .fine_multiplier = 0.1,
                    .coarse_multiplier = 10.0,
                    .step = 0.5,
                    .axis = Axis::Horizontal,
                    .drag_slop = 4.0F,
                    .controller = h_ctrl,
                },
                [changes](double) { changes = changes.Get() + 1; },
                [commits](double) { commits = commits.Get() + 1; })
                .Key("scrub-h"),
            ScrubValue(
                Text("vy:" + std::to_string(v_val)).Key("scrub-v-txt").With(Frame{.width = 120.0F, .height = 48.0F}),
                {
                    .initial_value = 10.0,
                    .min_value = 0.0,
                    .max_value = 100.0,
                    .sensitivity = 1.0,
                    .step = 1.0,
                    .axis = Axis::Vertical,
                    .controller = v_ctrl,
                })
                .Key("scrub-v"),
            Text("hv:" + std::to_string(h_x10)),
            Text(changes.Get() > 0 ? "chg>0" : "chg:0"),
            Text("cmt:" + std::to_string(commits.Get())),
            Row {
              Button("Set33").Key("scrub-s33").OnClick([h_ctrl] { h_ctrl.SetValue(33.3); }),
              Button("Nudge").Key("scrub-nud").OnClick([h_ctrl] { h_ctrl.Nudge(5.0); }),
              Button("Reset").Key("scrub-rst").OnClick([h_ctrl] { h_ctrl.Reset(); }),
            }.With(Spacing(6.0F)),
          }.With(Spacing(8.0F), Padding(16.0F));
        },
        {.window = {.initial_size = {320, 260}}, .show_debug_overlay = false});
    UiTest sui(scrub_app, {.viewport = {320, 260}});
    sui.PumpAndSettle();

    const Point h_center = Center(sui.Find(UiSelector::Key("scrub-h-box")).One().bounds);
    const Point v_center = Center(sui.Find(UiSelector::Key("scrub-v-txt")).One().bounds);
    CHECK(sui.Find(UiSelector::Text("sx:0 sd:0 ss:0 sp:N")).Exists());
    CHECK(sui.Find(UiSelector::Text("cmt:0")).Exists());

    // Drag inside drag_slop (< 4px) does not start scrubbing or emit change/commit.
    sui.SendPointer({.type = PointerEventType::Down,
                     .pointer_id = 1,
                     .position = h_center,
                     .changed_button = PointerButton::Primary,
                     .pressed_buttons = PointerButton::Primary});
    sui.PumpAndSettle();
    sui.SendPointer({.type = PointerEventType::Move,
                     .pointer_id = 1,
                     .position = {h_center.x + 2.0F, h_center.y},
                     .pressed_buttons = PointerButton::Primary});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("sx:0 sd:0 ss:0 sp:N")).Exists());
    CHECK(sui.Find(UiSelector::Text("chg:0")).Exists());

    // Move past slop (+10px -> value 10.0), then switch to Shift fine (+50px * 0.1 = +5.0 -> 15.0),
    // then Alt coarse (+5px * 10.0 = +50.0 -> 65.0) without any jump on modifier transitions.
    sui.SendPointer({.type = PointerEventType::Move,
                     .pointer_id = 1,
                     .position = {h_center.x + 10.0F, h_center.y},
                     .pressed_buttons = PointerButton::Primary});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("sx:100 sd:100 ss:1 sp:N")).Exists());
    CHECK(sui.Find(UiSelector::Text("cmt:0")).Exists());

    sui.SendPointer({.type = PointerEventType::Move,
                     .pointer_id = 1,
                     .position = {h_center.x + 60.0F, h_center.y},
                     .pressed_buttons = PointerButton::Primary,
                     .modifiers = {.shift = true}});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("sx:150 sd:150 ss:1 sp:F")).Exists());

    sui.SendPointer({.type = PointerEventType::Move,
                     .pointer_id = 1,
                     .position = {h_center.x + 65.0F, h_center.y},
                     .pressed_buttons = PointerButton::Primary,
                     .modifiers = {.alt = true}});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("sx:650 sd:650 ss:1 sp:C")).Exists());

    sui.SendPointer({.type = PointerEventType::Up,
                     .pointer_id = 1,
                     .position = {h_center.x + 65.0F, h_center.y},
                     .changed_button = PointerButton::Primary});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("sx:650 sd:0 ss:0 sp:N")).Exists());
    CHECK(sui.Find(UiSelector::Text("cmt:1")).Exists());

    // Verify initial focus & Tab / Shift+Tab focus navigation on scrub-h, then drag +20px -> 85.0 and Escape.
    CHECK(sui.Find(UiSelector::Text("sf:1")).Exists());
    sui.PressKey(Key::Tab);
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("sf:0")).Exists());
    sui.PressKey(Key::Tab, {.shift = true});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("sf:1")).Exists());

    sui.SendPointer({.type = PointerEventType::Down,
                     .pointer_id = 2,
                     .position = h_center,
                     .changed_button = PointerButton::Primary,
                     .pressed_buttons = PointerButton::Primary});
    sui.SendPointer({.type = PointerEventType::Move,
                     .pointer_id = 2,
                     .position = {h_center.x + 20.0F, h_center.y},
                     .pressed_buttons = PointerButton::Primary});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("hv:850")).Exists());
    sui.PressKey(Key::Escape);
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("hv:650")).Exists());
    CHECK(sui.Find(UiSelector::Text("cmt:1")).Exists());

    // Keyboard adjustments while focused: ArrowRight (+0.5 -> 65.5), Shift+ArrowRight (raw clamped step -> 65.5 -> min),
    // Alt+ArrowRight (+5.0 -> 70.5), End (100.0), Home (-50.0).
    sui.PressKey(Key::ArrowRight);
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("hv:655")).Exists());
    CHECK(sui.Find(UiSelector::Text("cmt:2")).Exists());
    sui.PressKey(Key::ArrowRight, {.alt = true});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("hv:705")).Exists());
    sui.PressKey(Key::End);
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("hv:1000")).Exists());
    sui.PressKey(Key::Home);
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("hv:-500")).Exists());

    // Double-click resets to default_value (12.0 -> hv:120).
    sui.TapAt(h_center);
    sui.Pump(std::chrono::milliseconds(80));
    sui.TapAt(h_center);
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("hv:120")).Exists());

    // Controller SetValue(33.3 -> quantized to step 0.5 = 33.5), Nudge(+5.0 -> 38.5), Reset (-> 12.0).
    sui.Find(UiSelector::Key("scrub-s33")).Tap();
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("hv:335")).Exists());
    sui.Find(UiSelector::Key("scrub-nud")).Tap();
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("hv:385")).Exists());
    sui.Find(UiSelector::Key("scrub-rst")).Tap();
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("hv:120")).Exists());

    // Vertical axis scrubber: upward drag (-dy) increases value (10 -> 40).
    sui.Drag(v_center, {v_center.x, v_center.y - 30.0F},
             {.duration = std::chrono::duration<double>(0.1), .segments = 5});
    sui.PumpAndSettle();
    CHECK(sui.Find(UiSelector::Text("vy:40")).Exists());
  }

  // RadialDial: bounded 270-deg sweep crossing +-180 seam, Shift fine drag, dead-zone guard,
  // Escape revert, wheel, keyboard, double-click reset, controller, and continuous multi-turn rotation.
  {
    Application dial_app(
        []() -> View {
          auto b_ctrl = UseRadialDialController(0.0);
          auto c_ctrl = UseRadialDialController(0.0);
          auto b_commits = UseState(0);
          const int bv10 = static_cast<int>(std::lround(b_ctrl.Value() * 10.0));
          const int ba = static_cast<int>(std::lround(b_ctrl.AngleDegrees()));
          const int cv = static_cast<int>(std::lround(c_ctrl.Value()));
          const int ct10 = static_cast<int>(std::lround((c_ctrl.Value() / 360.0) * 10.0));
          return Column {
            RadialDial(
                [](RadialDialSnapshot s) -> View {
                  const int v10 = static_cast<int>(std::lround(s.value * 10.0));
                  return Column {
                    Text("bv10:" + std::to_string(v10) + " bf:" + (s.focused ? "1" : "0")
                         + " bd:" + (s.dragging ? "1" : "0")),
                  }.Key("dial-b-box").With(Frame{.width = 100.0F, .height = 100.0F});
                },
                {
                    .mode = RadialDialMode::Bounded,
                    .initial_value = 0.0,
                    .default_value = 25.0,
                    .min_value = 0.0,
                    .max_value = 100.0,
                    .start_angle_degrees = 135.0,
                    .sweep_degrees = 270.0,
                    .keyboard_step = 5.0,
                    .fine_multiplier = 0.2,
                    .center_dead_zone = 6.0F,
                    .wheel_step = 5.0,
                    .controller = b_ctrl,
                },
                {}, [b_commits](double) { b_commits = b_commits.Get() + 1; })
                .Key("dial-b"),
            RadialDial(
                [](RadialDialSnapshot s) -> View {
                  return Text("snap_c:" + std::to_string(static_cast<int>(std::lround(s.turns * 10.0))))
                      .Key("dial-c-box")
                      .With(Frame{.width = 100.0F, .height = 100.0F});
                },
                {
                    .mode = RadialDialMode::Continuous,
                    .initial_value = 0.0,
                    .min_value = -std::numeric_limits<double>::infinity(),
                    .max_value = std::numeric_limits<double>::infinity(),
                    .start_angle_degrees = -90.0,
                    .value_per_turn = 360.0,
                    .controller = c_ctrl,
                })
                .Key("dial-c"),
            Row {
              Text("ba:" + std::to_string(ba)),
              Text("bvctl:" + std::to_string(bv10) + " bcmt:" + std::to_string(b_commits.Get())),
            }.With(Spacing(6.0F)),
            Text("cv:" + std::to_string(cv) + " ct10:" + std::to_string(ct10)),
            Row {
              Button("S60").Key("dial-s60").OnClick([b_ctrl] { b_ctrl.SetValue(60.0); }),
              Button("N15").Key("dial-n15").OnClick([b_ctrl] { b_ctrl.Nudge(15.0); }),
              Button("Rst").Key("dial-rst").OnClick([b_ctrl] { b_ctrl.Reset(); }),
            }.With(Spacing(6.0F)),
          }.With(Spacing(8.0F), Padding(16.0F));
        },
        {.window = {.initial_size = {360, 400}}, .show_debug_overlay = false});
    UiTest dui(dial_app, {.viewport = {360, 400}});
    dui.PumpAndSettle();

    const Point bc = Center(dui.Find(UiSelector::Key("dial-b-box")).One().bounds);
    const Point cc = Center(dui.Find(UiSelector::Key("dial-c-box")).One().bounds);
    constexpr double kDeg2Rad = 3.14159265358979323846 / 180.0;
    auto polar = [](Point origin, double deg, float r = 34.0F) -> Point {
      return {
          origin.x + r * static_cast<float>(std::cos(deg * kDeg2Rad)),
          origin.y + r * static_cast<float>(std::sin(deg * kDeg2Rad)),
      };
    };

    CHECK(dui.Find(UiSelector::Text("bv10:0 bf:0 bd:0")).Exists());
    CHECK(dui.Find(UiSelector::Text("ba:135")).Exists());
    CHECK(dui.Find(UiSelector::Text("bvctl:0 bcmt:0")).Exists());

    // Clockwise drag from 135 deg -> 175 -> -175 (crossing +-180 seam!) -> -90 (50%) -> 45 (100%) -> 90 (clamped).
    dui.SendPointer({.type = PointerEventType::Down,
                     .pointer_id = 1,
                     .position = polar(bc, 135.0),
                     .changed_button = PointerButton::Primary,
                     .pressed_buttons = PointerButton::Primary});
    for(double a : {175.0, -175.0, -90.0}) {
      dui.SendPointer({.type = PointerEventType::Move,
                       .pointer_id = 1,
                       .position = polar(bc, a),
                       .pressed_buttons = PointerButton::Primary});
    }
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("bv10:500 bf:1 bd:1")).Exists());
    CHECK(dui.Find(UiSelector::Text("ba:270")).Exists());
    CHECK(dui.Find(UiSelector::Text("bvctl:500 bcmt:0")).Exists());

    for(double a : {-45.0, 0.0, 45.0, 90.0}) {
      dui.SendPointer({.type = PointerEventType::Move,
                       .pointer_id = 1,
                       .position = polar(bc, a),
                       .pressed_buttons = PointerButton::Primary});
    }
    dui.SendPointer({.type = PointerEventType::Up,
                     .pointer_id = 1,
                     .position = polar(bc, 90.0),
                     .changed_button = PointerButton::Primary});
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("bv10:1000 bf:1 bd:0")).Exists());
    CHECK(dui.Find(UiSelector::Text("ba:405")).Exists());
    CHECK(dui.Find(UiSelector::Text("bvctl:1000 bcmt:1")).Exists());

    // Dead-zone guard inside center_dead_zone (< 6px): tiny center movement changes nothing.
    dui.SendPointer({.type = PointerEventType::Down,
                     .pointer_id = 2,
                     .position = bc,
                     .changed_button = PointerButton::Primary,
                     .pressed_buttons = PointerButton::Primary});
    dui.SendPointer({.type = PointerEventType::Move,
                     .pointer_id = 2,
                     .position = {bc.x + 2.0F, bc.y - 2.0F},
                     .pressed_buttons = PointerButton::Primary});
    dui.SendPointer({.type = PointerEventType::Up,
                     .pointer_id = 2,
                     .position = {bc.x + 2.0F, bc.y - 2.0F},
                     .changed_button = PointerButton::Primary});
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("ba:405")).Exists());
    CHECK(dui.Find(UiSelector::Text("bvctl:1000 bcmt:1")).Exists());

    // Shift fine drag counter-clockwise by 90 deg (-90 -> 180): -90/270 * 100 * 0.2 = -6.67 -> 93.3.
    dui.SendPointer({.type = PointerEventType::Down,
                     .pointer_id = 3,
                     .position = polar(bc, -90.0),
                     .changed_button = PointerButton::Primary,
                     .pressed_buttons = PointerButton::Primary});
    dui.SendPointer({.type = PointerEventType::Move,
                     .pointer_id = 3,
                     .position = polar(bc, 180.0),
                     .pressed_buttons = PointerButton::Primary,
                     .modifiers = {.shift = true}});
    dui.SendPointer({.type = PointerEventType::Up,
                     .pointer_id = 3,
                     .position = polar(bc, 180.0),
                     .changed_button = PointerButton::Primary});
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("ba:387")).Exists());
    CHECK(dui.Find(UiSelector::Text("bvctl:933 bcmt:2")).Exists());

    // Escape during drag reverts value without committing.
    dui.SendPointer({.type = PointerEventType::Down,
                     .pointer_id = 4,
                     .position = polar(bc, 180.0),
                     .changed_button = PointerButton::Primary,
                     .pressed_buttons = PointerButton::Primary});
    dui.SendPointer({.type = PointerEventType::Move,
                     .pointer_id = 4,
                     .position = polar(bc, 150.0),
                     .pressed_buttons = PointerButton::Primary});
    dui.PumpAndSettle();
    dui.PressKey(Key::Escape);
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("ba:387")).Exists());
    CHECK(dui.Find(UiSelector::Text("bvctl:933 bcmt:2")).Exists());

    // Wheel up/down & keyboard End/Home/ArrowRight while focused.
    dui.SendScroll({.position = bc, .delta_y = -120.0F});
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("bvctl:983 bcmt:3")).Exists());

    dui.PressKey(Key::Home);
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("ba:135")).Exists());
    CHECK(dui.Find(UiSelector::Text("bvctl:0 bcmt:4")).Exists());
    dui.PressKey(Key::ArrowRight);
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("bvctl:50 bcmt:5")).Exists());
    dui.PressKey(Key::End);
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("ba:405")).Exists());
    CHECK(dui.Find(UiSelector::Text("bvctl:1000 bcmt:6")).Exists());

    // Double-click center resets to default_value (25.0).
    dui.TapAt(bc);
    dui.Pump(std::chrono::milliseconds(80));
    dui.TapAt(bc);
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("bvctl:250 bcmt:7")).Exists());

    // Controller SetValue, Nudge, Reset.
    dui.Find(UiSelector::Key("dial-s60")).Tap();
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("bvctl:600 bcmt:8")).Exists());
    dui.Find(UiSelector::Key("dial-n15")).Tap();
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("bvctl:750 bcmt:9")).Exists());
    dui.Find(UiSelector::Key("dial-rst")).Tap();
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("bvctl:250 bcmt:10")).Exists());

    // Continuous multi-turn encoder: 1.5 clockwise turns (-90 -> ... -> 450 deg = 540, 1.5 turns).
    dui.SendPointer({.type = PointerEventType::Down,
                     .pointer_id = 5,
                     .position = polar(cc, -90.0),
                     .changed_button = PointerButton::Primary,
                     .pressed_buttons = PointerButton::Primary});
    for(int step = 1; step <= 12; ++step) {
      const double a = -90.0 + static_cast<double>(step) * 45.0;
      dui.SendPointer({.type = PointerEventType::Move,
                       .pointer_id = 5,
                       .position = polar(cc, a),
                       .pressed_buttons = PointerButton::Primary});
    }
    dui.SendPointer({.type = PointerEventType::Up,
                     .pointer_id = 5,
                     .position = polar(cc, 90.0),
                     .changed_button = PointerButton::Primary});
    dui.PumpAndSettle();
    CHECK(dui.Find(UiSelector::Text("cv:540 ct10:15")).Exists());
    CHECK(dui.Find(UiSelector::Text("snap_c:15")).Exists());
  }
}

#endif
