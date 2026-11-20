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
}

#endif
